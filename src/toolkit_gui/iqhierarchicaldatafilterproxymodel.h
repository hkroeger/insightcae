#ifndef IQHIERARCHICALDATAFILTERPROXYMODEL_H
#define IQHIERARCHICALDATAFILTERPROXYMODEL_H

#include "toolkit_gui_export.h"

#include <functional>
#include <string>

#include <QSortFilterProxyModel>

#include "base/hierarchicaldatafilter.h"
#include "base/predefinedfilters.h"

class QMenu;


/**
 * @brief The IQHierarchicalDataFilterProxyModel class
 * hides all elements of an IQHierarchicalDataModel (parameter set or result set),
 * whose path matches the filter, together with their children.
 */
class TOOLKIT_GUI_EXPORT IQHierarchicalDataFilterProxyModel
    : public QSortFilterProxyModel
{
    Q_OBJECT

    insight::hierarchicalData::Filter filter_;

public:
    IQHierarchicalDataFilterProxyModel(QObject *parent = nullptr);

    const insight::hierarchicalData::Filter& filter() const;
    void resetFilter(const insight::hierarchicalData::Filter& filter);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};




/**
 * @brief populatePredefinedFilterMenu
 * add an exclusive selection of the predefined filters,
 * which are applicable to the given analysis and target, to menu.
 * On selection, apply is called with the selected filter (an empty filter for "No filter").
 */
void TOOLKIT_GUI_EXPORT populatePredefinedFilterMenu(
    QMenu* menu,
    const std::string& analysisName,
    insight::hierarchicalData::PredefinedFilter::Target target,
    std::function<void(const insight::hierarchicalData::Filter&)> apply );


#endif // IQHIERARCHICALDATAFILTERPROXYMODEL_H
