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

#include "parameterwrapper.h"

#include <typeinfo>

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QGroupBox>
#include <QFileDialog>
#include <QInputDialog>

#include "boost/foreach.hpp"


void addWrapperToWidget(insight::ParameterSet& pset, QWidget *widget, QWidget *superform)
{
  QVBoxLayout *vlayout=new QVBoxLayout(widget);
  for(insight::ParameterSet::iterator i=pset.begin(); i!=pset.end(); i++)
      {
	ParameterWrapper *wrapper;
	if ( insight::DirectoryParameter* p = dynamic_cast<insight::DirectoryParameter*>(i->second) )
	{
	  wrapper=new DirectoryParameterWrapper(widget, i->first.c_str(), *p);
	}
	else if ( insight::DoubleRangeParameter* p = dynamic_cast<insight::DoubleRangeParameter*>(i->second) )
	{
	  wrapper=new DoubleRangeParameterWrapper(widget, i->first.c_str(), *p);
	}
	else if ( insight::PathParameter* p = dynamic_cast<insight::PathParameter*>(i->second) )
	{
	  wrapper=new PathParameterWrapper(widget, i->first.c_str(), *p);
	}
	else if ( insight::SelectionParameter* p = dynamic_cast<insight::SelectionParameter*>(i->second) )
	{
	  wrapper=new SelectionParameterWrapper(widget, i->first.c_str(), *p);
	}
	else if ( insight::IntParameter* p = dynamic_cast<insight::IntParameter*>(i->second) )
	{
	  wrapper=new IntParameterWrapper(widget, i->first.c_str(), *p);
	} 
	else if ( insight::DoubleParameter* p = dynamic_cast<insight::DoubleParameter*>(i->second) )
	{
	  wrapper=new DoubleParameterWrapper(widget, i->first.c_str(), *p);
	}
	else if ( insight::BoolParameter* p = dynamic_cast<insight::BoolParameter*>(i->second) )
	{
	  wrapper=new BoolParameterWrapper(widget, i->first.c_str(), *p);
	}
	else if ( insight::SubsetParameter* p = dynamic_cast<insight::SubsetParameter*>(i->second) )
	{
	  wrapper=new SubsetParameterWrapper(widget, i->first.c_str(), *p);
	}
	else 
	{
	  throw insight::Exception("Don't know how to handle parameter "+i->first);
	}
	vlayout->addWidget(wrapper);
	if (superform) QObject::connect(superform, SIGNAL(apply()), wrapper, SLOT(onApply()));
      }
}

ParameterWrapper::ParameterWrapper(QWidget* parent, const QString& name)
: QWidget(parent),
  name_(name)
{
}

ParameterWrapper::~ParameterWrapper()
{

}

IntParameterWrapper::IntParameterWrapper(QWidget* parent, const QString& name, insight::IntParameter& p)
: ParameterWrapper(parent, name),
  p_(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  le_=new QLineEdit(this);
  le_->setText(QString::number(p_()));
  le_->setValidator(new QIntValidator());
  le_->setToolTip(QString(p_.description().c_str()));
  layout->addWidget(le_);
  this->setLayout(layout);
}

void IntParameterWrapper::onApply()
{
  p_()=le_->text().toInt();
}

DoubleParameterWrapper::DoubleParameterWrapper(QWidget* parent, const QString& name, insight::DoubleParameter& p)
: ParameterWrapper(parent, name),
  p_(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  le_=new QLineEdit(this);
  le_->setText(QString::number(p_()));
  le_->setValidator(new QDoubleValidator());
  le_->setToolTip(QString(p_.description().c_str()));
  layout->addWidget(le_);
  this->setLayout(layout);
}

void DoubleParameterWrapper::onApply()
{
  p_()=le_->text().toDouble();
}

BoolParameterWrapper::BoolParameterWrapper(QWidget* parent, const QString& name, insight::BoolParameter& p)
: ParameterWrapper(parent, name),
  p_(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  cb_=new QCheckBox(this);
  if (p_())
    cb_->setCheckState(Qt::Checked);
  else
    cb_->setCheckState(Qt::Unchecked);
  cb_->setToolTip(QString(p_.description().c_str()));
  layout->addWidget(cb_);
  this->setLayout(layout);
}

void BoolParameterWrapper::onApply()
{
  p_() = (cb_->checkState() == Qt::Checked);
}

PathParameterWrapper::PathParameterWrapper(QWidget* parent, const QString& name, insight::PathParameter& p)
: ParameterWrapper(parent, name),
  p_(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  le_=new QLineEdit(this);
  le_->setText(p_().c_str());
  layout->addWidget(le_);
  dlgBtn_=new QPushButton("...", this);
  layout->addWidget(dlgBtn_);
  updateTooltip();
  this->setLayout(layout);
  
  connect(le_, SIGNAL(textChanged(QString)), this, SLOT(onDataEntered()));
  connect(dlgBtn_, SIGNAL(clicked(bool)), this, SLOT(openSelectionDialog()));
}

void PathParameterWrapper::updateTooltip()
{
  le_->setToolTip
  (
    QString(p_.description().c_str())
    +"\n"+
    "(Evaluates to \""+boost::filesystem::absolute(le_->text().toStdString()).c_str()+"\")"
  );
}

void PathParameterWrapper::onApply()
{
  p_()=le_->text().toStdString();
}

void PathParameterWrapper::openSelectionDialog()
{
  QString fn = QFileDialog::getOpenFileName(this, 
						"Select file",
						le_->text());
  if (!fn.isEmpty())
    le_->setText(fn);
}

void PathParameterWrapper::onDataEntered()
{
  updateTooltip();
}

DirectoryParameterWrapper::DirectoryParameterWrapper(QWidget* parent, const QString& name, insight::PathParameter& p)
: PathParameterWrapper(parent, name, p)
{
}

void DirectoryParameterWrapper::openSelectionDialog()
{
  QString fn = QFileDialog::getExistingDirectory(this, 
						 "Select directory",
						le_->text());
  if (!fn.isEmpty())
    le_->setText(fn);
}
  
SelectionParameterWrapper::SelectionParameterWrapper(QWidget* parent, const QString& name, insight::SelectionParameter& p)
: ParameterWrapper(parent, name),
  p_(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  selBox_=new QComboBox(this);
  BOOST_FOREACH( const std::string& s, p_.items() )
  {
    selBox_->addItem(s.c_str());
  }
  layout->addWidget(selBox_);
  this->setLayout(layout);
}

void SelectionParameterWrapper::onApply()
{
  //p_() = (cb_->checkState() == Qt::Checked);
}

SubsetParameterWrapper::SubsetParameterWrapper(QWidget* parent, const QString& name, insight::SubsetParameter& p)
: ParameterWrapper(parent, name),
  p_(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QGroupBox *nameLabel = new QGroupBox(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  addWrapperToWidget(p_(), nameLabel, this);
  layout->addWidget(nameLabel);
  this->setLayout(layout);
}

void SubsetParameterWrapper::onApply()
{
  emit(apply());
}

DoubleRangeParameterWrapper::DoubleRangeParameterWrapper(QWidget* parent, const QString& name, insight::DoubleRangeParameter& p)
: ParameterWrapper(parent, name),
  p_(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  
  lBox_=new QListWidget(this);
  rebuildList();
  layout->addWidget(lBox_);
  
  this->setLayout(layout);
  
  QVBoxLayout *sublayout=new QVBoxLayout(this);
  layout->addLayout(sublayout);
  
  QPushButton *addbtn=new QPushButton("Add...", this);
  sublayout->addWidget(addbtn);
  connect(addbtn, SIGNAL(clicked()), this, SLOT(onAddSingle()));
  QPushButton *addrangebtn=new QPushButton("Add Range...", this);
  sublayout->addWidget(addrangebtn);
  connect(addrangebtn, SIGNAL(clicked()), this, SLOT(onAddRange()));
  QPushButton *clearbtn=new QPushButton("Clear", this);
  sublayout->addWidget(clearbtn);
  connect(clearbtn, SIGNAL(clicked()), this, SLOT(onClear()));
}

void DoubleRangeParameterWrapper::rebuildList()
{
  int crow=lBox_->currentRow();
  lBox_->clear();
  for (insight::DoubleRangeParameter::RangeList::const_iterator i=p_.values().begin(); i!=p_.values().end(); i++)
  {
    lBox_->addItem( QString::number(*i) );
  }
  lBox_->setCurrentRow(crow);
}

void DoubleRangeParameterWrapper::onAddSingle()
{
  bool ok;
  double v=QInputDialog::getDouble(this, "Add Range", "Please specify value:", 0., -2147483647,  2147483647, 9, &ok);
  if (ok)
  {
    p_.insertValue(v);
    rebuildList();
  }
}

void DoubleRangeParameterWrapper::onAddRange()
{
  QString res=QInputDialog::getText(this, "Add Range", "Please specify range begin, range end and number of values, separated by spaces:");
  if (!res.isEmpty())
  {
    QStringList il=res.split(" ", QString::SkipEmptyParts);
    double x0=il[0].toDouble();
    double x1=il[1].toDouble();
    int num=il[2].toInt();
    for (int i=0; i<num; i++)
    {
      double x=x0+(x1-x0)*double(i)/double(num-1);
      p_.insertValue(x);
    }
    rebuildList();
  }
}

void DoubleRangeParameterWrapper::onClear()
{
  p_.values().clear();
  rebuildList();
}

void DoubleRangeParameterWrapper::onApply()
{
  //p_() = (cb_->checkState() == Qt::Checked);
}