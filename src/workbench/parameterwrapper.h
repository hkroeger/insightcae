/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2013  hannes <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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

#include "base/factory.h"
#include "base/parameter.h"
#include "base/parameterset.h"

#include "boost/tuple/tuple.hpp"
#include "boost/fusion/tuple.hpp"

class ParameterWrapper 
: public QWidget
{
  Q_OBJECT
  
public:
  typedef boost::tuple<QWidget *, const QString&, insight::Parameter&> ConstrP;
  
  declareFactoryTable(ParameterWrapper, ParameterWrapper::ConstrP);  

protected:
  QString name_;
  insight::Parameter& p_;
  
public:
  declareType("ParameterWrapper");
  ParameterWrapper(const ConstrP& p);
  virtual ~ParameterWrapper();
    
public slots:
    virtual void onApply() =0;
    virtual void onUpdate() =0;
};

class IntParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
protected:
  QLineEdit *le_;
public:
  declareType(insight::IntParameter::typeName_());
  
  IntParameterWrapper(const ConstrP& p);
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
  DoubleParameterWrapper(const ConstrP& p);
  inline insight::DoubleParameter& param() { return dynamic_cast<insight::DoubleParameter&>(p_); }
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
  StringParameterWrapper(const ConstrP& p);
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
  BoolParameterWrapper(const ConstrP& p);
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
  PathParameterWrapper(const ConstrP& p);
  inline insight::PathParameter& param() { return dynamic_cast<insight::PathParameter&>(p_); }

public slots:
  virtual void onApply();
  virtual void onUpdate();
  
protected slots:
  virtual void openSelectionDialog();
  virtual void onDataEntered();
};

class DirectoryParameterWrapper
: public PathParameterWrapper
{
  Q_OBJECT
public:
  declareType(insight::DirectoryParameter::typeName_());
  DirectoryParameterWrapper(const ConstrP& p);
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
  SelectionParameterWrapper(const ConstrP& p);
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
  SubsetParameterWrapper(const ConstrP& p);
  inline insight::SubsetParameter& param() { return dynamic_cast<insight::SubsetParameter&>(p_); }
  
public slots:
  virtual void onApply();
  virtual void onUpdate();
  
signals:
  void apply();
  void update();
};

class ArrayParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
protected:
  boost::ptr_vector<QWidget> entrywrappers_;
  QVBoxLayout *vlayout_;
  QGroupBox *group_;
  QSignalMapper *map_;
  
  void addWrapper();
  void rebuildWrappers();
  
public:
  declareType(insight::ArrayParameter::typeName_());
  ArrayParameterWrapper(const ConstrP& p);
  inline insight::ArrayParameter& param() { return dynamic_cast<insight::ArrayParameter&>(p_); }
  
protected slots:
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
  DoubleRangeParameterWrapper(const ConstrP& p);
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

void addWrapperToWidget(insight::ParameterSet& pset, QWidget *widget, QWidget *superform=NULL);

#endif // PARAMETERWRAPPER_H
