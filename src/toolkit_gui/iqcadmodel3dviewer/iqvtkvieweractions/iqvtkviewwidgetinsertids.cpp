#include "iqvtkviewwidgetinsertids.h"


std::map<insight::cad::EntityType, FeatureSelCmd> featureSelCmds = {
    { insight::cad::Vertex, { "vertex", "vid" } },
    { insight::cad::Edge, { "edge", "eid" } },
    { insight::cad::Face, { "face", "fid" } },
    { insight::cad::Solid, { "solid", "sid" } }
};




insight::cad::FeatureID IQVTKViewWidgetInsertPointIDs::getId(const insight::cad::Feature& feat, const TopoDS_Shape& s)
{
  return feat.vertexID(s);
}

IQVTKViewWidgetInsertPointIDs::IQVTKViewWidgetInsertPointIDs(IQVTKCADModel3DViewer &viewWidget)
  : IQVTKViewWidgetInsertIDs<insight::cad::Vertex>(viewWidget)
{}


QString IQVTKViewWidgetInsertPointIDs::description() const
{
    return "Select vertex IDs";
}




insight::cad::FeatureID IQVTKViewWidgetInsertEdgeIDs::getId(const insight::cad::Feature& feat, const TopoDS_Shape& s)
{
  return feat.edgeID(s);
}

IQVTKViewWidgetInsertEdgeIDs::IQVTKViewWidgetInsertEdgeIDs(IQVTKCADModel3DViewer &viewWidget)
  : IQVTKViewWidgetInsertIDs<insight::cad::Edge>(viewWidget)
{}


QString IQVTKViewWidgetInsertEdgeIDs::description() const
{
    return "Select edge IDs";
}



insight::cad::FeatureID IQVTKViewWidgetInsertFaceIDs::getId(const insight::cad::Feature& feat, const TopoDS_Shape& s)
{
  return feat.faceID(s);
}

IQVTKViewWidgetInsertFaceIDs::IQVTKViewWidgetInsertFaceIDs(IQVTKCADModel3DViewer &viewWidget)
  : IQVTKViewWidgetInsertIDs<insight::cad::Face>(viewWidget)
{}

QString IQVTKViewWidgetInsertFaceIDs::description() const
{
    return "Select face IDs";
}




insight::cad::FeatureID IQVTKViewWidgetInsertSolidIDs::getId(const insight::cad::Feature& feat, const TopoDS_Shape& s)
{
  return feat.solidID(s);
}

IQVTKViewWidgetInsertSolidIDs::IQVTKViewWidgetInsertSolidIDs(IQVTKCADModel3DViewer &viewWidget)
  : IQVTKViewWidgetInsertIDs<insight::cad::Solid>(viewWidget)
{}

QString IQVTKViewWidgetInsertSolidIDs::description() const
{
    return "Select solid IDs";
}
