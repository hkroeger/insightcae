#include "iqparametersetmodelobserver.h"

#include "iqparameter.h"
#include "base/exception.h"

#include <QMessageBox>




const insight::Parameter* findParameter(
    const IQParameterSetModel* model,
    const std::string& path )
{
    if (!model || !model->hasData())
        return nullptr;

    auto& root = model->getParameterSet();

    if (path.empty())
        return &root;

    if (!root.hasPath(path))
        return nullptr;

    return dynamic_cast<const insight::Parameter*>(
        &const_cast<insight::ParameterSet&>(root).getByPath(path) );
}




IQParameter* ensureParameterWrapper(
    IQParameterSetModel* model,
    const std::string& path )
{
    if (path.empty() || !findParameter(model, path))
        return nullptr;

    auto idx = model->indexOfPath(path, 0);
    return dynamic_cast<IQParameter*>(
        model->iqElementOfIndex(idx) );
}




bool modifyParameter(
    IQParameterSetModel* model,
    const std::string& path,
    std::function<void(insight::Parameter&)> modification,
    QWidget* messageParent )
{
    try
    {
        if (!ensureParameterWrapper(model, path))
            return false;

        modification(model->parameterRef(path));
        return true;
    }
    catch (const std::exception& ex)
    {
        QMessageBox::critical(
            messageParent,
            QObject::tr("Could not modify parameter"),
            QObject::tr("Modification of parameter %1 failed:\n%2")
                .arg(QString::fromStdString(path), QString::fromStdString(ex.what())) );
    }
    return false;
}




std::string joinParameterPath(
    const std::string& base, const std::string& rel )
{
    if (base.empty()) return rel;
    if (rel.empty()) return base;
    return base+"/"+rel;
}




IQParameterSetModelObserver::IQParameterSetModelObserver(
    IQParameterSetModel *model,
    QObject *parent )
    : QObject(parent),
      model_(model),
      timer_(new QTimer(this))
{
    timer_->setSingleShot(true);
    timer_->setInterval(0);
    connect(timer_, &QTimer::timeout,
            this, &IQParameterSetModelObserver::parameterSetChanged);

    if (!model_) return;

    connect(model_, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex& topLeft, const QModelIndex&, const QVector<int>&)
            { onSourceChange(topLeft); } );

    auto structuralChange =
        [this](const QModelIndex& parent, int, int)
        { onSourceChange(parent); };
    connect(model_, &QAbstractItemModel::rowsInserted, this, structuralChange);
    connect(model_, &QAbstractItemModel::rowsRemoved, this, structuralChange);
    connect(model_, &QAbstractItemModel::rowsMoved, this,
            &IQParameterSetModelObserver::scheduleNotification);
    connect(model_, &QAbstractItemModel::modelReset, this,
            &IQParameterSetModelObserver::scheduleNotification);
    connect(model_, &QAbstractItemModel::layoutChanged, this,
            &IQParameterSetModelObserver::scheduleNotification);
    connect(model_, &IQHierarchicalDataModel::bulkUpdateFinished, this,
            &IQParameterSetModelObserver::scheduleNotification);
}




IQParameterSetModel *IQParameterSetModelObserver::model() const
{
    return model_;
}




void IQParameterSetModelObserver::setWatchedPaths(
    const std::vector<std::string> &paths )
{
    watchedPaths_=paths;
}




bool IQParameterSetModelObserver::isRelevant(const QModelIndex &index) const
{
    if (watchedPaths_.empty())
        return true;

    if (!index.isValid())
        return true; // change at root level

    auto changedPath =
        model_->data(index.siblingAtColumn(IQParameterSetModel::stringPathCol))
            .toString().toStdString();

    for (const auto& wp: watchedPaths_)
    {
        // changed element is at or below watched path
        // or is a parent of the watched path (e.g. array insertion)
        if ( changedPath.compare(0, wp.size(), wp)==0
            || wp.compare(0, changedPath.size(), changedPath)==0 )
            return true;
    }
    return false;
}




void IQParameterSetModelObserver::onSourceChange(const QModelIndex &index)
{
    if (!model_ || model_->isBulkUpdateInProgress())
        return; // bulkUpdateFinished will follow

    if (isRelevant(index))
        scheduleNotification();
}




void IQParameterSetModelObserver::scheduleNotification()
{
    timer_->start();
}
