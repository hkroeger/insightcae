#ifndef IQREMOTEFOLDERMODEL_H
#define IQREMOTEFOLDERMODEL_H

#include "toolkit_gui_export.h"

#include <QAbstractItemModel>
#include "base/remoteserver.h"

struct IQRemoteFolder;




class TOOLKIT_GUI_EXPORT IQRemoteFolderModel : public QAbstractItemModel
{
  Q_OBJECT

  IQRemoteFolder* baseFolder_;

public:
  explicit IQRemoteFolderModel(
      QObject *parent,
      insight::RemoteServerPtr server,
      const boost::filesystem::path& basePath );

  // Header:
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  // Basic functionality:
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &index) const override;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;


//  // Fetch data dynamically:
//  bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

//  bool canFetchMore(const QModelIndex &parent) const override;
//  void fetchMore(const QModelIndex &parent) override;



  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  boost::filesystem::path folder(QModelIndex i) const;

  void refreshBelow(QModelIndex i);

private:
};

#endif // IQREMOTEFOLDERMODEL_H
