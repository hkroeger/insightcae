#ifndef CADMODEL3DVIEWER_H
#define CADMODEL3DVIEWER_H

#include "toolkit_gui_export.h"
#include "cadparametersetvisualizer.h"
#include "cadtypes.h"
#include "editorwithsavablestate.h"


class TOOLKIT_GUI_EXPORT CADModel3DViewer
: public EditorWithSavableState
{
public:

    typedef boost::variant<
        insight::cad::VectorPtr,
        insight::cad::DatumPtr,
        insight::cad::FeaturePtr,
        insight::cad::PostprocActionPtr,
        vtkSmartPointer<vtkDataObject>
        > CADEntity;

    struct SubshapeData {
        insight::cad::FeaturePtr feat;
        insight::cad::EntityType subshapeType_;
        insight::cad::FeatureID id_;

        bool operator==(const SubshapeData& o) const;
        bool operator<(const SubshapeData& o) const;
    };

    virtual void setCameraState(const insight::CameraState& camState) =0;
};

#endif // CADMODEL3DVIEWER_H
