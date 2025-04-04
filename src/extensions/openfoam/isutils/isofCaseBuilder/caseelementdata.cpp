#include "caseelementdata.h"
#include "parametereditorwidget.h"

#include <QApplication>



CaseElementData::CaseElementData(
    const std::string& type_name,
    insight::MultiCADParameterSetVisualizer::SubVisualizerList& mvl,
    MultivisualizationGenerator* visGen,
    IQParameterSetModel *psm,
    QObject *parent )

    : QObject(parent),
    type_name_(type_name),
    mv_(mvl),
    mvGen_(visGen),
    curp_(psm)
{
    curp_->setParent(this);

    connect(
        this, &CaseElementData::visualizationUpdateRequired,
        std::bind(&MultivisualizationGenerator::rebuildVisualization, visGen) );

    connectParameterSetChanged(
        curp_,
        this, &CaseElementData::visualizationUpdateRequired );

    // call when constructed and virtual function will work:
    QMetaObject::invokeMethod(
        qApp,
        [this]()
        {
            auto vf = getVisualizerFactoryFunction();
            if (vf)
            {
                mv_.insert({
                    this,
                    insight::MultiCADParameterSetVisualizer::IndividualVisualizer{
                        curp_, vf  }
                });

                QMetaObject::invokeMethod(
                    qApp,
                    std::bind(&MultivisualizationGenerator::rebuildVisualization, mvGen_),
                    Qt::QueuedConnection // execute not yet
                    );
            }
        },
        Qt::QueuedConnection // execute not yet but after constructors are all finished
        );
}




CaseElementData::~CaseElementData()
{
    if (mv_.count(this))
    {
        mv_.erase(this);

        QMetaObject::invokeMethod(
            qApp,
            std::bind(&MultivisualizationGenerator::rebuildVisualization, mvGen_),
            Qt::QueuedConnection // execute not yet but after destructors are all finished
            );
    }
}

IQParameterSetModel *CaseElementData::parameterSetModel() const
{
    return curp_;
}

insight::MultiCADParameterSetVisualizer::SubVisualizerList&
CaseElementData::multiVisualizer() const
{
    return mv_;
}

