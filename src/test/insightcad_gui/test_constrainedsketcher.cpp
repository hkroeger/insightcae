#include <locale>
#include <QLocale>
#include <QDir>
#include <QSplashScreen>
#include <QMainWindow>
#include <QToolBar>

#include "insightcaeapplication.h"
#include "qinsighterror.h"

#include "base/exception.h"
#include "base/linearalgebra.h"

#include "iqvtkcadmodel3dviewer.h"


using namespace boost;
using namespace std;

int main(int argc, char *argv[])
{
    insight::UnhandledExceptionHandling ueh;
    insight::GSLExceptionHandling gsl_errtreatment;


    InsightCAEApplication app(argc, argv, "InsightCAE GUI Test");
    std::locale::global(std::locale::classic());
    QLocale::setDefault(QLocale::C);

    QMainWindow w;
    IQCADItemModel m;
    auto v = new IQVTKCADModel3DViewer;

    auto atb=v->addToolBar("Test actions");
    auto ska = atb->addAction("Sketch");

    auto sketch =
        insight::cad::ConstrainedSketch::create(
            m.model()->lookupDatum("XY")
        );


    QObject::connect(ska, &QAction::triggered, ska,
        [&]()
        {
            v->editSketch(
                sketch,
                insight::ParameterSet(),
                [](const insight::ParameterSet&, vtkProperty* actprops)
                {
                    auto sec = QColorConstants::DarkCyan;
                    actprops->SetColor(
                        sec.redF(),
                        sec.greenF(),
                        sec.blueF() );
                    actprops->SetLineWidth(2);
                },
                [](){
                    // do nothing, just discard
                }
            );
        }
    );

    v->setModel(&m);
    w.setCentralWidget(v);
    w.show();
    return app.exec();
}
