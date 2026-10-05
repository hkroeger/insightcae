#include "iqhierarchicaldatafilterproxymodel.h"
#include "iqhierarchicaldatamodel.h"

#include "base/translations.h"

#include <QMenu>
#include <QActionGroup>


IQHierarchicalDataFilterProxyModel::IQHierarchicalDataFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{}




const insight::hierarchicalData::Filter &
IQHierarchicalDataFilterProxyModel::filter() const
{
    return filter_;
}




void IQHierarchicalDataFilterProxyModel::resetFilter(
    const insight::hierarchicalData::Filter &filter )
{
    filter_=filter;
    invalidateFilter();
}




bool IQHierarchicalDataFilterProxyModel::filterAcceptsRow(
    int sourceRow,
    const QModelIndex &sourceParent ) const
{
    if ( auto *hdm =
        dynamic_cast<IQHierarchicalDataModel*>(sourceModel()) )
    {
        QModelIndex index0 = hdm->index(sourceRow, 0, sourceParent);

        auto *e = IQHierarchicalDataModel::elementOfIndex(index0);
        bool isDisplayed = !( e && filter_.matches( e->path() ) );

        if ( sourceParent.isValid() )
        {
            QModelIndex pindex0 = hdm->parent(index0);
            isDisplayed = isDisplayed
                          && filterAcceptsRow(pindex0.row(), hdm->parent(pindex0));
        }
        return isDisplayed;
    }
    return true;
}




void populatePredefinedFilterMenu(
    QMenu *menu,
    const std::string &analysisName,
    insight::hierarchicalData::PredefinedFilter::Target target,
    std::function<void(const insight::hierarchicalData::Filter&)> apply )
{
    auto filters =
        insight::hierarchicalData::PredefinedFilterRegistry::global()
            .applicableFilters(analysisName, target);

    if (filters.empty())
    {
        auto *act = menu->addAction(_("(none defined)"));
        act->setEnabled(false);
        return;
    }

    auto *group = new QActionGroup(menu);
    group->setExclusive(true);

    auto *noFilter = menu->addAction(_("&No filter"));
    noFilter->setCheckable(true);
    noFilter->setChecked(true);
    group->addAction(noFilter);
    QObject::connect(
        noFilter, &QAction::triggered, menu,
        [apply]() { apply(insight::hierarchicalData::Filter()); } );

    menu->addSeparator();

    for (const auto* pf: filters)
    {
        auto *act = menu->addAction(QString::fromStdString(pf->label));
        act->setCheckable(true);
        group->addAction(act);
        auto filter = pf->filter;
        QObject::connect(
            act, &QAction::triggered, menu,
            [apply, filter]() { apply(filter); } );
    }
}
