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
#include "AIS_InteractiveObject.hxx"
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
    Handle_AIS_InteractiveObject ais_;
    int shadingMode_;
    double r_, g_, b_;

public:
    QDisplayableModelTreeItem
    (
            const std::string& name,
            bool visible,
            QTreeWidgetItem* parent
    );

    bool isVisible() const;
    bool isHidden() const;

    inline Handle_AIS_InteractiveObject ais() const { return ais_; }
    inline int shadingMode() const { return shadingMode_; }
    inline double red() const { return r_; }
    inline double green() const { return g_; }
    inline double blue() const { return b_; }
    Quantity_Color color() const;

    void show();
    void hide();

signals:
    void show(QDisplayableModelTreeItem* di);
    void hide(QDisplayableModelTreeItem* di);
};




class QModelTree
: public QTreeWidget
{
    Q_OBJECT
protected:

    /**
     * @brief scalars_
     * root node for scalar entries
     */
    QTreeWidgetItem *scalars_;

    /**
     * @brief vectors_
     * root node for vectors
     */
    QTreeWidgetItem *vectors_;

    /**
     * @brief features_
     * root node for model features
     */
    QTreeWidgetItem *features_;

    /**
     * @brief componentfeatures_
     * root node for components
     */
    QTreeWidgetItem *componentfeatures_;

    /**
     * @brief datums_
     * root node for datums
     */
    QTreeWidgetItem *datums_;

    /**
     * @brief postprocactions_
     * root node for postproc actions
     */
    QTreeWidgetItem *postprocactions_;
        
//    std::vector<QTreeWidgetItem*> featurenodes_;
        
//    std::map<std::string, ViewState>
//        vector_vs_,
//        feature_vs_,
//        datum_vs_,
//        postprocaction_vs_;

    template<class ItemType>
    ItemType* findItem(QTreeWidgetItem *parent, const QString& name)
    {
        ItemType *found=NULL;

        for (int i=0; i<p->childCount(); i++)
        {
            if (ItemType *mti =dynamic_cast<ItemType*>(p->child(i)))
            {
                if (mti->text(0) == name)
                  {
                    if (!found) found=mti;
                    else
                      {
                        throw insight::Exception("Internal error: duplicate identifier in model tree!");
                      }
                  }
            }
        }
        return found;
    }

    void replaceOrAdd(QTreeWidgetItem *parent, QTreeWidgetItem *newi, QTreeWidgetItem* oldi=NULL);


public:
    QModelTree(QWidget* parent);
    
    void clear();
        
public slots:
    void onAddScalar     (const QString& name, insight::cad::parser::scalar sv);
    void onAddVector     (const QString& name, insight::cad::parser::vector vv);
    void onAddFeature    (const QString& name, insight::cad::FeaturePtr smp, bool is_component);
    void onAddDatum      (const QString& name, insight::cad::DatumPtr smp);
    void onAddEvaluation (const QString& name, insight::cad::PostprocActionPtr smp);

    void onRemoveScalar      (const QString& sn);
    void onRemoveVector      (const QString& sn);
    void onRemoveFeature     (const QString& sn);
    void onRemoveDatum       (const QString& sn);
    void onRemoveEvaluation  (const QString& sn);

signals:
    void show(QFeatureItem*);
    void hide(QFeatureItem*);

private slots:
    /**
     * @brief onItemChanged
     * @param item
     * @param column
     * if visualization state changed: emit signals for view widget(s)
     */
    void onItemChanged( QTreeWidgetItem * item, int column );

public slots:
    void setUniformDisplayMode(const AIS_DisplayMode AM);
    void resetViz();
    
protected slots:
    void showContextMenuForWidget(const QPoint &);    
};


#endif
