#include "iqexternalprogramsmodel.h"

IQExternalProgramsModel::IQExternalProgramsModel(QObject *parent)
    : QAbstractItemModel(parent),
      externalPrograms_(insight::ExternalPrograms::globalInstance())
{
}

QVariant IQExternalProgramsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation==Qt::Horizontal && role==Qt::DisplayRole)
    {
      switch (section)
      {
        case 0: return "Executable name";
        case 1: return "Path to Executable";
      }
    }
    return QVariant();
}

QModelIndex IQExternalProgramsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
      if (row>=0 && row<externalPrograms_.size())
      {
        return createIndex(row, column, nullptr);
      }
    }
    return QModelIndex();
}

QModelIndex IQExternalProgramsModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

int IQExternalProgramsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
      return 0;
    return externalPrograms_.size();
}

int IQExternalProgramsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
      return 0;
    return 2;
}

QVariant IQExternalProgramsModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid())
    {
        auto i=externalPrograms_.begin();
        std::advance(i, index.row());

        if (role==Qt::DisplayRole)
        {
            switch (index.column())
            {
                case 0: return QString::fromStdString( i->first );
                case 1: return QString::fromStdString( i->second.string() );
            }
        }
    }

    return QVariant();
}

insight::ExternalPrograms::iterator IQExternalProgramsModel::externalProgram(const QModelIndex &index)
{
    if (index.isValid())
    {
        auto i=externalPrograms_.begin();
        std::advance(i, index.row());
        return i;
    }
    return externalPrograms_.end();
}

void IQExternalProgramsModel::resetProgramPath(const QModelIndex &index, const QString &newPath)
{
    auto ep = externalProgram(index);
    if (ep!=externalPrograms_.end())
    {
        ep->second = newPath.toStdString();
        Q_EMIT dataChanged(index, index);
    }
}

const insight::ExternalPrograms &IQExternalProgramsModel::externalPrograms() const
{
    return externalPrograms_;
}
