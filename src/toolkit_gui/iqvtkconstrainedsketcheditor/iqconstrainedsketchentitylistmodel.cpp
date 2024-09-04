#include "iqconstrainedsketchentitylistmodel.h"

#include "base/linearalgebra.h"
#include "cadfeatures/line.h"
#include <qnamespace.h>

IQConstrainedSketchEntityListModel::IQConstrainedSketchEntityListModel(
    IQVTKConstrainedSketchEditor *sk, QObject *parent )
    : QAbstractTableModel{parent},
    sketchEditor_(sk)
{}




int IQConstrainedSketchEntityListModel::rowCount(const QModelIndex &parent) const
{
    return geometry_.size();
}




int IQConstrainedSketchEntityListModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}




Qt::ItemFlags IQConstrainedSketchEntityListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    auto f=QAbstractTableModel::flags(index);

    switch (index.column())
    {
    // case 0:
    //     f|=Qt::ItemIsUserCheckable;
    // case 1:
    //     f|=Qt::ItemIsEditable;
    }

    return f;
}




QVariant IQConstrainedSketchEntityListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role==Qt::DisplayRole)
    {
        if (orientation==Qt::Horizontal)
        {
            switch (section)
            {
            case 0:
                return "ID";
            case 1:
                return "Type";
            case 2:
                return "Class";
            }
        }
    }
    return QVariant();
}




QVariant IQConstrainedSketchEntityListModel::data(
    const QModelIndex &index,
    int role ) const
{
    if (index.isValid())
    {
        if (role==Qt::DisplayRole || role==Qt::EditRole
            || role==Qt::ToolTipRole || role==Qt::BackgroundRole)
        {
            int r=index.row();
            if (r>=0 && r<geometry_.size())
            {
                auto i=geometry_.begin();
                std::advance(i, r);
                if (role==Qt::ToolTipRole)
                {
                    if (auto *p = dynamic_cast<insight::cad::SketchPoint*>(i->second))
                    {
                        return QString("x=%1\ny=%2")
                            .arg(p->getDoFValue(0))
                            .arg(p->getDoFValue(1))
                            ;
                    }
                    if (auto *l = dynamic_cast<insight::cad::Line*>(i->second))
                    {
                        auto p0=l->start();
                        auto p1=l->end();
                        auto L=arma::norm(p1->value() - p0->value(), 2);
                        return QString("L=%1").arg(L);
                    }
                    else
                        return QString();
                }
                else if (role==Qt::BackgroundRole)
                {
                    if (i->second->nConstraints())
                    {
                        bool fulfilled=true;
                        for (int j=0; j<i->second->nConstraints(); ++j)
                        {
                            fulfilled =
                                fulfilled
                                &&
                                (i->second->getConstraintError(j) < insight::SMALL)
                                ;
                        }
                        if (fulfilled)
                            return QBrush(Qt::green);
                        else
                            return QBrush(Qt::red);
                    }
                    else
                        return QVariant();
                }
                else
                {
                    switch (index.column())
                    {
                    case 0:
                        return QString::number(i->first);
                    case 1:
                    {
                        return QString::fromStdString(i->second->type());
                    }
                    case 2:
                        if (dynamic_cast<insight::cad::PostprocAction*>(i->second))
                        {
                            return "Dimension";
                        }
                        else if (i->second->nConstraints()>0)
                        {
                            return "Constraint";
                        }
                        else
                        {
                            return "Geometry";
                        }
                    }
                }
            }
        }
        // else if (role==Qt::CheckStateRole)
        // {
        //     int r=index.row();
        //     if (r>=0 && r<sk.size())
        //     {
        //         auto i=sk.begin();
        //         std::advance(i, r);
        //         switch (index.column())
        //         {
        //         case 0:
        //             return sketchEditor_->layerIsVisible(*i) ?
        //                        Qt::Checked : Qt::Unchecked;
        //         }
        //     }
        // }
    }
    return QVariant();
}




bool IQConstrainedSketchEntityListModel::setData(
    const QModelIndex &index,
    const QVariant &value,
    int role )
{
    // if (index.isValid())
    // {
    //     if (role==Qt::EditRole)
    //     {
    //         int r=index.row();
    //         if (r>=0 && r<layers_.size())
    //         {
    //             auto i=layers_.begin();
    //             std::advance(i, r);
    //             switch (index.column())
    //             {
    //             case 1:
    //                 Q_EMIT renameLayer(
    //                     *i,
    //                     value.toString().toStdString() );
    //                 return true;
    //             }
    //         }
    //     }
    //     else if (role==Qt::CheckStateRole)
    //     {
    //         int r=index.row();
    //         if (r>=0 && r<layers_.size())
    //         {
    //             auto i=layers_.begin();
    //             std::advance(i, r);
    //             switch (index.column())
    //             {
    //             case 0:
    //                 auto cs = value.value<Qt::CheckState>();
    //                 if (cs==Qt::Checked)
    //                     Q_EMIT showLayer(*i);
    //                 else if (cs==Qt::Unchecked)
    //                     Q_EMIT hideLayer(*i);
    //                 return true;
    //             }
    //         }
    //     }
    // }
    return false;
}




void IQConstrainedSketchEntityListModel::update()
{
    auto oldGeom=geometry_;

    geometry_.clear();
    std::transform(
        (*sketchEditor_)->begin(),
        (*sketchEditor_)->end(),
        std::inserter(geometry_, geometry_.begin()),
        [](const insight::cad::ConstrainedSketch::value_type& sg)
        {
            return decltype(geometry_)::value_type{sg.first, sg.second.get()};
        }
        );

    // compare
    //  remove vanished
    if (oldGeom.size())
    {
        for (auto i=oldGeom.rbegin(); i!=oldGeom.rend(); ++i) // start from end
        {
            auto ii=i; ii++;
            int r=std::distance(oldGeom.begin(), ii.base());
            if (geometry_.count(i->first)<1) // not in new list
            {
                beginRemoveRows(QModelIndex(), r, r);
                endRemoveRows();
            }
        }
    }
    //  add new
    for (auto i=geometry_.begin(); i!=geometry_.end(); ++i) // start from end
    {
        int r=std::distance(geometry_.begin(), i);
        if (oldGeom.count(i->first)<1) // not in old list
        {
            beginInsertRows(QModelIndex(), r, r);
            endInsertRows();
        }
    }
}
