#include "iqconstrainedsketchlayerlistmodel.h"




IQConstrainedSketchLayerListModel::IQConstrainedSketchLayerListModel(
    IQVTKConstrainedSketchEditor *se, QObject *parent)
    : QAbstractTableModel{parent},
    sketchEditor_(se)
{}




int IQConstrainedSketchLayerListModel::rowCount(const QModelIndex &parent) const
{
    return layers_.size();
}




int IQConstrainedSketchLayerListModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}




Qt::ItemFlags IQConstrainedSketchLayerListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    auto f=QAbstractTableModel::flags(index);

    switch (index.column())
    {
    case 0:
        f|=Qt::ItemIsUserCheckable;
    case 1:
        f|=Qt::ItemIsEditable;
    }

    return f;
}




QVariant IQConstrainedSketchLayerListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role==Qt::DisplayRole)
    {
        if (orientation==Qt::Horizontal)
        {
            switch (section)
            {
            case 0:
                return "Visible";
            case 1:
                return "Layer name";
            }
        }
    }
    return QVariant();
}




QVariant IQConstrainedSketchLayerListModel::data(
    const QModelIndex &index,
    int role ) const
{
    if (index.isValid())
    {
        if (role==Qt::DisplayRole || role==Qt::EditRole)
        {
            int r=index.row();
            if (r>=0 && r<layers_.size())
            {
                auto i=layers_.begin();
                std::advance(i, r);
                switch (index.column())
                {
                case 1:
                    return QString::fromStdString(*i);
                }
            }
        }
        else if (role==Qt::CheckStateRole)
        {
            int r=index.row();
            if (r>=0 && r<layers_.size())
            {
                auto i=layers_.begin();
                std::advance(i, r);
                switch (index.column())
                {
                case 0:
                    return sketchEditor_->layerIsVisible(*i) ?
                               Qt::Checked : Qt::Unchecked;
                }
            }
        }
    }
    return QVariant();
}




bool IQConstrainedSketchLayerListModel::setData(
    const QModelIndex &index,
    const QVariant &value,
    int role )
{
    if (index.isValid())
    {
        if (role==Qt::EditRole)
        {
            int r=index.row();
            if (r>=0 && r<layers_.size())
            {
                auto i=layers_.begin();
                std::advance(i, r);
                switch (index.column())
                {
                case 1:
                    Q_EMIT renameLayer(
                        *i,
                        value.toString().toStdString() );
                    return true;
                }
            }
        }
        else if (role==Qt::CheckStateRole)
        {
            int r=index.row();
            if (r>=0 && r<layers_.size())
            {
                auto i=layers_.begin();
                std::advance(i, r);
                switch (index.column())
                {
                case 0:
                    auto cs = value.value<Qt::CheckState>();
                    if (cs==Qt::Checked)
                        Q_EMIT showLayer(*i);
                    else if (cs==Qt::Unchecked)
                        Q_EMIT hideLayer(*i);
                    return true;
                }
            }
        }
    }
    return false;
}




void IQConstrainedSketchLayerListModel::update()
{
    auto oldLayers=layers_;
    layers_=(*sketchEditor_)->layerNames();

    // compare
    //  remove vanished
    if (oldLayers.size())
    {
        for (auto i=oldLayers.rbegin(); i!=oldLayers.rend(); ++i) // start from end
        {
            auto ii=i; ii++;
            int r=std::distance(oldLayers.begin(), ii.base());
            if (layers_.count(*i)<1) // not in new layer list
            {
                beginRemoveRows(QModelIndex(), r, r);
                endRemoveRows();
            }
        }
    }
    //  add new
    for (auto i=layers_.begin(); i!=layers_.end(); ++i) // start from end
    {
        int r=std::distance(layers_.begin(), i);
        if (oldLayers.count(*i)<1) // not in old layer list
        {
            beginInsertRows(QModelIndex(), r, r);
            endInsertRows();
        }
    }
}
