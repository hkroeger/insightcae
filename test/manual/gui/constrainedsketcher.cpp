#include <locale>
#include <QLocale>
#include <QDir>
#include <QSplashScreen>
#include <QMainWindow>
#include <QToolBar>
#include <memory>

#include "constrainedsketch.h"
#include "insightcaeapplication.h"
#include "qinsighterror.h"

#include "base/exception.h"
#include "base/linearalgebra.h"
#include "base/parameters/simpleparameter.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqvtkconstrainedsketcheditor.h"


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
            m.model()->lookupDatum("XY"), *insight::cad::noParametersDelegate
        );

    class PD : public insight::cad::ConstrainedSketchParametersDelegate
    {
    public:
        void
        changeDefaultParameters(insight::cad::ConstrainedSketchEntity& e) const override
        {
            auto deflGeoP=insight::ParameterSet::create();
            deflGeoP->insert("value", std::make_unique<insight::DoubleParameter>(1.33, ""));
            e.changeDefaultParameters(*deflGeoP);
        }
    };

    QObject::connect(ska, &QAction::triggered, ska,
        [&]()
        {
            v->editSketch(
                *sketch,
                std::make_shared<PD>(),
                defaultGUIConstrainedSketchPresentationDelegate,
                [](insight::cad::ConstrainedSketchPtr editedSk){
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
