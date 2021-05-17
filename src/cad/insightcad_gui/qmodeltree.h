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

#include "insightcad_gui_export.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "viewstate.h"
//#include "qoccviewercontext.h"
#include "iscadmetatyperegistrator.h"
#include "boost/variant.hpp"

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




class INSIGHTCAD_GUI_EXPORT IQISCADModelContainer
    : public QObject
{
  Q_OBJECT

public:
  IQISCADModelContainer(QObject* parent=nullptr);

Q_SIGNALS:

  void beginRebuild();

  void createdVariable    (const QString& sn, insight::cad::ScalarPtr sv);
  void createdVariable    (const QString& sn, insight::cad::VectorPtr vv, insight::cad::VectorVariableType vt);
  void createdFeature     (const QString& sn, insight::cad::FeaturePtr sm, bool is_component,
                           boost::variant<boost::blank,AIS_DisplayMode> ds = boost::blank() );
  void createdDatum       (const QString& sn, insight::cad::DatumPtr dm);
  void createdEvaluation  (const QString& sn, insight::cad::PostprocActionPtr em, bool visible);

  void finishedRebuild();

  void removedScalar      (const QString& sn);
  void removedVector      (const QString& sn, insight::cad::VectorVariableType vt);
  void removedFeature     (const QString& sn);
  void removedDatum       (const QString& sn);
  void removedEvaluation  (const QString& sn);

  void statusMessage(const QString& msg, double timeout=0);
  void statusProgress(int step, int totalSteps);

};



class INSIGHTCAD_GUI_EXPORT QModelTreeItem
: public QObject,
  public QTreeWidgetItem
{
    Q_OBJECT
    
protected:
    QString name_;
    
public:
    const static int COL_VIS=0;
    const static int COL_NAME=1;
    const static int COL_VALUE=2;
    
    QModelTreeItem(const QString& name, QTreeWidgetItem* parent);
    
    inline const QString& name() const { return name_; }
    QModelTree* modelTree() const;

Q_SIGNALS:
    void insertParserStatementAtCursor(const QString& statement);
    void insertIntoNotebook(const QString& statement);
    void jumpTo(const QString& name);

public Q_SLOTS:
    void insertName();
    void jumpToName();

    virtual void showContextMenu(const QPoint& gpos) =0;

};




class INSIGHTCAD_GUI_EXPORT QDisplayableModelTreeItem
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
    void setShadingMode(AIS_DisplayMode ds);
    inline double red() const { return r_; }
    inline double green() const { return g_; }
    inline double blue() const { return b_; }
    Quantity_Color color() const;

    void initDisplay();
    void setRandomColor();
    void copyDisplayProperties(QDisplayableModelTreeItem* di);

public Q_SLOTS:
    virtual void show();
    virtual void hide();
    virtual void wireframe();
    virtual void shaded();
    virtual void onlyThisShaded();
    virtual void randomizeColor();
    virtual void chooseColor();
    virtual void setResolution();

Q_SIGNALS:
    void showItem(QDisplayableModelTreeItem* di);
    void hideItem(QDisplayableModelTreeItem* di);

    void setDisplayMode(QDisplayableModelTreeItem* di, AIS_DisplayMode sm);
    void setColor(QDisplayableModelTreeItem* di, Quantity_Color c);
    void setItemResolution(QDisplayableModelTreeItem* di, double res);

    void focus(Handle_AIS_InteractiveObject ais);
    void unfocus();
};


struct SymbolsSnapshot
{
  std::set<QString>
      scalars_, points_, directions_,
      componentfeatures_, features_,
      datums_, postprocactions_;
};



class INSIGHTCAD_GUI_EXPORT QModelTree
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
     * root node for points
     */
    QTreeWidgetItem *points_;

    /**
     * @brief vectors_
     * root node for direction vectors
     */
    QTreeWidgetItem *directions_;

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


    SymbolsSnapshot symbolsSnapshot_;

    template<class ItemType>
    ItemType* findItem(QTreeWidgetItem *p, const QString& name)
    {
        ItemType *found=NULL;

        for (int i=0; i<p->childCount(); i++)
        {
            if (ItemType *mti =dynamic_cast<ItemType*>(p->child(i)))
            {
                if (mti->text(QModelTreeItem::COL_NAME) == name)
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
    
    void focusOutEvent(QFocusEvent *event);

    void getFeatureNames(std::set<std::string>& featnames) const;
    void getDatumNames(std::set<std::string>& datumnames) const;

    QDisplayableModelTreeItem* findFeature(const QString& name, bool is_component);

    void connectModel(IQISCADModelContainer* model);
    void disconnectModel(IQISCADModelContainer* model);

public Q_SLOTS:
    void storeSymbolSnapshot();

    void addCreatedScalarToSymbolSnapshot(const QString& name,insight::cad::ScalarPtr);
    void addCreatedVectorToSymbolSnapshot(const QString& name,insight::cad::VectorPtr,insight::cad::VectorVariableType t);
    void addCreatedFeatureToSymbolSnapshot(const QString& name, insight::cad::FeaturePtr, bool is_component);
    void addCreatedDatumToSymbolSnapshot(const QString& name, insight::cad::DatumPtr);
    void addCreatedPostprocActionToSymbolSnapshot(const QString& name, insight::cad::PostprocActionPtr, bool);

    void onAddScalar     (const QString& name, insight::cad::ScalarPtr sv);
    void onAddVector     (const QString& name, insight::cad::VectorPtr vv, insight::cad::VectorVariableType vt);
    void onAddFeature    (const QString& name, insight::cad::FeaturePtr smp, bool is_component,
                          boost::variant<boost::blank,AIS_DisplayMode> ds = boost::blank() );
    void onAddDatum      (const QString& name, insight::cad::DatumPtr smp);
    void onAddEvaluation (const QString& name, insight::cad::PostprocActionPtr smp, bool visible=false);
    void removeNonRecreatedSymbols();

    void onRemoveScalar      (const QString& sn);
    void onRemoveVector      (const QString& sn, insight::cad::VectorVariableType vt);
    void onRemoveFeature     (const QString& sn);
    void onRemoveDatum       (const QString& sn);
    void onRemoveEvaluation  (const QString& sn);


private Q_SLOTS:
    /**
     * @brief onItemChanged
     * @param item
     * @param column
     * if visualization state changed: emit signals for view widget(s)
     */
    void onItemChanged( QTreeWidgetItem * item, int column );
    void onItemSelectionChanged();
    void onClear();

public Q_SLOTS:
    void setUniformDisplayMode(const AIS_DisplayMode AM);
    void resetViz();
    void onlyOneShaded(QDisplayableModelTreeItem* shaded_item);
    void allShaded();
    void allWireframe();
    
protected Q_SLOTS:
    void showContextMenu(const QPoint &);

Q_SIGNALS:
    // relay signals
    void showItem(QDisplayableModelTreeItem* di);
    void hideItem(QDisplayableModelTreeItem* di);

    void setDisplayMode(QDisplayableModelTreeItem* di, AIS_DisplayMode sm);
    void setColor(QDisplayableModelTreeItem* di, Quantity_Color c);
    void setItemResolution(QDisplayableModelTreeItem* di, double res);

    void insertParserStatementAtCursor(const QString& statement);
    void insertIntoNotebook(const QString& statement);
    void jumpTo(const QString& name);

    void focus(Handle_AIS_InteractiveObject ais);
    void unfocus();
};


#endif
