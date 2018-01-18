
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

#include <QInputDialog>

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
: QModelTreeItem ( name, parent ),
  shadingMode_(AIS_Shaded)
{
    setCheckState(COL_VIS, visible ? Qt::Checked : Qt::Unchecked);
    setRandomColor();
}

QDisplayableModelTreeItem::~QDisplayableModelTreeItem()
{
  emit hide(this);
}


bool QDisplayableModelTreeItem::isVisible() const
{
    return (checkState(COL_VIS) == Qt::Checked);
}


bool QDisplayableModelTreeItem::isHidden() const
{
    return (checkState(COL_VIS) == Qt::Unchecked);
}

Handle_AIS_InteractiveObject QDisplayableModelTreeItem::ais()
{
  if (ais_.IsNull())
    {
      ais_=createAIS();
    }
  return ais_;
}

Quantity_Color QDisplayableModelTreeItem::color() const
{
  return Quantity_Color(r_, g_, b_, Quantity_TOC_RGB);
}

void QDisplayableModelTreeItem::setRandomColor()
{
  r_=0.5+0.5*( double(rand()) / double(RAND_MAX) );
  g_=0.5+0.5*( double(rand()) / double(RAND_MAX) );
  b_=0.5+0.5*( double(rand()) / double(RAND_MAX) );
}

void QDisplayableModelTreeItem::show()
{
  if (ais_.IsNull())
    {
      ais_=createAIS();
    }
  setCheckState(COL_VIS, Qt::Checked);
  emit show(this);
}


void QDisplayableModelTreeItem::hide()
{
  if (ais_.IsNull())
    {
      ais_=createAIS();
    }
  setCheckState(COL_VIS, Qt::Unchecked);
  emit hide(this);
}

void QDisplayableModelTreeItem::wireframe()
{
  emit setDisplayMode(this, AIS_WireFrame);
}

void QDisplayableModelTreeItem::shaded()
{
  emit setDisplayMode(this, AIS_Shaded);
}

void QDisplayableModelTreeItem::onlyThisShaded()
{
//  emit setShadingMode(this, AIS_WireFrame);
}

void QDisplayableModelTreeItem::randomizeColor()
{
  setRandomColor();
  emit setColor(this, color());
}

void QDisplayableModelTreeItem::setResolution()
{
  bool ok;
  double res=QInputDialog::getDouble(treeWidget(), "Set Resolution", "Resolution:", 0.001, 1e-7, 0.1, 7, &ok);
  if (ok)
  {
    emit setResolution(this, res);
  }
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


void QModelTree::connectDisplayableItem(QDisplayableModelTreeItem* newf)
{
  connect(newf, SIGNAL(show(QDisplayableModelTreeItem*)),
          this, SIGNAL(show(QDisplayableModelTreeItem*)));
  connect(newf, SIGNAL(hide(QDisplayableModelTreeItem*)),
          this, SIGNAL(hide(QDisplayableModelTreeItem*)));
  connect(newf, SIGNAL(setDisplayMode(QDisplayableModelTreeItem*, AIS_DisplayMode)),
          this, SIGNAL(setDisplayMode(QDisplayableModelTreeItem*, AIS_DisplayMode)));
  connect(newf, SIGNAL(setColor(QDisplayableModelTreeItem*, Quantity_Color)),
          this, SIGNAL(setColor(QDisplayableModelTreeItem*, Quantity_Color)));
  connect(newf, SIGNAL(setResolution(QDisplayableModelTreeItem*, double)),
          this, SIGNAL(setResolution(QDisplayableModelTreeItem*, double)));
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


void QModelTree::onAddScalar(const QString& name, insight::cad::ScalarPtr sv)
{
  SignalBlocker b(this);
  QScalarVariableItem* old = findItem<QScalarVariableItem>(scalars_, name);
  replaceOrAdd(scalars_, new QScalarVariableItem(name, sv->value(), scalars_), old);
}

void QModelTree::onAddVector(const QString& name, insight::cad::VectorPtr vv)
{
  SignalBlocker b(this);
  QVectorVariableItem* old = findItem<QVectorVariableItem>(vectors_, name);
  QVectorVariableItem* newf = new QVectorVariableItem(name, vv->value(), vectors_);
  replaceOrAdd(vectors_, newf, old);
  connectDisplayableItem(newf);
}

void QModelTree::onAddFeature(const QString& name, insight::cad::FeaturePtr smp, bool is_component)
{
  SignalBlocker b(this);

  QTreeWidgetItem* cat;
  if (is_component)
    cat=componentfeatures_;
  else
    cat=features_;

  QFeatureItem* old = findItem<QFeatureItem>(cat, name);
  QFeatureItem* newf = new QFeatureItem(name, smp, is_component, cat, is_component);
  replaceOrAdd(cat, newf, old);
  connectDisplayableItem(newf);
}


void QModelTree::onAddDatum(const QString& name, insight::cad::DatumPtr smp)
{
  SignalBlocker b(this);
  QDatumItem* old = findItem<QDatumItem>(datums_, name);
  QDatumItem* newf = new QDatumItem(name, smp, datums_);
  replaceOrAdd(datums_, newf, old);
  connectDisplayableItem(newf);
}

void QModelTree::onAddEvaluation(const QString& name, insight::cad::PostprocActionPtr smp)
{
  SignalBlocker b(this);
  QEvaluationItem* old = findItem<QEvaluationItem>(postprocactions_, name);
  QEvaluationItem* newf = new QEvaluationItem(name, smp, postprocactions_);
  replaceOrAdd(postprocactions_, newf, old);
  connectDisplayableItem(newf);
}


void QModelTree::onRemoveScalar      (const QString& sn)
{
  SignalBlocker(this);
  if (QScalarVariableItem* item = findItem<QScalarVariableItem>(scalars_, sn))
    scalars_->removeChild(item);
}

void QModelTree::onRemoveVector      (const QString& sn)
{
  SignalBlocker(this);
  if (QVectorVariableItem* item = findItem<QVectorVariableItem>(vectors_, sn))
    vectors_->removeChild(item);
}

void QModelTree::onRemoveFeature     (const QString& sn)
{
  SignalBlocker(this);
  if (QFeatureItem* item = findItem<QFeatureItem>(features_, sn))
    features_->removeChild(item);
  else if (QFeatureItem* item = findItem<QFeatureItem>(componentfeatures_, sn))
    componentfeatures_->removeChild(item);
}

void QModelTree::onRemoveDatum       (const QString& sn)
{
  SignalBlocker(this);
  if (QDatumItem* item = findItem<QDatumItem>(datums_, sn))
    datums_->removeChild(item);
}

void QModelTree::onRemoveEvaluation  (const QString& sn)
{
  SignalBlocker(this);
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

void QModelTree::onClear()
{
    componentfeatures_->takeChildren();
    scalars_->takeChildren();
    vectors_->takeChildren();
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
