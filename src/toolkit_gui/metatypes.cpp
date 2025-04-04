#include "metatypes.h"

ISMetaTypeRegistrator::ISMetaTypeRegistrator()
{
  // qRegisterMetaType<std::observer_ptr<insight::Parameter> >("observer_ptr_Parameter");
  // qRegisterMetaType<insight::ParameterSet>("insight::ParameterSet");
  qRegisterMetaType<insight::ResultSetPtr>("insight::ResultSetPtr");
  qRegisterMetaType<insight::ProgressState>("insight::ProgressState");
  qRegisterMetaType<insight::ProgressStatePtr>("insight::ProgressStatePtr");
  qRegisterMetaType<insight::Exception>("insight::Exception");
  qRegisterMetaType<insight::TaskSpoolerInterface::JobList>("insight::TaskSpoolerInterface::JobList");
  qRegisterMetaType<arma::mat>("arma::mat");
  qRegisterMetaType<std::exception_ptr>("std::exception_ptr");
  qRegisterMetaType<insight::supplementedInputDataBasePtr>("insight::supplementedInputDataBasePtr");
}


ISMetaTypeRegistrator ismetatyperegistrator;
