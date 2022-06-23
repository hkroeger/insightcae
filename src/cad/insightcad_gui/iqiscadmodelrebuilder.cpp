#include "iqiscadmodelrebuilder.h"

#include "iqcaditemmodel.h"
#include "iqiscadmodelgenerator.h"



IQISCADModelRebuilder::IQISCADModelRebuilder(
        IQCADItemModel *model,
        QList<IQISCADModelGenerator*> gens,
        QObject* parent )
    : QObject(parent),
      model_(model),
      generators_(gens)
{
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
        insight::cad::ScalarPtr sv)
{
    model_->addScalar(name.toStdString(), sv);
    symbolsSnapshot_.scalars_.erase(name.toStdString());
}




void IQISCADModelRebuilder::onAddVector(
        const QString& name,
        insight::cad::VectorPtr vv,
        insight::cad::VectorVariableType vt)
{
    if (vt==insight::cad::VectorVariableType::Point)
    {
        model_->addPoint(name.toStdString(), vv);
        symbolsSnapshot_.points_.erase(name.toStdString());
    }
    else if (vt==insight::cad::VectorVariableType::Direction)
    {
        model_->addDirection(name.toStdString(), vv);
        symbolsSnapshot_.directions_.erase(name.toStdString());
    }
}




void IQISCADModelRebuilder::onAddFeature(
        const QString& name,
        insight::cad::FeaturePtr smp,
        bool is_component,
        boost::variant<boost::blank,AIS_DisplayMode> ds )
{
    if (is_component)
    {
        model_->addComponent(name.toStdString(), smp);
    }
    else
    {
        model_->addModelstep(name.toStdString(), smp);
    }
    symbolsSnapshot_.features_.erase(name.toStdString());
}




void IQISCADModelRebuilder::onAddDatum(
        const QString& name,
        insight::cad::DatumPtr smp )
{
    model_->addDatum(name.toStdString(), smp);
    symbolsSnapshot_.datums_.erase(name.toStdString());
}




void IQISCADModelRebuilder::onAddEvaluation(
        const QString& name,
        insight::cad::PostprocActionPtr smp,
        bool visible )
{
    model_->addPostprocAction(name.toStdString(), smp);
    symbolsSnapshot_.postprocactions_.erase(name.toStdString());
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

  symbolsSnapshot_=sysn;
}



void IQISCADModelRebuilder::connectGenerator(IQISCADModelGenerator *gen)
{
    connect(gen, QOverload<const QString&,insight::cad::ScalarPtr>::of(&IQISCADModelGenerator::createdVariable),
            this, &IQISCADModelRebuilder::onAddScalar);
    connect(gen, QOverload<const QString&,insight::cad::VectorPtr,insight::cad::VectorVariableType>::of(&IQISCADModelGenerator::createdVariable),
            this, &IQISCADModelRebuilder::onAddVector);
    connect(gen, &IQISCADModelGenerator::createdFeature,
            this, &IQISCADModelRebuilder::onAddFeature);
    connect(gen, &IQISCADModelGenerator::createdDatum,
            this, &IQISCADModelRebuilder::onAddDatum);
    connect(gen, &IQISCADModelGenerator::createdEvaluation,
            this, &IQISCADModelRebuilder::onAddEvaluation );
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
#warning remove point
        }
    }

    auto directions = model_->directions();
    for (const auto& d: directions)
    {
        if (sysn.directions_.find(d.first)!=sysn.directions_.end())
        {
#warning remove vector
        }
    }

    auto modelsteps = model_->modelsteps();
    for (const auto& m: modelsteps)
    {
        if (sysn.features_.find(m.first)!=sysn.features_.end())
        {
#warning remove feature
        }
    }

    auto datums = model_->datums();
    for (const auto& d: datums)
    {
        if (sysn.datums_.find(d.first)!=sysn.datums_.end())
        {
#warning remove datum
        }
    }

    auto ppa = model_->postprocActions();
    for (const auto& p: ppa)
    {
        if (sysn.postprocactions_.find(p.first)!=sysn.postprocactions_.end())
        {
#warning remove postprocaction
        }
    }

}

