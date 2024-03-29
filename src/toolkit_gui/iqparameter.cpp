#include <QColor>
#include <QFont>
#include <QDebug>

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>

#include "helpwidget.h"
#include "iqparameter.h"

#include "base/exception.h"
#include "base/analysis.h"
#include "base/analysisparameterpropositions.h"

#include "cadparametersetvisualizer.h"
#include "iqparametersetmodel.h"


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
      LIST(
        QObject* parent,
        IQParameterSetModel* psmodel,
        const QString& name,
        insight::Parameter& parameter,
        const insight::ParameterSet& defaultParameterSet),
      LIST(parent,
         psmodel,
         name,
         parameter,
         defaultParameterSet)
);

IQParameter* IQParameter::create(
    QObject *parent,
    IQParameterSetModel* psmodel,
    const QString &name,
    insight::Parameter &p,
    const insight::ParameterSet &defaultParameterSet )
{
  IQParameter *np;
  try
  {
    np=IQParameter::lookup(p.type(), parent, psmodel, name, p, defaultParameterSet);
  }
  catch (const std::exception& e)
  {
    np=new IQParameter(parent, psmodel, name, p, defaultParameterSet);
  }

  np->connectSignals();

  return np;
}



IQParameter::IQParameter(
        QObject *parent,
        IQParameterSetModel* psmodel,
        const QString &name,
        insight::Parameter &parameter,
        const insight::ParameterSet &defaultParameterSet )
  : QObject(parent),
    model_(psmodel),
    name_(name),
    parameter_(parameter),
    defaultParameterSet_(defaultParameterSet)
{}

void IQParameter::connectSignals()
{
    // connect outside constructor because of virtual "path" function is involved
    disconnectAtEOL(
        parameterRef().valueChanged.connect(
            [this]() {
                model_->notifyParameterChange(
                    path().toStdString(),
                    true
                    );
            }
            )
        );
}



IQParameter* IQParameter::parentParameter() const
{
//  qDebug()<<"request parent of "<<name_;
  return dynamic_cast<IQParameter*>(parent());
}

int IQParameter::nChildParameters() const
{
  return size();
}

IQParameterSetModel *IQParameter::model() const
{
  return model_;
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

void IQParameter::populateContextMenu(
    QMenu* /*cm*/)
{}


QVBoxLayout* IQParameter::populateEditControls(
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

  auto analysisName = model_->getAnalysisName();
  if (!analysisName.empty())
  {
    auto propositions =
        insight::AnalysisParameterPropositions::getCombinedPropositionsForParameter(
            analysisName,
            path().toStdString(),
            model_->getParameterSet()
        );
    if (propositions.size()>0)
    {
        auto *proplist = new QListWidget;
        layout->addWidget(new QLabel("Proposed values:"));
        for (auto pp =propositions.begin(); pp!=propositions.end(); ++pp)
        {
          proplist->addItem(QString::fromStdString(
            pp.name()+": "+pp->plainTextRepresentation()
              ));
        }
        connect(proplist, &QListWidget::itemDoubleClicked, proplist,
                [this,propositions](QListWidgetItem *item)
                {
                    auto label = item->text().split(":").at(0);
                    this->applyProposition(propositions, label.toStdString());
                }
                );
        layout->addWidget(proplist);
    }
  }


  editControlsContainer->setLayout(layout);

  return layout;
}


const insight::Parameter& IQParameter::parameter() const
{
  return parameter_;
}


insight::Parameter &IQParameter::parameterRef()
{
  return parameter_;
}

void IQParameter::applyProposition(
    const insight::ParameterSet &propositions,
    const std::string &selectProposition )
{
#warning does nothing yet, should be abstract
}



