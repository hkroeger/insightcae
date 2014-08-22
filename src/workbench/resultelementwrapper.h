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


#ifndef RESULTELEMENTWRAPPER_H
#define RESULTELEMENTWRAPPER_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QTableWidget>

#ifndef Q_MOC_RUN
#include "base/factory.h"
#include "base/resultset.h"
#endif

class ResultElementWrapper 
: public QWidget
{
  Q_OBJECT
  
public:
  typedef boost::tuple<QWidget *, const QString&, insight::ResultElement&> ConstrP;
  
  declareFactoryTable(ResultElementWrapper, ResultElementWrapper::ConstrP);  

protected:
  QString name_;
  insight::ResultElement& p_;
  
public:
  declareType("ResultElementWrapper");
  ResultElementWrapper(const ConstrP& p);
  virtual ~ResultElementWrapper();
};

class ScalarResultWrapper
: public ResultElementWrapper
{
  Q_OBJECT
protected:
  QLabel *le_;
public:
  declareType(insight::ScalarResult::typeName_());
  ScalarResultWrapper(const ConstrP& p);
  inline insight::ScalarResult& res() { return dynamic_cast<insight::ScalarResult&>(p_); }
};

class ImageWrapper
: public ResultElementWrapper
{
  Q_OBJECT
protected:
  QLabel *le_;
public:
  declareType(insight::Image::typeName_());
  ImageWrapper(const ConstrP& p);
  inline insight::Image& res() { return dynamic_cast<insight::Image&>(p_); }
};

class TabularResultWrapper
: public ResultElementWrapper
{
  Q_OBJECT
protected:
  QTableWidget *le_;
public:
  declareType(insight::TabularResult::typeName_());
  TabularResultWrapper(const ConstrP& p);
  inline insight::TabularResult& res() { return dynamic_cast<insight::TabularResult&>(p_); }
};

class AttributeTableResultWrapper
: public ResultElementWrapper
{
  Q_OBJECT
protected:
  QTableWidget *le_;
public:
  declareType(insight::AttributeTableResult::typeName_());
  AttributeTableResultWrapper(const ConstrP& p);
  inline insight::AttributeTableResult& res() { return dynamic_cast<insight::AttributeTableResult&>(p_); }
};

void addWrapperToWidget(insight::ResultSet& rset, QWidget *widget, QWidget *superform=NULL);

#endif // RESULTELEMENTWRAPPER_H
