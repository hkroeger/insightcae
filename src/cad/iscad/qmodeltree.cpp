
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

#include "qmodeltree.h"

#include "qvariableitem.h"
#include "qmodelstepitem.h"
#include "qdatumitem.h"
#include "qevaluationitem.h"

#include "base/boost_include.h"

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


void QModelTreeItem::insertName()
{
  emit insertParserStatementAtCursor(name_);
}




QDisplayableModelTreeItem::QDisplayableModelTreeItem
(
  const QString& name,
  bool visible,
  QTreeWidgetItem* parent
)
: QModelTreeItem ( name, parent )
{
    setCheckState(COL_VIS, visible ? Qt::Checked : Qt::Unchecked);
}




bool QDisplayableModelTreeItem::isVisible() const
{
    return (checkState(COL_VIS) == Qt::Checked);
}


bool QDisplayableModelTreeItem::isHidden() const
{
    return (checkState(COL_VIS) == Qt::Unchecked);
}


Quantity_Color QDisplayableModelTreeItem::color() const
{
  return Quantity_Color(r_, g_, b_, Quantity_TOC_RGB);
}

void QDisplayableModelTreeItem::show()
{
  setCheckState(COL_VIS, Qt::Checked);
  emit show(this);
}


void QDisplayableModelTreeItem::hide()
{
  setCheckState(COL_VIS, Qt::Unchecked);
  emit hide(this);
}




void QModelTree::replaceOrAdd(QTreeWidgetItem *parent, QTreeWidgetItem *newi, QTreeWidgetItem* oldi)
{
  if (oldi)
    {
      parent = oldi->parent();
      int itemIndex = parent->indexOfChild(oldi);
      parent->removeChild(oldi);
      parent->insertChild(itemIndex, newi);
    }
  else
    {
      parent->addChild(newi);
    }
}


QModelTree::QModelTree(QWidget* parent)
  : QTreeWidget(parent)
{
  setHeaderLabels( QStringList() << "Symbol Name"<< ""  << "Value" );
  //     setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  setMinimumHeight(20);
  setContextMenuPolicy(Qt::CustomContextMenu);

  // handle right clicks
  connect
      (
        this, SIGNAL(customContextMenuRequested(const QPoint &)),
        this, SLOT(showContextMenu(const QPoint &))
        );

  // handle visible/hide checkbox changes
  connect
      (
        this, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
        this, SLOT(onItemChanged(QTreeWidgetItem*,int))
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

    vectors_ = new QTreeWidgetItem( this, QStringList() << "Vector Variables"<< ""  << "" );
    { QFont f=vectors_->font(COL_NAME); f.setBold(true); vectors_->setFont(COL_NAME, f); }
    vectors_->setFirstColumnSpanned(true);
    vectors_->setExpanded(true);

    datums_ = new QTreeWidgetItem( this, QStringList() << "Datums"<< ""  << "" );
    { QFont f=datums_->font(COL_NAME); f.setBold(true); datums_->setFont(COL_NAME, f); }
    datums_->setFirstColumnSpanned(true);
    datums_->setExpanded(true);

    postprocactions_ = new QTreeWidgetItem( this, QStringList() << "Postprocessing Actions"<< ""  << "" );
    { QFont f=postprocactions_->font(COL_NAME); f.setBold(true); postprocactions_->setFont(COL_NAME, f); }
    postprocactions_->setFirstColumnSpanned(true);
    postprocactions_->setExpanded(true);

}


void QModelTree::clear()
{
    componentfeatures_->takeChildren();
    scalars_->takeChildren();
    vectors_->takeChildren();
    features_->takeChildren();
    datums_->takeChildren();
    postprocactions_->takeChildren();
}


void QModelTree::onAddScalar(const QString& name, insight::cad::parser::scalar sv)
{
  QScalarVariableItem* old = findItem<QScalarVariableItem>(scalars_, name);
  replaceOrAdd(scalars_, new QScalarVariableItem(name, sv->value(), scalars_), old);
}

void QModelTree::onAddVector(const QString& name, insight::cad::parser::vector vv)
{
  QVectorVariableItem* old = findItem<QVectorVariableItem>(vectors_, name);
  replaceOrAdd(vectors_, new QVectorVariableItem(name, vv->value(), vectors_), old);
}

void QModelTree::onAddFeature(const QString& name, insight::cad::FeaturePtr smp, bool is_component)
{
//    ViewState vs;
//    auto itr = feature_vs_.find(name);
//    if (itr == feature_vs_.end())
//    {
//        vs.visible=is_component;
//    }
//    else
//    {
//        vs=itr->second;
//    }
//    return new QFeatureItem(name, smp, context, vs,
//                            is_component ? componentfeatures_ : features_,
//                            is_component);
}

void QModelTree::onAddDatum(const QString& name, insight::cad::DatumPtr smp)
{
//    return new QDatumItem(name, smp, model, context, datum_vs_[name], datums_);
}

void QModelTree::onAddEvaluation(const QString& name, insight::cad::PostprocActionPtr smp)
{
//    return new QEvaluationItem(name, smp, context, postprocaction_vs_[name], postprocactions_);
}


void QModelTree::onRemoveScalar      (const QString& sn)
{
  if (QScalarVariableItem* item = findItem<QScalarVariableItem>(scalars_, sn))
    scalars_->removeChild(item);
}

void QModelTree::onRemoveVector      (const QString& sn)
{
  if (QVectorVariableItem* item = findItem<QVectorVariableItem>(vectors_, sn))
    vectors_->removeChild(item);
}

void QModelTree::onRemoveFeature     (const QString& sn)
{
  if (QFeatureItem* item = findItem<QFeatureItem>(features_, sn))
    features_->removeChild(item);
  else if (QFeatureItem* item = findItem<QFeatureItem>(componentfeatures_, sn))
    componentfeatures_->removeChild(item);
}

void QModelTree::onRemoveDatum       (const QString& sn)
{
  if (QDatumItem* item = findItem<QDatumItem>(datums_, sn))
    datums_->removeChild(item);
}

void QModelTree::onRemoveEvaluation  (const QString& sn)
{
  if (QEvaluationItem* item = findItem<QEvaluationItem>(postprocactions_, sn))
    postprocactions_->removeChild(item);
}

void QModelTree::onItemChanged( QTreeWidgetItem *item, int)
{
    if (QDisplayableModelTreeItem *msi =dynamic_cast<QDisplayableModelTreeItem*>(item))
    {
        if (msi->isVisible()) msi->show();
        if (msi->isHidden()) msi->hide();
    }
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
            qmsi->resetDisplay();
            qmsi->shaded();
        }
    }
}

void QModelTree::showContextMenu(const QPoint &p)
{
    QModelTreeItem * mi=dynamic_cast<QModelTreeItem*>(itemAt(p));
    if (mi)
    {
        mi->showContextMenu(this->mapToGlobal(p));
    }
}
