#include "iqresultsetfiltermodel.h"

IQResultSetFilterModel::IQResultSetFilterModel(
        QObject *parent)
    : QAbstractListModel(parent)
{
}

QVariant IQResultSetFilterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role==Qt::DisplayRole)
    {
        switch(section)
        {
        case 0: return "Expression";
        };
    }
    return QVariant();
}

int IQResultSetFilterModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return rsf_.size();
}

QVariant IQResultSetFilterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role==Qt::DisplayRole)
    {
        auto i=rsf_.begin();
        std::advance(i, index.row());
        auto &f = *i;
        if (auto *c = boost::get<std::string>(&f))
        {
            return QString::fromStdString(*c);
        }
        else if (auto *ex = boost::get<boost::regex>(&f))
        {
            return QString::fromStdString(ex->str());
        }
    }

    return QVariant();
}

bool IQResultSetFilterModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    auto br=rsf_.begin(), er=rsf_.begin();
    std::advance(br, row);
    std::advance(er, row+count);
    beginRemoveRows(parent, row, row+count);
    rsf_.erase(br, er);
    endRemoveRows();
    return true;
}


void IQResultSetFilterModel::resetFilter(const insight::ResultSetFilter &rsf)
{
    beginResetModel();
    rsf_=rsf;
    endResetModel();
}

void IQResultSetFilterModel::clear()
{
    beginResetModel();
    rsf_.clear();
    endResetModel();
}

const insight::ResultSetFilter &IQResultSetFilterModel::filter() const
{
    return rsf_;
}
