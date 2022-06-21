#include "iqcaditemmodel.h"

IQCADItemModel::IQCADItemModel(insight::cad::ModelPtr model, QObject* parent)
  : QAbstractItemModel(parent),
    model_(model)
{
    model_->checkForBuildDuringAccess();
}


QVariant IQCADItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role==Qt::DisplayRole)
    {
      if (orientation==Qt::Horizontal)
      {
        if (section == 0)
            return QVariant("Visible");
        else if (section == 1)
            return QVariant("Name");
        else if (section == 2)
            return QVariant("Value");
      }
    }
    return QVariant();
}

QModelIndex IQCADItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        // top level
        if (row>=0 && row<CADModelSection::numberOf)
        {
            return createIndex(row, column, quintptr(INT_MAX));
        }
    }
    else
    {
        switch (parent.row())
        {
        case CADModelSection::scalarVariable:
            if (row>=0 && row<model_->scalars().size())
            {
                return createIndex(row, column, quintptr(parent.row()));
            }
            break;
        case CADModelSection::vectorVariable:
            if (row>=0 && row<model_->points().size())
            {
                return createIndex(row, column, quintptr(parent.row()));
            }
            break;
        case CADModelSection::datum:
            if (row>=0 && row<model_->datums().size())
            {
                return createIndex(row, column, quintptr(parent.row()));
            }
            break;
        case CADModelSection::feature:
            if (row>=0 && row<model_->modelsteps().size())
            {
                return createIndex(row, column, quintptr(parent.row()));
            }
            break;
        }
    }

    return QModelIndex();
}

QModelIndex IQCADItemModel::parent(const QModelIndex &index) const
{
    if (index.internalId()>=0 && index.internalId()<CADModelSection::numberOf)
    {
        return createIndex(index.internalId(), 0, quintptr(INT_MAX));
    }
    return QModelIndex();
}

int IQCADItemModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        return CADModelSection::numberOf;
    }
    else if (!parent.parent().isValid())
    {
        switch (parent.row())
        {
        case CADModelSection::scalarVariable:
            return model_->scalars().size();
            break;
        case CADModelSection::vectorVariable:
            return model_->points().size();
            break;
        case CADModelSection::datum:
            return model_->datums().size();
            break;
        case CADModelSection::feature:
            return model_->modelsteps().size();
            break;
        }
    }
    return 0;
}

int IQCADItemModel::columnCount(const QModelIndex &parent) const
{
    return 4;
}

QVariant IQCADItemModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid())
    {
        if (role==Qt::DisplayRole)
        {
            if (!index.parent().isValid() && index.column()==1)
            {
                switch (index.row())
                {
                case CADModelSection::scalarVariable:
                    return "Scalars";
                    break;
                case CADModelSection::vectorVariable:
                    return "Vectors";
                    break;
                case CADModelSection::datum:
                    return "Datums";
                    break;
                case CADModelSection::feature:
                    return "Features";
                    break;
                case CADModelSection::component:
                    return "Components";
                    break;
                }
            }
            else if (index.internalId()>=0 && index.internalId()<CADModelSection::numberOf)
            {
                switch (index.internalId())
                {
                case CADModelSection::scalarVariable:
                    {
                        auto scalars = model_->scalars();
                        auto i = scalars.begin();
                        std::advance(i, index.row());

                        if (index.column()==1)
                            return QString::fromStdString(i->first);
                        else if (index.column()==2)
                            return QString::number(i->second->value());
                    }
                    break;
                case CADModelSection::vectorVariable:
                    {
                        auto points = model_->points();
                        auto i = points.begin();
                        std::advance(i, index.row());

                        if (index.column()==1)
                            return QString::fromStdString(i->first);
                        else if (index.column()==2)
                        {
                            auto v=i->second->value();
                            return QString("[%1 %2 %3]").arg(v[0]).arg(v[1]).arg(v[2]);
                        }
                    }
                    break;
                case CADModelSection::datum:
                    {
                        auto datums = model_->datums();
                        auto i = datums.begin();
                        std::advance(i, index.row());
                        if (index.column()==1)
                        {
                            return QString::fromStdString(i->first);
                        }
                        else if (index.column()==3)
                        {
                            QVariant r;
                            r.setValue(i->second);
                            return r;
                        }
                    }
                    break;
                case CADModelSection::feature:
                    {
                        auto features = model_->modelsteps();
                        auto i = features.begin();
                        std::advance(i, index.row());
                        if (index.column()==1)
                        {
                            return QString::fromStdString(i->first);
                        }
                        else if (index.column()==2)
                        {
                            return QString::fromStdString(i->second->type());
                        }
                        else if (index.column()==3)
                        {
                            QVariant r;
                            r.setValue(i->second);
                            return r;
                        }
                    }
                    break;
                case CADModelSection::component:
                    return "Components";
                    break;
                }
            }
        }
        else if (role == Qt::CheckStateRole)
        {
            if (index.internalId()>=0 && index.internalId()<CADModelSection::numberOf)
            {
                switch (index.internalId())
                {
                    case CADModelSection::datum:
                        {
                            auto datums = model_->datums();
                            auto i = datums.begin();
                            std::advance(i, index.row());
                            if (index.column()==0)
                            {
                                return Qt::CheckState( datumVisibility_[i->second]?Qt::Checked:Qt::Unchecked );
                            }
                        }
                        break;
                    case CADModelSection::feature:
                        {
                            auto features = model_->modelsteps();
                            auto i = features.begin();
                            std::advance(i, index.row());
                            if (index.column()==0)
                            {
                                return Qt::CheckState( featureVisibility_[i->second]?Qt::Checked:Qt::Unchecked );
                            }
                        }
                }
            }

        }
    }

    return QVariant();
}




Qt::ItemFlags IQCADItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    if ( index.column() == 0 && (
             index.internalId()==CADModelSection::datum
             || index.internalId()==CADModelSection::feature
             ) )
    {
        flags |= Qt::ItemIsUserCheckable;
    }

    return flags;
}




bool IQCADItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole)
    {
        if ( index.column() == 0 )
        {
            if (index.internalId()==CADModelSection::datum)
            {
                auto datums = model_->datums();
                auto i=datums.begin();
                std::advance(i, index.row());
                datumVisibility_[i->second] =
                        ( value.value<Qt::CheckState>()==Qt::Checked );
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            else if (index.internalId()==CADModelSection::feature)
            {
                auto features = model_->modelsteps();
                auto i=features.begin();
                std::advance(i, index.row());
                featureVisibility_[i->second] =
                        ( value.value<Qt::CheckState>()==Qt::Checked );
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
        }
    }

    return false;
}
