#include "iscadmetatyperegistrator.h"

ISCADMetaTypeRegistrator::ISCADMetaTypeRegistrator()
{
  qRegisterMetaType<insight::cad::ScalarPtr>("insight::cad::ScalarPtr");
  qRegisterMetaType<insight::cad::VectorPtr>("insight::cad::VectorPtr");
  qRegisterMetaType<insight::cad::FeaturePtr>("insight::cad::FeaturePtr");
  qRegisterMetaType<insight::cad::DatumPtr>("insight::cad::DatumPtr");
  qRegisterMetaType<vtkSmartPointer<vtkDataObject> >("vtkSmartPointer<vtkDataObject>");
  qRegisterMetaType<insight::cad::PostprocActionPtr>("insight::cad::PostprocActionPtr");
  qRegisterMetaType<insight::cad::VectorVariableType>("insight::cad::VectorVariableType");
  qRegisterMetaType<Optional_AIS_DisplayMode>("boost::variant<boost::blank,AIS_DisplayMode>");
  qRegisterMetaType<QVector<int> >("QVector<int>");
  qRegisterMetaType<AIS_DisplayMode>("AIS_DisplayMode");
  qRegisterMetaType<Optional_FeatureVisualizationStyle>("boost::variant<boost::blank,insight::cad::FeatureVisualizationStyle>");
}

ISCADMetaTypeRegistrator iscadmetatyperegistrator;
