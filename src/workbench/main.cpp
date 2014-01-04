#include <QtGui/QApplication>
#include <boost/filesystem/path.hpp>
#include "workbench.h"


int main(int argc, char** argv)
{
    WorkbenchApplication app(argc, argv);
    workbench foo;
    if (argc>1)
    {
      boost::filesystem::path fn(argv[1]);
      foo.openAnalysis(fn.c_str());
    }
    foo.show();
    return app.exec();
}
