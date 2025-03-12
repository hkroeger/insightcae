#include "drawingviewsmodel.h"
#include <qnamespace.h>
#include <sstream>


std::string generateViewDefinitionExpression
    (const DrawingViewDefinition& vd)
{
    std::ostringstream os;
    os <<"(" << vd.onPointExpr<<", "<<vd.normalExpr;
    if (!vd.upExpr.empty()) os << ", up "<<vd.upExpr;
    if (vd.isSection) os << ", section";
    if (vd.poly) os<<", poly";
    if (vd.skipHL) os<<", skiphl";
    if (vd.add.size())
    {
        os<<", add ";
        for (auto& av: vd.add)
        {
            switch (av)
            {
            case AddedView::l: os<<"l"; break;
            case AddedView::r: os<<"r"; break;
            case AddedView::t: os<<"t"; break;
            case AddedView::b: os<<"b"; break;
            case AddedView::k: os<<"k"; break;
            }
        }
    }
    os<<")";
    return os.str();
}



DrawingViewsModel::DrawingViewsModel(QObject *parent)
    : QAbstractListModel(parent)
{}

QVariant DrawingViewsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // FIXME: Implement me!
    if ((orientation==Qt::Horizontal)&&(role==Qt::DisplayRole))
    {
        switch (section)
        {
        case 0: return "Name"; break;
        case 1: return "Definition"; break;
        };
    }
    return QVariant();
}

int DrawingViewsModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return viewDefinitions_.size();
}

int DrawingViewsModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant DrawingViewsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    if (role==Qt::DisplayRole)
    {
        auto& i=viewDefinitions_[index.row()];
        switch (index.column())
        {
        case 0:
            return QString::fromStdString(i.label);
            break;
        case 1:
            return QString::fromStdString(
                generateViewDefinitionExpression(i));
            break;
        };
    }

    return QVariant();
}

void DrawingViewsModel::appendView(
    const DrawingViewDefinition &vd)
{
    int i=viewDefinitions_.size();
    beginInsertRows(QModelIndex(), i, i);
    viewDefinitions_.push_back(vd);
    endInsertRows();
}

DrawingViewDefinition DrawingViewsModel::view(const QModelIndex &idx)
{
    return viewDefinitions_[idx.row()];
}

void DrawingViewsModel::editView(
    const QModelIndex &idx,
    const DrawingViewDefinition &vd )
{
    if (idx.isValid())
    {
        viewDefinitions_[idx.row()]=vd;
        emit dataChanged(idx.siblingAtColumn(0), idx.siblingAtColumn(1));
    }
}

void DrawingViewsModel::removeView(const QModelIndex &idx)
{
    if (idx.isValid())
    {
        int i=idx.row();
        beginRemoveRows(QModelIndex(), i, i);
        viewDefinitions_.erase(viewDefinitions_.begin()+i);
        endRemoveRows();
    }
}

const std::vector<DrawingViewDefinition> &
DrawingViewsModel::viewDefinitions() const
{
    return viewDefinitions_;
}
