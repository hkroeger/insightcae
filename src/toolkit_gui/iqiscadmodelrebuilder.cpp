#include "iqiscadmodelrebuilder.h"

#include <QThread>
#include <mutex>

#include "iqcaditemmodel.h"
#include "iqiscadmodelgenerator.h"
#include "qnamespace.h"



IQISCADModelRebuilder::IQISCADModelRebuilder(
        IQCADItemModel *model,
        QList<IQISCADModelGenerator*> gens,
        QObject* parent )
    : QObject(parent),
      model_(model),
      generators_(gens)
{
    insight::assertion(
        bool(model_),
        "the CAD model must be valid" );

    for( const auto& gen: generators_)
    {
        connectGenerator(gen);
    }

    storeSymbolSnapshot();
}


IQISCADModelRebuilder::~IQISCADModelRebuilder()
{
    removeNonRecreatedSymbols();
    insight::cad::cache.printSummary(std::cout);
}




void IQISCADModelRebuilder::onAddScalar(
        const QString& name,
        insight::cad::ScalarPtr sv )
{
    QMetaObject::invokeMethod(
          qApp,
          std::bind(&IQCADItemModel::addScalar, model_, name.toStdString(), sv)
     );
    symbolsSnapshot_.scalars_.erase(name.toStdString());
}




void IQISCADModelRebuilder::onAddVector(
        const QString& name,
        insight::cad::VectorPtr vv,
        insight::cad::VectorVariableType vt )
{
    if (vt==insight::cad::VectorVariableType::Point)
    {
        QMetaObject::invokeMethod(
              qApp,
              std::bind(&IQCADItemModel::addPoint, model_, name.toStdString(), vv)
        );
        symbolsSnapshot_.points_.erase(name.toStdString());
    }
    else if (vt==insight::cad::VectorVariableType::Direction)
    {
        QMetaObject::invokeMethod(
              qApp,
              std::bind(&IQCADItemModel::addDirection, model_, name.toStdString(), vv)
        );
        symbolsSnapshot_.directions_.erase(name.toStdString());
    }
}




void IQISCADModelRebuilder::onAddFeature(
    const QString& name,
    insight::cad::FeaturePtr smp,
    bool is_component,
    const boost::variant<boost::blank,insight::cad::FeatureVisualizationStyle>& fvs )
{
    QMetaObject::invokeMethod(
          qApp,
          std::bind(
            &IQCADItemModel::addModelstep, model_,
            name.toStdString(), smp,
            is_component, "", fvs )
    );
    symbolsSnapshot_.features_.erase(name.toStdString());
}




void IQISCADModelRebuilder::onAddDatum(
        const QString& name,
        insight::cad::DatumPtr smp,
        bool initialVisibility )
{
    QMetaObject::invokeMethod(
          qApp,
          std::bind(&IQCADItemModel::addDatum, model_, name.toStdString(), smp, initialVisibility)
    );
    symbolsSnapshot_.datums_.erase(name.toStdString());
}




void IQISCADModelRebuilder::onAddEvaluation(
        const QString& name,
        insight::cad::PostprocActionPtr smp,
        bool visible )
{
    QMetaObject::invokeMethod(
          qApp,
          std::bind(&IQCADItemModel::addPostprocAction, model_, name.toStdString(), smp)
    );
    symbolsSnapshot_.postprocactions_.erase(name.toStdString());
}


void IQISCADModelRebuilder::onAddDataset(const QString& name, vtkSmartPointer<vtkDataObject> ds, bool visible)
{
    QMetaObject::invokeMethod(
          qApp,
          std::bind(&IQCADItemModel::addDataset, model_, name.toStdString(), ds)
    );
    symbolsSnapshot_.datasets_.erase(name.toStdString());
}


void IQISCADModelRebuilder::storeSymbolSnapshot()
{
    SymbolsSnapshot sysn;

    auto scalars = model_->scalars();
    for (const auto& s: scalars) { sysn.scalars_.insert(s.first); }

    auto points = model_->points();
    for (const auto& p: points) { sysn.points_.insert(p.first); }

    auto directions = model_->directions();
    for (const auto& d: directions) { sysn.directions_.insert(d.first); }

    auto modelsteps = model_->modelsteps();
    for (const auto& m: modelsteps) { sysn.features_.insert(m.first); }

    auto datums = model_->datums();
    for (const auto& d: datums) { sysn.datums_.insert(d.first); }

    auto ppa = model_->postprocActions();
    for (const auto& p: ppa) { sysn.postprocactions_.insert(p.first); }

    auto ds = model_->datasets();
    for (const auto& d: ds) { sysn.datasets_.insert(d.first); }

    symbolsSnapshot_=sysn;
}



void IQISCADModelRebuilder::connectGenerator(IQISCADModelGenerator *gen)
{
    connect(gen, QOverload<const QString&,insight::cad::ScalarPtr>::of(
                &IQISCADModelGenerator::createdVariable),
            this, &IQISCADModelRebuilder::onAddScalar);
    connect(gen, QOverload<const QString&,insight::cad::VectorPtr,insight::cad::VectorVariableType>::of(
                &IQISCADModelGenerator::createdVariable),
            this, &IQISCADModelRebuilder::onAddVector);
    connect(gen, &IQISCADModelGenerator::createdFeature,
            this, &IQISCADModelRebuilder::onAddFeature);
    connect(gen, &IQISCADModelGenerator::createdDatum,
            this, &IQISCADModelRebuilder::onAddDatum);
    connect(gen, &IQISCADModelGenerator::createdEvaluation,
            this, &IQISCADModelRebuilder::onAddEvaluation );
    connect(gen, &IQISCADModelGenerator::createdDataset,
            this, &IQISCADModelRebuilder::onAddDataset );
}





void IQISCADModelRebuilder::removeNonRecreatedSymbols()
{
    auto sysn = symbolsSnapshot_;

    auto scalars = model_->scalars();
    for (const auto& s: scalars)
    {
        if (sysn.scalars_.find(s.first)!=sysn.scalars_.end())
        {
            model_->removeScalar(s.first);
        }
    }

    auto points = model_->points();
    for (const auto& p: points)
    {
        if (sysn.points_.find(p.first)!=sysn.points_.end())
        {
            model_->removePoint(p.first);
        }
    }

    auto directions = model_->directions();
    for (const auto& d: directions)
    {
        if (sysn.directions_.find(d.first)!=sysn.directions_.end())
        {
            model_->removeDirection(d.first);
        }
    }

    auto modelsteps = model_->modelsteps();
    for (const auto& m: modelsteps)
    {
        if (sysn.features_.find(m.first)!=sysn.features_.end()
                && !model_->isStaticModelStep(m.first) )
        {
            model_->removeModelstep(m.first);
        }
    }

    auto datums = model_->datums();
    for (const auto& d: datums)
    {
        if (sysn.datums_.find(d.first)!=sysn.datums_.end())
        {
            model_->removeDatum(d.first);
        }
    }

    auto ppa = model_->postprocActions();
    for (const auto& p: ppa)
    {
        if (sysn.postprocactions_.find(p.first)!=sysn.postprocactions_.end())
        {
            model_->removePostprocAction(p.first);
        }
    }

    auto ds = model_->datasets();
    for (const auto& d: ds)
    {
        if (sysn.datasets_.find(d.first)!=sysn.datasets_.end())
        {
            model_->removeDataset(d.first);
        }
    }
}

