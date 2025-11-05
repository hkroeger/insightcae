#include <QColor>
#include <QFont>
#include <QDebug>

#include <QObject>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>

#include "iqparameter.h"

#include "base/exception.h"
#include "base/analysis.h"
#include "base/analysisparameterpropositions.h"

#include "cadparametersetvisualizer.h"
#include "iqparametersetmodel.h"
#include "qtextensions.h"




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






IQParameter::IQParameter(
        QObject *parent,
        IQHierarchicalDataModel* hdmodel,
        insight::hierarchicalData::Element* parameter )
  : IQHierarchicalDataElement(parent, hdmodel, parameter)
{}



void IQParameter::resetModificationState()
{
  markedAsModified_.reset();
}

bool IQParameter::isModified() const
{
    if (!markedAsModified_)
    {
        if (auto *defl=defaultParameterSet())
        {
            markedAsModified_ =
                elementAs<insight::Parameter>()
                    .isModified(*defl);
        }
        else
        {
            markedAsModified_=false;
        }
    }

    return *markedAsModified_;
}

QVariant IQParameter::backgroundColor() const
{
    if (elementAs<insight::Parameter>().isNecessary())
    return QColor(Qt::yellow);

  return QVariant();
}

QVariant IQParameter::textColor() const
{
  if (elementAs<insight::Parameter>().isHidden())
    return QColor(Qt::gray);
  if (elementAs<insight::Parameter>().isExpert())
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



QVBoxLayout* IQParameter::populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer * )
{


  QVBoxLayout *layout=new QVBoxLayout;

  auto *nameLabel = new QLabel(
      QString::fromStdString((*this)->name()),
      editControlsContainer );
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);

  auto *shortDescLabel =
    new IQSimpleLatexView(
      elementAs<insight::Parameter>().description(),
      editControlsContainer );
  layout->addWidget(shortDescLabel);

  connect(
      this,
      &IQParameter::setControlsEnabled,
      editControlsContainer,
      [this,editControlsContainer](bool isEnabled)
      {
          for (auto *c: editControlsContainer->children())
          {
              if (auto *w = dynamic_cast<QWidget*>(c))
              {
                  w->setEnabled(isEnabled);
              }
          }
      });

  if (auto *psmodel = dynamic_cast<IQParameterSetModel*>(model()))
  {
      auto analysisName = psmodel->getAnalysisName();
      if (!analysisName.empty())
      {
        std::shared_ptr<insight::ParameterSet> propositions =
            insight::AnalysisParameterPropositions::getCombinedPropositionsForParameter(
                analysisName,
                (*this)->path(),
              psmodel->getParameterSet()
        );

        if (propositions->size()>0)
        {
            auto *proplist = new QListWidget;
            layout->addWidget(new QLabel("Proposed values:"));
            for (auto pp =propositions->begin(); pp!=propositions->end(); ++pp)
            {
              proplist->addItem(QString::fromStdString(
                pp.name()+": "+pp->plainTextRepresentation()
                  ));
            }
            connect(proplist, &QListWidget::itemDoubleClicked, proplist,
                    [this,propositions](QListWidgetItem *item)
                    {
                        auto label = item->text().split(":").at(0);
                        this->applyProposition(*propositions, label.toStdString());
                    }
                    );
            layout->addWidget(proplist);
        }
      }
  }


  editControlsContainer->setLayout(layout);

  return layout;
}




void IQParameter::applyProposition(
    const insight::ParameterSet &propositions,
    const std::string &selectProposition )
{
#warning does nothing yet, should be abstract
}

const insight::ParameterSet *IQParameter::defaultParameterSet() const
{
    auto &m = dynamic_cast<const IQParameterSetModel&>(*model());
    return m.defaultParameterSet();
}

IQParameterSetModel* IQParameter::psModel() const
{
    return dynamic_cast<IQParameterSetModel*>(model());
}




// defineStaticFunctionTableWithArgs(
//     IQParameterGridViewDelegateEditorWidget,
//     createDelegate, QAbstractItemDelegate*,
//     LIST( QObject* parent ),
//     LIST( parent )
// );

// IQParameterGridViewDelegateEditorWidget::IQParameterGridViewDelegateEditorWidget(
//     QObject* parent,
//     const IQParameter& parameter,
//     const QModelIndex& index)
//     : QObject(parent), index_(index)
// {
//     if (IQParameterGridViewDelegateEditorWidget::has_createDelegate(parameter.type()))
//     {
//         delegate_=
//             IQParameterGridViewDelegateEditorWidget::createDelegateFor(
//             parameter.type(), this);
//     }
//     else
//     {
//         delegate_=new QStyledItemDelegate(this);
//     }
// }





// const IQParameterGridViewDelegateEditorWidget*
// IQParameterGridViewSelectorDelegate::delegateWidgetForIndex(
//     const QModelIndex &idx) const
// {
//     for (auto& c: children())
//     {
//         if (auto *delw =
//             dynamic_cast<const IQParameterGridViewDelegateEditorWidget*>(c))
//         {
//             if (delw->index()==idx)
//                 return delw;
//         }
//     }
//     return nullptr;
// }




// void IQParameterGridViewSelectorDelegate::destroyEditor(
//     QWidget *editor, const QModelIndex &index) const
// {
//     if (auto *d=delegateWidgetForIndex(index))
//     {
//         d->delegate()->destroyEditor(editor, index);
//         const_cast<IQParameterGridViewDelegateEditorWidget*>(d)->deleteLater();
//     }
//     else
//         QStyledItemDelegate::destroyEditor(editor, index);
// }




// bool IQParameterGridViewSelectorDelegate::editorEvent(
//     QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
// {
//     if (auto *d=delegateWidgetForIndex(index))
//     {
//         return d->delegate()->editorEvent(event, model, option, index);
//     }
//     else
//         return QStyledItemDelegate::editorEvent(event, model, option, index);
// }




// bool IQParameterGridViewSelectorDelegate::helpEvent(
//     QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
// {
//     if (auto *d=delegateWidgetForIndex(index))
//     {
//         return d->delegate()->helpEvent(event, view, option, index);
//     }
//     else
//         return QStyledItemDelegate::helpEvent(event, view, option, index);
// }




// void IQParameterGridViewSelectorDelegate::paint(
//     QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
// {
//     if (auto *d=delegateWidgetForIndex(index))
//     {
//         d->delegate()->paint(painter, option, index);
//     }
//     else
//         QStyledItemDelegate::paint(painter, option, index);
// }




// QWidget* IQParameterGridViewSelectorDelegate::createEditor(
//     QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
// {
//     if (auto *psm =dynamic_cast<const IQParameterSetModel*>(
//             index.model()))
//     {
//         auto &iqp = dynamic_cast<const IQParameter&>(
//             *psm->iqElementOfIndex(index));
//         auto dw = new IQParameterGridViewDelegateEditorWidget(
//             const_cast<IQParameterGridViewSelectorDelegate*>(this),
//             iqp, index);
//         return dw->delegate()->createEditor(parent, option, index);
//     }
//     return QStyledItemDelegate::createEditor(parent, option, index);
// }



// void IQParameterGridViewSelectorDelegate::setEditorData(
//     QWidget *editor, const QModelIndex &index) const
// {
//     if (auto *d=delegateWidgetForIndex(index))
//     {
//         d->delegate()->setEditorData(editor, index);
//     }
//     else
//         QStyledItemDelegate::setEditorData(editor, index);
// }


// void IQParameterGridViewSelectorDelegate::setModelData(
//     QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
// {
//     if (auto *d=delegateWidgetForIndex(index))
//     {
//         d->delegate()->setModelData(editor, model, index);
//     }
//     else
//         QStyledItemDelegate::setModelData(editor, model, index);
// }



// QSize IQParameterGridViewSelectorDelegate::sizeHint(
//     const QStyleOptionViewItem &option, const QModelIndex &index) const
// {
//     if (auto *d=delegateWidgetForIndex(index))
//     {
//         return d->delegate()->sizeHint(option, index);
//     }
//     else
//         return QStyledItemDelegate::sizeHint(option, index);
// }


// void IQParameterGridViewSelectorDelegate::updateEditorGeometry(
//     QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
// {
//     if (auto *d=delegateWidgetForIndex(index))
//     {
//         d->delegate()->updateEditorGeometry(editor, option, index);
//     }
//     else
//         QStyledItemDelegate::updateEditorGeometry(editor, option, index);
// }





