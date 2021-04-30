#include <QColor>
#include <QFont>

#include <QVBoxLayout>
#include <QLabel>

#include "helpwidget.h"
#include "iqparameter.h"


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



IQParameter::IQParameter(QObject *parent, const QString &name, insight::Parameter &parameter, const insight::ParameterSet &defaultParameterSet)
  : QObject(parent),
    name_(name),
    parameter_(parameter),
    defaultParameterSet_(defaultParameterSet)
{}



IQParameter* IQParameter::parentParameter() const
{
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

const QString IQParameter::path() const
{
  QString thePath;
  if (const auto *p = parentParameter())
  {
    thePath=p->path();
  }
  if (!thePath.isEmpty())
  {
    thePath = thePath+"/"+name();
  }
  else
  {
    thePath=name();
  }
  return thePath;
}


QString IQParameter::valueText() const
{
  return QString::fromStdString(parameter_.type());
}

bool IQParameter::isModified() const
{
  try
  {
    const auto& dp = defaultParameterSet_.get<insight::Parameter>(path().toStdString());
    return parameter().isDifferent(dp);
  }
  catch (...)
  {
    return true;
  }
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


QVBoxLayout* IQParameter::populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer)
{
  QVBoxLayout *layout=new QVBoxLayout(editControlsContainer);

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



