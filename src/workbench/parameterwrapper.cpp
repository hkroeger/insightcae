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


VectorParameterWrapper::VectorParameterWrapper(const ConstrP& p)
: ParameterWrapper(p)
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  le_=new QLineEdit(this);
  
  //le_->setText(QString::number(param()()(0))+" "+QString::number(param()()(1))+" "+QString::number(param()()(2)));
  le_->setText(QString(insight::valueToString(param()()).c_str()));
  //le_->setValidator(new QDoubleValidator());
  le_->setToolTip(QString(param().description().c_str()));
  layout->addWidget(le_);
  this->setLayout(layout);
}

void VectorParameterWrapper::onApply()
{
//   QStringList sl=le_->text().split(" ", QString::SkipEmptyParts);
//   param()()=insight::vec3(sl[0].toDouble(), sl[1].toDouble(), sl[2].toDouble());
  insight::stringToValue(le_->text().toStdString(), param()());
}

void VectorParameterWrapper::onUpdate()
{
  //le_->setText(QString::number(param()()(0))+" "+QString::number(param()()(1))+" "+QString::number(param()()(2)));
  le_->setText(QString(insight::valueToString(param()()).c_str()));
  //le_->setText(QString::number(param()()));
}

defineType(VectorParameterWrapper);
addToFactoryTable(ParameterWrapper, VectorParameterWrapper, ParameterWrapper::ConstrP);


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

defineType(ArrayParameterWrapper);
addToFactoryTable(ParameterWrapper, ArrayParameterWrapper, ParameterWrapper::ConstrP);

void ArrayParameterWrapper::addWrapper()
{
  int i=entrywrappers_.size();
  
  insight::Parameter& pp=param()[i];
  
  QWidget *cont=new QWidget(group_);
  QHBoxLayout *innerlayout=new QHBoxLayout(cont);
  ParameterWrapper *wrapper = 
    ParameterWrapper::lookup
    (
      pp.type(),
      ParameterWrapper::ConstrP(cont, "["+QString::number(i)+"]", pp)
    );
  innerlayout->addWidget(wrapper);
  QPushButton *rmbtn=new QPushButton("-", cont);
  map_->setMapping(rmbtn, i);
  connect(rmbtn, SIGNAL(clicked()), map_, SLOT(map()));
  innerlayout->addWidget(rmbtn);
  vlayout_->addWidget(cont);
  entrywrappers_.push_back(cont);
  QObject::connect(this, SIGNAL(apply()), wrapper, SLOT(onApply()));
  QObject::connect(this, SIGNAL(update()), wrapper, SLOT(onUpdate()));  
}

void ArrayParameterWrapper::rebuildWrappers()
{
  entrywrappers_.clear();
  for(int i=0; i<param().size(); i++) addWrapper();
}

ArrayParameterWrapper::ArrayParameterWrapper(const ConstrP& p)
: ParameterWrapper(p),
  map_(new QSignalMapper(this))
{
  QHBoxLayout *layout=new QHBoxLayout(this);
  group_ = new QGroupBox(name_, this);
  QFont f=group_->font(); f.setBold(true); group_->setFont(f);
  
  vlayout_=new QVBoxLayout(group_);
  QPushButton *addbtn=new QPushButton("+ Add new", group_);
  connect(addbtn, SIGNAL(clicked()), this, SLOT(onAppendEmpty()));
  vlayout_->addWidget(addbtn);
  
  connect(map_, SIGNAL(mapped(int)), this, SLOT(onRemove(int)));

  rebuildWrappers();
      
  layout->addWidget(group_);
  this->setLayout(layout);
}

void ArrayParameterWrapper::onRemove(int i)
{
  emit(apply());
  entrywrappers_.erase(entrywrappers_.begin()+i);
  param().eraseValue(i);
  rebuildWrappers();
}

void ArrayParameterWrapper::onAppendEmpty()
{
  emit(apply());
  param().appendEmpty();
  rebuildWrappers();
}

void ArrayParameterWrapper::onApply()
{
  emit(apply());
}

void ArrayParameterWrapper::onUpdate()
{
  entrywrappers_.clear();
  rebuildWrappers();
  //emit(update());
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




SelectableSubsetParameterWrapper::SelectableSubsetParameterWrapper(const ConstrP& p)
: ParameterWrapper(p),
  name2Label_(NULL)
{
  layout0_=new QVBoxLayout(this);
  QHBoxLayout *layout=new QHBoxLayout(this);
  
  QLabel *nameLabel = new QLabel(name_, this);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  selBox_=new QComboBox(this);
  
  BOOST_FOREACH( const insight::SelectableSubsetParameter::ItemList::const_iterator::value_type& pair, param().items() )
  {
    selBox_->addItem(pair.first.c_str());
  }
  layout->addWidget(selBox_);
  layout0_->addLayout(layout);
  this->setLayout(layout0_);

  insertSubset();
  connect(selBox_, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(onCurrentIndexChanged(const QString&)));
}

void SelectableSubsetParameterWrapper::insertSubset()
{
  if (name2Label_)
  {
    layout0_->removeWidget(name2Label_);
    delete name2Label_;
  }
  
  param().selection()=selBox_->currentText().toStdString();
  name2Label_ = new QGroupBox(param().selection().c_str(), this);
  addWrapperToWidget(param()(), name2Label_, this);
  layout0_->addWidget(name2Label_);
  layout0_->update();
}

void SelectableSubsetParameterWrapper::onApply()
{
  param().selection()=selBox_->currentText().toStdString();
  emit(apply());
}

void SelectableSubsetParameterWrapper::onUpdate()
{
  //selBox_->setCurrentIndex(param()());
  selBox_->setCurrentIndex(selBox_->findText(QString(param().selection().c_str())));
  insertSubset();
  emit(update());
}

void SelectableSubsetParameterWrapper::onCurrentIndexChanged(const QString& qs)
{
  insertSubset();
}

defineType(SelectableSubsetParameterWrapper);
addToFactoryTable(ParameterWrapper, SelectableSubsetParameterWrapper, ParameterWrapper::ConstrP);
