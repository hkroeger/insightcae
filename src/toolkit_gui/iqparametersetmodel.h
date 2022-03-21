#ifndef IQPARAMETERSETMODEL_H
#define IQPARAMETERSETMODEL_H

#include "toolkit_gui_export.h"


#include <QAbstractItemModel>
#include <QScopedPointer>

#include "base/parameterset.h"
#include "iqparameter.h"

class IQArrayParameter;
class IQSelectableSubsetParameter;
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

public Q_SLOTS:
  void clearParameters();
  void resetParameters(const insight::ParameterSet& ps, const insight::ParameterSet& defaultps);
  void handleClick(const QModelIndex &index, QWidget* editControlsContainer);

 Q_SIGNALS:
  void parameterSetChanged();
};

#endif // IQPARAMETERSETMODEL_H
