#include "iqremotefoldermodel.h"

#include "base/exception.h"
#include "base/translations.h"

#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QPointer>
#include <QStyle>

#include <algorithm>
#include <thread>




struct IQRemoteFolder
  : public QObject,
    public QList<IQRemoteFolder*>
{
  enum Kind { Folder, Loading, Error } kind;
  enum State { NotListed, Listing, Listed } state = NotListed;

  QString name; // folder name or message

  IQRemoteFolder(QObject* parent, Kind k, const QString& n)
    : QObject(parent), kind(k), name(n)
  {}

  IQRemoteFolder* parentFolder() const
  {
    return dynamic_cast<IQRemoteFolder*>(parent());
  }

  boost::filesystem::path folderPath() const
  {
    if (kind!=Folder)
    {
      // placeholder rows: path of the folder above
      if (auto *pf=parentFolder()) return pf->folderPath();
      return boost::filesystem::path();
    }

    boost::filesystem::path pp;
    if (auto* pf=parentFolder())
      pp=pf->folderPath();
    return ( pp / boost::filesystem::path(name.toStdString()) ).generic_path();
  }
};




IQRemoteFolder* IQRemoteFolderModel::node(const QModelIndex& index) const
{
  if (index.isValid())
    return static_cast<IQRemoteFolder*>(index.internalPointer());
  return baseFolder_;
}




QModelIndex IQRemoteFolderModel::indexOf(IQRemoteFolder* folder) const
{
  if (!folder || folder==baseFolder_)
    return QModelIndex();

  auto *pf = folder->parentFolder();
  int row = pf ? pf->indexOf(folder) : -1;
  if (row<0) return QModelIndex();
  return createIndex(row, 0, folder);
}




void IQRemoteFolderModel::removeChildren(IQRemoteFolder* folder)
{
  if (folder->isEmpty()) return;

  auto idx = indexOf(folder);
  beginRemoveRows(idx, 0, folder->size()-1);
  for (auto* c: *folder)
    delete c; // also cancels pending results for subfolders (QPointer guard)
  folder->clear();
  endRemoveRows();
}




void IQRemoteFolderModel::startListing(IQRemoteFolder* folder)
{
  if (folder->kind!=IQRemoteFolder::Folder || folder->state==IQRemoteFolder::Listing)
    return;

  removeChildren(folder);
  folder->state=IQRemoteFolder::Listing;

  // placeholder row
  auto idx = indexOf(folder);
  beginInsertRows(idx, 0, 0);
  folder->append(new IQRemoteFolder(folder, IQRemoteFolder::Loading, _("Loading...")));
  endInsertRows();

  auto path = folder->folderPath();
  auto qpath = QString::fromStdString(path.generic_string());
  pendingPaths_.append(qpath);
  Q_EMIT listingStateChanged(pendingPaths_.size(), qpath);

  // list in background. The guards are only dereferenced in the GUI thread.
  QPointer<IQRemoteFolderModel> modelGuard(this);
  QPointer<IQRemoteFolder> folderGuard(folder);
  auto server = server_;
  std::thread(
      [server, path, qpath, modelGuard, folderGuard]()
      {
        std::vector<boost::filesystem::path> subfolders;
        QString error;
        try
        {
          subfolders = server->listRemoteSubdirectories(path);
        }
        catch (const std::exception& e)
        {
          error = QString::fromStdString(e.what());
        }

        QMetaObject::invokeMethod(
            qApp,
            [modelGuard, folderGuard, qpath, subfolders, error]()
            {
              if (!modelGuard) return;
              modelGuard->pendingPaths_.removeOne(qpath);
              if (folderGuard)
                modelGuard->listingFinished(folderGuard.data(), subfolders, error);
              Q_EMIT modelGuard->listingStateChanged(
                  modelGuard->pendingPaths_.size(),
                  modelGuard->pendingPaths_.isEmpty() ?
                      QString() : modelGuard->pendingPaths_.last() );
            },
            Qt::QueuedConnection );
      }
  ).detach();
}




void IQRemoteFolderModel::listingFinished(
    IQRemoteFolder* folder,
    const std::vector<boost::filesystem::path>& subfolders,
    const QString& error )
{
  removeChildren(folder); // placeholder
  folder->state=IQRemoteFolder::Listed;

  auto idx = indexOf(folder);

  if (!error.isEmpty())
  {
    beginInsertRows(idx, 0, 0);
    folder->append(new IQRemoteFolder(folder, IQRemoteFolder::Error, error));
    endInsertRows();
    return;
  }

  QStringList names;
  for (const auto& sf: subfolders)
    names.append(QString::fromStdString(sf.string()));
  std::sort(names.begin(), names.end(),
            [](const QString& a, const QString& b)
            { return a.compare(b, Qt::CaseInsensitive)<0; } );

  if (!names.isEmpty())
  {
    beginInsertRows(idx, 0, names.size()-1);
    for (const auto& n: names)
      folder->append(new IQRemoteFolder(folder, IQRemoteFolder::Folder, n));
    endInsertRows();
  }
  else if (idx.isValid())
  {
    // no subfolders: update the expansion indicator
    Q_EMIT dataChanged(idx, idx);
  }
}




IQRemoteFolderModel::IQRemoteFolderModel(
    QObject *parent,
    insight::RemoteServerPtr server,
    const boost::filesystem::path& basePath )
  : QAbstractItemModel(parent),
    server_(server)
{
  // the base folder is the (invisible) root. It is listed upon the first fetchMore().
  baseFolder_=new IQRemoteFolder(
        this, IQRemoteFolder::Folder,
        QString::fromStdString(basePath.generic_path().string()) );
}




QVariant IQRemoteFolderModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (section==0 && orientation==Qt::Horizontal && role==Qt::DisplayRole)
  {
    return _("Folder Name");
  }
  return QVariant();
}




QModelIndex IQRemoteFolderModel::index(int row, int column, const QModelIndex &parent) const
{
  auto *f = node(parent);
  if (f && row>=0 && row<f->size() && column==0)
  {
    return createIndex( row, column, f->at(row) );
  }
  return QModelIndex();
}




QModelIndex IQRemoteFolderModel::parent(const QModelIndex &index) const
{
  if (index.isValid())
  {
    if (auto *pf = node(index)->parentFolder())
      return indexOf(pf);
  }
  return QModelIndex();
}




int IQRemoteFolderModel::rowCount(const QModelIndex &parent) const
{
  // never lists remote folders: this is done in fetchMore
  if (parent.column()>0) return 0;
  return node(parent)->size();
}




int IQRemoteFolderModel::columnCount(const QModelIndex &/*parent*/) const
{
  return 1;
}




bool IQRemoteFolderModel::hasChildren(const QModelIndex &parent) const
{
  auto *f = node(parent);
  if (f->kind!=IQRemoteFolder::Folder) return false;
  if (f->state==IQRemoteFolder::NotListed) return true; // probably, show expander
  return !f->isEmpty();
}




bool IQRemoteFolderModel::canFetchMore(const QModelIndex &parent) const
{
  auto *f = node(parent);
  return f->kind==IQRemoteFolder::Folder && f->state==IQRemoteFolder::NotListed;
}




void IQRemoteFolderModel::fetchMore(const QModelIndex &parent)
{
  if (canFetchMore(parent))
    startListing(node(parent));
}




QVariant IQRemoteFolderModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  auto *f = node(index);

  switch (role)
  {
  case Qt::DisplayRole:
    if (f->kind==IQRemoteFolder::Error)
      return QString(_("could not list: %1")).arg(f->name);
    return f->name;

  case Qt::ToolTipRole:
    if (f->kind==IQRemoteFolder::Error)
      return f->name;
    break;

  case Qt::DecorationRole:
    if (f->kind==IQRemoteFolder::Folder)
      return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
    if (f->kind==IQRemoteFolder::Error)
      return QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning);
    break;

  case Qt::FontRole:
    if (f->kind!=IQRemoteFolder::Folder)
    {
      QFont fnt;
      fnt.setItalic(true);
      return fnt;
    }
    break;

  case Qt::ForegroundRole:
    if (f->kind==IQRemoteFolder::Loading)
      return QApplication::palette().brush(QPalette::Disabled, QPalette::Text);
    break;
  }

  return QVariant();
}




Qt::ItemFlags IQRemoteFolderModel::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return Qt::NoItemFlags;

  if (node(index)->kind!=IQRemoteFolder::Folder)
    return Qt::ItemIsEnabled; // placeholder rows are not selectable

  return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
}




boost::filesystem::path IQRemoteFolderModel::folder(QModelIndex i) const
{
  return node(i)->folderPath();
}




void IQRemoteFolderModel::refreshBelow(QModelIndex i)
{
  auto *f = node(i);
  if (f->kind!=IQRemoteFolder::Folder) f=f->parentFolder();
  if (!f || f->state==IQRemoteFolder::Listing) return; // result will be up to date

  f->state=IQRemoteFolder::NotListed;
  startListing(f);
}




bool IQRemoteFolderModel::isBusy() const
{
  return !pendingPaths_.isEmpty();
}
