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
defineFactoryTable
(
    IQParameter,
      LIST(
        QObject* parent,
        IQParameterSetModel* psmodel,
        insight::Parameter* parameter,
        const insight::ParameterSet& defaultParameterSet),
      LIST(parent,
         psmodel,
         parameter,
         defaultParameterSet)
);




IQParameter* IQParameter::create(
    QObject *parent,
    IQParameterSetModel* psmodel,
    insight::Parameter *p,
    const insight::ParameterSet &defaultParameterSet )
{
    IQParameter *np;
    if (IQParameter::has_factory(p->type()))
    {
        np = IQParameter::lookup(p->type(), parent, psmodel, p, defaultParameterSet);
    }
    else
    {
        np = new IQParameter(parent, psmodel, p, defaultParameterSet);
    }

    np->connectSignals();

    return np;
}




IQParameter::IQParameter(
        QObject *parent,
        IQParameterSetModel* psmodel,
        insight::Parameter *parameter,
        const insight::ParameterSet &defaultParameterSet )
  : std::observer_ptr<insight::Parameter>(parameter),
    QObject(parent),
    model_(psmodel),
    defaultParameterSet_(defaultParameterSet)
{}



IQParameter::~IQParameter()
{}




void IQParameter::removeFromViews(bool deleteLater)
{
    auto *parentParameter =
        dynamic_cast<IQParameter*>(
            this->parentParameter());

    auto *m=this->model();
    auto myIndex=m->indexFromParameterPath(
        this->get()->path(), 0);
    auto parentIndex=myIndex.parent();
    auto row=myIndex.row();

    m->beginRemoveRows(parentIndex, row, row);

    if (deleteLater)
    {
        this->deleteLater();
    }

    QObject::connect(
        this, &QObject::destroyed, this,
        [m]() { m->endRemoveRows(); } );
}


IQParameter *IQParameter::parentParameter() const
{
    if (get()->hasParent())
        if (auto *iqp=model_->findWrapper(get()->parent()))
            return iqp;
    return nullptr;
}


QList<IQParameter *> IQParameter::children() const
{
    QList<IQParameter *> ch;
    for (auto& p: *get())
    {
        if (auto *iqp=model_->findWrapper(p))
            ch.append(iqp);
    }
    return ch;
}

void IQParameter::connectSignals()
{
    // connect outside constructor because of virtual "path" function is involved
    disconnectAtEOL(
        (*this)->valueChanged.connect(
            [this]() {
                model_->notifyParameterChange( *this );
            }
            )
        );

    disconnectAtEOL(
        (*this)->beforeChildInsertion.connect(
            [this](int r0, int r1)
            {
                QModelIndex idx=model_->indexFromParameter(*this, 0);
                model_->beginInsertRows( idx, r0, r1 );
            }
            )
        );

    disconnectAtEOL(
        (*this)->childInsertionDone.connect(
            [this](int, int)
            {
                model_->endInsertRows();
            }
            )
        );

    disconnectAtEOL(
        (*this)->beforeChildRemoval.connect(
            [this](int r0, int r1)
            {
                QModelIndex idx=model_->indexFromParameter(*this, 0);
                model_->beginRemoveRows( idx, r0, r1 );
            }
            )
        );

    disconnectAtEOL(
        (*this)->childRemovalDone.connect(
            [this](int, int)
            {
                model_->endRemoveRows();
            }
            )
        );
}





IQParameterSetModel *IQParameter::model() const
{
  return model_;
}

QString IQParameter::name() const
{
    return QString::fromStdString(get()->name());
}





QString IQParameter::valueText() const
{
  return QString::fromStdString((*this)->type());
}



bool IQParameter::setValue(QVariant)
{
    return false;
}



void IQParameter::resetModificationState()
{
  markedAsModified_.reset();
}

bool IQParameter::isModified() const
{
  if (!markedAsModified_)
  {
    markedAsModified_ =
          (*this)->isModified(defaultParameterSet_);
  }

  return *markedAsModified_;
}

QVariant IQParameter::backgroundColor() const
{
    if ((*this)->isNecessary())
    return QColor(Qt::yellow);

  return QVariant();
}

QVariant IQParameter::textColor() const
{
  if ((*this)->isHidden())
    return QColor(Qt::gray);
  if ((*this)->isExpert())
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

  QLabel *nameLabel = new QLabel(
      QString::fromStdString((*this)->name()),
      editControlsContainer );
  QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
  layout->addWidget(nameLabel);

  auto *shortDescLabel =
    new IQSimpleLatexView( (*this)->description(), editControlsContainer );
  layout->addWidget(shortDescLabel);

  auto analysisName = model_->getAnalysisName();
  if (!analysisName.empty())
  {
    std::shared_ptr<insight::ParameterSet> propositions =
        insight::AnalysisParameterPropositions::getCombinedPropositionsForParameter(
            analysisName,
            (*this)->path(),
            model_->getParameterSet()
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


  editControlsContainer->setLayout(layout);

  return layout;
}




void IQParameter::applyProposition(
    const insight::ParameterSet &propositions,
    const std::string &selectProposition )
{
#warning does nothing yet, should be abstract
}

defineStaticFunctionTableWithArgs(
    IQParameterGridViewDelegateEditorWidget,
    createDelegate, QAbstractItemDelegate*,
    LIST( QObject* parent ),
    LIST( parent )
);

IQParameterGridViewDelegateEditorWidget::IQParameterGridViewDelegateEditorWidget(
    QObject* parent,
    const IQParameter& parameter,
    const QModelIndex& index)
    : QObject(parent), index_(index)
{
    if (IQParameterGridViewDelegateEditorWidget::has_createDelegate(parameter.type()))
    {
        delegate_=
            IQParameterGridViewDelegateEditorWidget::createDelegateFor(
            parameter.type(), this);
    }
    else
    {
        delegate_=new QStyledItemDelegate(this);
    }
}





const IQParameterGridViewDelegateEditorWidget*
IQParameterGridViewSelectorDelegate::delegateWidgetForIndex(
    const QModelIndex &idx) const
{
    for (auto& c: children())
    {
        if (auto *delw =
            dynamic_cast<const IQParameterGridViewDelegateEditorWidget*>(c))
        {
            if (delw->index()==idx)
                return delw;
        }
    }
    return nullptr;
}




void IQParameterGridViewSelectorDelegate::destroyEditor(
    QWidget *editor, const QModelIndex &index) const
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        d->delegate()->destroyEditor(editor, index);
        const_cast<IQParameterGridViewDelegateEditorWidget*>(d)->deleteLater();
    }
    else
        QStyledItemDelegate::destroyEditor(editor, index);
}




bool IQParameterGridViewSelectorDelegate::editorEvent(
    QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        return d->delegate()->editorEvent(event, model, option, index);
    }
    else
        return QStyledItemDelegate::editorEvent(event, model, option, index);
}




bool IQParameterGridViewSelectorDelegate::helpEvent(
    QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        return d->delegate()->helpEvent(event, view, option, index);
    }
    else
        return QStyledItemDelegate::helpEvent(event, view, option, index);
}




void IQParameterGridViewSelectorDelegate::paint(
    QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        d->delegate()->paint(painter, option, index);
    }
    else
        QStyledItemDelegate::paint(painter, option, index);
}




QWidget* IQParameterGridViewSelectorDelegate::createEditor(
    QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (auto *psm =dynamic_cast<const IQParameterSetModel*>(
            index.model()))
    {
        auto *iqp = psm->iqIndexData(index);
        auto dw = new IQParameterGridViewDelegateEditorWidget(
            const_cast<IQParameterGridViewSelectorDelegate*>(this),
            *iqp, index);
        return dw->delegate()->createEditor(parent, option, index);
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}



void IQParameterGridViewSelectorDelegate::setEditorData(
    QWidget *editor, const QModelIndex &index) const
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        d->delegate()->setEditorData(editor, index);
    }
    else
        QStyledItemDelegate::setEditorData(editor, index);
}


void IQParameterGridViewSelectorDelegate::setModelData(
    QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        d->delegate()->setModelData(editor, model, index);
    }
    else
        QStyledItemDelegate::setModelData(editor, model, index);
}



QSize IQParameterGridViewSelectorDelegate::sizeHint(
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        return d->delegate()->sizeHint(option, index);
    }
    else
        return QStyledItemDelegate::sizeHint(option, index);
}


void IQParameterGridViewSelectorDelegate::updateEditorGeometry(
    QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        d->delegate()->updateEditorGeometry(editor, option, index);
    }
    else
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
}





