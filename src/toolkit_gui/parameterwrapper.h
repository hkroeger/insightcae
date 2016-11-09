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
 *
 */

#ifndef PARAMETERWRAPPER_H
#define PARAMETERWRAPPER_H

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QSignalMapper>
#include <QTreeWidgetItem>
#include <QAction>
#include <QMenu>

#ifndef Q_MOC_RUN
#include "base/factory.h"
#include "base/parameter.h"
#include "base/parameterset.h"

#include "boost/tuple/tuple.hpp"
#include "boost/fusion/tuple.hpp"
#endif


class ParameterWrapper 
: public QObject,
  public QTreeWidgetItem
{
  Q_OBJECT
  
public:
//   typedef boost::tuple<QTreeWidgetItem *, const QString&, insight::Parameter&, QWidget*, QWidget*> ConstrP;
  
  declareFactoryTable
  (
      ParameterWrapper, 
        LIST(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform),
        LIST(parent, name, p, detailw, superform)
  );  

protected:
  QString name_;
  insight::Parameter& p_;
  QWidget* detaileditwidget_;
  QWidget* superform_;
  
  bool widgetsDisplayed_;
  
  virtual void focusInEvent( QFocusEvent* );
  
public:
  declareType("ParameterWrapper");
  ParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform);
  virtual ~ParameterWrapper();
    
  virtual void createWidgets();
  virtual void removedWidgets();
  
public slots:
    virtual void onApply() =0;
    virtual void onUpdate() =0;
    virtual void onSelectionChanged();
    virtual void onSelection();
    virtual void onDestruction();
};

class IntParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
protected:
  QLineEdit *le_;
public:
  declareType(insight::IntParameter::typeName_());
  
  IntParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform);
  virtual void createWidgets();
  inline insight::IntParameter& param() { return dynamic_cast<insight::IntParameter&>(p_); }
  
public slots:
  virtual void onApply();
  virtual void onUpdate();
};

class DoubleParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
protected:
  QLineEdit *le_;
public:
  declareType(insight::DoubleParameter::typeName_());
  DoubleParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform);
  virtual void createWidgets();
  inline insight::DoubleParameter& param() { return dynamic_cast<insight::DoubleParameter&>(p_); }
public slots:
  virtual void onApply();
  virtual void onUpdate();
};

class VectorParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
protected:
  QLineEdit *le_;
public:
  declareType(insight::VectorParameter::typeName_());
  VectorParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform);
  virtual void createWidgets();
  inline insight::VectorParameter& param() { return dynamic_cast<insight::VectorParameter&>(p_); }
public slots:
  virtual void onApply();
  virtual void onUpdate();
};

class StringParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
protected:
  QLineEdit *le_;
public:
  declareType(insight::StringParameter::typeName_());
  StringParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform);
  virtual void createWidgets();
  inline insight::StringParameter& param() { return dynamic_cast<insight::StringParameter&>(p_); }
public slots:
  virtual void onApply();
  virtual void onUpdate();
};

class BoolParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
protected:
  QCheckBox *cb_;

public:
  declareType(insight::BoolParameter::typeName_());
  BoolParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform);
  virtual void createWidgets();
  inline insight::BoolParameter& param() { return dynamic_cast<insight::BoolParameter&>(p_); }

public slots:
  virtual void onApply();
  virtual void onUpdate();
};

class PathParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
protected:
  QLineEdit *le_;
  QPushButton *dlgBtn_;
  
  virtual void updateTooltip();
  
public:
  declareType(insight::PathParameter::typeName_());
  PathParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform);
  virtual void createWidgets();
  inline insight::PathParameter& param() { return dynamic_cast<insight::PathParameter&>(p_); }

public slots:
  virtual void onApply();
  virtual void onUpdate();
  
protected slots:
  virtual void openSelectionDialog();
  virtual void onDataEntered();
};

class MatrixParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
protected:
  QLineEdit *le_;
  QPushButton *dlgBtn_;
  
public:
  declareType(insight::MatrixParameter::typeName_());
  MatrixParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform);
  virtual void createWidgets();
  inline insight::MatrixParameter& param() { return dynamic_cast<insight::MatrixParameter&>(p_); }

public slots:
  virtual void onApply();
  virtual void onUpdate();
  
protected slots:
  virtual void openSelectionDialog();
};

class DirectoryParameterWrapper
: public PathParameterWrapper
{
  Q_OBJECT
public:
  declareType(insight::DirectoryParameter::typeName_());
  DirectoryParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform);
  inline insight::DirectoryParameter& param() { return dynamic_cast<insight::DirectoryParameter&>(p_); }

protected slots:
  virtual void openSelectionDialog();

};


class SelectionParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
protected:
  
  QComboBox* selBox_;

public:
  declareType(insight::SelectionParameter::typeName_());
  SelectionParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform);
  virtual void createWidgets();
  inline insight::SelectionParameter& param() { return dynamic_cast<insight::SelectionParameter&>(p_); }
public slots:
  virtual void onApply();
  virtual void onUpdate();
};

class SubsetParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
public:
  declareType(insight::SubsetParameter::typeName_());
  SubsetParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform);
  virtual void createWidgets();
  inline insight::SubsetParameter& param() { return dynamic_cast<insight::SubsetParameter&>(p_); }
  
public slots:
  virtual void onApply();
  virtual void onUpdate();
  
signals:
  void apply();
  void update();
};

// class ArrayParameterElementWrapper
// : public QTreeWidgetItem
// {
//   Q_OBJECT
//   
//   ParameterWrapper* pw_;
//   
// public:
//   declareType("ArrayParameterElementWrapper");
//   
//   ArrayParameterElementWrapper(const ConstrP& p)
//   : PW(p)
//   {}
//   
//   virtual void createWidgets()
//   {
//   }
//   
//   void showContextMenu(const QPoint& gpos)
//   {
//     QMenu myMenu;
//     QAction *tit=new QAction("Delete", &myMenu);
//     myMenu.addAction(tit);
// 
//     QAction* selectedItem = myMenu.exec(gpos);
//   }
// };

class ArrayParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
protected:
//   boost::ptr_vector<QWidget> entrywrappers_;
//   QVBoxLayout *vlayout_;
//   QGroupBox *group_;
//   QSignalMapper *map_;
  
  void addWrapper(int i);
  void rebuildWrappers();
  
public:
  declareType(insight::ArrayParameter::typeName_());
  ArrayParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform);
  virtual void createWidgets();
  inline insight::ArrayParameter& param() { return dynamic_cast<insight::ArrayParameter&>(p_); }
  
protected slots:
  void showContextMenuForWidget(const QPoint &p);
  void onRemove(int i);
  void onAppendEmpty();
  
public slots:
  virtual void onApply();
  virtual void onUpdate();
  
signals:
  void apply();
  void update();
};

class DoubleRangeParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
protected:
  QListWidget* lBox_;
  
  void rebuildList();

public:
  declareType(insight::DoubleRangeParameter::typeName_());
  DoubleRangeParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform);
  virtual void createWidgets();
  inline insight::DoubleRangeParameter& param() { return dynamic_cast<insight::DoubleRangeParameter&>(p_); }
  
public slots:
  virtual void onApply();
  virtual void onUpdate();
  void onAddSingle();
  void onAddRange();
  void onClear();
  
signals:
  void apply();
};

class SelectableSubsetParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
  QComboBox* selBox_;
//   QVBoxLayout *layout0_;
//   QGroupBox *name2Label_;
//   QPushButton* apply_;
  
  void insertSubset();
  
public:
  declareType(insight::SelectableSubsetParameter::typeName_());
  SelectableSubsetParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform);
  virtual void createWidgets();
  inline insight::SelectableSubsetParameter& param() { return dynamic_cast<insight::SelectableSubsetParameter&>(p_); }
  
public slots:
  virtual void onApply();
  virtual void onUpdate();
//   virtual void onCurrentIndexChanged(const QString& qs);
  
signals:
  void apply();
  void update();
};

// void addWrapperToWidget(insight::ParameterSet& pset, QWidget *widget, QWidget *superform=NULL);
void addWrapperToWidget
(
  insight::ParameterSet& pset, 
  QTreeWidgetItem *parentnode, 
  QWidget *detaileditwidget,
  QWidget *superform
);

#endif // PARAMETERWRAPPER_H
