#include <QtGui/QApplication>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "base/exception.h"
#include "iscadapplication.h"

#include <QMainWindow>

int main(int argc, char** argv)
{
    ISCADApplication app(argc, argv);
    ISCADMainWindow w;
    if (argc>1) 
      w.loadFile(argv[1]);
    w.show();
    return app.exec();
}