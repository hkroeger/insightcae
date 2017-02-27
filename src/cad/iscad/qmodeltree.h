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

#ifndef INSIGHT_QMODELTREE_H
#define INSIGHT_QMODELTREE_H

#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "viewstate.h"
#include "qoccviewercontext.h"

#ifndef Q_MOC
#include "cadtypes.h"
#include "AIS_DisplayMode.hxx"
#endif




class QScalarVariableItem;
class QVectorVariableItem;
class QFeatureItem;
class QDatumItem;
class QEvaluationItem;




class QModelTreeItem
: public QObject,
  public QTreeWidgetItem
{
    Q_OBJECT
    
protected:
    QString name_;
    
public:
    const static int COL_VIS=1;
    const static int COL_NAME=0;
    const static int COL_VALUE=2;
    
    QModelTreeItem(const std::string& name, QTreeWidgetItem* parent);
    
    inline const QString& name() const { return name_; }

public slots:
    virtual void showContextMenu(const QPoint& gpos) =0;

};




class QDisplayableModelTreeItem
: public QModelTreeItem
{
    Q_OBJECT
    
protected:
    QoccViewerContext* context_;
    
public:
    ViewState state_;
    
    QDisplayableModelTreeItem(const std::string& name, QoccViewerContext* context, 
		const ViewState& state, QTreeWidgetItem* parent);
    
    inline const ViewState& viewstate() const { return state_; }
    virtual void updateDisplay() =0;
};




class QModelTree
: public QTreeWidget
{
    Q_OBJECT
    
public:
    QTreeWidgetItem 
        *scalars_, 
        *vectors_, 
        *features_, 
        *componentfeatures_, 
        *datums_, 
        *postprocactions_;
        
    std::vector<QTreeWidgetItem*> featurenodes_;
        
    std::map<std::string, ViewState>
        vector_vs_,
        feature_vs_,
        datum_vs_,
        postprocaction_vs_;
    
    QModelTree(QWidget* parent);
    
    void storeViewStates();
    void clear();
        
    QScalarVariableItem* addScalarVariableItem(const std::string& name, double value);
    QVectorVariableItem* addVectorVariableItem(const std::string& name, const arma::mat& value, QoccViewerContext* context);
    QFeatureItem* addFeatureItem(const std::string& name, insight::cad::FeaturePtr smp, QoccViewerContext* context, bool is_component);
    QDatumItem* addDatumItem(const std::string& name, insight::cad::DatumPtr smp, insight::cad::ModelPtr model, QoccViewerContext* context);
    QEvaluationItem* addEvaluationItem(const std::string& name, insight::cad::PostprocActionPtr smp, QoccViewerContext* context);

public slots:
    void setUniformDisplayMode(const AIS_DisplayMode AM);
    void resetViz();
    
protected slots:
    void showContextMenuForWidget(const QPoint &);    
};


#endif
