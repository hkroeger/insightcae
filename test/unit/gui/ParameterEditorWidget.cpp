
#include <QApplication>
#include <QAbstractItemModelTester>
#include <QTreeView>
#include <QMainWindow>
#include <QTimer>
#include <QHeaderView>

#include "cadfeatures/quad.h"
#include "cadparameters/constantvector.h"
#include "iqparametersetmodel.h"
#include "parametereditorwidget.h"
#include "test_pdl.h"

using namespace insight;

class TestPDL_ParameterSet_Visualizer
    : public CADParameterSetModelVisualizer
{
public:
    using CADParameterSetModelVisualizer::CADParameterSetModelVisualizer;

    std::shared_ptr<supplementedInputDataBase> computeSupplementedInput() override
    {
        return std::make_shared<supplementedInputDataFromParameters>(
            ParameterSetInput(this->parameters()).forward<TestPDL::Parameters>(),
            this->workDir_, this->progress_ );
    }

    const supplementedInputDataFromParameters& sp() const
    {
        return dynamic_cast<
            const supplementedInputDataFromParameters&>(
            *this->sid_ );
    }


    void recreateVisualizationElements() override
    {
        addFeature
            (
                "sketch",
                sp().parameters().get<CADSketchParameter>("sketch").geometry(),
                { insight::Wireframe }
            );
    }
};

int main(int argc, char*argv[])
{
    try
    {
        auto tps = TestPDL::defaultParameters();

        // auto &sk=tps->get<CADSketchParameter>("sketch");
        // sk.setScript(
        //     "layer standard\n"
        //     "SketchPoint( 1, -0.401374, 0.523062, layer standard),"
        //     "SketchPoint( 2, 0.277723, 0.523062, layer standard),"
        //     "Line(4, 1, 2, layer standard)"
        //     );



        IQParameterSetModel modelToBeTested( std::move(tps) );

        QAbstractItemModelTester tester(
            &modelToBeTested,
            QAbstractItemModelTester::FailureReportingMode::Fatal);

        if (argc<=1 || std::string(argv[1])!="nogui")
        {
            QApplication app(argc, argv);
            QMainWindow dlg;


            insight::ParameterSet_ValidatorPtr vali;

            auto peditor_=new ParameterEditorWidget(
                &dlg,
                [](QObject* _1, IQParameterSetModel *_2)
                {
                    return new TestPDL_ParameterSet_Visualizer(
                        _1, _2, "", insight::consoleProgressDisplayer);
                },
                [](
                    const std::string&,
                    QObject *,
                    IQCADModel3DViewer *,
                    IQParameterSetModel *
                    ) -> insight::GUIActionList { return {}; },
                vali
                );
            peditor_->setModel(&modelToBeTested);

            dlg.setCentralWidget(peditor_);
            dlg.show();

            // connect(
            //     peditor_, &ParameterEditorWidget::updateSupplementedInputData,
            //     this, &AnalysisForm::onUpdateSupplementedInputData
            //     );



            // QTimer::singleShot(
            //     0,
            //     [&]()
            //     {
            //         tv->scrollToBottom();
            //     });

            QTimer::singleShot(
                0,
                [&]()
                {
                    auto tps2 = TestPDL::defaultParameters();
                    auto &sk=tps2->get<CADSketchParameter>("sketch");
                    sk.setScript(
                        "layer standard\n"
                        "SketchPoint( 0, 0.277723, 0.197252, layer standard),"
                        "SketchPoint( 1, -0.401374, 0.523062, layer standard),"
                        "SketchPoint( 2, 0.277723, 0.523062, layer standard),"
                        "SketchPoint( 3, -0.401374, 0.197252, layer standard),"
                        "Line(4, 1, 2, layer standard),"
                        "Line(5, 2, 0, layer standard),"
                        "Line(6, 0, 3, layer standard),"
                        "Line(7, 3, 1, layer standard),"
                        "FixedPoint( 8, 1, layer standard),"
                        "DistanceConstraint( 9, 1, 2, layer standard),"
                        "DistanceConstraint( 10, 2, 0, layer standard),"
                        "HorizontalConstraint( 11, 4, layer standard),"
                        "HorizontalConstraint( 12, 6, layer standard),"
                        "VerticalConstraint( 13, 5, layer standard),"
                        "VerticalConstraint( 14, 7, layer standard)"
                        );

                    modelToBeTested.resetParameterValues(*tps2);

                });
            return app.exec();
        }
    }
    catch (std::exception& ex)
    {
        std::cerr<<"Failed: "<<ex.what()<<std::endl;
        return -1;
    }
    std::cout<<"Passed"<<std::endl;
    return 0;
}
