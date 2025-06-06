
/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <QtGlobal>
#include <QInputDialog>
#include <QColorDialog>

#include "base/exception.h"
#include "base/qt5_helper.h"
#include "iqiscadmodelgenerator.h"
#include "qmodeltree.h"

#include "qvariableitem.h"
#include "qmodelstepitem.h"
#include "qdatumitem.h"
#include "qevaluationitem.h"

#include "base/boost_include.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <boost/random/exponential_distribution.hpp>

#include "iqcaditemmodel.h"

boost::mt19937 boostRenGen;








QModelTreeItem::QModelTreeItem
(
  const QString& name,
  QTreeWidgetItem* parent

)
  : QTreeWidgetItem ( parent ),
    name_ ( name )
{
  setText(COL_NAME, name_);
}


QModelTree* QModelTreeItem::modelTree() const
{
  return dynamic_cast<QModelTree*>(treeWidget());
}

void QModelTreeItem::insertName()
{
  Q_EMIT insertParserStatementAtCursor(name_);
}

void QModelTreeItem::jumpToName()
{
  Q_EMIT jumpTo(name_);
}


QDisplayableModelTreeItem::QDisplayableModelTreeItem
(
  const QString& name,
  bool visible,
  AIS_DisplayMode dm,
  QTreeWidgetItem* parent
)
: QModelTreeItem ( name, parent ),
  shadingMode_(dm)
{
  setCheckState(COL_VIS, visible ? Qt::Checked : Qt::Unchecked);
  setRandomColor();
}

QDisplayableModelTreeItem::~QDisplayableModelTreeItem()
{
//  emit hide(this); // leads to crash
}


bool QDisplayableModelTreeItem::isVisible() const
{
    return (checkState(COL_VIS) == Qt::Checked);
}


bool QDisplayableModelTreeItem::isHidden() const
{
    return (checkState(COL_VIS) == Qt::Unchecked);
}

Handle_AIS_InteractiveObject QDisplayableModelTreeItem::ais(AIS_InteractiveContext& context)
{
  insight::CurrentExceptionContext ec("get AIS interactive object");
  if (ais_.IsNull())
  {
    ais_=createAIS(context);
  }
  return ais_;
}

Handle_AIS_InteractiveObject QDisplayableModelTreeItem::existingAis() const
{
  insight::assertion( !ais_.IsNull(), "Internal error: The interactive representation is not existing yet!");
  return ais_;
}


void QDisplayableModelTreeItem::setShadingMode(AIS_DisplayMode ds)
{
  shadingMode_=ds;
//  if (isVisible())
//    emit setDisplayMode(this, shadingMode_);
}

Quantity_Color QDisplayableModelTreeItem::color() const
{
  return Quantity_Color(r_, g_, b_, Quantity_TOC_RGB);
}

void QDisplayableModelTreeItem::setRandomColor()
{
  boost::random::exponential_distribution<> ed(2);
  r_ = std::max(0., std::min(1., ed(boostRenGen)));
  g_ = std::max(0., std::min(1., ed(boostRenGen)));
  b_ = std::max(0., std::min(1., ed(boostRenGen)));
//  r_=0.5+0.5*( double(rand()) / double(RAND_MAX) );
//  g_=0.5+0.5*( double(rand()) / double(RAND_MAX) );
//  b_=0.5+0.5*( double(rand()) / double(RAND_MAX) );
}

void QDisplayableModelTreeItem::copyDisplayProperties(QDisplayableModelTreeItem* di)
{
  r_ = di->r_;
  g_ = di->g_;
  b_ = di->b_;
  shadingMode_ = di->shadingMode_;
  setCheckState(COL_VIS, di->checkState(COL_VIS));
}

void QDisplayableModelTreeItem::initDisplay()
{
  if (isVisible())
    show();
}


void QDisplayableModelTreeItem::show()
{
  setCheckState(COL_VIS, Qt::Checked);
  Q_EMIT showItem(this);
}


void QDisplayableModelTreeItem::hide()
{
  setCheckState(COL_VIS, Qt::Unchecked);
  if (!ais_.IsNull())
    {
      Q_EMIT hideItem(this);
    }
}

void QDisplayableModelTreeItem::wireframe()
{
  shadingMode_=AIS_WireFrame;
  if (isVisible())
    Q_EMIT setDisplayMode(this, AIS_WireFrame);
}

void QDisplayableModelTreeItem::shaded()
{
  shadingMode_=AIS_Shaded;
  if (isVisible())
    Q_EMIT setDisplayMode(this, AIS_Shaded);
}

void QDisplayableModelTreeItem::onlyThisShaded()
{
  if (QModelTree* mt=modelTree())
    {
      mt->onlyOneShaded(this);
    }
}

void QDisplayableModelTreeItem::randomizeColor()
{
  setRandomColor();
  Q_EMIT setColor(this, color());
}

void QDisplayableModelTreeItem::chooseColor()
{
  QColor c(r_*255., g_*255., b_*255.);
  auto nc = QColorDialog::getColor(c, treeWidget(), "Please select color");
  if (nc.isValid())
  {
    r_=nc.red()/255.;
    g_=nc.green()/255.;
    b_=nc.blue()/255.;
    Q_EMIT setColor(this, color());
  }
}

void QDisplayableModelTreeItem::setResolution()
{
  bool ok;
  double res=QInputDialog::getDouble(treeWidget(), "Set Resolution", "Resolution:", 0.001, 1e-7, 0.1, 7, &ok);
  if (ok)
  {
    Q_EMIT setItemResolution(this, res);
  }
}


void QModelTree::removeModelItem(QTreeWidgetItem* oldi)
{
  QTreeWidgetItem *parent = oldi->parent();
  parent->removeChild(oldi);
  if (QDisplayableModelTreeItem *i = dynamic_cast<QDisplayableModelTreeItem*>(oldi))
    {
      i->hide();
    }
  if (QModelTreeItem *i = dynamic_cast<QModelTreeItem*>(oldi))
    {
      i->deleteLater();
    }
}

void QModelTree::replaceOrAdd(QTreeWidgetItem *parent, QTreeWidgetItem *newi, QTreeWidgetItem* oldi)
{
  if (oldi)
    {
      parent = oldi->parent();
      int itemIndex = parent->indexOfChild(oldi);

      removeModelItem(oldi);

      parent->insertChild(itemIndex, newi);
    }
  else
    {
      parent->addChild(newi);
    }
}


void QModelTree::connectDisplayableItem(QDisplayableModelTreeItem* newf)
{
  connect(newf, &QDisplayableModelTreeItem::showItem,
          this, &QModelTree::showItem);
  connect(newf, &QDisplayableModelTreeItem::hideItem,
          this, &QModelTree::hideItem);
  connect(newf, &QDisplayableModelTreeItem::setDisplayMode,
          this, &QModelTree::setDisplayMode);
  connect(newf, &QDisplayableModelTreeItem::setColor,
          this, &QModelTree::setColor);
  connect(newf, &QDisplayableModelTreeItem::setItemResolution,
          this, &QModelTree::setItemResolution);
  connect(newf, &QDisplayableModelTreeItem::insertParserStatementAtCursor,
          this, &QModelTree::insertParserStatementAtCursor);
  connect(newf, &QDisplayableModelTreeItem::jumpTo,
          this, &QModelTree::jumpTo);
  connect(newf, &QDisplayableModelTreeItem::focus,
          this, &QModelTree::focus);
  connect(newf, &QDisplayableModelTreeItem::unfocus,
          this, &QModelTree::unfocus);
  connect(newf, &QDisplayableModelTreeItem::insertIntoNotebook,
          this, &QModelTree::insertIntoNotebook);
}

QModelTree::QModelTree(QWidget* parent)
  : QTreeWidget(parent)
{
  QStringList heads({"", "", ""});
  heads[QModelTreeItem::COL_NAME]="Symbol Name";
  heads[QModelTreeItem::COL_VALUE]="Value";
  setHeaderLabels( heads );

  //     setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  setMinimumHeight(20);
  setContextMenuPolicy(Qt::CustomContextMenu);

  // handle right clicks
  connect
      (
        this, &QModelTree::customContextMenuRequested,
        this, &QModelTree::showContextMenu
        );

  // handle visible/hide checkbox changes
  connect
      (
        this, &QModelTree::itemChanged,
        this, &QModelTree::onItemChanged
        );
    
  connect
      (
        this, &QModelTree::itemSelectionChanged,
        this, &QModelTree::onItemSelectionChanged
        );

    int COL_NAME=QModelTreeItem::COL_NAME;
    componentfeatures_ = new QTreeWidgetItem( this, QStringList() << "Components" << ""  << "" );
    { QFont f=componentfeatures_->font(COL_NAME); f.setBold(true); componentfeatures_->setFont(COL_NAME, f); }
    componentfeatures_->setFirstColumnSpanned(true);
    componentfeatures_->setExpanded(true);

    features_ = new QTreeWidgetItem( this, QStringList() << "Features"<< ""  << "" );
    { QFont f=features_->font(COL_NAME); f.setBold(true); features_->setFont(COL_NAME, f); }
    features_->setFirstColumnSpanned(true);
    
    scalars_ = new QTreeWidgetItem( this, QStringList() << "Scalar Variables"<< ""  << "" );
    { QFont f=scalars_->font(COL_NAME); f.setBold(true); scalars_->setFont(COL_NAME, f); }
    scalars_->setFirstColumnSpanned(true);
    scalars_->setExpanded(true);

    points_ = new QTreeWidgetItem( this, QStringList() << "Point Variables"<< ""  << "" );
    { QFont f=points_->font(COL_NAME); f.setBold(true); points_->setFont(COL_NAME, f); }
    points_->setFirstColumnSpanned(true);
    points_->setExpanded(true);

    directions_ = new QTreeWidgetItem( this, QStringList() << "Vector Variables"<< ""  << "" );
    { QFont f=directions_->font(COL_NAME); f.setBold(true); directions_->setFont(COL_NAME, f); }
    directions_->setFirstColumnSpanned(true);
    directions_->setExpanded(true);

    datums_ = new QTreeWidgetItem( this, QStringList() << "Datums"<< ""  << "" );
    { QFont f=datums_->font(COL_NAME); f.setBold(true); datums_->setFont(COL_NAME, f); }
    datums_->setFirstColumnSpanned(true);
    datums_->setExpanded(true);

    postprocactions_ = new QTreeWidgetItem( this, QStringList() << "Postprocessing Actions"<< ""  << "" );
    { QFont f=postprocactions_->font(COL_NAME); f.setBold(true); postprocactions_->setFont(COL_NAME, f); }
    postprocactions_->setFirstColumnSpanned(true);
    postprocactions_->setExpanded(true);

}

class SignalBlocker
{

private:
    QObject *object;
    bool alreadyBlocked;

public:
    SignalBlocker(QObject *o)
      : object(o),
        alreadyBlocked(object->signalsBlocked())
    {
        if (!alreadyBlocked)
          {
            object->blockSignals(true);
          }
    }

    ~SignalBlocker()
    {
        if (!alreadyBlocked)
          {
            object->blockSignals(false);
          }
    }

};

QDisplayableModelTreeItem* QModelTree::findFeature(const QString& name, bool is_component)
{
  if (is_component)
  {
    return findItem<QDisplayableModelTreeItem>(componentfeatures_, name);
  }
  else
  {
    return findItem<QDisplayableModelTreeItem>(features_, name);
  }
}





//void QModelTree::addCreatedScalarToSymbolSnapshot(const QString& name,insight::cad::ScalarPtr)
//{
//  symbolsSnapshot_.scalars_.erase(name);
//}

//void QModelTree::addCreatedVectorToSymbolSnapshot(const QString& name,insight::cad::VectorPtr,insight::cad::VectorVariableType t)
//{
//  if (t==insight::cad::VectorVariableType::Point)
//    symbolsSnapshot_.points_.erase(name);
//  else if (t==insight::cad::VectorVariableType::Direction)
//    symbolsSnapshot_.directions_.erase(name);
//}

//void QModelTree::addCreatedFeatureToSymbolSnapshot(const QString& name, insight::cad::FeaturePtr, bool is_component)
//{
//  if (is_component)
//    symbolsSnapshot_.componentfeatures_.erase(name);
//  else
//    symbolsSnapshot_.componentfeatures_.erase(name);
//}

//void QModelTree::addCreatedDatumToSymbolSnapshot(const QString& name, insight::cad::DatumPtr)
//{
//  symbolsSnapshot_.datums_.erase(name);
//}

//void QModelTree::addCreatedPostprocActionToSymbolSnapshot(const QString& name, insight::cad::PostprocActionPtr, bool)
//{
//  symbolsSnapshot_.postprocactions_.erase(name);
//}


void QModelTree::connectGenerator(IQISCADModelGenerator *model)
{
//  connect(model, &IQISCADModelGenerator::beginRebuild,
//          this, &QModelTree::storeSymbolSnapshot );
  connect(model, QOverload<const QString&,insight::cad::ScalarPtr>::of(&IQISCADModelGenerator::createdVariable),
          this, &QModelTree::onAddScalar);
  connect(model, QOverload<const QString&,insight::cad::VectorPtr,insight::cad::VectorVariableType,bool>::of(
                     &IQISCADModelGenerator::createdVariable),
          this, &QModelTree::onAddVector);
  connect(model, &IQISCADModelGenerator::createdFeature,
          this, &QModelTree::onAddFeature);
  connect(model, &IQISCADModelGenerator::createdDatum,
          this, &QModelTree::onAddDatum);
  connect(model, &IQISCADModelGenerator::createdEvaluation,
          this, &QModelTree::onAddEvaluation );

//  connect(model, QOverload<const QString&,insight::cad::ScalarPtr>::of(&IQISCADModelGenerator::createdVariable),
//          this, &QModelTree::addCreatedScalarToSymbolSnapshot);
//  connect(model, QOverload<const QString&,insight::cad::VectorPtr,insight::cad::VectorVariableType>::of(&IQISCADModelGenerator::createdVariable),
//          this, &QModelTree::addCreatedVectorToSymbolSnapshot);
//  connect(model, &IQISCADModelGenerator::createdFeature,
//          this, &QModelTree::addCreatedFeatureToSymbolSnapshot);
//  connect(model, &IQISCADModelGenerator::createdDatum,
//          this, &QModelTree::addCreatedDatumToSymbolSnapshot);
//  connect(model, &IQISCADModelGenerator::createdEvaluation,
//          this, &QModelTree::addCreatedPostprocActionToSymbolSnapshot);

//  connect(model, &IQISCADModelGenerator::removedScalar,
//          this, &QModelTree::onRemoveScalar);
//  connect(model, &IQISCADModelGenerator::removedVector,
//          this, &QModelTree::onRemoveVector);
//  connect(model, &IQISCADModelGenerator::removedFeature,
//          this, &QModelTree::onRemoveFeature);
//  connect(model, &IQISCADModelGenerator::removedDatum,
//          this, &QModelTree::onRemoveDatum);
//  connect(model, &IQISCADModelGenerator::removedEvaluation,
//          this, &QModelTree::onRemoveEvaluation);

//  connect(model, &IQISCADModelGenerator::finishedRebuild,
//          this, &QModelTree::removeNonRecreatedSymbols);

}


void QModelTree::disconnectGenerator(IQISCADModelGenerator* model)
{
  this->disconnect(model, 0, this, 0);
}


void QModelTree::onAddScalar(const QString& name, insight::cad::ScalarPtr sv)
{
  SignalBlocker b(this);
  QScalarVariableItem* old = findItem<QScalarVariableItem>(scalars_, name);
  replaceOrAdd(scalars_, new QScalarVariableItem(name, sv->value(), scalars_), old);
}

void QModelTree::onAddVector(const QString& name, insight::cad::VectorPtr vv, insight::cad::VectorVariableType vt)
{
  QTreeWidgetItem* base;
  switch (vt)
  {
    case insight::cad::VectorVariableType::Point: base=points_; break;
    case insight::cad::VectorVariableType::Direction: base=directions_; break;
  }

  QVectorVariableItem *newf, *old;
  {
    SignalBlocker b(this);
    old = findItem<QVectorVariableItem>(base, name);
    newf = new QVectorVariableItem(name, vv->value(), base);
    if (old) newf->copyDisplayProperties(old);
    connectDisplayableItem(newf);
  }
  replaceOrAdd(base, newf, old);
  newf->initDisplay();
}

void QModelTree::onAddFeature(
    const QString& name, insight::cad::FeaturePtr smp,
    bool is_component,
    const boost::variant<boost::blank,insight::cad::FeatureVisualizationStyle>& fvs)
{
  insight::CurrentExceptionContext ex("adding feature "+name.toStdString()+" to model tree");

  QFeatureItem *newf, *old;
  QTreeWidgetItem* cat;
  {
    SignalBlocker b(this);

    AIS_DisplayMode ads;
    if (is_component)
      {
        cat=componentfeatures_;
        ads=AIS_Shaded;
      }
    else
      {
        cat=features_;
        ads=AIS_WireFrame;
      }

      if (const auto* ds=boost::get<insight::cad::FeatureVisualizationStyle>(&fvs))
          if (const auto* eds = boost::get<insight::DatasetRepresentation>(&ds->style)) //override
            ads = *eds==insight::Surface?AIS_Shaded:AIS_WireFrame;

    old = findItem<QFeatureItem>(cat, name);
    newf = new QFeatureItem(name, smp, is_component, cat, is_component);
    if (old)
    {
      insight::dbg()<<"copy display properties of old "<<name.toStdString()<<std::endl;
      newf->copyDisplayProperties(old);
    }
    else
    {
      insight::dbg()<<"set display properties of "<<name.toStdString()<<" as "<<ads<<std::endl;
      newf->setShadingMode(ads);
    }

    connectDisplayableItem(newf);
    connect(newf, &QFeatureItem::addEvaluation,
            this, &QModelTree::onAddEvaluation);
  }
  replaceOrAdd(cat, newf, old);

  insight::dbg()<<"initDisplay of "<<name.toStdString()<<std::endl;
  newf->initDisplay();
}


void QModelTree::onAddDatum(const QString& name, insight::cad::DatumPtr smp)
{
  QDatumItem *old, *newf;
  {
    SignalBlocker b(this);
    old = findItem<QDatumItem>(datums_, name);
    newf = new QDatumItem(name, smp, datums_);
    if (old) newf->copyDisplayProperties(old);
    connectDisplayableItem(newf);
  }
  replaceOrAdd(datums_, newf, old);
  newf->initDisplay();
}

void QModelTree::onAddEvaluation(const QString& name, insight::cad::PostprocActionPtr smp, bool visible)
{
  insight::dbg()<<"start onAddEvaluation"<<std::endl;
  QEvaluationItem *old, *newf;
  {
    SignalBlocker b(this);
    old = findItem<QEvaluationItem>(postprocactions_, name);
    newf = new QEvaluationItem(name, smp, postprocactions_, visible);
    if (old) newf->copyDisplayProperties(old);
    connectDisplayableItem(newf);
  }
  insight::dbg()<<"replaceOrAdd"<<std::endl;
  replaceOrAdd(postprocactions_, newf, old);
  insight::dbg()<<"init display"<<std::endl;
  newf->initDisplay();
  insight::dbg()<<"finish onAddEvaluation"<<std::endl;
}


void QModelTree::onRemoveScalar      (const QString& sn)
{
  SignalBlocker(this);
  if (QScalarVariableItem* item = findItem<QScalarVariableItem>(scalars_, sn))
    removeModelItem(item);;
}

void QModelTree::onRemoveVector      (const QString& sn, insight::cad::VectorVariableType vt)
{
  SignalBlocker(this);

  QTreeWidgetItem* base;
  switch (vt)
  {
    case insight::cad::VectorVariableType::Point: base=points_; break;
    case insight::cad::VectorVariableType::Direction: base=directions_; break;
  }

  if (QVectorVariableItem* item = findItem<QVectorVariableItem>(base, sn))
    base->removeChild(item);
}

void QModelTree::onRemoveFeature     (const QString& sn)
{
  SignalBlocker(this);
  if (QFeatureItem* item = findItem<QFeatureItem>(features_, sn))
    removeModelItem(item);
  else if (QFeatureItem* item = findItem<QFeatureItem>(componentfeatures_, sn))
    removeModelItem(item);
}

void QModelTree::onRemoveDatum       (const QString& sn)
{
  SignalBlocker(this);
  if (QDatumItem* item = findItem<QDatumItem>(datums_, sn))
    removeModelItem(item);
}

void QModelTree::onRemoveEvaluation  (const QString& sn)
{
  SignalBlocker(this);
  if (QEvaluationItem* item = findItem<QEvaluationItem>(postprocactions_, sn))
    removeModelItem(item);
}

void QModelTree::onItemChanged( QTreeWidgetItem *item, int)
{
    if (QDisplayableModelTreeItem *msi =dynamic_cast<QDisplayableModelTreeItem*>(item))
    {
        if (msi->isVisible()) msi->show();
        if (msi->isHidden()) msi->hide();
    }
}

void QModelTree::onItemSelectionChanged()
{
  QTreeWidgetItem *item = currentItem();
  if (auto * m = dynamic_cast<QFeatureItem*>(item))
  {
    Q_EMIT focus(m->existingAis());
  }
}

void QModelTree::focusOutEvent(QFocusEvent */*event*/)
{
  Q_EMIT unfocus();
}

void QModelTree::getFeatureNames(std::set<std::string>& featnames) const
{
  featnames.clear();
  for (int i=0; i<features_->childCount(); i++)
  {
      if ( QFeatureItem *qmsi=dynamic_cast<QFeatureItem*>(features_->child(i)) )
      {
          featnames.insert( qmsi->text( QModelTreeItem::COL_NAME ).toStdString() );
      }
  }
  for (int i=0; i<componentfeatures_->childCount(); i++)
  {
      if ( QFeatureItem *qmsi=dynamic_cast<QFeatureItem*>(componentfeatures_->child(i)) )
      {
          featnames.insert( qmsi->text( QModelTreeItem::COL_NAME ).toStdString() );
      }
  }
}

void QModelTree::getDatumNames(std::set<std::string>& datumnames) const
{
  datumnames.clear();
  for (int i=0; i<datums_->childCount(); i++)
  {
      if ( QDatumItem *qmsi=dynamic_cast<QDatumItem*>(datums_->child(i)) )
      {
          datumnames.insert( qmsi->text( QModelTreeItem::COL_NAME ).toStdString() );
      }
  }
}

void QModelTree::onClear()
{
    componentfeatures_->takeChildren();
    scalars_->takeChildren();
    points_->takeChildren();
    directions_->takeChildren();
    features_->takeChildren();
    datums_->takeChildren();
    postprocactions_->takeChildren();
}


void QModelTree::setUniformDisplayMode(const AIS_DisplayMode AM)
{
    for (int i=0; i<features_->childCount(); i++)
    {
        if (QFeatureItem *msi =dynamic_cast<QFeatureItem*>(features_->child(i)))
        {
            if (AM==AIS_WireFrame)
                msi->wireframe();
            else if (AM==AIS_Shaded)
                msi->shaded();
        }
    }
}

void QModelTree::resetViz()
{
    for (int i=0; i<features_->childCount(); i++)
    {
        if ( QFeatureItem *qmsi=dynamic_cast<QFeatureItem*>(features_->child(i)) )
        {
            qmsi->hide();
            qmsi->wireframe();
        }
        if ( QFeatureItem *qmsi=dynamic_cast<QFeatureItem*>(componentfeatures_->child(i)) )
        {
            qmsi->show();
            qmsi->shaded();
        }
    }
}


void QModelTree::onlyOneShaded(QDisplayableModelTreeItem* si)
{
  QTreeWidgetItem *items[]={features_, componentfeatures_};
  for (int j=0; j<2; j++)
    {
      QTreeWidgetItem* pn =items[j];
      for (int i=0; i<pn->childCount(); i++)
      {
          if ( QDisplayableModelTreeItem *qmsi=dynamic_cast<QDisplayableModelTreeItem*>(pn->child(i)) )
          {
              if (qmsi==si)
                {
                  qmsi->shaded();
                }
              else
                {
                  qmsi->wireframe();
                }
          }
      }
    }
}

void QModelTree::allShaded()
{
  setUniformDisplayMode(AIS_Shaded);
}

void QModelTree::allWireframe()
{
  setUniformDisplayMode(AIS_WireFrame);
}



void QModelTree::showContextMenu(const QPoint &p)
{
    QModelTreeItem * mi=dynamic_cast<QModelTreeItem*>(itemAt(p));
    if (mi)
    {
        mi->showContextMenu(this->mapToGlobal(p));
    }
}

