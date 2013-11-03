#include <QtGui/QApplication>
#include "workbench.h"


int main(int argc, char** argv)
{
    WorkbenchApplication app(argc, argv);
    workbench foo;
    foo.show();
    return app.exec();
}
