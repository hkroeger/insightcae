#include <QtGui/QApplication>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include "workbench.h"

#include "base/linearalgebra.h"

int main(int argc, char** argv)
{
  insight::GSLExceptionHandling gsl_errtreatment;
  
  WorkbenchApplication app(argc, argv);
  workbench foo;
  if (argc>1)
  {
    boost::filesystem::path fn(argv[1]);
    foo.openAnalysis(boost::filesystem::absolute(fn).c_str());
  }
  foo.show();
  return app.exec();
}
