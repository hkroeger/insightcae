#ifndef IQPARAMETER_H
#define IQPARAMETER_H

#include "toolkit_gui_export.h"


#include <QObject>
#include <QVariant>
#include <QMenu>

#include "base/factory.h"
#include "base/parameter.h"
#include "base/parameterset.h"
#include "base/cppextensions.h"


class QVBoxLayout;
class IQParameterSetModel;
class IQCADModel3DViewer;

QString mat2Str(const arma::mat& m);

class TOOLKIT_GUI_EXPORT IQParameter
: public QObject,
  public QList<IQParameter*>,
  public insight::ObjectWithBoostSignalConnections
{
  Q_OBJECT

  friend class IQArrayElementParameterBase;

public:
  declareFactoryTable
  (
      IQParameter,
        LIST(
            QObject* parent,
            IQParameterSetModel* psmodel,
            const QString& name,
            insight::Parameter& parameter,
            const insight::ParameterSet& defaultParameterSet ),
        LIST(
            parent,
            psmodel,
            name,
            parameter,
            defaultParameterSet )
  );

  static IQParameter* create(
      QObject* parent,
      IQParameterSetModel* psmodel,
      const QString& name,
      insight::Parameter& p,
      const insight::ParameterSet& defaultParameterSet );

private:
  QString name_;
  insight::Parameter& parameter_;
  const insight::ParameterSet& defaultParameterSet_;
  IQParameterSetModel *model_;

  mutable std::unique_ptr<bool> markedAsModified_;

public:
  declareType("IQParameter");

  IQParameter
  (
      QObject* parent,
      IQParameterSetModel* psmodel,
      const QString& name,
      insight::Parameter& parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  // in separate function for beeing able to call
  // virtual functions
  virtual void connectSignals();

  IQParameter* parentParameter() const;
  int nChildParameters() const;
  IQParameterSetModel *model() const;

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

  virtual void populateContextMenu(QMenu* m);
  virtual QVBoxLayout* populateEditControls(
          QWidget* editControlsContainer,
          IQCADModel3DViewer *viewer );

  const insight::Parameter& parameter() const;
  insight::Parameter& parameterRef();

  virtual void applyProposition(
      const insight::ParameterSet& propositions,
      const std::string& selectProposition );
};




#endif // IQPARAMETER_H
