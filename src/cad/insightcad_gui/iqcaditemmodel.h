#ifndef IQCADMODELCONTAINER_H
#define IQCADMODELCONTAINER_H

#include "insightcad_gui_export.h"
#include "iscadmetatyperegistrator.h"

#include "cadmodel.h"

#include <QAbstractItemModel>


class INSIGHTCAD_GUI_EXPORT IQCADItemModel
    : public QAbstractItemModel
{
  Q_OBJECT

  insight::cad::ModelPtr model_;

  mutable std::map<insight::cad::DatumPtr, bool> datumVisibility_;
  mutable std::map<insight::cad::FeaturePtr, bool> featureVisibility_;

public:
  enum CADModelSection {
      scalarVariable = 0,
      vectorVariable = 1,
      datum = 2,
      feature = 3,
      component = 4,
      numberOf =5
  };

public:
  IQCADItemModel(insight::cad::ModelPtr model, QObject* parent=nullptr);

  // Header:
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  // Basic functionality:
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &index) const override;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  Qt::ItemFlags flags(const QModelIndex &index) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;

};

#endif // IQCADMODELCONTAINER_H
