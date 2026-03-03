#include <locale>
#include <QLocale>
#include <QDir>
#include <QSplashScreen>
#include <QMainWindow>
#include <QToolBar>
#include <memory>
#include <sstream>

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

    auto pd = std::make_shared<PD>();

    {
        std::istringstream is(
            "layer standard\n"
            "SketchPoint( 1, 195.677, 37.3457, layer standard),\n"
            "FixedPoint( 0, 1, layer standard),\n"
            "SketchPoint( 2, 215.105, 37.2737, layer standard),\n"
            "FixedPoint( 3, 2, layer standard),\n"
            "SketchPoint( 4, 224.137, 37.193, layer standard),\n"
            "FixedPoint( 5, 4, layer standard)");
        sketch->readFromStream(is, *pd);
    }

    QObject::connect(ska, &QAction::triggered, ska,
        [&]()
        {
            v->editSketch(
                *sketch,
                pd,
                defaultGUIConstrainedSketchPresentationDelegate,
                [&](insight::cad::ConstrainedSketchPtr editedSk){
                    // do nothing, just discard
                    // clone once
                    string scr;
                    for (int i=0; i<3; ++i)
                    {
                        std::cout<<"round "<<i<<std::endl;

                        std::ostringstream so;
                        editedSk->generateScript(so);
                        scr=so.str();

                        std::cout<<" skript: \n\"\"\"\n"<<scr<<"\"\"\""<<std::endl;

                        editedSk.reset();

                        editedSk =
                            insight::cad::ConstrainedSketch::create(
                                m.model()->lookupDatum("XY"),
                            *insight::cad::noParametersDelegate
                                );
                        std::istringstream is(scr);
                        editedSk->readFromStream(
                            is, *insight::cad::noParametersDelegate
                            );
                    }
                }
            );
        }
    );

    v->setModel(&m);
    w.setCentralWidget(v);
    w.show();
    return app.exec();
}
