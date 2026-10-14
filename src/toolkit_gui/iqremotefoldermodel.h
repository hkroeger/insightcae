#ifndef IQREMOTEFOLDERMODEL_H
#define IQREMOTEFOLDERMODEL_H

#include "toolkit_gui_export.h"

#include <QAbstractItemModel>
#include <QStringList>
#include "base/remoteserver.h"

struct IQRemoteFolder;




/**
 * @brief The IQRemoteFolderModel class
 * tree of the folders on a remote server.
 * The folders are listed lazily (when expanded) and in background,
 * so that the GUI remains responsive. While a folder is being listed,
 * a placeholder row is displayed below it.
 * Optionally, the files are displayed as well.
 */
class TOOLKIT_GUI_EXPORT IQRemoteFolderModel : public QAbstractItemModel
{
  Q_OBJECT

  insight::RemoteServerPtr server_;
  bool showFiles_;
  IQRemoteFolder* baseFolder_;
  QStringList pendingPaths_; // folders being listed

  IQRemoteFolder* node(const QModelIndex& index) const;
  QModelIndex indexOf(IQRemoteFolder* folder) const;

  void startListing(IQRemoteFolder* folder);
  void listingFinished(
      IQRemoteFolder* folder,
      const std::vector<boost::filesystem::path>& subfolders,
      const std::vector<boost::filesystem::path>& files,
      const QString& error );

  void removeChildren(IQRemoteFolder* folder);

public:
  explicit IQRemoteFolderModel(
      QObject *parent,
      insight::RemoteServerPtr server,
      const boost::filesystem::path& basePath,
      bool showFiles = false );

  // Header:
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  // Basic functionality:
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &index) const override;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  // Fetch data dynamically:
  bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
  bool canFetchMore(const QModelIndex &parent) const override;
  void fetchMore(const QModelIndex &parent) override;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  /**
   * @brief folder
   * @return
   * the remote path of the folder at the index.
   * For placeholder rows, the path of the folder above.
   * For an invalid index, the base path.
   */
  boost::filesystem::path folder(QModelIndex i) const;

  /**
   * @brief relativePath
   * @return
   * the path of the file or folder at the index, relative to the base path.
   * Empty for placeholder rows and for an invalid index.
   */
  boost::filesystem::path relativePath(QModelIndex i) const;

  /**
   * @brief isEntry
   * @return
   * true, if the index refers to a file or folder (not a placeholder row)
   */
  bool isEntry(QModelIndex i) const;

  /**
   * @brief refreshBelow
   * list the folder at the index again (in background)
   */
  void refreshBelow(QModelIndex i);

  bool isBusy() const;

Q_SIGNALS:
  /**
   * @brief listingStateChanged
   * emitted, when a listing starts or ends
   * @param pendingListings
   * number of listings in progress
   * @param lastStartedPath
   * path of the folder, whose listing was started most recently
   * (empty, if none is pending)
   */
  void listingStateChanged(int pendingListings, const QString& lastStartedPath);
};

#endif // IQREMOTEFOLDERMODEL_H
