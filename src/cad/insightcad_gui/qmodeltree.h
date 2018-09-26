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

#ifndef Q_MOC_RUN
#include "cadtypes.h"
#include "AIS_DisplayMode.hxx"
#include "AIS_InteractiveObject.hxx"
#endif




class QScalarVariableItem;
class QVectorVariableItem;
class QFeatureItem;
class QDatumItem;
class QEvaluationItem;
class QModelTree;




class QModelTreeItem
: public QObject,
  public QTreeWidgetItem
{
    Q_OBJECT
    
protected:
    QString name_;
    
public:
    const static int COL_NAME=0;
    const static int COL_VIS=1;
    const static int COL_VALUE=2;
    
    QModelTreeItem(const QString& name, QTreeWidgetItem* parent);
    
    inline const QString& name() const { return name_; }
    QModelTree* modelTree() const;

signals:
    void insertParserStatementAtCursor(const QString& statement);
    void jumpTo(const QString& name);

public slots:
    void insertName();
    void jumpToName();

    virtual void showContextMenu(const QPoint& gpos) =0;

};




class QDisplayableModelTreeItem
: public QModelTreeItem
{
    Q_OBJECT

protected:
    Handle_AIS_InteractiveObject ais_;
    AIS_DisplayMode shadingMode_;
    double r_, g_, b_;

    virtual Handle_AIS_InteractiveObject createAIS(AIS_InteractiveContext& context) =0;

public:
    QDisplayableModelTreeItem
    (
            const QString& name,
            bool visible,
            AIS_DisplayMode dm,
            QTreeWidgetItem* parent
    );
    virtual ~QDisplayableModelTreeItem();

    bool isVisible() const;
    bool isHidden() const;

    Handle_AIS_InteractiveObject ais(AIS_InteractiveContext& context);
    inline AIS_DisplayMode shadingMode() const { return shadingMode_; }
    inline double red() const { return r_; }
    inline double green() const { return g_; }
    inline double blue() const { return b_; }
    Quantity_Color color() const;

    void initDisplay();
    void setRandomColor();
    void copyDisplayProperties(QDisplayableModelTreeItem* di);

public slots:
    virtual void show();
    virtual void hide();
    virtual void wireframe();
    virtual void shaded();
    virtual void onlyThisShaded();
    virtual void randomizeColor();
    virtual void setResolution();

signals:
    void show(QDisplayableModelTreeItem* di);
    void hide(QDisplayableModelTreeItem* di);

    void setDisplayMode(QDisplayableModelTreeItem* di, AIS_DisplayMode sm);
    void setColor(QDisplayableModelTreeItem* di, Quantity_Color c);
    void setResolution(QDisplayableModelTreeItem* di, double res);
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


    template<class ItemType>
    ItemType* findItem(QTreeWidgetItem *p, const QString& name)
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

    void removeModelItem(QTreeWidgetItem* oldi);
    void replaceOrAdd(QTreeWidgetItem *parent, QTreeWidgetItem *newi, QTreeWidgetItem* oldi=NULL);

    void connectDisplayableItem(QDisplayableModelTreeItem* i);

public:
    QModelTree(QWidget* parent);
    
        
public slots:
    void onAddScalar     (const QString& name, insight::cad::ScalarPtr sv);
    void onAddVector     (const QString& name, insight::cad::VectorPtr vv);
    void onAddFeature    (const QString& name, insight::cad::FeaturePtr smp, bool is_component);
    void onAddDatum      (const QString& name, insight::cad::DatumPtr smp);
    void onAddEvaluation (const QString& name, insight::cad::PostprocActionPtr smp, bool visible=false);

    void onRemoveScalar      (const QString& sn);
    void onRemoveVector      (const QString& sn);
    void onRemoveFeature     (const QString& sn);
    void onRemoveDatum       (const QString& sn);
    void onRemoveEvaluation  (const QString& sn);


private slots:
    /**
     * @brief onItemChanged
     * @param item
     * @param column
     * if visualization state changed: emit signals for view widget(s)
     */
    void onItemChanged( QTreeWidgetItem * item, int column );
    void onClear();

public slots:
    void setUniformDisplayMode(const AIS_DisplayMode AM);
    void resetViz();
    void onlyOneShaded(QDisplayableModelTreeItem* shaded_item);
    void allShaded();
    void allWireframe();
    
protected slots:
    void showContextMenu(const QPoint &);

signals:
    // relay signals
    void show(QDisplayableModelTreeItem* di);
    void hide(QDisplayableModelTreeItem* di);

    void setDisplayMode(QDisplayableModelTreeItem* di, AIS_DisplayMode sm);
    void setColor(QDisplayableModelTreeItem* di, Quantity_Color c);
    void setResolution(QDisplayableModelTreeItem* di, double res);

    void insertParserStatementAtCursor(const QString& statement);
    void jumpTo(const QString& name);

};


#endif
