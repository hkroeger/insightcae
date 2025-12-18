#ifndef IQPARAMETERSETMODEL_H
#define IQPARAMETERSETMODEL_H

#include "base/parameter.h"
#include "base/parameters/subsetparameter.h"
#include "parametersetwithguicontext.h"

#include "toolkit_gui_export.h"

#include <QSet>
#include <QSharedPointer>
#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QScopedPointer>
#include <atomic>
#include <memory>

#include "base/parameterset.h"
#include "iqparameter.h"
#include "iqhierarchicaldatamodel.h"
#include "cadtypes.h"



class IQArrayParameter;
class IQSelectableSubsetParameter;
class IQCADModel3DViewer;
template<class IQBaseParameter, const char* N> class IQArrayElementParameter;
class IQParameterSetModel;




namespace insight {
class CADParameterSetModelVisualizer;
class LabeledArrayParameter;
namespace cad {
class Feature;
typedef std::shared_ptr<Feature> FeaturePtr;
}
}



class TOOLKIT_GUI_EXPORT IQParameterSetModel
    : public IQHierarchicalDataModel
{
  Q_OBJECT

private:

  friend class IQParameter;
  friend class IQArrayParameter;
  friend class IQArrayElementParameterBase;
  friend class IQSelectableSubsetParameter;

  template<class IQBaseParameter, const char* N>
  friend class IQArrayElementParameter;


  std::unique_ptr<insight::ParameterSet> defaultParameterSet_;

  std::pair<QString, const insight::Parameter*> getParameterAndName(const QModelIndex& index) const;

public:
  IQParameterSetModel(
      std::unique_ptr<insight::ParameterSet>&& ps,
      boost::optional<const insight::ParameterSet&> defaultps
        = boost::optional<const insight::ParameterSet&>(),
      QObject* parent=nullptr);


  Qt::ItemFlags flags(const QModelIndex &index) const override;
  Qt::DropActions supportedDropActions() const override;
  QStringList mimeTypes() const override;
  QMimeData * mimeData(const QModelIndexList & indexes) const override;
  bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) override;

  // QVariant	data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;

  // edit functions
  bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;

  void copy(const QModelIndexList & indexes) const;
  void paste(const QModelIndexList & indexes);

  static void contextMenu(
      QWidget* pw,
      const QModelIndex& index,
      const QPoint& p,
      IQCADModel3DViewer *viewer = nullptr );

  // class ParameterEditor
  // {
  //     IQParameterSetModel& model_;
  //     const QModelIndex index_;
  // public:
  //     insight::Parameter& parameter;
  //     ParameterEditor(IQParameterSetModel& psm, const std::string& parameterPath);
  //     ~ParameterEditor();
  // };
  // friend class ParameterContext;


  const insight::ParameterSet& getParameterSet() const;

  bool hasDefaultParameterSet() const;
  const insight::ParameterSet* defaultParameterSet() const;

//  IQParameter* iqParameter(const QModelIndex &index) const;

  static IQParameter* parameterFromIndex(const QModelIndex& index);

  insight::Parameter& parameterRef(const QModelIndex &index);
  insight::Parameter& parameterRef(const std::string &path);



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

  insight::ParameterSetGUIContext& GUIContext();

  void pack();
  void clearPackedData();

  std::string getAnalysisName() const;




public Q_SLOTS:

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
