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

#undef None
#undef Bool
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QGroupBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QTextEdit>
#include <QMessageBox>

#include "helpwidget.h"

#ifndef Q_MOC_RUN
#include "boost/foreach.hpp"
#endif

#include "base/tools.h"
#include "boost/spirit/include/classic.hpp"

#include "parametereditorwidget.h"

using namespace boost;


void addWrapperToWidget
(
    insight::ParameterSet& pset,
    const insight::ParameterSet& default_pset,
    QTreeWidgetItem *parentnode,
    QWidget *detaileditwidget,
    QObject *superform
)
{
//   QVBoxLayout *vlayout=new QVBoxLayout(widget);
    for ( insight::ParameterSet::iterator i=pset.begin(); i!=pset.end(); i++ )
    {

      if (! i->second->isHidden())
      {
        ParameterWrapper *wrapper =
            ParameterWrapper::lookup
            (
              i->second->type(),
              parentnode, i->first.c_str(),
              *i->second, *(default_pset.find(i->first)->second), // parameter, default parameter
              detaileditwidget, superform
            );

        QObject::connect ( parentnode->treeWidget(), &QTreeWidget::itemSelectionChanged,
                           wrapper, &ParameterWrapper::onSelectionChanged );

        if ( superform )
        {
            QObject::connect ( superform, SIGNAL ( apply() ), wrapper, SLOT ( onApply() ) );
            QObject::connect ( superform, SIGNAL ( update() ), wrapper, SLOT ( onUpdate() ) );
            QObject::connect ( wrapper, SIGNAL ( parameterSetChanged() ), superform, SLOT ( onUpdateVisualization() ) );
            QObject::connect ( wrapper, SIGNAL ( parameterSetChanged() ), superform, SLOT ( onCheckValidity() ) );
            QObject::connect ( wrapper, SIGNAL ( parameterSetChanged() ), superform, SLOT ( onParameterSetChanged() ) );
        }
      }
    }
}



defineType(ParameterWrapper);
defineFactoryTable
(
    ParameterWrapper, 
    LIST(QTreeWidgetItem *parent, const QString& name, insight::Parameter& p, const insight::Parameter& defp, QWidget*detailw, QObject*superform),
    LIST(parent, name, p, defp, detailw, superform)
);


void ParameterWrapper::focusInEvent(QFocusEvent*)
{
//     QWidget::focusInEvent(e);
//     std::cout<<p_.description()<<std::endl;
}


ParameterWrapper::ParameterWrapper(QTreeWidgetItem* parent, const QString& name, insight::Parameter& p, const insight::Parameter& defp, QWidget* detailw, QObject* superform)
: QObject(),
  QTreeWidgetItem(parent),
  name_(name),
  p_(p),
  pDefault_(defp),
  detaileditwidget_(detailw),
  superform_(superform),
  widgetsDisplayed_(false)
{
  setText(0, name_);
  QFont f=font(1);
  f.setItalic(true);

  if (p.isNecessary())
  {
    f.setBold(true);
    setBackground(0, Qt::green);
    setBackground(1, Qt::green);
  }
  else if (p.isExpert())
  {
    QBrush c(QColor(255,145,145));
    setBackground(0, c);
    setBackground(1, c);
  }
  else if (p.isHidden())
  {
    setForeground(0, Qt::lightGray);
    setForeground(1, Qt::lightGray);
  }

  setFont(1, f);

  connect
  (
    treeWidget(), &QTreeWidget::customContextMenuRequested,
    this, &ParameterWrapper::showContextMenuForWidget
  );
}


QMenu* ParameterWrapper::createContextMenu()
{
  QMenu *myMenu = new QMenu(treeWidget());

  connect(myMenu->addAction ( "Reset to default" ), &QAction::triggered,
          this, &ParameterWrapper::onResetToDefault);

  return myMenu;
}

bool ParameterWrapper::showContextMenuForWidget ( const QPoint &p )
{
  if
  (
   ParameterWrapper * mi =
   dynamic_cast<ParameterWrapper*> ( treeWidget()->itemAt ( p ) )
  )
  {
    if ( mi == this )
    {
      QMenu* myMenu = createContextMenu();

      myMenu->exec ( treeWidget()->mapToGlobal ( p ) );

      return true;
    }
  }

  return false;
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

void ParameterWrapper::onResetToDefault()
{
  onApply();
  qDebug()<<"reset to def";
  p_.reset(pDefault_);
  onUpdate();
  emit parameterSetChanged();
}




defineType(IntParameterWrapper);
addToFactoryTable(ParameterWrapper, IntParameterWrapper);

IntParameterWrapper::IntParameterWrapper
(
    QTreeWidgetItem* parent,
    const QString& name,
    insight::Parameter& p,
    const insight::Parameter& defp,
    QWidget* detailw,
    QObject* superform
)
: ParameterWrapper(parent, name, p, defp, detailw, superform)
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
  
  HelpWidget *shortDescLabel =
    new HelpWidget( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  le_=new QLineEdit(detaileditwidget_);
  connect(le_, &QLineEdit::destroyed, this, &IntParameterWrapper::onDestruction);
  le_->setText(QString::number(param()()));
  le_->setValidator(new QIntValidator());
  connect(le_, &QLineEdit::returnPressed, this, &IntParameterWrapper::onApply);
  layout2->addWidget(le_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, &QPushButton::pressed, this, &IntParameterWrapper::onApply);
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
    emit parameterSetChanged();
  }
}

void IntParameterWrapper::onUpdate()
{
  setText(1, QString::number(param()()));
  if (widgetsDisplayed_) le_->setText(QString::number(param()()));
}




defineType(DoubleParameterWrapper);
addToFactoryTable(ParameterWrapper, DoubleParameterWrapper);

DoubleParameterWrapper::DoubleParameterWrapper
(
    QTreeWidgetItem* parent,
    const QString& name,
    insight::Parameter& p,
    const insight::Parameter& defp,
    QWidget* detailw,
    QObject* superform
)
: ParameterWrapper(parent, name, p, defp, detailw, superform),
  le_(nullptr)
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
  
  HelpWidget *shortDescLabel =
    new HelpWidget( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  le_=new QLineEdit(detaileditwidget_);
  connect(le_, &QLineEdit::destroyed, this, &DoubleParameterWrapper::onDestruction);
  le_->setText(QString::number(param()()));
  le_->setValidator(new QDoubleValidator());
  connect(le_, &QLineEdit::returnPressed, this, &DoubleParameterWrapper::onApply);
  layout2->addWidget(le_);
  
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, &QPushButton::pressed, this, &DoubleParameterWrapper::onApply);
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
    emit parameterSetChanged();
  }
}

void DoubleParameterWrapper::onUpdate()
{
  setText(1, QString::number(param()()));
  if (widgetsDisplayed_) le_->setText(QString::number(param()()));
}




defineType(StringParameterWrapper);
addToFactoryTable(ParameterWrapper, StringParameterWrapper);


VectorParameterWrapper::VectorParameterWrapper
(
    QTreeWidgetItem* parent,
    const QString& name,
    insight::Parameter& p,
    const insight::Parameter& defp,
    QWidget* detailw,
    QObject* superform
)
: ParameterWrapper(parent, name, p, defp, detailw, superform),
  le_(nullptr)
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
  
  HelpWidget *shortDescLabel =
    new HelpWidget( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  le_=new QLineEdit(detaileditwidget_);
  connect(le_, &QLineEdit::destroyed, this, &VectorParameterWrapper::onDestruction);
  le_->setText(QString(insight::valueToString(param()()).c_str()));
  connect(le_, &QLineEdit::returnPressed, this, &VectorParameterWrapper::onApply);
  layout2->addWidget(le_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, &QPushButton::pressed, this, &VectorParameterWrapper::onApply);
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
    emit parameterSetChanged();
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


StringParameterWrapper::StringParameterWrapper
(
    QTreeWidgetItem* parent,
    const QString& name,
    insight::Parameter& p,
    const insight::Parameter& defp,
    QWidget* detailw,
    QObject* superform
)
: ParameterWrapper(parent, name, p, defp, detailw, superform),
  le_(nullptr)
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
  
  HelpWidget *shortDescLabel =
    new HelpWidget( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  le_=new QLineEdit(detaileditwidget_);
  connect(le_, &QLineEdit::destroyed, this, &StringParameterWrapper::onDestruction);
  connect(le_, &QLineEdit::returnPressed, this, &StringParameterWrapper::onApply);
  layout2->addWidget(le_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, &QPushButton::pressed, this, &StringParameterWrapper::onApply);
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
    emit parameterSetChanged();
  }
}

void StringParameterWrapper::onUpdate()
{
  setText(1, param()().c_str());
  if (widgetsDisplayed_) le_->setText(param()().c_str());
}




defineType(BoolParameterWrapper);
addToFactoryTable(ParameterWrapper, BoolParameterWrapper);

BoolParameterWrapper::BoolParameterWrapper
(
    QTreeWidgetItem* parent,
    const QString& name,
    insight::Parameter& p,
    const insight::Parameter& defp,
    QWidget* detailw,
    QObject* superform
)
: ParameterWrapper(parent, name, p, defp, detailw, superform),
  cb_(nullptr)
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
  
  HelpWidget *shortDescLabel =
    new HelpWidget( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  cb_=new QCheckBox(detaileditwidget_);
  connect(cb_, &QCheckBox::destroyed, this, &BoolParameterWrapper::onDestruction);
  if (param()())
    cb_->setCheckState(Qt::Checked);
  else
    cb_->setCheckState(Qt::Unchecked);
  layout2->addWidget(cb_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, &QPushButton::pressed, this, &BoolParameterWrapper::onApply);
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
    emit parameterSetChanged();
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

PathParameterWrapper::PathParameterWrapper
(
    QTreeWidgetItem* parent,
    const QString& name,
    insight::Parameter& p,
    const insight::Parameter& defp,
    QWidget* detailw,
    QObject* superform
)
: ParameterWrapper(parent, name, p, defp, detailw, superform),
  le_(nullptr)
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
  
  HelpWidget *shortDescLabel =
    new HelpWidget( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  le_=new QLineEdit(detaileditwidget_);
  connect(le_, &QLineEdit::destroyed, this, &PathParameterWrapper::onDestruction);
  connect(le_, &QLineEdit::returnPressed, this, &PathParameterWrapper::onApply);
  le_->setText(param()().c_str());
  layout2->addWidget(le_);
  dlgBtn_=new QPushButton("...", detaileditwidget_);
  layout2->addWidget(dlgBtn_);
  openBtn_=new QPushButton("Open", detaileditwidget_);
  layout2->addWidget(openBtn_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, &QPushButton::pressed, this, &PathParameterWrapper::onApply);
  layout->addWidget(apply);

  layout->addStretch();
  detaileditwidget_->setLayout(layout);

  
  connect(le_, &QLineEdit::textChanged, this, &PathParameterWrapper::onDataEntered);
  connect(dlgBtn_, &QPushButton::clicked, this, &PathParameterWrapper::openSelectionDialog);
  connect(openBtn_, &QPushButton::clicked, this, &PathParameterWrapper::openFile);
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
    emit parameterSetChanged();
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

void PathParameterWrapper::openFile()
{
  //if ( !QDesktopServices::openUrl(QUrl("file://"+le_->text())) )
  boost::filesystem::path fp( le_->text().toStdString() );
  std::string ext=fp.extension().string();
  boost::algorithm::to_lower(ext);

  QString program;
  if ( (ext==".stl")||(ext==".stlb") )
  {
    program="paraview";
  }
  else if ( (ext==".stp")||(ext==".step")||(ext==".igs")||(ext==".iges")||(ext==".iscad")||(ext==".brep") )
  {
    program="iscad";
  }

  if (!program.isEmpty())
  {
    QProcess *sp = new QProcess(treeWidget());
    sp->start(program, QStringList() << le_->text() );

    if (!sp->waitForStarted())
    {
      QMessageBox::critical(treeWidget(), "Could not open file", "Could not launch program: "+program);
    }
  }
  else
  {
    if (!QDesktopServices::openUrl(QUrl("file://"+le_->text())))
    {
      QMessageBox::critical(treeWidget(), "Could not open file", "Could not open the file using QDesktopServices!");
    }
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
  for (arma::uword i=0; i<m.n_rows; i++)
  {
    for (arma::uword j=0; j<m.n_cols; j++)
    {
      oss<<m(i,j);
      if (j!=m.n_cols-1) oss<<" ";
    }
    if (i!=m.n_rows-1) oss<<";";
  }
  return QString(oss.str().c_str());
}

MatrixParameterWrapper::MatrixParameterWrapper
(
    QTreeWidgetItem* parent,
    const QString& name,
    insight::Parameter& p,
    const insight::Parameter& defp,
    QWidget* detailw,
    QObject* superform
)
: ParameterWrapper(parent, name, p, defp, detailw, superform),
  le_(nullptr)
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
  
  HelpWidget *shortDescLabel =
    new HelpWidget( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  le_=new QLineEdit(detaileditwidget_);
  connect(le_, &QLineEdit::destroyed, this, &MatrixParameterWrapper::onDestruction);
  connect(le_, &QLineEdit::returnPressed, this, &MatrixParameterWrapper::onApply);
  le_->setText(mat2Str(param()()));
  layout2->addWidget(le_);
  dlgBtn_=new QPushButton("...", detaileditwidget_);
  layout2->addWidget(dlgBtn_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, &QPushButton::pressed, this, &MatrixParameterWrapper::onApply);
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
  
  connect(dlgBtn_, &QPushButton::clicked, this, &MatrixParameterWrapper::openSelectionDialog);
}



void MatrixParameterWrapper::onApply()
{
  if (widgetsDisplayed_)
  {
    param()()=arma::mat(le_->text().toStdString());
    emit parameterSetChanged();
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

DirectoryParameterWrapper::DirectoryParameterWrapper
(
    QTreeWidgetItem* parent,
    const QString& name,
    insight::Parameter& p,
    const insight::Parameter& defp,
    QWidget* detailw,
    QObject* superform
)
: PathParameterWrapper(parent, name, p, defp, detailw, superform)
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

SelectionParameterWrapper::SelectionParameterWrapper
(
    QTreeWidgetItem* parent,
    const QString& name,
    insight::Parameter& p,
    const insight::Parameter& defp,
    QWidget* detailw,
    QObject* superform
)
: ParameterWrapper(parent, name, p, defp, detailw, superform),
  selBox_(nullptr)
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
  
  HelpWidget *shortDescLabel =
    new HelpWidget( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QLabel *promptLabel = new QLabel("Selection:", detaileditwidget_);
  layout2->addWidget(promptLabel);
  selBox_=new QComboBox(detaileditwidget_);
  connect(selBox_, &QComboBox::destroyed, this, &SelectionParameterWrapper::onDestruction);
  for ( const std::string& s: param().items() )
  {
    selBox_->addItem(s.c_str());
  }
  selBox_->setCurrentIndex(param()());
  layout2->addWidget(selBox_);
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, &QPushButton::pressed, this, &SelectionParameterWrapper::onApply);
  layout->addWidget(apply);

  layout->addStretch();
  detaileditwidget_->setLayout(layout);

}

void SelectionParameterWrapper::onApply()
{
  if (widgetsDisplayed_)
  {
    param()()=selBox_->currentIndex();
    setText(1, param().selection().c_str());
    emit parameterSetChanged();
  }
}

void SelectionParameterWrapper::onUpdate()
{
  setText(1, param().selection().c_str());
  if (widgetsDisplayed_) selBox_->setCurrentIndex(param()());
}




defineType(SubsetParameterWrapper);
addToFactoryTable(ParameterWrapper, SubsetParameterWrapper);


SubsetParameterWrapper::SubsetParameterWrapper
(
    QTreeWidgetItem* parent,
    const QString& name,
    insight::Parameter& p,
    const insight::Parameter& defp,
    QWidget* detailw,
    QObject* superform
)
: ParameterWrapper(parent, name, p, defp, detailw, superform)
{
  addWrapperToWidget(param()(), dynamic_cast<const insight::SubsetParameter&>(defp),
                     this, detaileditwidget_, this);
}

void SubsetParameterWrapper::createWidgets()
{
  ParameterWrapper::createWidgets();

  QVBoxLayout *layout=new QVBoxLayout(detaileditwidget_);
  
  QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);
  
  HelpWidget *shortDescLabel =
    new HelpWidget( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);
  
//   QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
//   connect(apply, SIGNAL(pressed()), this, SLOT(onApply()));
//   layout->addWidget(apply);

  layout->addStretch();
  detaileditwidget_->setLayout(layout);
  connect(detaileditwidget_, &QWidget::destroyed, this, &SubsetParameterWrapper::onDestruction);
  
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

void SubsetParameterWrapper::onUpdateVisualization()
{
  emit updateVisualization();
}

void SubsetParameterWrapper::onCheckValidity()
{
  emit checkValidity();
}

void SubsetParameterWrapper::onParameterSetChanged()
{
  emit parameterSetChanged();
}



defineType(ArrayParameterWrapper);
addToFactoryTable(ParameterWrapper, ArrayParameterWrapper);

void ArrayParameterWrapper::addWrapper(int i)
{
  auto& oap=param();
  insight::Parameter& pp=oap[i];

  ParameterWrapper *wrapper = 
    ParameterWrapper::lookup
    (
        pp.type(),
        this, "["+QString::number(i)+"]",
        pp,
        oap.defaultValue(),
        detaileditwidget_, this
    );

  QObject::connect(treeWidget(), &QTreeWidget::itemSelectionChanged, wrapper, &ParameterWrapper::onSelectionChanged);
  QObject::connect(this, &ArrayParameterWrapper::apply, wrapper, &ParameterWrapper::onApply);
  QObject::connect(this, &ArrayParameterWrapper::update, wrapper, &ParameterWrapper::onUpdate);
  QObject::connect ( wrapper, &ParameterWrapper::parameterSetChanged, this, &ArrayParameterWrapper::onParameterSetChanged );
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

ArrayParameterWrapper::ArrayParameterWrapper
(
    QTreeWidgetItem* parent,
    const QString& name,
    insight::Parameter& p,
    const insight::Parameter& defp,
    QWidget* detailw,
    QObject* superform
)
: ParameterWrapper(parent, name, p, defp, detailw, superform)
{
  setText(1, "array");
  rebuildWrappers();
}

bool ArrayParameterWrapper::showContextMenuForWidget(const QPoint &p)
{
  if (!ParameterWrapper::showContextMenuForWidget(p))
  {
    if
    (
      ParameterWrapper* sel =
       dynamic_cast<ParameterWrapper*>(treeWidget()->itemAt(p))
    )
    {
      ArrayParameterWrapper * mi =
        dynamic_cast<ArrayParameterWrapper*> ( sel->QTreeWidgetItem::parent() );
      if ( mi==this )
        { // context menu for child element (array element)

          // get child id
          int childid=-1;
          for ( int j=0; j<this->childCount(); j++ )
            {
              if ( this->child ( j ) == sel )
                {
                  childid=j;
                  break;
                }
            }
          if ( childid>=0 )
          {
            QMenu* myMenu = sel->createContextMenu();
            myMenu->addAction ( "Delete" );

            sel->connect(myMenu->addAction("Remove array element"), &QAction::triggered,
                      this,
                       [=]() { this->onRemove(childid); }
            );

            myMenu->exec( treeWidget()->mapToGlobal(p) );

            return true;
          }
        }
    }
  }

  return false;
}

QMenu* ArrayParameterWrapper::createContextMenu()
{
  QMenu *m = ParameterWrapper::createContextMenu();

  connect(m->addAction("Clear array"), &QAction::triggered,
          this, &ArrayParameterWrapper::onRemoveAll);

  return m;
}

void ArrayParameterWrapper::createWidgets()
{
  ParameterWrapper::createWidgets();

  QVBoxLayout *layout=new QVBoxLayout(detaileditwidget_);
  QLabel *label = new QLabel(name_, detaileditwidget_);
  QFont f=label->font(); f.setBold(true); label->setFont(f);
  layout->addWidget(label);
  
  HelpWidget *shortDescLabel =
    new HelpWidget( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  QPushButton *addbtn=new QPushButton("+ Add new", detaileditwidget_);
  connect(addbtn, &QPushButton::clicked, this, &ArrayParameterWrapper::onAppendEmpty);
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
  emit parameterSetChanged();
}

void ArrayParameterWrapper::onRemoveAll()
{
  emit(apply());
  param().clear();
  rebuildWrappers();
  emit parameterSetChanged();
}

void ArrayParameterWrapper::onAppendEmpty()
{
  emit(apply());
  param().appendEmpty();
  rebuildWrappers();
  emit parameterSetChanged();
}

void ArrayParameterWrapper::onApply()
{
  emit(apply());
}

void ArrayParameterWrapper::onUpdate()
{
//   entrywrappers_.clear();
  rebuildWrappers();
//  emit(update());
}

void ArrayParameterWrapper::onUpdateVisualization()
{
  emit updateVisualization();
}

void ArrayParameterWrapper::onCheckValidity()
{
  emit checkValidity();
}

void ArrayParameterWrapper::onParameterSetChanged()
{
  emit parameterSetChanged();
}


defineType(DoubleRangeParameterWrapper);
addToFactoryTable(ParameterWrapper, DoubleRangeParameterWrapper);

DoubleRangeParameterWrapper::DoubleRangeParameterWrapper
(
    QTreeWidgetItem* parent,
    const QString& name,
    insight::Parameter& p,
    const insight::Parameter& defp,
    QWidget* detailw,
    QObject* superform
)
: ParameterWrapper(parent, name, p, defp, detailw, superform)
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
  
  HelpWidget *shortDescLabel =
    new HelpWidget( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);

  QLabel *promptLabel = new QLabel("Selection:", detaileditwidget_);
  layout->addWidget(promptLabel);

  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  lBox_=new QListWidget(detaileditwidget_);
  connect(lBox_, &QListWidget::destroyed, this, &DoubleRangeParameterWrapper::onDestruction);
  rebuildList();
  layout2->addWidget(lBox_);
  
  QVBoxLayout *sublayout=new QVBoxLayout(detaileditwidget_);
  
  QPushButton *addbtn=new QPushButton("Add...", detaileditwidget_);
  sublayout->addWidget(addbtn);
  connect(addbtn, &QPushButton::clicked, this, &DoubleRangeParameterWrapper::onAddSingle);
  QPushButton *addrangebtn=new QPushButton("Add Range...", detaileditwidget_);
  sublayout->addWidget(addrangebtn);
  connect(addrangebtn, &QPushButton::clicked, this, &DoubleRangeParameterWrapper::onAddRange);
  QPushButton *clearbtn=new QPushButton("Clear", detaileditwidget_);
  sublayout->addWidget(clearbtn);
  connect(clearbtn, &QPushButton::clicked, this, &DoubleRangeParameterWrapper::onClear);
  layout2->addLayout(sublayout);
  
  layout->addLayout(layout2);
  
  QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
  connect(apply, &QPushButton::pressed, this, &DoubleRangeParameterWrapper::onApply);
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
    emit parameterSetChanged();
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
      emit parameterSetChanged();
    }
    rebuildList();
  }
}

void DoubleRangeParameterWrapper::onClear()
{
  param().values().clear();
  rebuildList();
  emit parameterSetChanged();
}

void DoubleRangeParameterWrapper::onApply()
{
}

void DoubleRangeParameterWrapper::onUpdate()
{
  if (widgetsDisplayed_) rebuildList();
}




SelectableSubsetParameterWrapper::SelectableSubsetParameterWrapper
(
    QTreeWidgetItem* parent,
    const QString& name,
    insight::Parameter& p,
    const insight::Parameter& defp,
    QWidget* detailw,
    QObject* superform
)
: ParameterWrapper(parent, name, p, defp, detailw, superform),
  selBox_(nullptr)
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

  HelpWidget *shortDescLabel =
    new HelpWidget( detaileditwidget_ );
  shortDescLabel->setHtml( param().description().toHTML().c_str() );
  layout->addWidget(shortDescLabel);
  
  QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
  layout2->addWidget(new QLabel("Selection:", detaileditwidget_));
  selBox_=new QComboBox(detaileditwidget_);
  connect(selBox_, &QComboBox::destroyed, this, &SelectableSubsetParameterWrapper::onDestruction);
  for ( auto pair: param().items() )
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

  connect(apply, &QPushButton::clicked, this, &SelectableSubsetParameterWrapper::onApply);

}

void SelectableSubsetParameterWrapper::insertSubset()
{
  QList<QTreeWidgetItem*> cl=this->takeChildren();
  foreach(QTreeWidgetItem * ci, cl)
  {
    delete ci;
  }
  
  const auto& defss = (*param().items().find( param().selection() )->second);
  addWrapperToWidget(param()(), defss,
                     this, detaileditwidget_, this);
}

void SelectableSubsetParameterWrapper::onApply()
{
//   std::cout<<"1selbox="<<selBox_<<std::endl;
  if (widgetsDisplayed_)
  {
    param().selection()=selBox_->currentText().toStdString();
    insertSubset();
    setText(1, param().selection().c_str());
    emit parameterSetChanged();
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


void SelectableSubsetParameterWrapper::onUpdateVisualization()
{
  emit updateVisualization();
}

void SelectableSubsetParameterWrapper::onCheckValidity()
{
  emit checkValidity();
}

void SelectableSubsetParameterWrapper::onParameterSetChanged()
{
  emit parameterSetChanged();
}

// void SelectableSubsetParameterWrapper::onCurrentIndexChanged(const QString& qs)
// {
//   insertSubset();
// }

defineType(SelectableSubsetParameterWrapper);
addToFactoryTable(ParameterWrapper, SelectableSubsetParameterWrapper);
