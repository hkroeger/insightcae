#include "iqfilteredresultsetmodel.h"


namespace insight
{

IQFilteredResultSetModel::IQFilteredResultSetModel(QObject *parent)
{}




IQResultElement *IQFilteredResultSetModel::getResultElement(const QModelIndex &idx)
{
    if ( auto *rsm =
        dynamic_cast<IQResultSetModel*>(sourceModel()) )
    {
        return rsm->getResultElement( mapToSource(idx) );
    }
    return nullptr;
}

const hierarchicalData::Filter &IQFilteredResultSetModel::filter() const
{
    return filter_;
}



void IQFilteredResultSetModel::resetFilter(const hierarchicalData::Filter &filter)
{
    filter_=filter;
    invalidateFilter();
}


// void IQFilteredResultSetModel::addChildren(
//     const QModelIndex& pidx,
//     insight::ResultElementCollection* re) const
// {
//     for (int row=0; row<rowCount(pidx); ++row)
//     {
//         auto cidx = index(row, 0, pidx);
//         auto scidx = mapToSource(cidx);
//         if (auto e = IQHierarchicalDataModel::elementOfIndex(scidx))
//         {
//             //            if (e->isChecked()==Qt::Checked || e->isChecked()==Qt::PartiallyChecked)
//             {
//                 auto toBeInserted = e->clone();
//                 if ( auto rec =
//                     dynamic_cast<insight::ResultElementCollection*>(toBeInserted.get()) )
//                 {
//                     rec->clear();
//                     addChildren(cidx, rec);
//                 }
//                 re->insert( e->name(),  std::move(toBeInserted) );
//             }
//         }
//     }
// }

// ResultSetPtr IQFilteredResultSetModel::filteredResultSet() const
// {
//     auto *orgResultModel =
//         dynamic_cast<IQResultSetModel*>(sourceModel());
//     auto &orgResultSet = orgResultModel->resultSet();

//     std::string author = orgResultSet.author();
//     std::string date = orgResultSet.date();
//     auto fr = std::make_unique<ResultSet>(
//         orgResultSet.parameters().cloneAs<ParameterSet>(),
//         orgResultSet.title(),
//         orgResultSet.subtitle(),
//         &author, &date
//         );
//     addChildren( QModelIndex(), fr.get() );
//     return fr;
// }


bool IQFilteredResultSetModel::filterAcceptsRow(
    int sourceRow,
    const QModelIndex &sourceParent ) const
{
    if ( auto *rsm =
        dynamic_cast<IQResultSetModel*>(sourceModel()) )
    {
        QModelIndex index0 = rsm->index(sourceRow, 0, sourceParent);

        bool isDisplayed = !filter_.matches(
            rsm->elementOfIndex(index0)->path() );

        if ( sourceParent.isValid() )
        {
            QModelIndex pindex0 = rsm->parent(index0);
            isDisplayed = isDisplayed
                          && filterAcceptsRow(pindex0.row(), rsm->parent(pindex0));
        }
        return isDisplayed;
    }
    return true;
}

}
