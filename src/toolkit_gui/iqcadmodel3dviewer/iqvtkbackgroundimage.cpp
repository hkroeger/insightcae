#include "iqvtkbackgroundimage.h"

#include "base/spatialtransformation.h"
#include "base/translations.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqcadmodel3dviewer/iqvtkvieweractions/orientbackgroundimage.h"

#include "vtkImageReader2.h"
#include "vtkImageReader2Factory.h"
#include "vtkImageData.h"



using namespace insight;




IQVTKCADModel3DViewer &BackgroundImage::viewer()
{
    return dynamic_cast<IQVTKCADModel3DViewer&>(*parent());
}




BackgroundImage::BackgroundImage(
    const boost::filesystem::path& imageFile,
    IQVTKCADModel3DViewer& v )

    : QObject(&v),
    imageFileName_(imageFile),
    label_(QString::fromStdString(imageFile.filename().string()))
{
    insight::assertion(
        boost::filesystem::exists(imageFileName_),
        _("image file \"%s\" does not exist"), imageFileName_.string().c_str() );

    auto readerFactory = vtkSmartPointer<vtkImageReader2Factory>::New();
    vtkSmartPointer<vtkImageReader2> imageReader;
    auto ir=readerFactory->CreateImageReader2(imageFileName_.string().c_str());

    insight::assertion(
        ir!=nullptr,
        _("could not create reader for file \"%s\""), imageFileName_.string().c_str() );

    imageReader.TakeReference(ir);
    imageReader->SetFileName(imageFileName_.string().c_str());
    imageReader->Update();

    auto imageData = imageReader->GetOutput();

    {
        int na=imageData->GetPointData()->GetNumberOfArrays();
        insight::assertion(
            na==1,
            "expected a single image array! Got %d arrays.", na);

        int nc=imageData->GetPointData()->GetArray(0)->GetNumberOfComponents();
        insight::assertion(
            (nc==3)||(nc==1),
            "expected image array with three or one components! Got %d.", nc);
    }

    imageActor_ = vtkSmartPointer<vtkImageActor>::New();
    imageActor_->SetInputData(imageData);

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
    IQVTKCADModel3DViewer& v )

    : QObject(&v)
{
    label_= QString::fromStdString(
        getMandatoryAttribute(node,"label") );

    imageFileName_= boost::filesystem::path(
        getMandatoryAttribute(node, "imageFileName") );

    insight::assertion(
        boost::filesystem::exists(imageFileName_),
        _("image file \"%s\" does not exist"), imageFileName_.string().c_str() );

    auto readerFactory = vtkSmartPointer<vtkImageReader2Factory>::New();
    vtkSmartPointer<vtkImageReader2> imageReader;
    auto ir=readerFactory->CreateImageReader2(imageFileName_.string().c_str());

    insight::assertion(
        ir!=nullptr,
        _("could not create reader for file \"%s\""), imageFileName_.string().c_str() );

    imageReader.TakeReference(ir);
    imageReader->SetFileName(imageFileName_.string().c_str());
    imageReader->Update();
    auto imageData = imageReader->GetOutput();
    imageActor_ = vtkSmartPointer<vtkImageActor>::New();
    imageActor_->SetInputData(imageData);
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

    viewer().scheduleRedraw();
}




QString BackgroundImage::label() const
{
    return label_;
}




void BackgroundImage::write(
    rapidxml::xml_document<> &doc,
    rapidxml::xml_node<> &node ) const
{
    insight::appendAttribute(
        doc, node,
        "label", label_.toStdString());

    insight::appendAttribute(
        doc, node, "imageFileName",
        imageFileName_.string());

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
