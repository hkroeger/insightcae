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
        const QString& name,
        insight::Parameter& parameter,
        const insight::ParameterSet& defaultParameterSet
        )
    : IQParameter(parent, psmodel, name, parameter, defaultParameterSet)
{
}


void IQCADSketchParameter::connectSignals()
{
    IQParameter::connectSignals();

    auto& sp=dynamic_cast<insight::CADSketchParameter&>(parameterRef());
    disconnectAtEOL(
        sp.childValueChanged.connect(
            [this]() {
                auto blocker = block_all();
                model()->notifyParameterChange(
                    path().toStdString(),
                    true
                    );
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
    const auto&p = dynamic_cast<const insight::CADSketchParameter&>(parameter());

    auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);


    auto *teScript = new QTextEdit(editControlsContainer);
    teScript->document()->setPlainText(QString::fromStdString(p.script()));
    layout->addWidget(teScript);


    QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
    layout->addWidget(apply);

    QPushButton* edit=new QPushButton("&Edit sketch", editControlsContainer);
    layout->addWidget(edit);

    auto applyFunction = [=]()
    {
        auto&p = dynamic_cast<insight::CADSketchParameter&>(this->parameterRef());
        p.setUpdateValueSignalBlockage(true);
        p.setScript( teScript->document()->toPlainText().toStdString() );
        p.setUpdateValueSignalBlockage(false);
        p.triggerValueChanged();
    };
    connect(apply, &QPushButton::pressed, applyFunction);

    auto editFunction = [=]()
    {
        auto&p = dynamic_cast<insight::CADSketchParameter&>(
            this->parameterRef());

        auto sk = p.featureGeometryRef();

        viewer->editSketch(
            *sk,
            p.defaultGeometryParameters(),

#warning replace!!
            [](
                const insight::ParameterSet& seps,
                vtkProperty* actprops)
            {
                if (seps.size()>0)
                {
                    auto &selp = seps.get<insight::SelectableSubsetParameter>("type");
                    if (selp.selection()=="wall")
                    {
                        auto c = QColorConstants::Black;
                        actprops->SetColor(
                            c.redF(),
                            c.greenF(),
                            c.blueF() );
                        actprops->SetLineWidth(3);
                    }
                    else if (selp.selection()=="window")
                    {
                        auto c = QColorConstants::DarkYellow;
                        actprops->SetColor(
                            c.redF(),
                            c.greenF(),
                            c.blueF() );
                        actprops->SetLineWidth(4);
                    }
                    else if (selp.selection()=="door")
                    {
                        auto c = QColorConstants::DarkMagenta;
                        actprops->SetColor(
                            c.redF(),
                            c.greenF(),
                            c.blueF() );
                        actprops->SetLineWidth(4);
                    }
                }
                else
                {
                    auto c = QColorConstants::DarkCyan;
                    actprops->SetColor(
                        c.redF(),
                        c.greenF(),
                        c.blueF() );
                    actprops->SetLineWidth(2);
                }
            },

            [this,sk,teScript](insight::cad::ConstrainedSketchPtr accSk) // on accept
            {
                auto& tp = dynamic_cast<insight::CADSketchParameter&>(this->parameterRef());

                {
                    auto blocker = parameterRef().blockUpdateValueSignal();
                    *sk = *accSk;

                    std::ostringstream os;
                    sk->generateScript(os);

                    tp.setScript(os.str());
                }

                tp.triggerValueChanged();

                teScript->document()->setPlainText(
                    QString::fromStdString(tp.script()) );
            },

            [](insight::cad::ConstrainedSketchPtr) {} // on cancel: just nothing to do

            );
    };
    connect(edit, &QPushButton::pressed, editFunction);

    return layout;
}
