#include "iqhierarchicaldatamodel.h"
#include "base/hierarchicalelement.h"
#include "iqhierarchicaldataelement.h"
#include "qtextensions.h"

#include <QTimer>
#include <QApplication>
#include <QStyle>
#include <QStatusBar>


ParameterErrorState::ParameterErrorState(
    IQHierarchicalDataModel& model,
    const std::string& parameterPath,
    const QString& explanation,
    Severity s )
    : model_(model),
    parameterPath_(parameterPath),
    severity_(s),
    explanation_(explanation)
{
    model_.errors_.insert(this);

    auto idx=model_.indexOfPath(parameterPath_, 0);
    model_.notifyElementChange(idx);

    if (auto *mainWin = getMainWindow())
    {
        mainWin->statusBar()
            ->showMessage(explanation_, 10000);
    }
}




ParameterErrorState::~ParameterErrorState()
{
    model_.errors_.erase(this);
    auto idx=model_.indexOfPath(parameterPath_, 0);
    model_.notifyElementChange(idx);
}






QVariant ParameterErrorState::data(const QModelIndex &index, int role) const
{
    if (model_.indexOfPath(parameterPath_, index.column()) == index)
    {
        switch (role)
        {
        case Qt::BackgroundRole:
            if (severity_ == Red)
                return QBrush(Qt::red);
            else if (severity_ == Yellow)
                return QBrush(Qt::yellow);

        case Qt::DecorationRole:
            if (index.column() == IQHierarchicalDataModel::labelCol)
            {
                if (severity_ == Red)
                    return QApplication::style()
                        ->standardIcon(QStyle::SP_MessageBoxCritical);
                else
                    return QApplication::style()
                        ->standardIcon(QStyle::SP_MessageBoxWarning);
            }
            break;

        case Qt::ToolTipRole:
            return explanation_;
        }
    }
    return QVariant();
}




IQHierarchicalDataElement*
IQHierarchicalDataModel::findWrapper(
    const insight::hierarchicalData::Element& e) const
{
    auto i =

    std::find_if
    (
        wrappers_.begin(), wrappers_.end(),

        [&](decltype(wrappers_)::const_reference_type v)
         { return v.first.valid() && (v.first.get()==&e); }

     );

    if (i==wrappers_.end())
        return nullptr;
    else
        return const_cast<IQHierarchicalDataElement*>(&(*i).first);
}




int IQHierarchicalDataModel::countDisplayedChildren(const QModelIndex &index) const
{
    auto iqp=iqElementOfIndex(index);
    int n=0;
    for (auto& c: iqp->children())
    {
        if (dynamic_cast<IQHierarchicalDataElement*>(c))
            ++n;
    }
    return n;
}




void IQHierarchicalDataModel::editingOff()
{
    editingIsDisabled_=true;
}




void IQHierarchicalDataModel::editingOn()
{
    editingIsDisabled_=false;
}




bool IQHierarchicalDataModel::editingIsEnabled() const
{
    return !editingIsDisabled_;
}




IQHierarchicalDataModel::UndoState::UndoState(
    const QString &description,
    std::unique_ptr<insight::hierarchicalData::Element>&& sk)
    : IQUndoRedoStackState(description),
    std::unique_ptr<insight::hierarchicalData::Element>(std::move(sk))
{}




void IQHierarchicalDataModel::applyUndoState(
    const IQUndoRedoStackState& state )
{
    auto& s=dynamic_cast<const UndoState&>(state);
    insight::CurrentExceptionContext ex(
        "applying undo state %s", s.description().toStdString().c_str() );


    auto block = blockUndoRecording();

    data_->assignFrom(*s);

    dataBeforeLastChange_ =
        data_->clone();
}




IQUndoRedoStackStatePtr
IQHierarchicalDataModel::createUndoState(
    const QString& description ) const
{
    return std::make_shared<UndoState>(
        description,
        std::move(dataBeforeLastChange_) );
}




void IQHierarchicalDataModel::setCheckState(const QModelIndex &idx, bool checked)
{
    if (auto *e = dynamic_cast<IQHierarchicalDataElement*>(
            iqElementOfIndex(idx)))
    {
        e->setChecked( checked?Qt::Checked:Qt::Unchecked );
        emit dataChanged(idx, idx);
        setChildrenCheckstate( idx, checked );
        updateParentCheckState( idx );
    }
}




void IQHierarchicalDataModel::updateParentCheckState(const QModelIndex &idx)
{
    auto pidx=parent(idx);
    if (pidx.isValid())
    {
        if (auto *e = dynamic_cast<IQHierarchicalDataElement*>(
                iqElementOfIndex(pidx) ))
        {
            int nChecked=0, nUnchecked=0, nPartChecked=0;
            for (int row=0; row<rowCount(pidx); ++row)
            {
                auto cidx=index(row, 0, pidx);
                if (auto *c = dynamic_cast<IQHierarchicalDataElement*>(
                        iqElementOfIndex(cidx)))
                {
                    switch(c->isChecked())
                    {
                    case Qt::Checked: nChecked++; break;
                    case Qt::Unchecked: nUnchecked++; break;
                    case Qt::PartiallyChecked: nPartChecked++; break;
                    }
                }
            }

            auto oldcs=e->isChecked();
            Qt::CheckState newcs=oldcs;
            if (nChecked>0 && nUnchecked==0 && nPartChecked==0)
                newcs=Qt::Checked;
            else if (nUnchecked>0 && nChecked==0 && nPartChecked==0)
                newcs=Qt::Unchecked;
            else
                newcs=Qt::PartiallyChecked;
            insight::dbg()<<oldcs<<" "<<nChecked<<" "<<nUnchecked<<" "<<nPartChecked<<" "<<newcs<<std::endl;
            if (newcs!=oldcs)
            {
                e->setChecked(newcs);
                emit dataChanged(pidx, pidx);
                updateParentCheckState(pidx);
            }
        }
    }
}




void IQHierarchicalDataModel::setChildrenCheckstate(const QModelIndex& idx, bool checked)
{
    for (int row=0; row<rowCount(idx); ++row)
    {
        auto cidx=index(row, 0, idx);
        if (auto *c = dynamic_cast<IQHierarchicalDataElement*>(
                iqElementOfIndex(cidx)) )
        {
            c->setChecked( checked?Qt::Checked:Qt::Unchecked );
            emit dataChanged(cidx, cidx);
            setChildrenCheckstate(cidx, checked);
        }
    }
}




void IQHierarchicalDataModel::handleDataChangeForUndo(
    const QModelIndex &topLeft,
    const QModelIndex &bottomRight,
    const QVector<int> &roles)
{
    // qDebug() << "store undo "<<topLeft<<bottomRight<<roles;
    // qDebug()<<data(topLeft.siblingAtColumn(labelCol), Qt::DisplayRole);
    // qDebug()<<data(bottomRight.siblingAtColumn(labelCol), Qt::DisplayRole);

    QString description  =
        "modification of "+
        data(topLeft.siblingAtColumn(labelCol), Qt::DisplayRole)
            .toString()
        ;

    if (topLeft.row()==bottomRight.row())
    {
        storeUndoState(description);
    }
    else
    {
        storeUndoState(description+" and more");
    }

    dataBeforeLastChange_ =
        data_->clone();
}




insight::hierarchicalData::Element*
IQHierarchicalDataModel::elementOfIndex(const QModelIndex& idx)
{
    return
        static_cast<insight::hierarchicalData::Element*>(
            idx.internalPointer() );
}




IQHierarchicalDataElement* IQHierarchicalDataModel::iqElementOfIndex(const QModelIndex& idx)
{
    // create wrapper on the fly

    if (auto *p = elementOfIndex(idx))
    {
        auto *wrapper = findWrapper(*p);

        if (!wrapper)
        {
            // get non-const reference
            auto *model = const_cast<IQHierarchicalDataModel*>(this);

            if (!idx.parent().isValid())
            {
                wrapper=IQHierarchicalDataElement::create(model, model, p);
            }
            else
            {
                auto parentWrapper=iqElementOfIndex(idx.parent());
                wrapper=parentWrapper->createForChild(model, p);
            }

            wrappers_.insert( wrapper, 0 );
        }

        return wrapper;
    }

    return nullptr;
}




const IQHierarchicalDataElement* IQHierarchicalDataModel::iqElementOfIndex(
    const QModelIndex& idx) const
{
    return const_cast<IQHierarchicalDataModel*>(this)
    ->iqElementOfIndex(idx);
}




const insight::hierarchicalData::Element*
searchVisibleParent(const insight::hierarchicalData::Element& p, int* row=nullptr)
{
    const insight::hierarchicalData::Element *pp, *upmostParent;
    upmostParent = pp = &p.parent();
    while (pp)
    {
        int r=pp->childElementIndex(&p);
        if (r>=0)
        {
            upmostParent=pp;
            if (row) *row=r;

            if (pp->hasParent())
            {
                pp=&pp->parent();
                continue;
            }
        }
        break;
    }
    return upmostParent;
}




QModelIndex IQHierarchicalDataModel::indexOfElement(
    const insight::hierarchicalData::Element &p, int col) const
{
    auto *element = const_cast<insight::hierarchicalData::Element*>(&p);

    if (&p == data_.get())
        return QModelIndex();

    int row=-1;
    insight::assertion(
        searchVisibleParent(p, &row),
        "no parent of hierarchical element found");

    return createIndex(
        row, col, element );
}




QModelIndex IQHierarchicalDataModel::indexOfPath(
    const std::string &pp, int col ) const
{
    return indexOfElement(
        data_->get<insight::hierarchicalData::Element>(pp), col );
}




void IQHierarchicalDataModel::resetData(
    std::unique_ptr<insight::hierarchicalData::Element> &&data )
{
    beginResetModel();
    data_.reset();
    endResetModel();

    // create decorators that store parent relationship

    beginInsertRows(QModelIndex(), 0, data->nChildren()-1);
    data_ = std::move(data);
    endInsertRows();

    dataBeforeLastChange_ =
        data_->clone();
}




void IQHierarchicalDataModel::resetValue(
    const insight::hierarchicalData::Element &data)
{
    data_->assignFrom(data);
}




IQHierarchicalDataModel::IQHierarchicalDataModel(
    std::unique_ptr<insight::hierarchicalData::Element> &&data,
    QObject *parent,
    bool selectableElements,
    bool editingIsDisabled )
: QAbstractItemModel(parent),
  editingIsDisabled_(editingIsDisabled),
  selectableElements_(selectableElements)
{
    resetData( std::move(data) );

    connect(
        this, &IQHierarchicalDataModel::dataChanged,
        this, &IQHierarchicalDataModel::handleDataChangeForUndo
        );
}




int IQHierarchicalDataModel::columnCount(const QModelIndex &parent) const
{
  return 2;
}




QVariant IQHierarchicalDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role==Qt::DisplayRole)
    {
        if (orientation==Qt::Horizontal)
        {
            if (section==0)
                return QString("Name");
            else if (section==1)
                return QString("Value");
        }
    }

    return QVariant();
}







int	IQHierarchicalDataModel::rowCount(const QModelIndex &parent) const
{
    int s=0;

    if (data_)
    {
        if (!parent.isValid())
        {
            s=data_->nChildren();
        }
        else if (auto* p = elementOfIndex(parent))
        {
            if (parent.column()==0)
            {
                s = p->nChildren(); // only first column has children
            }
        }
    }

    return s;
}




QModelIndex	IQHierarchicalDataModel::index(int row, int column, const QModelIndex &parent) const
{
    QModelIndex i;

    if (!parent.isValid()) // in root dict
    {
        if (row>=0 && row<data_->nChildren())
        {
            i=indexOfElement(
                data_->childElement(row), column );
        }
    }
    else if (auto* p=elementOfIndex(parent)) // down in tree
    {
        if ( (row>=0) && (row<p->nChildren()) )
        {
            i=indexOfElement(
                p->childElement(row), column );
        }
    }

    return i;
}




QModelIndex	IQHierarchicalDataModel::parent(const QModelIndex &index) const
{
    QModelIndex i;
    int row;

    if ( index.isValid() )
    {
        if (auto* e = elementOfIndex(index))
        {
            if (auto *pp=searchVisibleParent(*e, &row))
            {
                i = indexOfElement(*pp, 0);
            }
        }
    }

    return i;
}




Qt::ItemFlags IQHierarchicalDataModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return 0;
    }

    auto flags=QAbstractItemModel::flags(index);

    auto *iqp=iqElementOfIndex(index);
    auto *p=iqp->get();


    flags |= Qt::ItemIsDragEnabled;

    if (editingIsEnabled())
    {
        flags |= Qt::ItemIsDropEnabled;

        if (index.column()==valueCol)
        {
            if ( iqp->canSetValue() )
                flags |= Qt::ItemIsEditable;

            if ( p->isBooleanData() )
                flags |= Qt::ItemIsUserCheckable;

            if (IQHierarchicalDataGridViewDelegateEditorWidget::has_createDelegate(iqp->type()))
            {
                flags |= Qt::ItemIsEditable;
            }
        }
    }

    if ( index.column() == 0 && selectableElements_ )
        flags |= Qt::ItemIsUserCheckable;

    return flags;
}




QVariant IQHierarchicalDataModel::data(const QModelIndex &index, int role) const
{
    for (auto*e: errors_)
    {
        auto r=e->data(index, role);
        if (!r.isNull())
            return r;
    }

    if (auto *iqp = iqElementOfIndex(index))
    {
        auto *p=iqp->get();


        switch (role)
        {

        case Qt::DisplayRole:
        case Qt::EditRole:
            if (index.column()==labelCol) // name
            {
                return iqp->name();
            }
            else if (index.column()==valueCol) // data
            {
                return iqp->value();
            }
            else if (index.column()==stringPathCol) // full path (hidden)
            {
                return QString::fromStdString(p->path());
            }
            else if (index.column()==iqParamCol) // pointer to decorator (hidden)
            {
                return QVariant::fromValue(static_cast<void*>(
                    const_cast<IQHierarchicalDataElement*>(iqp)));
            }
            break;

        case Qt::CheckStateRole:
            if ( index.column()==0 && selectableElements_)
            {
                return QVariant( iqp->isChecked() );
            }
            else if (index.column()==valueCol) // data
            {
                if (p->isBooleanData())
                    return p->getAsBoolean() ? Qt::Checked : Qt::Unchecked;
            }
            break;

        case Qt::BackgroundRole:
            return iqp->backgroundColor();
            break;

        case Qt::ForegroundRole:
            return iqp->textColor();
            break;

        case Qt::FontRole:
            return iqp->textFont();
            break;

        }
    }
    return QVariant();
}




bool IQHierarchicalDataModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (editingIsEnabled())
    {
        if (auto *iqp = iqElementOfIndex(index))
        {
            auto *p=iqp->get();

            switch (role)
            {

            case Qt::EditRole:
                if (index.column()==valueCol) // data
                {
                    return iqp->setValue(value);
                }
                break;

            case Qt::CheckStateRole:
                if ( (index.column()==0) && selectableElements_)
                {
                    setCheckState(index, value.toBool()?Qt::Checked:Qt::Unchecked);
                    return true;
                }
                else if (index.column()==valueCol) // data
                {
                    if (p->canSetFromBoolean())
                    {
                        p->setBoolean( value.value<Qt::CheckState>()==Qt::Checked );
                        return true;
                    }
                }
                break;
            }
        }
    }

    if (selectableElements_)
    {
        if ( (role==Qt::CheckStateRole) && (index.column()==0) )
        {
            setCheckState(index, value.toBool()?Qt::Checked:Qt::Unchecked);
            return true;
        }
    }

    return false;
}




IQHierarchicalDataModel::EditingDisabler::EditingDisabler(IQHierarchicalDataModel &psm)
    : psm_(psm)
{
    insight::dbg(insight::DetailedBusiness)<<"hierarchical data model: editing disabled";
    psm_.editingOff();
}



IQHierarchicalDataModel::EditingDisabler::~EditingDisabler()
{
    insight::dbg(insight::DetailedBusiness)<<"hierarchical data model: editing enabled";
    psm_.editingOn();
    if (additionalCleanup) additionalCleanup();
}



std::shared_ptr<IQHierarchicalDataModel::EditingDisabler>
IQHierarchicalDataModel::disableEditing()
{
    return std::make_shared<EditingDisabler>(*this);
}




void IQHierarchicalDataModel::notifyElementChange(const insight::hierarchicalData::Element& p)
{
    auto idx=indexOfElement(p, 0);
    if (idx.isValid())
    {
        notifyElementChange( idx );
    }
}




void IQHierarchicalDataModel::notifyElementChange(const QModelIndex &index)
{
    Q_ASSERT(index.isValid());

    Q_EMIT dataChanged(
        index.siblingAtColumn(0),
        index.siblingAtColumn(columnCount(index)-1)
        );
}




const insight::hierarchicalData::Element &IQHierarchicalDataModel::getHierarchicalData() const
{
    return *data_;
}




void IQHierarchicalDataModel::issueEphemeralError(
    const std::string &parameterPath,
    const QString &explanation,
    ParameterErrorState::Severity s)
{
    auto err=std::make_shared<ParameterErrorState>(
        *this, parameterPath, explanation, s);

    QTimer::singleShot(
        10000,
        [err]() mutable { err.reset(); }
    );
}

