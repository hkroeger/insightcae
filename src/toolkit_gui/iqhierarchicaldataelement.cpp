#include "iqhierarchicaldataelement.h"
#include "base/hierarchicalelement.h"

#include <QLabel>
#include "qtextensions.h"

void IQHierarchicalDataElement::invalidate()
{
    deleteLater();
    std::observer_ptr<insight::hierarchicalData::Element>::invalidate();
}




defineType(IQHierarchicalDataElement);
defineFactoryTable
    (
        IQHierarchicalDataElement,
        LIST(
            QObject* parent,
            IQHierarchicalDataModel* model,
            insight::hierarchicalData::Element* element),
        LIST(parent,
             model,
             element )
        );




// used in createForChild and for root element
IQHierarchicalDataElement* IQHierarchicalDataElement::create(
    QObject *parent,
    IQHierarchicalDataModel* model,
    insight::hierarchicalData::Element *e )
{
    IQHierarchicalDataElement *ne{ nullptr };

    if (IQHierarchicalDataElement::has_factory(e->type()))
    {
        ne = IQHierarchicalDataElement::lookup(e->type(), parent, model, e);
    }
    else
    {
        ne = new IQHierarchicalDataElement(parent, model, e);
    }

    ne->connectSignals();

    return ne;
}

IQHierarchicalDataElement *IQHierarchicalDataElement::createForChild(
    IQHierarchicalDataModel *model,
    insight::hierarchicalData::Element *ce )
{
    return create( this, model, ce );
}



IQHierarchicalDataElement::IQHierarchicalDataElement(
    QObject *parent,
    IQHierarchicalDataModel* model,
    insight::hierarchicalData::Element *element )
    : std::observer_ptr<insight::hierarchicalData::Element>(element),
    QObject(parent),
    model_(model)
{}



IQHierarchicalDataElement::~IQHierarchicalDataElement()
{}




void IQHierarchicalDataElement::removeFromViews(bool deleteLater)
{
    auto *parentParameter =
        dynamic_cast<IQHierarchicalDataElement*>(
            this->parentElement());

    auto *m=this->model();
    auto myIndex=m->indexOfElement(*this->get(), 0);
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

IQHierarchicalDataElement *IQHierarchicalDataElement::visibleParent() const
{
    return dynamic_cast<IQHierarchicalDataElement*>(parent());
}


IQHierarchicalDataElement *IQHierarchicalDataElement::parentElement() const
{
    if (get()->hasParent())
        if (auto *iqp=model_->findWrapper(get()->parent()))
            return iqp;
    return nullptr;
}


QList<IQHierarchicalDataElement *> IQHierarchicalDataElement::children() const
{
    QList<IQHierarchicalDataElement *> ch;
    for (auto& p: *get())
    {
        if (auto *iqp=model_->findWrapper(p))
            ch.append(iqp);
    }
    return ch;
}

void IQHierarchicalDataElement::connectSignals()
{
    // connect outside constructor because of virtual "path" function is involved
    disconnectAtEOL(
        (*this)->valueChanged.connect(
            [this]() {
                insight::dbg(insight::DetailedBusiness)
                    << "received valueChanged from "<<(*this)->name()
                    <<" => call notifyElementChange from IQHierarchicalDataElement" << std::endl;
                model_->notifyElementChange( *this );
            }
            )
        );

    disconnectAtEOL(
        (*this)->beforeChildInsertion.connect(
            [this](int r0, int r1)
            {
                insight::dbg(insight::DetailedBusiness)
                << "received beforeChildInsertion => call beginInsertRows from IQHierarchicalDataElement" << std::endl;
                QModelIndex idx=model_->indexOfElement(*this, 0);
                model_->beginInsertRows( idx, r0, r1 );
            }
            )
        );

    disconnectAtEOL(
        (*this)->childInsertionDone.connect(
            [this](int, int)
            {
                insight::dbg(insight::DetailedBusiness)
                << "received childInsertionDone => call endInsertRows from IQHierarchicalDataElement" << std::endl;
                model_->endInsertRows();
            }
            )
        );

    disconnectAtEOL(
        (*this)->beforeChildRemoval.connect(
            [this](int r0, int r1)
            {
                insight::dbg(insight::DetailedBusiness)
                << "received beforeChildRemoval => call beginRemoveRows from IQHierarchicalDataElement" << std::endl;
                QModelIndex idx=model_->indexOfElement(*this, 0);
                model_->beginRemoveRows( idx, r0, r1 );
            }
            )
        );

    disconnectAtEOL(
        (*this)->childRemovalDone.connect(
            [this](int, int)
            {
                insight::dbg(insight::DetailedBusiness)
                << "received childRemovalDone => call endRemoveRows from IQHierarchicalDataElement" << std::endl;
                model_->endRemoveRows();
            }
            )
        );
}





IQHierarchicalDataModel *IQHierarchicalDataElement::model() const
{
    return model_;
}

QString IQHierarchicalDataElement::name() const
{
    return QString::fromStdString(get()->name());
}





QVariant IQHierarchicalDataElement::value() const
{
    return QString::fromStdString(
        (*this)->plainTextRepresentation() );
}

bool IQHierarchicalDataElement::canSetValue() const
{
    return get()->canSetDataFromString();
}



bool IQHierarchicalDataElement::setValue(QVariant value)
{
    auto &e=(*this);
    if (e->canSetDataFromString())
    {
        bool ok=false;
        e->setDataFromString(value.toString().toStdString(), &ok);
        return ok;
    }
    else
    {
        return false;
    }
}




QVariant IQHierarchicalDataElement::backgroundColor() const
{
    return QVariant();
}

QVariant IQHierarchicalDataElement::textColor() const
{
    return QVariant();
}

QVariant IQHierarchicalDataElement::textFont() const
{
    return QVariant();
}

void IQHierarchicalDataElement::populateContextMenu(
    QMenu* /*cm*/,
    IQCADModel3DViewer */*viewer*/)
{}


QVBoxLayout* IQHierarchicalDataElement::populateEditControls(
    QWidget* editControlsContainer,
    IQCADModel3DViewer * )
{


    QVBoxLayout *layout=new QVBoxLayout;

    auto *nameLabel = new QLabel(
        QString::fromStdString((*this)->name()),
        editControlsContainer );
    QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
    layout->addWidget(nameLabel);


    editControlsContainer->setLayout(layout);

    return layout;
}

void IQHierarchicalDataElement::checkEnabledOrDisabled()
{
    Q_EMIT setControlsEnabled(!model_->editingIsDisabled_);
}

















defineStaticFunctionTableWithArgs(
    IQHierarchicalDataGridViewDelegateEditorWidget,
    createDelegate, QAbstractItemDelegate*,
    LIST( QObject* parent ),
    LIST( parent )
    );

IQHierarchicalDataGridViewDelegateEditorWidget::IQHierarchicalDataGridViewDelegateEditorWidget(
    QObject* parent,
    const IQHierarchicalDataElement& parameter,
    const QModelIndex& index)
    : QObject(parent), index_(index)
{
    if (IQHierarchicalDataGridViewDelegateEditorWidget::has_createDelegate(parameter.type()))
    {
        delegate_=
            IQHierarchicalDataGridViewDelegateEditorWidget::createDelegateFor(
                parameter.type(), this);
    }
    else
    {
        delegate_=new QStyledItemDelegate(this);
    }
}





const IQHierarchicalDataGridViewDelegateEditorWidget*
IQHierarchicalDataGridViewSelectorDelegate::delegateWidgetForIndex(
    const QModelIndex &idx) const
{
    for (auto& c: children())
    {
        if (auto *delw =
            dynamic_cast<const IQHierarchicalDataGridViewDelegateEditorWidget*>(c))
        {
            if (delw->index()==idx)
                return delw;
        }
    }
    return nullptr;
}




void IQHierarchicalDataGridViewSelectorDelegate::destroyEditor(
    QWidget *editor, const QModelIndex &index) const
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        d->delegate()->destroyEditor(editor, index);
        const_cast<IQHierarchicalDataGridViewDelegateEditorWidget*>(d)->deleteLater();
    }
    else
        QStyledItemDelegate::destroyEditor(editor, index);
}




bool IQHierarchicalDataGridViewSelectorDelegate::editorEvent(
    QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        return d->delegate()->editorEvent(event, model, option, index);
    }
    else
        return QStyledItemDelegate::editorEvent(event, model, option, index);
}




bool IQHierarchicalDataGridViewSelectorDelegate::helpEvent(
    QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        return d->delegate()->helpEvent(event, view, option, index);
    }
    else
        return QStyledItemDelegate::helpEvent(event, view, option, index);
}




void IQHierarchicalDataGridViewSelectorDelegate::paint(
    QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        d->delegate()->paint(painter, option, index);
    }
    else
        QStyledItemDelegate::paint(painter, option, index);
}




QWidget* IQHierarchicalDataGridViewSelectorDelegate::createEditor(
    QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (auto *psm =dynamic_cast<const IQHierarchicalDataModel*>(
            index.model()))
    {
        auto &iqp = dynamic_cast<const IQHierarchicalDataElement&>(
            *psm->iqElementOfIndex(index));
        auto dw = new IQHierarchicalDataGridViewDelegateEditorWidget(
            const_cast<IQHierarchicalDataGridViewSelectorDelegate*>(this),
            iqp, index);
        return dw->delegate()->createEditor(parent, option, index);
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}



void IQHierarchicalDataGridViewSelectorDelegate::setEditorData(
    QWidget *editor, const QModelIndex &index) const
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        d->delegate()->setEditorData(editor, index);
    }
    else
        QStyledItemDelegate::setEditorData(editor, index);
}


void IQHierarchicalDataGridViewSelectorDelegate::setModelData(
    QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        d->delegate()->setModelData(editor, model, index);
    }
    else
        QStyledItemDelegate::setModelData(editor, model, index);
}



QSize IQHierarchicalDataGridViewSelectorDelegate::sizeHint(
    const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        return d->delegate()->sizeHint(option, index);
    }
    else
        return QStyledItemDelegate::sizeHint(option, index);
}


void IQHierarchicalDataGridViewSelectorDelegate::updateEditorGeometry(
    QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (auto *d=delegateWidgetForIndex(index))
    {
        d->delegate()->updateEditorGeometry(editor, option, index);
    }
    else
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
}


