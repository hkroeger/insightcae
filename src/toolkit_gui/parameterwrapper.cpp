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
#include <QWebView>

#ifndef Q_MOC_RUN
#include "boost/foreach.hpp"
#endif

#include "base/tools.h"
#include "boost/spirit.hpp"

using namespace boost;


void addWrapperToWidget
(
    insight::ParameterSet& pset,
    QTreeWidgetItem *parentnode,
    QWidget *detaileditwidget,
    QWidget *superform
)
{
//   QVBoxLayout *vlayout=new QVBoxLayout(widget);
    for ( insight::ParameterSet::iterator i=pset.begin(); i!=pset.end(); i++ ) {
        ParameterWrapper *wrapper =
            ParameterWrapper::lookup
            (
                i->second->type(),
                parentnode, i->first.c_str(), *i->second, detaileditwidget, superform
            );

        QObject::connect ( parentnode->treeWidget(), SIGNAL ( itemSelectionChanged() ),
                           wrapper, SLOT ( onSelectionChanged() ) );
// 	vlayout->addWidget(wrapper);
        if ( superform ) {
            QObject::connect ( superform, SIGNAL ( apply() ), wrapper, SLOT ( onApply() ) );
            QObject::connect ( superform, SIGNAL ( update() ), wrapper, SLOT ( onUpdate() ) );
        }
    }
}


defineType(ParameterWrapper);
defineFactoryTable
(
    ParameterWrapper, 
    LIST(QTreeWidgetItem *parent, const QString& name, insight::Parameter& p, QWidget*detailw, QWidget*superform),
    LIST(parent, name, p, detailw, superform)
);


void ParameterWrapper::focusInEvent(QFocusEvent* e)
{
//     QWidget::focusInEvent(e);
//     std::cout<<p_.description()<<std::endl;
}


ParameterWrapper::ParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform)
: QTreeWidgetItem(parent),
  QObject(),
  name_(name),
  p_(p),
  detaileditwidget_(detailw),
  superform_(superform),
  widgetsDisplayed_(false)
{
  setText(0, name_);
  QFont f=font(1);
  f.setItalic(true);
  setFont(1, f);
}

ParameterWrapper::~ParameterWrapper()
{
}

void ParameterWrapper::createWidgets()
{
  widgetsDisplayed_=true;
}

void ParameterWrapper::removedWidgets()
{
  widgetsDisplayed_=false;
}

void ParameterWrapper::onSelectionChanged()
{
  QList<QTreeWidgetItem*> sel=treeWidget()->selectedItems();
  ParameterWrapper* ptr=dynamic_cast<ParameterWrapper*>(sel[0]);
//   std::cout<<sel.size()<<"; "<<ptr<<" <> "<<this<<"  : "<<(ptr==this)<<std::endl;
  if ( (sel.size()==1) && ptr )
  {
    if (ptr==this)
    {
      onSelection();
    }
  }
}

void ParameterWrapper::onSelection()
{
//   std::cout<<name_.toStdString()<<"!"<<detaileditwidget_<< std::endl;
  
  QList<QWidget*> widgets = detaileditwidget_->findChildren<QWidget*>();
  foreach(QWidget* widget, widgets)
  {
    widget->deleteLater();
  }
  
  if (detaileditwidget_->layout())
  {
    delete detaileditwidget_->layout();
  }

  createWidgets();
}

void ParameterWrapper::onDestruction()
{
  removedWidgets();
}




defineType(IntParameterWrapper);
addToFactoryTable(ParameterWrapper, IntParameterWrapper);

IntParameterWrapper::IntParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform)
: ParameterWrapper(parent, name, p, detailw, superform)
{
  onUpdate();
}

void IntParameterWrapper::createWidgets()
{
  ParameterWrapper::createWidgets();
  
  QVBoxLayout *layout=new QVBoxLayout(detaileditwidget_);
  
  QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  
  QWebView *shortDescLabel = 
    new QWebView( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  le_=new QLineEdit(detaileditwidget_);
  connect(le_, SIGNAL(destroyed(void)), this, SLOT(onDestruction(void)));
  le_->setText(QString::number(param()()));
  le_->setValidator(new QIntValidator());
  connect(le_, SIGNAL(returnPressed()), this, SLOT(onApply()));
  layout2->addWidget(le_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, SIGNAL(pressed()), this, SLOT(onApply()));
  layout->addWidget(apply);
  
  layout->addStretch();

  detaileditwidget_->setLayout(layout);
}

void IntParameterWrapper::onApply()
{
  if (widgetsDisplayed_)
  {
    param()()=le_->text().toInt();
    setText(1, QString::number(param()()));
  }
}

void IntParameterWrapper::onUpdate()
{
  setText(1, QString::number(param()()));
  if (widgetsDisplayed_) le_->setText(QString::number(param()()));
}




defineType(DoubleParameterWrapper);
addToFactoryTable(ParameterWrapper, DoubleParameterWrapper);

DoubleParameterWrapper::DoubleParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform)
: ParameterWrapper(parent, name, p, detailw, superform),
  le_(NULL)
{
  onUpdate();
}

void DoubleParameterWrapper::createWidgets()
{
  ParameterWrapper::createWidgets();

  QVBoxLayout *layout=new QVBoxLayout(detaileditwidget_);
  
  QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  
  QWebView *shortDescLabel = 
    new QWebView( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  le_=new QLineEdit(detaileditwidget_);
  connect(le_, SIGNAL(destroyed(void)), this, SLOT(onDestruction(void)));
  le_->setText(QString::number(param()()));
  le_->setValidator(new QDoubleValidator());
  connect(le_, SIGNAL(returnPressed()), this, SLOT(onApply()));
  layout2->addWidget(le_);
  
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, SIGNAL(pressed()), this, SLOT(onApply()));
  layout->addWidget(apply);
  
  layout->addStretch();
  detaileditwidget_->setLayout(layout);

//   QHBoxLayout *layout=new QHBoxLayout(detaileditwidget_);
//   QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
//   QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
//   layout->addWidget(nameLabel);
//   le_=new QLineEdit(detaileditwidget_);
//   le_->setText(QString::number(param()()));
//   le_->setValidator(new QDoubleValidator());
//   le_->setToolTip(QString(param().description().c_str()));
//   layout->addWidget(le_);
//   detaileditwidget_->setLayout(layout);
}

void DoubleParameterWrapper::onApply()
{
  if (widgetsDisplayed_)
  {
    param()()=le_->text().toDouble();
    setText(1, QString::number(param()()));
  }
}

void DoubleParameterWrapper::onUpdate()
{
  setText(1, QString::number(param()()));
  if (widgetsDisplayed_) le_->setText(QString::number(param()()));
}




defineType(StringParameterWrapper);
addToFactoryTable(ParameterWrapper, StringParameterWrapper);


VectorParameterWrapper::VectorParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform)
: ParameterWrapper(parent, name, p, detailw, superform),
  le_(NULL)
{
  onUpdate();
}

void VectorParameterWrapper::createWidgets()
{
  ParameterWrapper::createWidgets();

  QVBoxLayout *layout=new QVBoxLayout(detaileditwidget_);
  
  QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  
  QWebView *shortDescLabel = 
    new QWebView( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  le_=new QLineEdit(detaileditwidget_);
  connect(le_, SIGNAL(destroyed(void)), this, SLOT(onDestruction(void)));
  le_->setText(QString(insight::valueToString(param()()).c_str()));
  connect(le_, SIGNAL(returnPressed()), this, SLOT(onApply()));
  layout2->addWidget(le_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, SIGNAL(pressed()), this, SLOT(onApply()));
  layout->addWidget(apply);

  layout->addStretch();
  detaileditwidget_->setLayout(layout);

//   QHBoxLayout *layout=new QHBoxLayout(detaileditwidget_);
//   QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
//   QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
//   layout->addWidget(nameLabel);
//   le_=new QLineEdit(detaileditwidget_);
//   
//   le_->setText(QString(insight::valueToString(param()()).c_str()));
//   le_->setToolTip(QString(param().description().c_str()));
//   layout->addWidget(le_);
//   detaileditwidget_->setLayout(layout);
}

void VectorParameterWrapper::onApply()
{
//   QStringList sl=le_->text().split(" ", QString::SkipEmptyParts);
//   param()()=insight::vec3(sl[0].toDouble(), sl[1].toDouble(), sl[2].toDouble());
  if (widgetsDisplayed_)
  {
    insight::stringToValue(le_->text().toStdString(), param()());
    setText(1, QString(insight::valueToString(param()()).c_str()));
  }
}

void VectorParameterWrapper::onUpdate()
{
  setText(1, QString(insight::valueToString(param()()).c_str()));
  //le_->setText(QString::number(param()()(0))+" "+QString::number(param()()(1))+" "+QString::number(param()()(2)));
  if (widgetsDisplayed_) le_->setText(QString(insight::valueToString(param()()).c_str()));
  //le_->setText(QString::number(param()()));
}




defineType(VectorParameterWrapper);
addToFactoryTable(ParameterWrapper, VectorParameterWrapper);


StringParameterWrapper::StringParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform)
: ParameterWrapper(parent, name, p, detailw, superform),
  le_(NULL)
{
  onUpdate();
}

void StringParameterWrapper::createWidgets()
{
  ParameterWrapper::createWidgets();

  QVBoxLayout *layout=new QVBoxLayout(detaileditwidget_);
  
  QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  
  QWebView *shortDescLabel = 
    new QWebView( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  le_=new QLineEdit(detaileditwidget_);
  connect(le_, SIGNAL(destroyed(void)), this, SLOT(onDestruction(void)));
  connect(le_, SIGNAL(returnPressed()), this, SLOT(onApply()));
  layout2->addWidget(le_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, SIGNAL(pressed()), this, SLOT(onApply()));
  layout->addWidget(apply);

  layout->addStretch();
  detaileditwidget_->setLayout(layout);

//   QHBoxLayout *layout=new QHBoxLayout(detaileditwidget_);
//   QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
//   QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
//   layout->addWidget(nameLabel);
//   le_=new QLineEdit(detaileditwidget_);
//   le_->setToolTip(QString(param().description().c_str()));
//   layout->addWidget(le_);
//   detaileditwidget_->setLayout(layout);
  onUpdate();
}

void StringParameterWrapper::onApply()
{
  if (widgetsDisplayed_)
  {
    param()()=le_->text().toStdString();
    setText(1, param()().c_str());
  }
}

void StringParameterWrapper::onUpdate()
{
  setText(1, param()().c_str());
  if (widgetsDisplayed_) le_->setText(param()().c_str());
}




defineType(BoolParameterWrapper);
addToFactoryTable(ParameterWrapper, BoolParameterWrapper);

BoolParameterWrapper::BoolParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform)
: ParameterWrapper(parent, name, p, detailw, superform),
  cb_(NULL)
{
  onUpdate();
}

void BoolParameterWrapper::createWidgets()
{
  ParameterWrapper::createWidgets();

  QVBoxLayout *layout=new QVBoxLayout(detaileditwidget_);
  
  QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  
  QWebView *shortDescLabel = 
    new QWebView( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  cb_=new QCheckBox(detaileditwidget_);
  connect(cb_, SIGNAL(destroyed(void)), this, SLOT(onDestruction(void)));
  if (param()())
    cb_->setCheckState(Qt::Checked);
  else
    cb_->setCheckState(Qt::Unchecked);
  layout2->addWidget(cb_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, SIGNAL(pressed()), this, SLOT(onApply()));
  layout->addWidget(apply);

  layout->addStretch();
  detaileditwidget_->setLayout(layout);

//   QHBoxLayout *layout=new QHBoxLayout(detaileditwidget_);
//   QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
//   QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
//   layout->addWidget(nameLabel);
//   cb_=new QCheckBox(detaileditwidget_);
//   if (param()())
//     cb_->setCheckState(Qt::Checked);
//   else
//     cb_->setCheckState(Qt::Unchecked);
//   cb_->setToolTip(QString(param().description().c_str()));
//   layout->addWidget(cb_);
//   detaileditwidget_->setLayout(layout);
}

void BoolParameterWrapper::onApply()
{
  if (widgetsDisplayed_)
  {
    param()() = (cb_->checkState() == Qt::Checked);
    setText(1, param()() ? "true" : "false");
  }
}

void BoolParameterWrapper::onUpdate()
{
  setText(1, param()() ? "true" : "false");
  if (widgetsDisplayed_)
  {
    if (param()())
      cb_->setCheckState(Qt::Checked);
    else
      cb_->setCheckState(Qt::Unchecked);
  }
}




defineType(PathParameterWrapper);
addToFactoryTable(ParameterWrapper, PathParameterWrapper);

PathParameterWrapper::PathParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform)
: ParameterWrapper(parent, name, p, detailw, superform),
  le_(NULL)
{
  onUpdate();
}

void PathParameterWrapper::createWidgets()
{
  ParameterWrapper::createWidgets();

  QVBoxLayout *layout=new QVBoxLayout(detaileditwidget_);
  
  QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  
  QWebView *shortDescLabel = 
    new QWebView( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  le_=new QLineEdit(detaileditwidget_);
  connect(le_, SIGNAL(destroyed(void)), this, SLOT(onDestruction(void)));
  connect(le_, SIGNAL(returnPressed()), this, SLOT(onApply()));
  le_->setText(param()().c_str());
  layout2->addWidget(le_);
  dlgBtn_=new QPushButton("...", detaileditwidget_);
  layout2->addWidget(dlgBtn_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, SIGNAL(pressed()), this, SLOT(onApply()));
  layout->addWidget(apply);

  layout->addStretch();
  detaileditwidget_->setLayout(layout);

//   QHBoxLayout *layout=new QHBoxLayout(detaileditwidget_);
//   QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
//   QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
//   layout->addWidget(nameLabel);
//   le_=new QLineEdit(detaileditwidget_);
//   le_->setText(param()().c_str());
//   layout->addWidget(le_);
//   dlgBtn_=new QPushButton("...", detaileditwidget_);
//   layout->addWidget(dlgBtn_);
//   updateTooltip();
//   detaileditwidget_->setLayout(layout);
  
  connect(le_, SIGNAL(textChanged(QString)), this, SLOT(onDataEntered()));
  connect(dlgBtn_, SIGNAL(clicked(bool)), this, SLOT(openSelectionDialog()));
}

void PathParameterWrapper::updateTooltip()
{
  le_->setToolTip
  (
    QString(param().description().toHTML().c_str())
    +"\n"+
    "(Evaluates to \""+boost::filesystem::absolute(le_->text().toStdString()).c_str()+"\")"
  );
}

void PathParameterWrapper::onApply()
{
  if (widgetsDisplayed_)
  {
    param()()=le_->text().toStdString();
    setText(1, param()().c_str());
  }
}

void PathParameterWrapper::onUpdate()
{
  setText(1, param()().c_str());
  if (widgetsDisplayed_) le_->setText(param()().c_str());
}

void PathParameterWrapper::openSelectionDialog()
{
  QString fn = QFileDialog::getOpenFileName(treeWidget(), 
						"Select file",
						le_->text());
  if (!fn.isEmpty())
  {
    le_->setText(fn);
    onApply();
  }
}

void PathParameterWrapper::onDataEntered()
{
  updateTooltip();
}




defineType(MatrixParameterWrapper);
addToFactoryTable(ParameterWrapper, MatrixParameterWrapper);

QString mat2Str(const arma::mat& m)
{
  std::ostringstream oss;
  for (int i=0; i<m.n_rows; i++)
  {
    for (int j=0; j<m.n_cols; j++)
    {
      oss<<m(i,j);
      if (j!=m.n_cols-1) oss<<" ";
    }
    if (i!=m.n_rows-1) oss<<";";
  }
  return QString(oss.str().c_str());
}

MatrixParameterWrapper::MatrixParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform)
: ParameterWrapper(parent, name, p, detailw, superform),
  le_(NULL)
{
  onUpdate();
}

void MatrixParameterWrapper::createWidgets()
{
  ParameterWrapper::createWidgets();

  QVBoxLayout *layout=new QVBoxLayout(detaileditwidget_);
  
  QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  
  QWebView *shortDescLabel = 
    new QWebView( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  le_=new QLineEdit(detaileditwidget_);
  connect(le_, SIGNAL(destroyed(void)), this, SLOT(onDestruction(void)));
  connect(le_, SIGNAL(returnPressed()), this, SLOT(onApply()));
  le_->setText(mat2Str(param()()));
  layout2->addWidget(le_);
  dlgBtn_=new QPushButton("...", detaileditwidget_);
  layout2->addWidget(dlgBtn_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, SIGNAL(pressed()), this, SLOT(onApply()));
  layout->addWidget(apply);

  layout->addStretch();
  detaileditwidget_->setLayout(layout);

//   QHBoxLayout *layout=new QHBoxLayout(detaileditwidget_);
//   QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
//   QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
//   layout->addWidget(nameLabel);
//   le_=new QLineEdit(detaileditwidget_);
//   le_->setText(mat2Str(param()()));
//   layout->addWidget(le_);
//   dlgBtn_=new QPushButton("...", detaileditwidget_);
//   layout->addWidget(dlgBtn_);
//   detaileditwidget_->setLayout(layout);
  
  connect(dlgBtn_, SIGNAL(clicked(bool)), this, SLOT(openSelectionDialog()));
}



void MatrixParameterWrapper::onApply()
{
  if (widgetsDisplayed_)
  {
    param()()=arma::mat(le_->text().toStdString());
  }
}

void MatrixParameterWrapper::onUpdate()
{
  setText(1, "matrix");
  if (widgetsDisplayed_) le_->setText(mat2Str(param()()));
}

void MatrixParameterWrapper::openSelectionDialog()
{
  QString fn = QFileDialog::getOpenFileName(treeWidget(), 
						"Select file",
						le_->text());
  if (!fn.isEmpty())
  {
    arma::mat m;
    m.load(fn.toStdString().c_str(), arma::raw_ascii);
    le_->setText(mat2Str(m));
  }
}




defineType(DirectoryParameterWrapper);
addToFactoryTable(ParameterWrapper, DirectoryParameterWrapper);

DirectoryParameterWrapper::DirectoryParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform)
: PathParameterWrapper(parent, name, p, detailw, superform)
{
}

void DirectoryParameterWrapper::openSelectionDialog()
{
  QString fn = QFileDialog::getExistingDirectory(treeWidget(), 
						 "Select directory",
						le_->text());
  if (!fn.isEmpty())
    le_->setText(fn);
}




defineType(SelectionParameterWrapper);
addToFactoryTable(ParameterWrapper, SelectionParameterWrapper);

SelectionParameterWrapper::SelectionParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform)
: ParameterWrapper(parent, name, p, detailw, superform),
  selBox_(NULL)
{
  onUpdate();
}

void SelectionParameterWrapper::createWidgets()
{
  ParameterWrapper::createWidgets();

  QVBoxLayout *layout=new QVBoxLayout(detaileditwidget_);
  
  QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  
  QWebView *shortDescLabel = 
    new QWebView( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Selection:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  selBox_=new QComboBox(detaileditwidget_);
  connect(selBox_, SIGNAL(destroyed(void)), this, SLOT(onDestruction(void)));
  BOOST_FOREACH( const std::string& s, param().items() )
  {
    selBox_->addItem(s.c_str());
  }
  selBox_->setCurrentIndex(param()());
  layout2->addWidget(selBox_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, SIGNAL(pressed()), this, SLOT(onApply()));
  layout->addWidget(apply);

  layout->addStretch();
  detaileditwidget_->setLayout(layout);

//   QHBoxLayout *layout=new QHBoxLayout(detaileditwidget_);
//   QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
//   QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
//   layout->addWidget(nameLabel);
//   selBox_=new QComboBox(detaileditwidget_);
//   BOOST_FOREACH( const std::string& s, param().items() )
//   {
//     selBox_->addItem(s.c_str());
//   }
//   layout->addWidget(selBox_);
//   detaileditwidget_->setLayout(layout);
}

void SelectionParameterWrapper::onApply()
{
  if (widgetsDisplayed_)
  {
    param()()=selBox_->currentIndex();
    setText(1, param().selection().c_str());
  }
}

void SelectionParameterWrapper::onUpdate()
{
  setText(1, param().selection().c_str());
  if (widgetsDisplayed_) selBox_->setCurrentIndex(param()());
}




defineType(SubsetParameterWrapper);
addToFactoryTable(ParameterWrapper, SubsetParameterWrapper);


SubsetParameterWrapper::SubsetParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform)
: ParameterWrapper(parent, name, p, detailw, superform)
{
  addWrapperToWidget(param()(), this, detaileditwidget_, superform_);
}

void SubsetParameterWrapper::createWidgets()
{
  ParameterWrapper::createWidgets();

  QVBoxLayout *layout=new QVBoxLayout(detaileditwidget_);
  
  QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  
  QWebView *shortDescLabel = 
    new QWebView( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);
  
//   QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
//   connect(apply, SIGNAL(pressed()), this, SLOT(onApply()));
//   layout->addWidget(apply);

  layout->addStretch();
  detaileditwidget_->setLayout(layout);
  connect(detaileditwidget_, SIGNAL(destroyed(void)), this, SLOT(onDestruction(void)));
  
//   QHBoxLayout *layout=new QHBoxLayout(detaileditwidget_);
//   QGroupBox *nameLabel = new QGroupBox(name_, detaileditwidget_);
//   QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
//   layout->addWidget(nameLabel);
//   detaileditwidget_->setLayout(layout);

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
addToFactoryTable(ParameterWrapper, ArrayParameterWrapper);

void ArrayParameterWrapper::addWrapper(int i)
{
  insight::Parameter& pp=param()[i];
  
  ParameterWrapper *wrapper = 
    ParameterWrapper::lookup
    (
      pp.type(),
      this, "["+QString::number(i)+"]", pp, detaileditwidget_, superform_
    );

  QObject::connect(treeWidget(), SIGNAL(itemSelectionChanged()), wrapper, SLOT(onSelectionChanged()));
  QObject::connect(this, SIGNAL(apply()), wrapper, SLOT(onApply()));
  QObject::connect(this, SIGNAL(update()), wrapper, SLOT(onUpdate()));  
}

void ArrayParameterWrapper::rebuildWrappers()
{
  QList<QTreeWidgetItem*> cl=this->takeChildren();
  foreach(QTreeWidgetItem * ci, cl)
  {
    delete ci;
  }
  
  for(int i=0; i<param().size(); i++) 
  {
    addWrapper(i);
  }
}

ArrayParameterWrapper::ArrayParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform)
: ParameterWrapper(parent, name, p, detailw, superform)/*,
  map_(new QSignalMapper(this))*/
{
  setText(1, "array");
  connect
  (
    treeWidget(),
    SIGNAL(customContextMenuRequested(const QPoint &)),
    this,
    SLOT(showContextMenuForWidget(const QPoint &))
  );
  
  rebuildWrappers();
}

void ArrayParameterWrapper::showContextMenuForWidget ( const QPoint &p )
{
  QTreeWidgetItem* sel=treeWidget()->itemAt ( p );
  if ( sel )
    {
      ArrayParameterWrapper * mi =
        dynamic_cast<ArrayParameterWrapper*> ( sel->parent() );

//       std::cout<<"sel="<<sel<<", mi="<<mi<<", this="<<this<<std::endl;

      if ( mi==this )
        {
          QMenu myMenu;
          myMenu.addAction ( "Delete" );

          QAction* selectedItem = myMenu.exec ( treeWidget()->mapToGlobal ( p ) );

          if ( selectedItem )
            {
              if ( selectedItem->text() =="Delete" )
                {
                  int todel=-1;
                  for ( int j=0; j<this->childCount(); j++ )
                    {
                      if ( this->child ( j ) ==sel )
                        {
//                           std::cout<<"j="<<j<<std::endl;
                          todel=j;
                          break;
                        }
                    }
                  if ( todel>=0 )
                    {
                      onRemove ( todel );
                    }
                }
            }
          else
            {
              // nothing was chosen
            }
        }
    }
}

void ArrayParameterWrapper::createWidgets()
{
  ParameterWrapper::createWidgets();

  QVBoxLayout *layout=new QVBoxLayout(detaileditwidget_);
  QLabel *label = new QLabel(name_, detaileditwidget_);
  QFont f=label->font(); f.setBold(true); label->setFont(f);
  layout->addWidget(label);
  
  QWebView *shortDescLabel = 
    new QWebView( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QPushButton *addbtn=new QPushButton("+ Add new", detaileditwidget_);
  connect(addbtn, SIGNAL(clicked()), this, SLOT(onAppendEmpty()));
  layout2->addWidget(addbtn);
  
//   connect(map_, SIGNAL(mapped(int)), detaileditwidget_, SLOT(onRemove(int)));

  rebuildWrappers();
      
  layout->addLayout(layout2);
  layout->addStretch();
  detaileditwidget_->setLayout(layout);
}

void ArrayParameterWrapper::onRemove(int i)
{
  emit(apply());
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
//   entrywrappers_.clear();
  rebuildWrappers();
  //emit(update());
}




defineType(DoubleRangeParameterWrapper);
addToFactoryTable(ParameterWrapper, DoubleRangeParameterWrapper);

DoubleRangeParameterWrapper::DoubleRangeParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform)
: ParameterWrapper(parent, name, p, detailw, superform)
{
  setText(1, "doubleRange");
}

void DoubleRangeParameterWrapper::createWidgets()
{
  ParameterWrapper::createWidgets();

  QVBoxLayout *layout=new QVBoxLayout(detaileditwidget_);
  
  QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  
  QWebView *shortDescLabel = 
    new QWebView( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QLabel *promptLabel = new QLabel("Selection:", detaileditwidget_);
  layout->addWidget(promptLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  lBox_=new QListWidget(detaileditwidget_);
  connect(lBox_, SIGNAL(destroyed(void)), this, SLOT(onDestruction(void)));
  rebuildList();
  layout2->addWidget(lBox_);
  
  QVBoxLayout *sublayout=new QVBoxLayout(detaileditwidget_);
  
  QPushButton *addbtn=new QPushButton("Add...", detaileditwidget_);
  sublayout->addWidget(addbtn);
  connect(addbtn, SIGNAL(clicked()), detaileditwidget_, SLOT(onAddSingle()));
  QPushButton *addrangebtn=new QPushButton("Add Range...", detaileditwidget_);
  sublayout->addWidget(addrangebtn);
  connect(addrangebtn, SIGNAL(clicked()), detaileditwidget_, SLOT(onAddRange()));
  QPushButton *clearbtn=new QPushButton("Clear", detaileditwidget_);
  sublayout->addWidget(clearbtn);
  connect(clearbtn, SIGNAL(clicked()), detaileditwidget_, SLOT(onClear()));
  layout2->addLayout(sublayout);
  
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, SIGNAL(pressed()), this, SLOT(onApply()));
  layout->addWidget(apply);

  layout->addStretch();
  detaileditwidget_->setLayout(layout);
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
  double v=QInputDialog::getDouble(treeWidget(), "Add Range", "Please specify value:", 0., -2147483647,  2147483647, 9, &ok);
  if (ok)
  {
    param().insertValue(v);
    rebuildList();
  }
}

void DoubleRangeParameterWrapper::onAddRange()
{
  QString res=QInputDialog::getText(treeWidget(), "Add Range", "Please specify range begin, range end and number of values, separated by spaces:");
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
  if (widgetsDisplayed_) rebuildList();
}




SelectableSubsetParameterWrapper::SelectableSubsetParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, QWidget* detailw, QWidget* superform)
: ParameterWrapper(parent, name, p, detailw, superform),
  selBox_(NULL)
{
//   setText(1, "selectableSubset");
  onUpdate();
}

void SelectableSubsetParameterWrapper::createWidgets()
{
  ParameterWrapper::createWidgets();

  QVBoxLayout* layout=new QVBoxLayout(detaileditwidget_);

  QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);

  QWebView *shortDescLabel = 
    new QWebView( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);
  
  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  layout2->addWidget(new QLabel("Selection:", detaileditwidget_));
  selBox_=new QComboBox(detaileditwidget_);
  connect(selBox_, SIGNAL(destroyed(void)), this, SLOT(onDestruction(void)));
  BOOST_FOREACH( const insight::SelectableSubsetParameter::ItemList::const_iterator::value_type& pair, param().items() )
  {
//     std::cout<<"inserted text:"<<pair.first<<std::endl;
    selBox_->addItem(pair.first.c_str());
  }
  selBox_->setCurrentIndex(selBox_->findText(QString(param().selection().c_str())));
  layout2->addWidget(selBox_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  layout->addWidget(apply);

  layout->addStretch();
  detaileditwidget_->setLayout(layout);

  connect(apply, SIGNAL(clicked()), this, SLOT(onApply()));
//   connect(selBox_, SIGNAL(currentIndexChanged(const QString&)), 
// 	  this, SLOT(onCurrentIndexChanged(const QString&)));
}

void SelectableSubsetParameterWrapper::insertSubset()
{
  QList<QTreeWidgetItem*> cl=this->takeChildren();
  foreach(QTreeWidgetItem * ci, cl)
  {
    delete ci;
  }
  
  addWrapperToWidget(param()(), this, detaileditwidget_, superform_);
}

void SelectableSubsetParameterWrapper::onApply()
{
//   std::cout<<"1selbox="<<selBox_<<std::endl;
  if (widgetsDisplayed_)
  {
    param().selection()=selBox_->currentText().toStdString();
    insertSubset();
    setText(1, param().selection().c_str());
  }
  emit(apply());
}

void SelectableSubsetParameterWrapper::onUpdate()
{
  //selBox_->setCurrentIndex(param()());
  setText(1, param().selection().c_str());
  insertSubset();
  emit(update());
  if (widgetsDisplayed_) 
  {
    selBox_->setCurrentIndex(selBox_->findText(QString(param().selection().c_str())));
  }
}

// void SelectableSubsetParameterWrapper::onCurrentIndexChanged(const QString& qs)
// {
//   insertSubset();
// }

defineType(SelectableSubsetParameterWrapper);
addToFactoryTable(ParameterWrapper, SelectableSubsetParameterWrapper);
