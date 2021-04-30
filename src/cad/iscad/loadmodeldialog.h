#ifndef LOADMODELDIALOG_H
#define LOADMODELDIALOG_H

#include <QDialog>

#include <QAbstractItemModel>

#include "base/boost_include.h"

namespace Ui {
class LoadModelDialog;
}

class LibraryLocation;

class LibraryModel
    : public QObject
{
  Q_OBJECT

public:
  std::string modelName_;
  LibraryLocation* getParent() const;

  LibraryModel(QObject* parent);
};




class LibraryLocation
    : public QObject
{
  Q_OBJECT

public:
  boost::filesystem::path directory_;
  QList<LibraryModel*> models_;

  LibraryLocation(QObject* parent);
};




class AvailableModelsModel
    : public QAbstractItemModel
{
public:
  QList<LibraryLocation*> locations_;

  AvailableModelsModel();

  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &index) const  override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orient, int role) const override;
};




class LoadModelDialog : public QDialog
{
  Q_OBJECT

  AvailableModelsModel avm_;

public:
  explicit LoadModelDialog(QWidget *parent = nullptr);
  ~LoadModelDialog();

  std::string expression() const;

private:
  Ui::LoadModelDialog *ui;
};

#endif // LOADMODELDIALOG_H
