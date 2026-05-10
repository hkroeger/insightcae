#include "iqconstrainedsketchlayerlistmodel.h"

#include "base/translations.h"

#include <QInputDialog>


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
        return Qt::ItemFlags();

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
                return _("Visible");
            case 1:
                return _("Layer name");
            }
        }
    }
    return QVariant();
}


void IQConstrainedSketchLayerListModel::contextMenu(
    QWidget* pw, const QModelIndex& index, const QPoint& p)
{
    if (index.isValid())
    {
        QMenu ctxMenu;

        {
            auto *removeAct = new QAction(_("Remove"));
            connect(removeAct, &QAction::triggered, removeAct,
                    [index,this]()
            {
                removeLayer(index);
            });
            ctxMenu.addAction(removeAct);
        }
        {
            auto *renameAct = new QAction(_("Rename..."));
            connect(
                renameAct, &QAction::triggered, renameAct,
                [index,pw,this]()
                {
                    auto newName=QInputDialog::getText(
                        pw,
                        _("Rename layer"),
                        _("Please enter new layer name:"),
                        QLineEdit::Normal,
                        data(index.siblingAtColumn(1)).toString() );
                    if (!newName.isEmpty())
                    {
                        setData(index.siblingAtColumn(1), newName);
                    }
                });
            ctxMenu.addAction(renameAct);
        }

        ctxMenu.exec(pw->mapToGlobal(p));
    }
}

void IQConstrainedSketchLayerListModel::removeLayer(QModelIndex index)
{
    if (index.isValid())
    {
        int r=index.row();
        auto i=layers_.begin();
        std::advance(i, r);

        (*sketchEditor_)->removeLayer(*i);

        update();
    }
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
        else if (role == Qt::BackgroundRole)
        {
            int r=index.row();
            if (r>=0 && r<layers_.size())
            {
                auto i=layers_.begin();
                std::advance(i, r);
                auto &c=(*sketchEditor_)->layerProperties(*i).color;
                if (c.n_elem==3)
                {
                    return QColor( 255.*c[0], 255.*c[1], 255.*c[2] );
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
