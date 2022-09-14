#ifndef IQPARAMETERSETMODEL_H
#define IQPARAMETERSETMODEL_H

#include "toolkit_gui_export.h"


#include <QAbstractItemModel>
#include <QScopedPointer>

#include "base/parameterset.h"
#include "iqparameter.h"

class IQArrayParameter;
class IQSelectableSubsetParameter;
class IQCADModel3DViewer;
template<class IQBaseParameter, const char* N> class IQArrayElementParameter;

class TOOLKIT_GUI_EXPORT IQParameterSetModel
    : public QAbstractItemModel
{
  Q_OBJECT

  friend class IQArrayParameter;
  friend class IQSelectableSubsetParameter;

  template<class IQBaseParameter, const char* N>
  friend class IQArrayElementParameter;

  insight::ParameterSet parameterSet_, defaultParameterSet_;
  QList<IQParameter*> rootParameters_;

  mutable std::map<QString, insight::cad::FeaturePtr> transformedGeometry_;

  /**
   * @brief vectorBasePoints_
   * if a vector parameter represents a direction, this map contains the base point.
   * If there is no base point for a vector parameter, it treated as a point (location vector)
   */
  mutable std::map<QString, arma::mat> vectorBasePoints_;

  std::pair<QString, const insight::Parameter*> getParameterAndName(const QModelIndex& index) const;

  QList<IQParameter*> decorateSubdictContent(QObject*, const insight::ParameterSet&, int);
  IQParameter* decorateArrayElement(QObject* parent, int i, insight::Parameter& cp, int level);
  QList<IQParameter*> decorateArrayContent(QObject*, insight::ArrayParameterBase&, int);
  void decorateChildren(QObject* parent, insight::Parameter* p, int level);

public:
  IQParameterSetModel(const insight::ParameterSet& ps, const insight::ParameterSet& defaultps, QObject* parent=nullptr);

  int	columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant	headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  int	rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex	index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex	parent(const QModelIndex &index) const override;

  QVariant	data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  void contextMenu(QWidget* pw, const QModelIndex& index, const QPoint& p);


  const insight::ParameterSet& getParameterSet() const;

  insight::Parameter& parameterRef(const QModelIndex &index);
  void notifyParameterChange(const QModelIndex &index);

  QList<int> pathFromIndex(const QModelIndex& i) const;
  QModelIndex indexFromPath(const QList<int>& p) const;

  void addGeometryToSpatialTransformationParameter(
          const QString& parameterPath, insight::cad::FeaturePtr geom );
  void addVectorBasePoint(
          const QString& parameterPath, const arma::mat& pBase );

  insight::cad::FeaturePtr
  getGeometryToSpatialTransformationParameter(
          const QString& parameterPath );
  const arma::mat* const getVectorBasePoint(
          const QString& parameterPath );

public Q_SLOTS:
  void clearParameters();
  void resetParameters(const insight::ParameterSet& ps, const insight::ParameterSet& defaultps);
  void handleClick(const QModelIndex &index, QWidget* editControlsContainer, IQCADModel3DViewer *vri=nullptr);

 Q_SIGNALS:
  void parameterSetChanged();
};

#endif // IQPARAMETERSETMODEL_H
