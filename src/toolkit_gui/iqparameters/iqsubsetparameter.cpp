#include "iqsubsetparameter.h"

#include "iqparametersetmodel.h"

#include "base/tools.h"

#include <QFileDialog>

defineType(IQSubsetParameter);
addToFactoryTable(IQParameter, IQSubsetParameter);

IQSubsetParameter::IQSubsetParameter(
    QObject *parent,
    IQParameterSetModel* psmodel,
    const QString &name,
    insight::Parameter &parameter,
    const insight::ParameterSet &defaultParameterSet
    )
: IQParameter(parent, psmodel, name, parameter, defaultParameterSet)
{}



void IQSubsetParameter::populateContextMenu(QMenu *cm)
{
    auto *saveAction = new QAction("Save to file...");
    cm->addAction(saveAction);
    auto *loadAction = new QAction("Load from file...");
    cm->addAction(loadAction);

//    auto *iq = static_cast<IQParameter*>(index.internalPointer());

    QObject::connect(saveAction, &QAction::triggered, this,
                     [this]()
         {
        auto fn = QFileDialog::getSaveFileName(nullptr, "Save subset contents", QString(), "(*.isp)");
            if (!fn.isEmpty())
            {
            auto &iqp = dynamic_cast<const insight::SubsetParameter&>(
                this->parameter() );
                iqp.saveToFile(insight::ensureFileExtension(fn.toStdString(), "isp"));
            }
         }
    );


    QObject::connect(loadAction, &QAction::triggered, this,
         [this]()
         {
             auto fn = QFileDialog::getOpenFileName(nullptr, "Load subset contents", QString(), "(*.isp)");
             if (!fn.isEmpty())
             {
                 auto &iqp = dynamic_cast<insight::SubsetParameter&>(
                     this->parameterRef() );
                 iqp.readFromFile(insight::ensureFileExtension(fn.toStdString(), "isp"));

//                 model->notifyParameterChange(index, true);
             }
         }
    );
}
