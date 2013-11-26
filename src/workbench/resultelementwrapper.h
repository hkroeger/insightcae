/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  Hannes Kroeger <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef RESULTELEMENTWRAPPER_H
#define RESULTELEMENTWRAPPER_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>

#include "base/factory.h"
#include "base/resultset.h"

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

void addWrapperToWidget(insight::ResultSet& rset, QWidget *widget, QWidget *superform=NULL);

#endif // RESULTELEMENTWRAPPER_H
