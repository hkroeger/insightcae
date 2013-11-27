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

using namespace boost;

void addWrapperToWidget(insight::ParameterSet& pset, QWidget *widget, QWidget *superform)
{
  QVBoxLayout *vlayout=new QVBoxLayout(widget);
  for(insight::ParameterSet::iterator i=pset.begin(); i!=pset.end(); i++)
      {
	ParameterWrapper *wrapper = 
	  ParameterWrapper::lookup
	  (
	    i->second->type(),
	    ParameterWrapper::ConstrP(widget, i->first.c_str(), *i->second)
	  );
	vlayout->addWidget(wrapper);
	if (superform) 
	{
	  QObject::connect(superform, SIGNAL(apply()), wrapper, SLOT(onApply()));
	  QObject::connect(superform, SIGNAL(update()), wrapper, SLOT(onUpdate()));
	}
      }
}

defineType(ParameterWrapper);
defineFactoryTable(ParameterWrapper, ParameterWrapper::ConstrP);

ParameterWrapper::ParameterWrapper(const ConstrP& p)
: QWidget(get<0>(p)),
  name_(get<1>(p)),
  p_(get<2>(p))
{
}

ParameterWrapper::~ParameterWrapper()
{

}

defineType(IntParameterWrapper);
addToFactoryTable(ParameterWrapper, IntParameterWrapper, ParameterWrapper::ConstrP);

IntParameterWrapper::IntParameterWrapper(const ConstrP& p)
: ParameterWrapper(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  le_=new QLineEdit(this);
  le_->setText(QString::number(param()()));
  le_->setValidator(new QIntValidator());
  le_->setToolTip(QString(param().description().c_str()));
  layout->addWidget(le_);
  this->setLayout(layout);
}

void IntParameterWrapper::onApply()
{
  param()()=le_->text().toInt();
}

void IntParameterWrapper::onUpdate()
{
  le_->setText(QString::number(param()()));
}

defineType(DoubleParameterWrapper);
addToFactoryTable(ParameterWrapper, DoubleParameterWrapper, ParameterWrapper::ConstrP);

DoubleParameterWrapper::DoubleParameterWrapper(const ConstrP& p)
: ParameterWrapper(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  le_=new QLineEdit(this);
  le_->setText(QString::number(param()()));
  le_->setValidator(new QDoubleValidator());
  le_->setToolTip(QString(param().description().c_str()));
  layout->addWidget(le_);
  this->setLayout(layout);
}

void DoubleParameterWrapper::onApply()
{
  param()()=le_->text().toDouble();
}

void DoubleParameterWrapper::onUpdate()
{
  le_->setText(QString::number(param()()));
}

defineType(StringParameterWrapper);
addToFactoryTable(ParameterWrapper, StringParameterWrapper, ParameterWrapper::ConstrP);

StringParameterWrapper::StringParameterWrapper(const ConstrP& p)
: ParameterWrapper(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  le_=new QLineEdit(this);
  le_->setToolTip(QString(param().description().c_str()));
  layout->addWidget(le_);
  this->setLayout(layout);
  onUpdate();
}

void StringParameterWrapper::onApply()
{
  param()()=le_->text().toStdString();
}

void StringParameterWrapper::onUpdate()
{
  le_->setText(param()().c_str());
}

defineType(BoolParameterWrapper);
addToFactoryTable(ParameterWrapper, BoolParameterWrapper, ParameterWrapper::ConstrP);

BoolParameterWrapper::BoolParameterWrapper(const ConstrP& p)
: ParameterWrapper(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  cb_=new QCheckBox(this);
  if (param()())
    cb_->setCheckState(Qt::Checked);
  else
    cb_->setCheckState(Qt::Unchecked);
  cb_->setToolTip(QString(param().description().c_str()));
  layout->addWidget(cb_);
  this->setLayout(layout);
}

void BoolParameterWrapper::onApply()
{
  param()() = (cb_->checkState() == Qt::Checked);
}

void BoolParameterWrapper::onUpdate()
{
  if (param()())
    cb_->setCheckState(Qt::Checked);
  else
    cb_->setCheckState(Qt::Unchecked);
}

defineType(PathParameterWrapper);
addToFactoryTable(ParameterWrapper, PathParameterWrapper, ParameterWrapper::ConstrP);

PathParameterWrapper::PathParameterWrapper(const ConstrP& p)
: ParameterWrapper(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  le_=new QLineEdit(this);
  le_->setText(param()().c_str());
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
    QString(param().description().c_str())
    +"\n"+
    "(Evaluates to \""+boost::filesystem::absolute(le_->text().toStdString()).c_str()+"\")"
  );
}

void PathParameterWrapper::onApply()
{
  param()()=le_->text().toStdString();
}

void PathParameterWrapper::onUpdate()
{
  le_->setText(param()().c_str());
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

defineType(DirectoryParameterWrapper);
addToFactoryTable(ParameterWrapper, DirectoryParameterWrapper, ParameterWrapper::ConstrP);

DirectoryParameterWrapper::DirectoryParameterWrapper(const ConstrP& p)
: PathParameterWrapper(p)
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

defineType(SelectionParameterWrapper);
addToFactoryTable(ParameterWrapper, SelectionParameterWrapper, ParameterWrapper::ConstrP);

SelectionParameterWrapper::SelectionParameterWrapper(const ConstrP& p)
: ParameterWrapper(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  selBox_=new QComboBox(this);
  BOOST_FOREACH( const std::string& s, param().items() )
  {
    selBox_->addItem(s.c_str());
  }
  layout->addWidget(selBox_);
  this->setLayout(layout);
}

void SelectionParameterWrapper::onApply()
{
  param()()=selBox_->currentIndex();
}

void SelectionParameterWrapper::onUpdate()
{
  selBox_->setCurrentIndex(param()());
}

defineType(SubsetParameterWrapper);
addToFactoryTable(ParameterWrapper, SubsetParameterWrapper, ParameterWrapper::ConstrP);


SubsetParameterWrapper::SubsetParameterWrapper(const ConstrP& p)
: ParameterWrapper(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QGroupBox *nameLabel = new QGroupBox(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  addWrapperToWidget(param()(), nameLabel, this);
  layout->addWidget(nameLabel);
  this->setLayout(layout);
}

void SubsetParameterWrapper::onApply()
{
  emit(apply());
}

void SubsetParameterWrapper::onUpdate()
{
  emit(update());
}

defineType(DoubleRangeParameterWrapper);
addToFactoryTable(ParameterWrapper, DoubleRangeParameterWrapper, ParameterWrapper::ConstrP);

DoubleRangeParameterWrapper::DoubleRangeParameterWrapper(const ConstrP& p)
: ParameterWrapper(p)
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
  for (insight::DoubleRangeParameter::RangeList::const_iterator i=param().values().begin(); i!=param().values().end(); i++)
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
    param().insertValue(v);
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
      param().insertValue(x);
    }
    rebuildList();
  }
}

void DoubleRangeParameterWrapper::onClear()
{
  param().values().clear();
  rebuildList();
}

void DoubleRangeParameterWrapper::onApply()
{
}

void DoubleRangeParameterWrapper::onUpdate()
{
  rebuildList();
}