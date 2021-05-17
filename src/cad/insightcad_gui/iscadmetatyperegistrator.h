#ifndef ISCADMETATYPEREGISTRATOR_H
#define ISCADMETATYPEREGISTRATOR_H

#include "insightcad_gui_export.h"

#include <QMetaType>
#include <QVector>

#include <boost/variant.hpp>

#include "cadparameters.h"
#include "cadfeature.h"

#include "AIS_DisplayMode.hxx"

Q_DECLARE_METATYPE(insight::cad::ScalarPtr)
Q_DECLARE_METATYPE(insight::cad::VectorPtr)
Q_DECLARE_METATYPE(insight::cad::FeaturePtr)
Q_DECLARE_METATYPE(insight::cad::DatumPtr)
Q_DECLARE_METATYPE(insight::cad::PostprocActionPtr)
Q_DECLARE_METATYPE(insight::cad::VectorVariableType)
typedef boost::variant<boost::blank,AIS_DisplayMode> Optional_AIS_DisplayMode;
Q_DECLARE_METATYPE(Optional_AIS_DisplayMode)
Q_DECLARE_METATYPE(QVector<int>)
Q_DECLARE_METATYPE(AIS_DisplayMode)

class INSIGHTCAD_GUI_EXPORT ISCADMetaTypeRegistrator
{
public:
  ISCADMetaTypeRegistrator();
};

INSIGHTCAD_GUI_EXPORT extern ISCADMetaTypeRegistrator iscadmetatyperegistrator;



#endif // ISCADMETATYPEREGISTRATOR_H
