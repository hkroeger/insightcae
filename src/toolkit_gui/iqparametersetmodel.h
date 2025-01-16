#ifndef IQPARAMETERSETMODEL_H
#define IQPARAMETERSETMODEL_H

#include "base/parameter.h"
#include "toolkit_gui_export.h"

#include <QSet>
#include <QSharedPointer>
#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QScopedPointer>

#include "base/parameterset.h"
#include "iqparameter.h"




class IQArrayParameter;
class IQSelectableSubsetParameter;
class IQCADModel3DViewer;
template<class IQBaseParameter, const char* N> class IQArrayElementParameter;
class IQParameterSetModel;




namespace insight {
class CADParameterSetModelVisualizer;
class LabeledArrayParameter;
}



class TOOLKIT_GUI_EXPORT IQParameterSetModel
    : public QAbstractItemModel
{
  Q_OBJECT

public:
  static const int
      labelCol=0,
      valueCol=1,
      stringPathCol=2,
      iqParamCol=3;

private:

  friend class IQParameter;
  friend class IQArrayParameter;
  friend class IQArrayElementParameterBase;
  friend class IQSelectableSubsetParameter;

  template<class IQBaseParameter, const char* N>
  friend class IQArrayElementParameter;

  std::unique_ptr<insight::ParameterSet> parameterSet_;

  std::unique_ptr<insight::ParameterSet> defaultParameterSet_;
  std::string analysisName_; // reqd for parameter proposition engine

  mutable std::map<std::string, insight::cad::FeaturePtr> transformedGeometry_;
  mutable std::key_observer_map<IQParameter, int> wrappers_;

  IQParameter* findWrapper(const insight::Parameter& p) const;
  int countDisplayedChildren(const QModelIndex& index) const;

  /**
   * @brief vectorBasePoints_
   * if a vector parameter represents a direction, this map contains the base point.
   * If there is no base point for a vector parameter, it treated as a point (location vector)
   */
  mutable std::map<std::string, arma::mat> vectorBasePoints_;


  std::pair<QString, const insight::Parameter*> getParameterAndName(const QModelIndex& index) const;

public:
  static insight::Parameter* indexData(const QModelIndex& idx);
  const IQParameter* iqIndexData(const QModelIndex& idx) const;
  IQParameter* iqIndexData(const QModelIndex& idx);

  IQParameterSetModel(
      std::unique_ptr<insight::ParameterSet>&& ps,
      boost::optional<const insight::ParameterSet&> defaultps
        = boost::optional<const insight::ParameterSet&>(),
      QObject* parent=nullptr);

    const insight::Parameter* visibleParent(const insight::Parameter&p, int& row) const;

    // access functions
    QModelIndex indexFromParameter(const insight::Parameter& p, int col) const;
    QModelIndex indexFromParameterPath(const std::string& pp, int col) const;


  int	columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant	headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  int	rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex	index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex	parent(const QModelIndex &index) const override;

  Qt::ItemFlags flags(const QModelIndex &index) const override;
  Qt::DropActions supportedDropActions() const override;
  QStringList mimeTypes() const override;
  QMimeData * mimeData(const QModelIndexList & indexes) const override;
  bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) override;

  QVariant	data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;

  // edit functions
  bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;

  void copy(const QModelIndexList & indexes) const;
  void paste(const QModelIndexList & indexes);

  static void contextMenu(QWidget* pw, const QModelIndex& index, const QPoint& p);

  class ParameterEditor
  {
      IQParameterSetModel& model_;
      const QModelIndex index_;
  public:
      insight::Parameter& parameter;
      ParameterEditor(IQParameterSetModel& psm, const std::string& parameterPath);
      ~ParameterEditor();
  };
  friend class ParameterContext;


  const insight::ParameterSet& getParameterSet() const;

//  IQParameter* iqParameter(const QModelIndex &index) const;

  static IQParameter* parameterFromIndex(const QModelIndex& index);

  insight::Parameter& parameterRef(const QModelIndex &index);
  insight::Parameter& parameterRef(const std::string &path);


  /**
   * @brief notifyParameterChange
   * update parameter and redecorate all children, if necessary
   * @param path
   * path (slash separated) to changed parameter
   */
  void notifyParameterChange(const insight::Parameter& p);

  /**
   * @brief notifyParameterChange
   * update parameter and redecorate all children, if necessary
   * @param index
   */
  void notifyParameterChange(const QModelIndex &index);

  void appendArrayElement(const QModelIndex &index, const insight::Parameter& elem);
  /**
   * @brief insertArrayElement
   * @param index
   * index of array element before which shall be inserted or parent array (appended in this case)
   * @param elem
   */
  void insertArrayElement(const QModelIndex &index, const insight::Parameter& elem);
  void removeArrayElement(const QModelIndex &index);
  void removeLabeledArrayElement(const QModelIndex &index);


  void addGeometryToSpatialTransformationParameter(
          const std::string& parameterPath, insight::cad::FeaturePtr geom );
  void addVectorBasePoint(
          const std::string& parameterPath, const arma::mat& pBase );

  insight::cad::FeaturePtr
  getGeometryToSpatialTransformationParameter(
          const std::string& parameterPath );
  const arma::mat* const getVectorBasePoint(
          const std::string& parameterPath );

  void pack();
  void clearPackedData();

  void setAnalysisName(const std::string& analysisName);
  const std::string& getAnalysisName() const;

  void resetParameters(
      std::unique_ptr<insight::ParameterSet>&& ps );

public Q_SLOTS:
  void clearParameters();

  void resetParameterValues(
      const insight::ParameterSet& ps,
      boost::optional<const insight::ParameterSet&> defaultps
      = boost::optional<const insight::ParameterSet&>() );

};

IQParameterSetModel *parameterSetModel(QAbstractItemModel* model);
const insight::ParameterSet& getParameterSet(QAbstractItemModel* model);
const std::string& getAnalysisName(QAbstractItemModel* model);

template<class ...Args>
void connectParameterSetChanged(
    QAbstractItemModel* source,
    Args&&... slotArgs )
{
  QObject::connect( source, &QAbstractItemModel::dataChanged,
          std::forward<Args>(slotArgs)... );

  QObject::connect( source, &QAbstractItemModel::rowsInserted,
          std::forward<Args>(slotArgs)... );

  QObject::connect( source, &QAbstractItemModel::rowsRemoved,
          std::forward<Args>(slotArgs)... );
}




void disconnectParameterSetChanged(
    QAbstractItemModel* source, QObject *target);





#endif // IQPARAMETERSETMODEL_H
