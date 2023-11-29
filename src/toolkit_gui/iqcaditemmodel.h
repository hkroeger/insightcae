#ifndef IQCADMODELCONTAINER_H
#define IQCADMODELCONTAINER_H

#include "toolkit_gui_export.h"
#include "iscadmetatyperegistrator.h"

#include "cadmodel.h"

#include "base/tools.h"
#include "base/vtkrendering.h"

#include <QAbstractItemModel>
#include <QDebug>
#include <QApplication>
#include <QColor>
#include <QMenu>


class IQCADModel3DViewer;
class IQParameterSetModel;

class TOOLKIT_GUI_EXPORT IQCADItemModel
    : public QAbstractItemModel
{
  Q_OBJECT

    /**
   * @brief model_
   * The underlying CAD model.
   * Should be modified only through access functions of this class to ensure consistency of the views.
   */
  insight::cad::ModelPtr model_;

  QAbstractItemModel* associatedParameterSetModel_;

  mutable std::map<std::string, bool>
        pointVisibility_,
        vectorVisibility_,
        postprocVisibility_,
        datumVisibility_;

  struct FeatureVisibility
  {
      bool visible;
      double opacity;
      QColor color;
      insight::DatasetRepresentation representation;
      std::vector<std::string> assocParamPaths;

      FeatureVisibility();
      FeatureVisibility(const insight::cad::FeatureVisualizationStyle& fvs);
  };
  mutable std::map<std::string, FeatureVisibility> featureVisibility_;

  struct DatasetVisibility
  {
      bool visible = true;

      /**
       * @brief fieldName
       * empty string: first field
       */
      std::string fieldName = "";

      insight::FieldSupport fieldSupport = insight::OnPoint;

      insight::DatasetRepresentation representation = insight::Surface;

      /**
       * @brief fieldComponent
       * -1 for mag
       */
      int fieldComponent = -1;

      boost::optional<double> minVal;
      boost::optional<double> maxVal;
  };
  mutable std::map<std::string, DatasetVisibility> datasetVisibility_;

  std::set<std::string>
    staticFeatures_;

public:
  enum CADModelSection {
      scalarVariable = 0,
      pointVariable = 1,
      vectorVariable = 2,
      datum = 3,
      feature = 4,
      postproc = 5,
      dataset = 6,
      numberOf =7
  };

  static const int
      visibilityCol=0,
      labelCol=1,
      valueCol=2,

      datasetFieldNameCol = 3,
      datasetPointCellCol = 4,
      datasetComponentCol = 5,
      datasetMinCol = 6,
      datasetMaxCol = 7,
      datasetRepresentationCol = 8,

      entityColorCol = 3,
      entityOpacityCol = 4,
      entityRepresentationCol = 5, // insight::DatasetRepresentation
      entityCol=99,

      assocParamPathsCol=98;

private:
  template<class E>
  void addEntity(
          const std::string& name,
          E value,
          std::function<std::map<std::string,E>(void)> efunc,
          std::function<QModelIndex(const std::string&)> eidxfunc,
          std::function<void(const std::string&,E)> modeladdfunc,
          CADModelSection esect
          )
  {
      auto s=efunc();
      auto ss = s.find(name);
      if (ss!=s.end())
      {
          auto i = eidxfunc(name);
          auto ie = index(i.row(), entityCol, i.parent());
          if (ss->second==value)
          {
              // is present and the same
              Q_EMIT dataChanged(ie, ie, {Qt::EditRole});
          }
          else
          {
              // replace
              modeladdfunc(name, value);
              Q_EMIT dataChanged(ie, ie, {Qt::EditRole});
          }
      }
      else
      {
          // insert new
          auto newrow = insight::predictInsertionLocation(s, name);
          beginInsertRows(index(esect, 0), newrow, newrow);
          modeladdfunc(name, value);
          endInsertRows();
      }
  }

  template<class E>
  void removeEntity(
          const std::string& name,
          std::function<QModelIndex(const std::string&)> eidxfunc,
          std::function<void(const std::string&)> modelremovefunc,
          CADModelSection esect
          )
  {
      auto idx = eidxfunc(name);
      if (idx.isValid())
      {
          beginRemoveRows(index(esect, 0), idx.row(), idx.row());
          modelremovefunc(name);
          endRemoveRows();
      }
  }


  template<class E>
  QModelIndex sectionIndex(
          const std::string& name,
          std::function<std::map<std::string,E>(void)> getList,
          CADModelSection entitySection
          ) const
  {
      auto list = getList();
      auto si = list.find(name);
      if (si == list.end())
      {
          return QModelIndex();
      }
      else
      {
          auto row = std::distance(list.begin(), si);
          return index(row, 0, index(entitySection, 0) );
      }
  }

  void addSymbolsToSubmenu(
          const QString& name,
          QMenu *menu,
          insight::cad::FeaturePtr feat,
          bool *someSubMenu = nullptr,
          bool *someHoverDisplay = nullptr );

public:
  IQCADItemModel(insight::cad::ModelPtr model=insight::cad::ModelPtr(), QObject* parent=nullptr);
  virtual ~IQCADItemModel();

  /**
   * abstract item model overrides
   */
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


  const insight::cad::ModelPtr model() const;

  void setAssociatedParameterSetModel(QAbstractItemModel* psm);
  QAbstractItemModel *associatedParameterSetModel() const;

  /**
   * read access functions
   */
  insight::cad::Model::ScalarTableContents scalars() const;
  insight::cad::Model::VectorTableContents points() const;
  insight::cad::Model::VectorTableContents directions() const;
  insight::cad::Model::DatumTableContents datums() const;
  insight::cad::Model::ModelstepTableContents modelsteps() const;
  insight::cad::Model::PostprocActionTableContents postprocActions() const;
  const insight::cad::Model::DatasetTableContents& datasets() const;

  QModelIndex scalarIndex(const std::string& name) const;
  QModelIndex pointIndex(const std::string& name) const;
  QModelIndex directionIndex(const std::string& name) const;
  QModelIndex datumIndex(const std::string& name) const;
  QModelIndex modelstepIndex(const std::string& name) const;
  QModelIndex modelstepIndexFromValue(insight::cad::FeaturePtr feat) const;
  QModelIndex postprocActionIndex(const std::string& name) const;
  QModelIndex datasetIndex(const std::string& name) const;


  /**
   * modifier functions
   */
  void addScalar(const std::string& name, insight::cad::ScalarPtr value);
  void addPoint(const std::string& name, insight::cad::VectorPtr value);
  void addDirection(const std::string& name, insight::cad::VectorPtr value);
  void addDatum(const std::string& name, insight::cad::DatumPtr value, bool initialVisibility=false);
  void addModelstep(
      const std::string& name,
      insight::cad::FeaturePtr value,
      const std::string& featureDescription = std::string(),
      const insight::cad::FeatureVisualizationStyle& fvs =
        insight::cad::FeatureVisualizationStyle::intermediateFeatureStyle() );

  void setStaticModelStep(const std::string& name, bool isStatic);
  bool isStaticModelStep(const std::string& name);
  void addComponent(
      const std::string& name,
      insight::cad::FeaturePtr value,
      const std::string& featureDescription = std::string(),
      const insight::cad::FeatureVisualizationStyle& fvs =
        insight::cad::FeatureVisualizationStyle::componentStyle() );

  void addPostprocAction(const std::string& name, insight::cad::PostprocActionPtr value);
  void addDataset(const std::string& name, vtkSmartPointer<vtkDataObject> value);

  void removeScalar(const std::string& name);
  void removePoint(const std::string& name);
  void removeDirection(const std::string& name);
  void removeDatum(const std::string& name);
  void removeModelstep(const std::string& name );
  void removePostprocAction(const std::string& name);
  void removeDataset(const std::string& name);

  void populateClipPlaneMenu(QMenu* clipplanemenu, IQCADModel3DViewer* v);

public Q_SLOTS:
  void showContextMenu(const QModelIndex& idx, const QPoint &pos, IQCADModel3DViewer* viewer);

  void addPlane();
  void addImportedFeature();
  void addImportedSketch(insight::cad::DatumPtr plane);

Q_SIGNALS:
  void insertIntoNotebook(const QString& text);

  // no QModelIndex as parameter, since non-indexed shape may be highlighted
  void highlightInView(insight::cad::FeaturePtr feat);
  void undoHighlightInView();

  void jumpToDefinition(const QString& name);
  void insertParserStatementAtCursor(const QString& name);
};

#endif // IQCADMODELCONTAINER_H
