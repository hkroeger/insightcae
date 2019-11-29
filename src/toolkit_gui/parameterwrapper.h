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
#include "base/parameters/simpleparameter.h"
#include "base/parameters/selectionparameter.h"
#include "base/parameters/pathparameter.h"
#include "base/parameters/matrixparameter.h"
#include "base/parameters/doublerangeparameter.h"
#include "base/parameters/arrayparameter.h"
#include "base/parameterset.h"
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
        LIST(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, const insight::Parameter& defp, QWidget* detailw, QObject* superform),
        LIST(parent, name, p, defp, detailw, superform)
  );  

protected:
  QString name_;
  insight::Parameter& p_;
  const insight::Parameter& pDefault_;
  QWidget* detaileditwidget_;
  QObject* superform_;
  
  bool widgetsDisplayed_;
  
  virtual void focusInEvent( QFocusEvent* );
  
public:
  declareType("ParameterWrapper");
  ParameterWrapper
  (
      QTreeWidgetItem* parent,
      const QString& name,
      insight::Parameter& p,
      const insight::Parameter& defp,
      QWidget* detailw,
      QObject* superform
  );
  virtual ~ParameterWrapper();
    
  virtual void createWidgets();
  virtual void removedWidgets();

  virtual QMenu* createContextMenu();
  virtual bool showContextMenuForWidget(const QPoint &p);

public Q_SLOTS:
    virtual void onApply() =0; // Incoming signal from application: ParameterSet => GUI
    virtual void onUpdate() =0; // Incoming signal from application: GUI => ParameterSet
    virtual void onParameterSetChanged(); // Incoming signal from child widgets: propagate upwards
    virtual void onSelectionChanged();
    virtual void onSelection();
    virtual void onDestruction();
    virtual void onResetToDefault();

Q_SIGNALS:
    void parameterSetChanged();
};




class IntParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
protected:
  QLineEdit *le_;

public:
  declareType(insight::IntParameter::typeName_());
  
  IntParameterWrapper
  (
      QTreeWidgetItem* parent,
      const QString& name,
      insight::Parameter& p,
      const insight::Parameter& defp,
      QWidget* detailw,
      QObject* superform
  );
  void createWidgets() override;
  inline insight::IntParameter& param() { return dynamic_cast<insight::IntParameter&>(p_); }
  
public Q_SLOTS:
  void onApply() override;
  void onUpdate() override;
};




class DoubleParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
protected:
  QLineEdit *le_;
public:
  declareType(insight::DoubleParameter::typeName_());
  DoubleParameterWrapper
  (
      QTreeWidgetItem* parent,
      const QString& name,
      insight::Parameter& p,
      const insight::Parameter& defp,
      QWidget* detailw,
      QObject* superform
  );
  void createWidgets() override;
  inline insight::DoubleParameter& param() { return dynamic_cast<insight::DoubleParameter&>(p_); }

public Q_SLOTS:
  void onApply() override;
  void onUpdate() override;
};




class VectorParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
protected:
  QLineEdit *le_;
public:
  declareType(insight::VectorParameter::typeName_());
  VectorParameterWrapper
  (
      QTreeWidgetItem* parent,
      const QString& name,
      insight::Parameter& p,
      const insight::Parameter& defp,
      QWidget* detailw,
      QObject* superform
  );
  void createWidgets() override;
  inline insight::VectorParameter& param() { return dynamic_cast<insight::VectorParameter&>(p_); }

public Q_SLOTS:
  void onApply() override;
  void onUpdate() override;
};




class StringParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
protected:
  QLineEdit *le_;
public:
  declareType(insight::StringParameter::typeName_());
  StringParameterWrapper
  (
      QTreeWidgetItem* parent,
      const QString& name,
      insight::Parameter& p,
      const insight::Parameter& defp,
      QWidget* detailw,
      QObject* superform
      );
  void createWidgets() override;
  inline insight::StringParameter& param()
  { return dynamic_cast<insight::StringParameter&>(p_); }

public Q_SLOTS:
  void onApply() override;
  void onUpdate() override;
};




class BoolParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
protected:
  QCheckBox *cb_;

public:
  declareType(insight::BoolParameter::typeName_());
  BoolParameterWrapper
  (
      QTreeWidgetItem* parent,
      const QString& name,
      insight::Parameter& p,
      const insight::Parameter& defp,
      QWidget* detailw,
      QObject* superform
  );
  void createWidgets() override;
  inline insight::BoolParameter& param() { return dynamic_cast<insight::BoolParameter&>(p_); }

public Q_SLOTS:
  void onApply() override;
  void onUpdate() override;
};




class PathParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
protected:
  QLineEdit *le_;
  QPushButton *dlgBtn_, *openBtn_;
  
  virtual void updateTooltip();
  
public:
  declareType(insight::PathParameter::typeName_());
  PathParameterWrapper
  (
      QTreeWidgetItem* parent,
      const QString& name,
      insight::Parameter& p,
      const insight::Parameter& defp,
      QWidget* detailw,
      QObject* superform
  );
  void createWidgets() override;
  inline insight::PathParameter& param()
  { return dynamic_cast<insight::PathParameter&>(p_); }

public Q_SLOTS:
  void onApply() override;
  void onUpdate() override;
  
protected Q_SLOTS:
  virtual void openSelectionDialog();
  virtual void openFile();
  virtual void onDataEntered();
  virtual void onExportFile();
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
  MatrixParameterWrapper
  (
      QTreeWidgetItem* parent,
      const QString& name,
      insight::Parameter& p,
      const insight::Parameter& defp,
      QWidget* detailw,
      QObject* superform
  );
  void createWidgets() override;
  inline insight::MatrixParameter& param()
  { return dynamic_cast<insight::MatrixParameter&>(p_); }

public Q_SLOTS:
  void onApply() override;
  void onUpdate() override;
  
protected Q_SLOTS:
  virtual void openSelectionDialog();
};




class DirectoryParameterWrapper
: public PathParameterWrapper
{
  Q_OBJECT
public:
  declareType(insight::DirectoryParameter::typeName_());
  DirectoryParameterWrapper
  (
      QTreeWidgetItem* parent,
      const QString& name,
      insight::Parameter& p,
      const insight::Parameter& defp,
      QWidget* detailw,
      QObject* superform
  );
  inline insight::DirectoryParameter& param()
  { return dynamic_cast<insight::DirectoryParameter&>(p_); }

protected Q_SLOTS:
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
  SelectionParameterWrapper
  (
      QTreeWidgetItem* parent,
      const QString& name,
      insight::Parameter& p,
      const insight::Parameter& defp,
      QWidget* detailw,
      QObject* superform
  );
  void createWidgets() override;
  inline insight::SelectionParameter& param()
  { return dynamic_cast<insight::SelectionParameter&>(p_); }

public Q_SLOTS:
  void onApply() override;
  void onUpdate() override;
};




class SubsetParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
public:
  declareType(insight::SubsetParameter::typeName_());
  SubsetParameterWrapper
  (
      QTreeWidgetItem* parent,
      const QString& name,
      insight::Parameter& p,
      const insight::Parameter& defp,
      QWidget* detailw,
      QObject* superform
  );
  void createWidgets() override;
  inline insight::SubsetParameter& param()
  { return dynamic_cast<insight::SubsetParameter&>(p_); }
  
public Q_SLOTS:
  void onApply() override;
  void onUpdate() override;

Q_SIGNALS:
  void apply();
  void update();
};





class ArrayParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
protected:
  void addWrapper(int i);
  void rebuildWrappers();
  
public:
  declareType(insight::ArrayParameter::typeName_());
  ArrayParameterWrapper
  (
      QTreeWidgetItem* parent,
      const QString& name,
      insight::Parameter& p,
      const insight::Parameter& defp,
      QWidget* detailw,
      QObject* superform
  );
  void createWidgets() override;
  inline insight::ArrayParameter& param()
  { return dynamic_cast<insight::ArrayParameter&>(p_); }
  
protected Q_SLOTS:
  bool showContextMenuForWidget(const QPoint &p) override;
  QMenu* createContextMenu() override;

  void onRemove(int i);
  void onRemoveAll();
  void onAppendEmpty();
  
public Q_SLOTS:
  void onApply() override;
  void onUpdate() override;

Q_SIGNALS:
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
  DoubleRangeParameterWrapper
  (
      QTreeWidgetItem* parent,
      const QString& name,
      insight::Parameter& p,
      const insight::Parameter& defp,
      QWidget* detailw,
      QObject* superform
  );
  void createWidgets() override;
  inline insight::DoubleRangeParameter& param() { return dynamic_cast<insight::DoubleRangeParameter&>(p_); }
  
public Q_SLOTS:
  void onApply() override;
  void onUpdate() override;
  void onAddSingle();
  void onAddRange();
  void onClear();
  
Q_SIGNALS:
  void apply();
};




class SelectableSubsetParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
  QComboBox* selBox_;
  
  void insertSubset();
  
public:
  declareType(insight::SelectableSubsetParameter::typeName_());
  SelectableSubsetParameterWrapper
  (
      QTreeWidgetItem* parent,
      const QString& name,
      insight::Parameter& p,
      const insight::Parameter& defp,
      QWidget* detailw,
      QObject* superform
  );
  void createWidgets() override;
  inline insight::SelectableSubsetParameter& param()
  { return dynamic_cast<insight::SelectableSubsetParameter&>(p_); }
  
public Q_SLOTS:
  void onApply() override;
  void onUpdate() override;

Q_SIGNALS:
  void apply();
  void update();
};




void addWrapperToWidget
(
  insight::ParameterSet& pset,
  const insight::ParameterSet& default_pset,
  QTreeWidgetItem *parentnode, 
  QWidget *detaileditwidget,
  QObject *superform
);




#endif // PARAMETERWRAPPER_H
