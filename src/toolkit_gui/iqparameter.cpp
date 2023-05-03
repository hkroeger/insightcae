#include <QColor>
#include <QFont>
#include <QDebug>

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "helpwidget.h"
#include "iqparameter.h"

#include "base/exception.h"

#include "cadparametersetvisualizer.h"


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


defineType(IQParameter);
defineFactoryTable
(
    IQParameter,
      LIST(QObject* parent, const QString& name, insight::Parameter& parameter, const insight::ParameterSet& defaultParameterSet),
      LIST(parent, name, parameter, defaultParameterSet)
);

IQParameter* IQParameter::create(QObject *parent, const QString &name, insight::Parameter &p, const insight::ParameterSet &defaultParameterSet)
{
  try
  {
    return IQParameter::lookup(p.type(), parent, name, p, defaultParameterSet);
  }
  catch (const std::exception& e)
  {
    return new IQParameter(parent, name, p, defaultParameterSet);
  }
}



IQParameter::IQParameter(
        QObject *parent,
        const QString &name,
        insight::Parameter &parameter,
        const insight::ParameterSet &defaultParameterSet )
  : QObject(parent),
    name_(name),
    parameter_(parameter),
    defaultParameterSet_(defaultParameterSet)
{}



IQParameter* IQParameter::parentParameter() const
{
//  qDebug()<<"request parent of "<<name_;
  return dynamic_cast<IQParameter*>(parent());
}

int IQParameter::nChildParameters() const
{
  return size();
}

const QString& IQParameter::name() const
{
  return name_;
}

void IQParameter::setName(const QString &newName)
{
  name_=newName;
}


const QString IQParameter::buildPath(const QString& name, bool redirectArrayElementsToDefault) const
{
  QString thePath;
  if (const auto *p = parentParameter())
  {
    thePath = p->path(redirectArrayElementsToDefault);
  }
  if (!thePath.isEmpty())
  {
    thePath = thePath+"/"+name;
  }
  else
  {
    thePath =name;
  }
  return thePath;
}

const QString IQParameter::path(bool redirectArrayElementsToDefault) const
{
  return buildPath(name(), redirectArrayElementsToDefault);
}


QString IQParameter::valueText() const
{
  return QString::fromStdString(parameter_.type());
}



void IQParameter::resetModificationState()
{
  markedAsModified_.reset();
}

bool IQParameter::isModified() const
{
  if (!markedAsModified_)
  {
    bool cmodified = false;
    try
    {
      if (size()>0) // has children
      {
        for (auto& p: (*this))
        {
          cmodified |= p->isModified();
        }
      }
      else
      {
        const auto& dp = defaultParameterSet_.get<insight::Parameter>(path(true).toStdString());
        cmodified = parameter().isDifferent(dp);
      }
    }
    catch (...)
    {
      cmodified=true;
    }

    markedAsModified_.reset(new bool(cmodified));
  }

  return *markedAsModified_;
}

QVariant IQParameter::backgroundColor() const
{
  if (parameter_.isNecessary())
    return QColor(Qt::yellow);

  return QVariant();
}

QVariant IQParameter::textColor() const
{
  if (parameter_.isHidden())
    return QColor(Qt::gray);
  if (parameter_.isExpert())
    return QColor(Qt::lightGray);

  return QVariant();
}

QVariant IQParameter::textFont() const
{
  if (isModified())
  {
    QFont f;
    f.setBold(true);
    return f;
  }
  return QVariant();
}

void IQParameter::populateContextMenu(IQParameterSetModel* model, const QModelIndex &index, QMenu*)
{
}


QVBoxLayout* IQParameter::populateEditControls(
        IQParameterSetModel* model,
        const QModelIndex &index,
        QWidget* editControlsContainer,
        IQCADModel3DViewer * )
{
  QVBoxLayout *layout=new QVBoxLayout;

  QLabel *nameLabel = new QLabel(name(), editControlsContainer);
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);

  HelpWidget *shortDescLabel =
    new HelpWidget( editControlsContainer, parameter().description() );
  layout->addWidget(shortDescLabel);

  editControlsContainer->setLayout(layout);

  return layout;
}

const insight::Parameter& IQParameter::parameter() const
{
  return parameter_;
}



