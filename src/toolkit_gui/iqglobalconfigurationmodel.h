#ifndef IQGLOBALCONFIGURATIONMODEL_H
#define IQGLOBALCONFIGURATIONMODEL_H


#include <QAbstractItemModel>
#include <QApplication>
#include <QFont>

#include "base/globalconfiguration.h"

template<class GlobalConfigurationClass>
class IQGlobalConfigurationModel : public QAbstractItemModel
{

protected:
    GlobalConfigurationClass config_;

public:
    explicit IQGlobalConfigurationModel(QObject *parent = nullptr)
        : QAbstractItemModel(parent),
          config_(GlobalConfigurationClass::globalInstance())
    {}

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation==Qt::Horizontal && role==Qt::DisplayRole)
        {
          switch (section)
          {
            case 0: return "Label";
          }
        }
        return QVariant();
    }

    // Basic functionality:
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override
    {
        if (!parent.isValid())
        {
          if (row>=0 && row<config_.size())
          {
            return createIndex(row, column, nullptr);
          }
        }
        return QModelIndex();
    }


    QModelIndex parent(const QModelIndex &index) const override
    {
        return QModelIndex();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return config_.size();
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return 1;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (index.isValid())
        {
            auto i=config_.begin();
            std::advance(i, index.row());

            if (role==Qt::DisplayRole)
            {
                switch (index.column())
                {
                    case 0: return QString::fromStdString( i->first );
                }
            }
        }

        return QVariant();
    }

    void addItem(const typename GlobalConfigurationClass::mapped_type& item)
    {
        std::string key = item.label();

        int inew=insight::predictInsertionLocation(config_, key);
        beginInsertRows(QModelIndex(), inew, inew);

        config_.insertItem(item);

        endInsertRows();
    }

    const GlobalConfigurationClass& configuration() const
    {
        return config_;
    }

private:
};




template<class GlobalConfigurationClass>
class IQGlobalConfigurationWithDefaultModel
        : public IQGlobalConfigurationModel<GlobalConfigurationClass>
{
public:
    explicit IQGlobalConfigurationWithDefaultModel(QObject *parent = nullptr)
        : IQGlobalConfigurationModel<GlobalConfigurationClass>(parent)
    {}


    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (index.isValid())
        {
            if ( role==Qt::FontRole)
            {
                auto i=this->config_.begin();
                std::advance(i, index.row());
                if ( i==this->config_.defaultItemIterator() )
                {
                    QFont font(QApplication::font());
                    font.setBold(true);
                    return font;
                }
            }
            else
            {
                return IQGlobalConfigurationModel<GlobalConfigurationClass>::data(
                            index, role );
            }

        }
        return QVariant();
    }


    void setDefaultItem(const QModelIndex &index)
    {
        if (index.isValid())
        {
            auto iold = this->config_.defaultItemIterator();
            auto indexold = this->index( std::distance(this->config_.cbegin(), iold), 0, QModelIndex());

            auto i=this->config_.begin();
            std::advance(i, index.row());
            this->config_.setDefaultItem(i->first);

            this->dataChanged(index, index);
            this->dataChanged(indexold, indexold);
        }
    }

    void removeItem(const QModelIndex &index)
    {
        if (index.isValid())
        {
            auto i=this->config_.begin();
            std::advance(i, index.row());

            insight::assertion(
                        i!=this->config_.defaultItemIterator(),
                        "cannot remove default template! Please set a new default template first.");

            insight::assertion(
                        i->first!="builtin",
                        "cannot remove builtin template!");

            this->beginRemoveRows(QModelIndex(), index.row(), index.row());
            this->config_.erase(i);
            this->endRemoveRows();
        }
    }
};

#endif // IQGLOBALCONFIGURATIONMODEL_H
