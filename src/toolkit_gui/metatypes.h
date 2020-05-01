#ifndef METATYPES_H
#define METATYPES_H

#include <QMetaType>

#include "base/parameterset.h"
#include "base/resultset.h"
#include "base/exception.h"
#include "base/taskspoolerinterface.h"
#include "base/progressdisplayer.h"

#include <armadillo>

Q_DECLARE_METATYPE(insight::ParameterSet);
Q_DECLARE_METATYPE(insight::ResultSetPtr);
Q_DECLARE_METATYPE(insight::Exception);
Q_DECLARE_METATYPE(insight::ProgressState);
Q_DECLARE_METATYPE(insight::ProgressStatePtr);
Q_DECLARE_METATYPE(insight::TaskSpoolerInterface::JobList);
Q_DECLARE_METATYPE(arma::mat);

class ISMetaTypeRegistrator
{
public:
  ISMetaTypeRegistrator();
};

extern ISMetaTypeRegistrator ismetatyperegistrator;


#endif // METATYPES_H
