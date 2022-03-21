#ifndef METATYPES_H
#define METATYPES_H

#include "toolkit_gui_export.h"


#include <QMetaType>

#include "base/parameterset.h"
#include "base/resultset.h"
#include "base/exception.h"
#include "base/taskspoolerinterface.h"
#include "base/progressdisplayer.h"
#include "base/supplementedinputdata.h"

#include <armadillo>

Q_DECLARE_METATYPE(insight::ParameterSet);
Q_DECLARE_METATYPE(insight::ResultSetPtr);
Q_DECLARE_METATYPE(insight::Exception);
Q_DECLARE_METATYPE(insight::ProgressState);
Q_DECLARE_METATYPE(insight::ProgressStatePtr);
Q_DECLARE_METATYPE(insight::TaskSpoolerInterface::JobList);
Q_DECLARE_METATYPE(arma::mat);
Q_DECLARE_METATYPE(insight::supplementedInputDataBasePtr);

class TOOLKIT_GUI_EXPORT ISMetaTypeRegistrator
{
public:
  ISMetaTypeRegistrator();
};

TOOLKIT_GUI_EXPORT extern ISMetaTypeRegistrator ismetatyperegistrator;


#endif // METATYPES_H
