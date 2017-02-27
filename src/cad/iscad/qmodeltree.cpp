
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
  const std::string& name,
  QTreeWidgetItem* parent

)
  : QTreeWidgetItem ( parent ),
    name_ ( QString::fromStdString ( name ) )
{
}


QDisplayableModelTreeItem::QDisplayableModelTreeItem
(
  const std::string& name,
  QoccViewerContext* context,
  const ViewState& state,
  QTreeWidgetItem* parent
)
  : QModelTreeItem ( name, parent ),
    context_ ( context ),
    state_ ( state )
{
}




QModelTree::QModelTree(QWidget* parent)
: QTreeWidget(parent)
{
    setHeaderLabels( QStringList() << "Symbol Name"<< ""  << "Value" );
//     setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    setMinimumHeight(20);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect
    (
        this,
        SIGNAL(customContextMenuRequested(const QPoint &)),
        this,
        SLOT(showContextMenuForWidget(const QPoint &))
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
    
    featurenodes_ = boost::assign::list_of<QTreeWidgetItem*>
        (features_)
        (componentfeatures_)
        .convert_to_container<std::vector<QTreeWidgetItem*> >();
}

void QModelTree::storeViewStates()
{
    feature_vs_.clear();
    vector_vs_.clear();
    datum_vs_.clear();
    postprocaction_vs_.clear();
    
    for (int i=0; i<features_->childCount(); i++)
    {
        QDisplayableModelTreeItem *it = dynamic_cast<QDisplayableModelTreeItem*>(features_->child(i));
        if (it) feature_vs_[it->name().toStdString()] = it->viewstate();
    }
    for (int i=0; i<componentfeatures_->childCount(); i++)
    {
        QDisplayableModelTreeItem *it = dynamic_cast<QDisplayableModelTreeItem*>(componentfeatures_->child(i));
        if (it) feature_vs_[it->name().toStdString()] = it->viewstate();
    }
    for (int i=0; i<vectors_->childCount(); i++)
    {
        QDisplayableModelTreeItem *it = dynamic_cast<QDisplayableModelTreeItem*>(vectors_->child(i));
        if (it) vector_vs_[it->name().toStdString()] = it->viewstate();
    }
    for (int i=0; i<datums_->childCount(); i++)
    {
        QDisplayableModelTreeItem *it = dynamic_cast<QDisplayableModelTreeItem*>(datums_->child(i));
        if (it) datum_vs_[it->name().toStdString()] = it->viewstate();
    }
    for (int i=0; i<postprocactions_->childCount(); i++)
    {
        QDisplayableModelTreeItem *it = dynamic_cast<QDisplayableModelTreeItem*>(postprocactions_->child(i));
        if (it) postprocaction_vs_[it->name().toStdString()] = it->viewstate();
    }
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


QScalarVariableItem* QModelTree::addScalarVariableItem(const std::string& name, double value)
{
    return new QScalarVariableItem(name, value, scalars_);
}

QVectorVariableItem* QModelTree::addVectorVariableItem(const std::string& name, const arma::mat& value, QoccViewerContext* context)
{
    ViewState vs;
    auto itr = vector_vs_.find(name);
    if (itr == vector_vs_.end())
    {
        vs.visible=false;
    }
    else
    {
        vs=itr->second;
    }
    return new QVectorVariableItem(name, value, context, vs, vectors_);
}

QFeatureItem* QModelTree::addFeatureItem(const std::string& name, insight::cad::FeaturePtr smp, QoccViewerContext* context, bool is_component)
{
    ViewState vs;
    auto itr = feature_vs_.find(name);
    if (itr == feature_vs_.end())
    {
        vs.visible=is_component;
    }
    else
    {
        vs=itr->second;
    }
    return new QFeatureItem(name, smp, context, vs, 
                            is_component ? componentfeatures_ : features_, 
                            is_component);
}

QDatumItem* QModelTree::addDatumItem(const std::string& name, insight::cad::DatumPtr smp, insight::cad::ModelPtr model, QoccViewerContext* context)
{
    return new QDatumItem(name, smp, model, context, datum_vs_[name], datums_);
}

QEvaluationItem* QModelTree::addEvaluationItem(const std::string& name, insight::cad::PostprocActionPtr smp, QoccViewerContext* context)
{
    return new QEvaluationItem(name, smp, context, postprocaction_vs_[name], postprocactions_);
}

void QModelTree::setUniformDisplayMode(const AIS_DisplayMode AM)
{
    BOOST_FOREACH(QTreeWidgetItem* p, featurenodes_)
    {
        for (int i=0; i<p->childCount(); i++)
        {
            if (QFeatureItem *msi =dynamic_cast<QFeatureItem*>(p->child(i)))
            {
                if (AM==AIS_WireFrame)
                    msi->wireframe();
                else if (AM==AIS_Shaded)
                    msi->shaded();
            }
        }
    }
}

void QModelTree::resetViz()
{
    BOOST_FOREACH(QTreeWidgetItem* p, featurenodes_)
    {
        for (int i=0; i<p->childCount(); i++)
        {
            if ( QFeatureItem *qmsi=dynamic_cast<QFeatureItem*>(p->child(i)) )
            {
                qmsi->resetDisplay();
                qmsi->shaded();
            }
        }
    }
}

void QModelTree::showContextMenuForWidget(const QPoint &p)
{
    QModelTreeItem * mi=dynamic_cast<QModelTreeItem*>(itemAt(p));
    if (mi)
    {
        mi->showContextMenu(this->mapToGlobal(p));
    }
}
