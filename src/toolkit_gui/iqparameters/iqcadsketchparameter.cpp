#include "iqcadsketchparameter.h"

#include "boost/signals2/connection.hpp"
#include "boost/signals2/shared_connection_block.hpp"
#include "constrainedsketch.h"
#include "iqparametersetmodel.h"
#include "iqcadmodel3dviewer.h"
#include "iqcaditemmodel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QInputDialog>

#include "base/tools.h"
#include "base/parameters/selectablesubsetparameter.h"

defineType(IQCADSketchParameter);
addToFactoryTable(IQParameter, IQCADSketchParameter);




IQCADSketchParameter::IQCADSketchParameter
    (
        QObject* parent,
        IQParameterSetModel* psmodel,
        insight::Parameter* parameter,
        const insight::ParameterSet& defaultParameterSet
        )
    : IQSpecializedParameter<insight::CADSketchParameter>(
          parent, psmodel, parameter, defaultParameterSet)
{}


void IQCADSketchParameter::connectSignals()
{
    IQParameter::connectSignals();

    disconnectAtEOL(
        parameterRef().childValueChanged.connect(
            [this]() {
                auto blocker = block_all();
                model()->notifyParameterChange( *this );
            }
            )
        );
}




QString IQCADSketchParameter::valueText() const
{
    return "sketch";
}




QVBoxLayout* IQCADSketchParameter::populateEditControls(
    QWidget* editControlsContainer,
    IQCADModel3DViewer *viewer)
{

    auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);


    auto *teScript = new QTextEdit(editControlsContainer);
    teScript->document()->setPlainText(QString::fromStdString(parameter().script()));
    layout->addWidget(teScript);


    QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
    layout->addWidget(apply);

    if (viewer)
    {
        QPushButton* edit=new QPushButton("&Edit sketch", editControlsContainer);
        layout->addWidget(edit);

        auto applyFunction = [=]()
        {
            auto&p = this->parameterRef();
            p.setUpdateValueSignalBlockage(true);
            p.setScript( teScript->document()->toPlainText().toStdString() );
            p.setUpdateValueSignalBlockage(false);
            p.triggerValueChanged();
        };
        connect(apply, &QPushButton::pressed, applyFunction);


        auto editFunction = [=]()
        {
            auto&p = this->parameterRef();

            auto sk = p.featureGeometryRef();

            viewer->editSketch(

                *sk,

                p.entityProperties(),
                p.presentationDelegateKey(),

                [this,sk,teScript](insight::cad::ConstrainedSketchPtr accSk) // on accept
                {
                    auto& tp = this->parameterRef();

                    {
                        auto blocker{parameterRef().blockUpdateValueSignal()};
                        *sk = *accSk;

                        std::ostringstream os;
                        sk->generateScript(os);

                        tp.setScript(os.str());
                        tp.featureGeometry(); //trigger rebuild
                    }

                    tp.triggerValueChanged();

                    teScript->document()->setPlainText(
                        QString::fromStdString(tp.script()) );
                },

                [](insight::cad::ConstrainedSketchPtr) {} // on cancel: just nothing to do

                );
        };
        connect(edit, &QPushButton::pressed, editFunction);
    }

    return layout;
}
