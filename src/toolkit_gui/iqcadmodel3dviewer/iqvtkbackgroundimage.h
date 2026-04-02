#ifndef IQVTKBACKGROUNDIMAGE_H
#define IQVTKBACKGROUNDIMAGE_H

#include "base/linearalgebra.h"
#include "base/boost_include.h"
#include "base/rapidxml.h"
#include "base/spatialtransformation.h"

#include <QObject>

#include "vtkSmartPointer.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"

class IQVTKCADModel3DViewer;


struct OrientationSpec
{
    arma::mat p_[2];
    arma::mat xy_[2];

    insight::SpatialTransformation trsf() const;
};


class BackgroundImage
    : public QObject
{
    Q_OBJECT

    QString label_;
    boost::filesystem::path imageFileName_;
    int pdfRenderDpi_ = 300;
    vtkSmartPointer<vtkImageActor> imageActor_;
    vtkRenderer *usedRenderer_;
    OrientationSpec os_;

    IQVTKCADModel3DViewer& viewer();

    static vtkSmartPointer<vtkImageData>
        loadImageData(const boost::filesystem::path& fp, int pdfRenderDpi = 300);

public:
    /**
     * @brief BackgroundImage
     * load image and launch orientation action
     * @param fp
     * @param viewer
     */
    BackgroundImage(
        const boost::filesystem::path& fp,
        IQVTKCADModel3DViewer& viewer,
        int pdfRenderDpi = 300 );

    /**
     * @brief BackgroundImage
     * restore from XML saved config
     * @param node
     * @param viewer
     */
    BackgroundImage(
        const rapidxml::xml_node<>& node,
        const boost::filesystem::path& parentPath,
        IQVTKCADModel3DViewer& viewer );

    ~BackgroundImage();

    OrientationSpec orientation() const;
    void setOrientation(OrientationSpec os);
    void reorientImage();

    QString label() const;

    void write(
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        const boost::filesystem::path& parentPath ) const;

    vtkImageActor* imageActor();

public Q_SLOTS:
    void toggleVisibility(bool show);
};


#endif // IQVTKBACKGROUNDIMAGE_H
