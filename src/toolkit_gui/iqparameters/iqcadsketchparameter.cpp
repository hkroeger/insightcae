#include "iqcadsketchparameter.h"

#include "boost/signals2/connection.hpp"
#include "boost/signals2/shared_connection_block.hpp"
#include "constrainedsketch.h"
#include "iqparametersetmodel.h"
#include "iqcadmodel3dviewer.h"
#include "iqcaditemmodel.h"
#include "qtextensions.h"

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
        IQHierarchicalDataModel* hdmodel,
        insight::hierarchicalData::Element* element
        )
    : IQSpecializedParameter<insight::CADSketchParameter>(
          parent, hdmodel, element)
{}


void IQCADSketchParameter::connectSignals()
{
    IQParameter::connectSignals();

    disconnectAtEOL(
        parameterRef().childValueChanged.connect(
            [this]() {
                auto blocker = block_all();
                model()->notifyElementChange( *this );
            }
        )
    );
}




QVariant IQCADSketchParameter::value() const
{
    return "(sketch)";
}




QVBoxLayout* IQCADSketchParameter::populateEditControls(
    QWidget* editControlsContainer,
    IQCADModel3DViewer *viewer)
{

    auto* layout = IQParameter::populateEditControls(editControlsContainer, viewer);


    auto *teScript = new QTextEdit(editControlsContainer);
    auto updateScriptEdit = [this,teScript]() {
        teScript->document()->setPlainText(
            QString::fromStdString(parameter().script()));
    };

    updateScriptEdit();

    ::disconnectAtEOL(
        teScript,
        (*this)->valueChanged.connect(
            updateScriptEdit
            )
        );

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
            if (auto editctrl = viewer->editSketchParameter(
                    this->parameter().path() ))
            {
                this->setControlsEnabled(false);
                editctrl->additionalCleanup=[this]() {
                    this->setControlsEnabled(true); };
            }
        };
        connect(edit, &QPushButton::pressed, editFunction);
    }

    return layout;
}


/*
IQCADSketchDelegate::IQCADSketchDelegate(QObject * parent)
    : QStyledItemDelegate(parent)
{}

QWidget * IQCADSketchDelegate::createEditor(
    QWidget * parent,
    const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    auto *iqp=static_cast<IQParameter*>(
        index.siblingAtColumn(IQParameterSetModel::iqParamCol)
            .data().value<void*>() );
    auto&p = *dynamic_cast<insight::CADSketchParameter*>(iqp->get());

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
    return nullptr;
}

void IQCADSketchDelegate::setEditorData(
    QWidget *editor,
    const QModelIndex &index ) const
{}

void IQCADSketchDelegate::setModelData(
    QWidget *editor,
    QAbstractItemModel *model,
    const QModelIndex &index ) const
{}
*/
