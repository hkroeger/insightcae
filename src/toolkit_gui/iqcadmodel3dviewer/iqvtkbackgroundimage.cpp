#include "iqvtkbackgroundimage.h"

#include "base/boost_include.h"
#include "base/spatialtransformation.h"
#include "base/translations.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqcadmodel3dviewer/iqvtkvieweractions/orientbackgroundimage.h"

#include "vtkImageReader2.h"
#include "vtkImageReader2Factory.h"
#include "vtkImageData.h"
#include "vtkImageMapper3D.h"

#include "cpp/poppler-document.h"
#include "cpp/poppler-page.h"
#include "cpp/poppler-page-renderer.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <limits>



using namespace insight;


vtkSmartPointer<vtkImageData>
BackgroundImage::loadImageData(const boost::filesystem::path& fp, int pdfRenderDpi)
{
    std::string ext = fp.extension().string();
    boost::algorithm::to_lower(ext);

    if (ext == ".pdf")
    {
        std::shared_ptr<poppler::document> doc(
            poppler::document::load_from_file(fp.string()));
        insight::assertion(
            doc && !doc->is_locked(),
            "could not open PDF file \"%s\"", fp.string().c_str());
        insight::assertion(
            doc->pages() >= 1,
            "PDF has no pages: \"%s\"", fp.string().c_str());

        std::shared_ptr<poppler::page> page(doc->create_page(0));
        insight::assertion(
            page != nullptr,
            "could not read page 0 of \"%s\"", fp.string().c_str());

        // Pre-validate: poppler uses int arithmetic for buffer allocation.
        // width * height * 4 overflowing int causes a silent "Bogus memory
        // allocation size" without throwing any C++ exception.
        {
            auto rect = page->page_rect();
            auto expectedW = static_cast<long long>(rect.width()  * pdfRenderDpi / 72.0 + 0.5);
            auto expectedH = static_cast<long long>(rect.height() * pdfRenderDpi / 72.0 + 0.5);
            insight::assertion(
                expectedW > 0 && expectedH > 0 &&
                expectedW * expectedH * 4 <= std::numeric_limits<int>::max(),
                "DPI %d would require a %lldx%lld pixel image for \"%s\","
                " which exceeds the poppler rendering limit; reduce the DPI",
                pdfRenderDpi, expectedW, expectedH, fp.string().c_str());
        }

        poppler::page_renderer pr;
        pr.set_render_hint(poppler::page_renderer::antialiasing, true);
        pr.set_render_hint(poppler::page_renderer::text_antialiasing, true);
        poppler::image img = pr.render_page(page.get(), pdfRenderDpi, pdfRenderDpi);
        insight::assertion(
            img.is_valid() && img.data() != nullptr,
            "PDF rendering failed for \"%s\" (try a lower DPI value)", fp.string().c_str());

        int w = img.width(), h = img.height();
        auto vtkImg = vtkSmartPointer<vtkImageData>::New();
        vtkImg->SetDimensions(w, h, 1);
        vtkImg->AllocateScalars(VTK_UNSIGNED_CHAR, 3); // RGB

        // poppler renders top-down; VTK stores bottom-up → flip Y.
        // format_argb32 stores pixels as [B, G, R, A] in memory (little-endian ARGB).
        for (int y = 0; y < h; ++y)
        {
            const unsigned char* src =
                reinterpret_cast<const unsigned char*>(img.data())
                + (h - 1 - y) * img.bytes_per_row();
            for (int x = 0; x < w; ++x)
            {
                unsigned char* dst = static_cast<unsigned char*>(
                    vtkImg->GetScalarPointer(x, y, 0));
                dst[0] = src[x*4 + 2]; // R
                dst[1] = src[x*4 + 1]; // G
                dst[2] = src[x*4 + 0]; // B
            }
        }
        return vtkImg;
    }
    else
    {
        auto readerFactory = vtkSmartPointer<vtkImageReader2Factory>::New();
        vtkSmartPointer<vtkImageReader2> imageReader;
        auto ir = readerFactory->CreateImageReader2(fp.string().c_str());

        insight::assertion(
            ir != nullptr,
            _("could not create reader for file \"%s\""), fp.string().c_str());

        imageReader.TakeReference(ir);
        imageReader->SetFileName(fp.string().c_str());
        imageReader->Update();

        auto imageData = imageReader->GetOutput();

        {
            int na = imageData->GetPointData()->GetNumberOfArrays();
            insight::assertion(
                na == 1,
                "expected a single image array! Got %d arrays.", na);

            int nc = imageData->GetPointData()->GetArray(0)->GetNumberOfComponents();
            insight::assertion(
                (nc == 3) || (nc == 1),
                "expected image array with three or one components! Got %d.", nc);
        }

        return imageData;
    }
}

void BackgroundImage::setDimFilter()
{
    dimFilter_ = vtkSmartPointer<vtkImageShiftScale>::New();
    dimFilter_->SetInputData(imageData_);
    dimFilter_->SetScale(1);
    dimFilter_->SetShift(0);
    dimFilter_->SetOutputScalarTypeToUnsignedChar();
    dimFilter_->ClampOverflowOn();

    imageActor_->GetMapper()->SetInputConnection(dimFilter_->GetOutputPort());
}




IQVTKCADModel3DViewer &BackgroundImage::viewer()
{
    return dynamic_cast<IQVTKCADModel3DViewer&>(*parent());
}




BackgroundImage::BackgroundImage(
    const boost::filesystem::path& imageFile,
    IQVTKCADModel3DViewer& v,
    int pdfRenderDpi )

    : QObject(&v),
    imageFileName_(imageFile),
    pdfRenderDpi_(pdfRenderDpi),
    label_(QString::fromStdString(imageFile.filename().string()))
{
    insight::assertion(
        boost::filesystem::exists(imageFileName_),
        _("image file \"%s\" does not exist"), imageFileName_.string().c_str() );

    imageData_ = loadImageData(imageFileName_, pdfRenderDpi_);

    imageActor_ = vtkSmartPointer<vtkImageActor>::New();
    imageActor_->SetInputData(imageData_);
    setDimFilter();

    imageActor_->SetScale( 1 );
    imageActor_->SetOrientation(0, 0, 0);
    imageActor_->SetPosition( 0, 0, 0 );

    imageActor_->Update();
    double b[6]; // (xmin,xmax, ymin,ymax, zmin,zmax).
    imageActor_->GetBounds(b);

    os_.xy_[0] = os_.p_[0] = vec3Zero();
    os_.xy_[1] = os_.p_[1] = vec3(b[1], b[3], 0);


    reorientImage();
}




void BackgroundImage::reorientImage()
{
    imageActor_->SetOpacity(1.0);// can't pick if opacity is lower than 1

    usedRenderer_=viewer().renderer();
    usedRenderer_->AddActor(imageActor_);
    viewer().scheduleRedraw();

    auto obia=make_viewWidgetAction<IQVTKOrientBackgroundImage>(viewer(), *this);
    obia->orientationSelected.connect(
        [this](OrientationSpec os)
        {
            DBG_SLOT(orientationSelected);
            setOrientation(os);
        }
        );

    viewer().launchAction(
        std::move(obia),
        true );
}




BackgroundImage::BackgroundImage(
    const rapidxml::xml_node<>& node,
    const boost::filesystem::path& parentPath,
    IQVTKCADModel3DViewer& v )

    : QObject(&v)
{
    label_= QString::fromStdString(
        getMandatoryAttribute(node,"label") );

    imageFileName_= boost::filesystem::path(
        getMandatoryAttribute(node, "imageFileName") );

    if (imageFileName_.is_relative())
    {
        imageFileName_ = parentPath/imageFileName_;
    }

    pdfRenderDpi_ = getOptionalAttributeOrDefault<int>(node, "pdfRenderDpi", 300);

    insight::assertion(
        boost::filesystem::exists(imageFileName_),
        _("image file \"%s\" does not exist"), imageFileName_.string().c_str() );

    imageData_ = loadImageData(imageFileName_, pdfRenderDpi_);

    imageActor_ = vtkSmartPointer<vtkImageActor>::New();
    imageActor_->SetInputData(imageData_);
    setDimFilter();

    usedRenderer_=viewer().renderer();
    usedRenderer_->AddActor(imageActor_);

    OrientationSpec os;
    os.p_[0]=getMandatoryAttribute<arma::mat>(node, "p1");
    os.p_[1]=getMandatoryAttribute<arma::mat>(node, "p2");
    os.xy_[0]=getMandatoryAttribute<arma::mat>(node, "xy1");
    os.xy_[1]=getMandatoryAttribute<arma::mat>(node, "xy2");

    setOrientation(os);
}




BackgroundImage::~BackgroundImage()
{
    usedRenderer_->RemoveActor( imageActor_ );
    viewer().scheduleRedraw();
}




OrientationSpec BackgroundImage::orientation() const
{
    return os_;
}

insight::SpatialTransformation OrientationSpec::trsf() const
{

    arma::mat D=xy_[1]-xy_[0];
    arma::mat d=p_[1]-p_[0];
    double scale =
        arma::norm(D,2)
        / arma::norm(d,2);

    double ang=atan2(D[1], D[0]);
    double ang2=atan2(d[1], d[0]);

    SpatialTransformation tr(-p_[0]);
    SpatialTransformation sR(
        vec3Zero(),
        vec3Z((ang-ang2)*180./M_PI), scale);
    SpatialTransformation tr2(xy_[0]);

    return tr2*sR*tr;
}


void BackgroundImage::setOrientation(OrientationSpec os)
{
    os_=os;

    imageActor_->SetUserTransform(
        os_.trsf().toVTKTransform() );

    imageActor_->SetOpacity(0.8); // restore opacity

    dimFilter_->SetScale(1.5); // bleech out
    dimFilter_->SetShift(100.0);

    viewer().scheduleRedraw();
}




QString BackgroundImage::label() const
{
    return label_;
}




void BackgroundImage::write(
    rapidxml::xml_document<> &doc,
    rapidxml::xml_node<> &node,
    const boost::filesystem::path& parentPath ) const
{
    insight::appendAttribute(
        doc, node,
        "label", label_.toStdString());

    insight::appendAttribute(
        doc, node,
        "pdfRenderDpi", pdfRenderDpi_);

    auto fn=boost::filesystem::absolute(imageFileName_);
    if (boost::filesystem::path_contains_file(parentPath, fn))
    {
        fn=boost::filesystem::make_relative(parentPath, fn);
    }
    insight::appendAttribute(
        doc, node, "imageFileName",
        fn.string());

    insight::appendAttribute(
        doc, node,
        "p1", os_.p_[0] );
    insight::appendAttribute(
        doc, node,
        "p2", os_.p_[1] );
    insight::appendAttribute(
        doc, node,
        "xy1", os_.xy_[0] );
    insight::appendAttribute(
        doc, node,
        "xy2", os_.xy_[1] );
}




vtkImageActor *BackgroundImage::imageActor()
{
    return imageActor_;
}




void BackgroundImage::toggleVisibility(bool show)
{
    imageActor_->SetVisibility(show);
    viewer().scheduleRedraw();
}


void BackgroundImage::changeBleech(int percent)
{
    dimFilter_->SetScale(int( 1. +0.02*double(percent) )); // bleech out
    dimFilter_->SetShift( 33.0 *pow(double(percent), 0.25) );
    viewer().scheduleRedraw();
}
