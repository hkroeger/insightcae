#include "iqsubsetparameter.h"

#include "iqparametersetmodel.h"

#include "base/tools.h"

#include <QFileDialog>

#include "qtextensions.h"

defineType(IQSubsetParameter);
addToFactoryTable(IQParameter, IQSubsetParameter);

IQSubsetParameter::IQSubsetParameter(
    QObject *parent,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element
    )
: IQSpecializedParameter<insight::ParameterSet>(
          parent, hdmodel, element)
{}



void IQSubsetParameter::populateContextMenu(
    QMenu *cm, IQCADModel3DViewer *viewer )
{
    auto *saveAction = new QAction("Save to file...");
    cm->addAction(saveAction);
    auto *loadAction = new QAction("Load from file...");
    cm->addAction(loadAction);


    QObject::connect(
        saveAction, &QAction::triggered, this,
        [this]()
        {
            if (auto fn = getFileName(
                nullptr, "Save subset contents",
                GetFileMode::Save,
                    {{ "isp", "Subset contents" }} ) )
            {
                parameter().saveToFile(fn);
            }
        }
        );


    QObject::connect(
        loadAction, &QAction::triggered, this,
        [this]()
        {
            if (auto fn = getFileName(
                nullptr, "Load subset contents",
                GetFileMode::Open,
                {{ "isp", "Subset parameter contents" }})
                )
            {
                parameterRef().readFromFile(fn);
            }
        }
        );
}

QVariant IQSubsetParameter::value() const
{
    return QVariant();
}
