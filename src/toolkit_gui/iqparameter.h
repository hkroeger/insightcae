#ifndef IQPARAMETER_H
#define IQPARAMETER_H

#include "toolkit_gui_export.h"


#include <QObject>
#include <QVariant>
#include <QMenu>

#include "base/factory.h"
#include "base/parameter.h"
#include "base/parameterset.h"

class QVBoxLayout;
class IQParameterSetModel;

QString mat2Str(const arma::mat& m);

class TOOLKIT_GUI_EXPORT IQParameter
: public QObject,
  public QList<IQParameter*>
{
  Q_OBJECT

public:
  declareFactoryTable
  (
      IQParameter,
        LIST(QObject* parent, const QString& name, insight::Parameter& parameter, const insight::ParameterSet& defaultParameterSet),
        LIST(parent, name, parameter, defaultParameterSet)
  );

  static IQParameter* create(QObject* parent, const QString& name, insight::Parameter& p, const insight::ParameterSet& defaultParameterSet);

private:
  QString name_;
  insight::Parameter& parameter_;
  const insight::ParameterSet& defaultParameterSet_;

  mutable std::unique_ptr<bool> markedAsModified_;

public:
  declareType("IQParameter");

  IQParameter
  (
      QObject* parent,
      const QString& name,
      insight::Parameter& parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  IQParameter* parentParameter() const;
  int nChildParameters() const;

  const QString& name() const;
  void setName(const QString& newName);
  const QString buildPath(const QString& name, bool redirectArrayElementsToDefault) const;
  virtual const QString path(bool redirectArrayElementsToDefault=false) const;
  virtual QString valueText() const;

  void resetModificationState();
  virtual bool isModified() const;

  QVariant backgroundColor() const;
  QVariant textColor() const;
  QVariant textFont() const;

  virtual void populateContextMenu(IQParameterSetModel* model, const QModelIndex &index, QMenu* m);
  virtual QVBoxLayout* populateEditControls(IQParameterSetModel* model, const QModelIndex &index, QWidget* editControlsContainer);

  const insight::Parameter& parameter() const;
};




#endif // IQPARAMETER_H
