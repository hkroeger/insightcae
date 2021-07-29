#include "iqremotefoldermodel.h"

#include <QDebug>


template<class TT>
TT* dynamic_qobject_cast(void* ptr)
{
  if (QObject *o=static_cast<QObject*>(ptr))
  {
    if (TT* to = dynamic_cast<TT*>(o))
    {
      return to;
    }
  }
  return nullptr;
}



struct IQRemoteFolder
  : public QObject,
    public QList<IQRemoteFolder*>
{
  insight::RemoteServerPtr server_;
  QString folderName;
  bool populated=false;

  IQRemoteFolder* parentFolder()
  {
    return dynamic_cast<IQRemoteFolder*>(parent());
  }

  boost::filesystem::path parentFolderPath()
  {
    if (auto* pf=parentFolder())
    {
      return pf->folderPath();
    }
    return boost::filesystem::path();
  }

  boost::filesystem::path folderPath()
  {
    return (
        parentFolderPath() /
        boost::filesystem::path(folderName.toStdString())
          ).generic_path();
  }

  IQRemoteFolder(QObject* parent, insight::RemoteServerPtr server, const QString& name)
    : QObject(parent),
      server_(server),
      folderName(name)
  {}

  void populate()
  {
    insight::CurrentExceptionContext ex("populate remote folder node");

    auto sfs = server_->listRemoteSubdirectories(folderPath());
    for (const auto& sf: sfs)
    {
      insight::dbg()<<sf<<std::endl;
      auto* entry = new IQRemoteFolder(this, server_, QString::fromStdString(sf.string()) );
      this->append( entry );
    }
    populated=true;
  }
};





IQRemoteFolderModel::IQRemoteFolderModel(
    QObject *parent,
    insight::RemoteServerPtr server,
    const boost::filesystem::path& basePath )
  : QAbstractItemModel(parent)
{
  baseFolder_=new IQRemoteFolder(
        parent,
        server,
        QString::fromStdString(basePath.generic_path().string()) );

  baseFolder_->populate();
}

QVariant IQRemoteFolderModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (section==0 && orientation==Qt::Horizontal && role==Qt::DisplayRole)
  {
    return "Folder Name";

  }
  return QVariant();
}

QModelIndex IQRemoteFolderModel::index(int row, int column, const QModelIndex &parent) const
{  
//  qDebug()<<"index "<<row<<" "<<column<<" "<<parent;
  IQRemoteFolder* iqf;
  if (parent.isValid())
    iqf = dynamic_qobject_cast<IQRemoteFolder>(parent.internalPointer());
  else
    iqf = baseFolder_;

//  qDebug()<<iqf;
  if (iqf)
  {
    if (row>=0 && row<iqf->size() && column>=0 && column<1)
    {
      return createIndex( row, column, iqf->QList<IQRemoteFolder*>::operator[](row) );
    }
  }
  return QModelIndex();
}

QModelIndex IQRemoteFolderModel::parent(const QModelIndex &index) const
{
//  qDebug()<<"parent "<<index;
  if ( index.isValid() && index.column()>=0 && index.column()<1 )
  {
    if (auto* iqf = dynamic_qobject_cast<IQRemoteFolder>(index.internalPointer()))
    {
//      qDebug()<<iqf;
      if (auto* pf = iqf->parentFolder())
      {
//        qDebug()<<pf;
        // find row of parent
        if (auto* ppf = pf->parentFolder())
        {
//          qDebug()<<ppf;
          int prow = ppf->QList<IQRemoteFolder*>::indexOf(pf);
          return createIndex(prow, index.column(), pf);
        }
      }
    }
  }
  return QModelIndex();
}

int IQRemoteFolderModel::rowCount(const QModelIndex &parent) const
{
//  qDebug()<<"rowCount "<<parent;

  IQRemoteFolder* iqf;
  if (parent.isValid())
    iqf = dynamic_qobject_cast<IQRemoteFolder>(parent.internalPointer());
  else
    iqf=baseFolder_;

  if (iqf)
  {
      if (!iqf->populated)
      {
        iqf->populate();
      }
//      qDebug()<<iqf->size();
      return iqf->size();
  }

  return 0;
}

int IQRemoteFolderModel::columnCount(const QModelIndex &/*parent*/) const
{
  return 1;
}


//bool IQRemoteFolderModel::hasChildren(const QModelIndex &parent) const
//{
//  if (parent.isValid())
//  {
//    if (auto* iqf = dynamic_qobject_cast<IQRemoteFolder>(parent.internalPointer()))
//    {
//      if (iqf->populated)
//      {
//        return iqf->size();
//      }
//      else
//        return 1;
//    }
//  }
//}

//bool IQRemoteFolderModel::canFetchMore(const QModelIndex &parent) const
//{
//  // FIXME: Implement me!
//  return false;
//}

//void IQRemoteFolderModel::fetchMore(const QModelIndex &parent)
//{
//  // FIXME: Implement me!
//}


QVariant IQRemoteFolderModel::data(const QModelIndex &index, int role) const
{
//  qDebug()<<"data "<<index<<" "<<role;
  if (!index.isValid())
    return QVariant();

  if (role==Qt::DisplayRole)
  {
    if (auto* iqf = dynamic_qobject_cast<IQRemoteFolder>(index.internalPointer()))
    {
      return iqf->folderName;
    }
  }

  return QVariant();
}

boost::filesystem::path IQRemoteFolderModel::folder(QModelIndex i) const
{
  if (i.isValid())
  {
    if (auto * selfol = dynamic_qobject_cast<IQRemoteFolder>(i.internalPointer()))
    {
      return selfol->folderPath();
    }
  }
  return "";
}

void IQRemoteFolderModel::refreshBelow(QModelIndex i)
{
  int n=rowCount(i);
  if (auto *p = dynamic_qobject_cast<IQRemoteFolder>(i.internalPointer()))
  {
    beginRemoveRows(i, 0, n-1);
    for (auto* n: *p)
    {
      delete n;
    }
    p->clear();
    endRemoveRows();

    beginInsertRows(i, 0, n);
    p->populate();
    endInsertRows();
  }
}
