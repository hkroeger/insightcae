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

#include "base/parameter.h"
#include "base/parameterset.h"

class ParameterWrapper 
: public QWidget
{
  Q_OBJECT
protected:
  QString name_;
public:
    ParameterWrapper(QWidget *parent, const QString& name);
    virtual ~ParameterWrapper();
    
public slots:
    virtual void onApply() =0;
};

class IntParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
protected:
  insight::IntParameter& p_;
  QLineEdit *le_;
public:
  IntParameterWrapper(QWidget* parent, const QString& name, insight::IntParameter& p);

public slots:
  virtual void onApply();
};

class DoubleParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
protected:
  insight::DoubleParameter& p_;
  QLineEdit *le_;
public:
  DoubleParameterWrapper(QWidget* parent, const QString& name, insight::DoubleParameter& p);
public slots:
  virtual void onApply();
};

class BoolParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
protected:
  insight::BoolParameter& p_;
  QCheckBox *cb_;
public:
  BoolParameterWrapper(QWidget* parent, const QString& name, insight::BoolParameter& p);
public slots:
  virtual void onApply();
};

class PathParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
protected:
  insight::PathParameter& p_;
  QLineEdit *le_;
  QPushButton *dlgBtn_;
  
  virtual void updateTooltip();
  
public:
  PathParameterWrapper(QWidget* parent, const QString& name, insight::PathParameter& p);
  
public slots:
  virtual void onApply();
  
protected slots:
  virtual void openSelectionDialog();
  virtual void onDataEntered();
};

class DirectoryParameterWrapper
: public PathParameterWrapper
{
  Q_OBJECT
public:
  DirectoryParameterWrapper(QWidget* parent, const QString& name, insight::PathParameter& p);

protected slots:
  virtual void openSelectionDialog();

};


class SelectionParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
protected:
  insight::SelectionParameter& p_;
  
  QComboBox* selBox_;

public:
  SelectionParameterWrapper(QWidget* parent, const QString& name, insight::SelectionParameter& p);
public slots:
  virtual void onApply();
};

class SubsetParameterWrapper
: public ParameterWrapper
{
  Q_OBJECT
  
protected:
  insight::SubsetParameter& p_;

public:
  SubsetParameterWrapper(QWidget* parent, const QString& name, insight::SubsetParameter& p);
  
public slots:
  virtual void onApply();
  
signals:
  void apply();
};

void addWrapperToWidget(insight::ParameterSet& pset, QWidget *widget, QWidget *superform=NULL);

#endif // PARAMETERWRAPPER_H
