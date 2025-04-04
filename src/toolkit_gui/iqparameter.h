#ifndef IQPARAMETER_H
#define IQPARAMETER_H

#include "toolkit_gui_export.h"

#include <functional>

#include <QObject>
#include <QVariant>
#include <QMenu>
#include <QStyledItemDelegate>

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
      public std::observer_ptr<insight::Parameter>,
      public insight::ObjectWithBoostSignalConnections
{
  Q_OBJECT

  friend class IQArrayElementParameterBase;

protected:
  void invalidate() override
  {
      deleteLater();
      std::observer_ptr<insight::Parameter>::invalidate();
  }

public:
  declareFactoryTable
  (
      IQParameter,
        LIST(
            QObject* parent,
            IQParameterSetModel* psmodel,
            insight::Parameter* parameter,
            const insight::ParameterSet& defaultParameterSet ),
        LIST(
            parent,
            psmodel,
            parameter,
            defaultParameterSet )
  );

  declareStaticFunctionTableWithArgs
  (
      createIQParameterGridViewDelegate,
      QAbstractItemDelegate*,
      LIST(QWidget *),
      LIST(QWidget *parent)
  );


  static IQParameter* create(
      QObject* parent,
      IQParameterSetModel* psmodel,
      insight::Parameter* p,
      const insight::ParameterSet& defaultParameterSet );


private:
  const insight::ParameterSet& defaultParameterSet_;
  IQParameterSetModel *model_;

  // cache state
  mutable boost::optional<bool> markedAsModified_;

public:
  declareType("IQParameter");

  IQParameter
  (
      QObject* parent,
      IQParameterSetModel* psmodel,
      insight::Parameter* parameter,
      const insight::ParameterSet& defaultParameterSet
  );

  ~IQParameter();

  virtual void removeFromViews(bool deleteLater=true);

  IQParameter* parentParameter() const;
  QList<IQParameter*> children() const;

  // in separate function for beeing able to call
  // virtual functions
  virtual void connectSignals();


  IQParameterSetModel *model() const;

  QString name() const;
  virtual QString valueText() const;

  /**
   * @brief setValue
   * @param value
   * @return
   * true, if value could be successfully interpreted
   */
  virtual bool setValue(QVariant value);

  void resetModificationState();
  virtual bool isModified() const;

  QVariant backgroundColor() const;
  QVariant textColor() const;
  QVariant textFont() const;

  virtual void populateContextMenu(QMenu* m);
  virtual QVBoxLayout* populateEditControls(
          QWidget* editControlsContainer,
          IQCADModel3DViewer *viewer );

  virtual void applyProposition(
      const insight::ParameterSet& propositions,
      const std::string& selectProposition );

  static QAbstractItemDelegate* createIQParameterGridViewDelegate(QWidget *parent);
};




template<class P>
class IQSpecializedParameter
: public IQParameter
{
public:
    template<class ...Args>
    IQSpecializedParameter(Args&&... addArgs)
        : IQParameter( std::forward<Args>(addArgs)... )
    {}

    const P& parameter() const
    {
        return dynamic_cast<const P&>(*get());
    }

    P& parameterRef()
    {
        return dynamic_cast<P&>(*get());
    }
};





class IQParameterGridViewDelegateEditorWidget
    : public QObject
{
    QPersistentModelIndex index_;
    QAbstractItemDelegate* delegate_;

public:
    declareStaticFunctionTableWithArgs
        (
            createDelegate, QAbstractItemDelegate*,
            LIST(
                QObject* ),
            LIST(
                QObject* parent )
            );

public:
    IQParameterGridViewDelegateEditorWidget(
        QObject* parent,
        const IQParameter& parameter,
        const QModelIndex& index );

    inline const QAbstractItemDelegate* delegate() const { return delegate_; }
    inline QAbstractItemDelegate* delegate() { return delegate_; }
    inline QModelIndex index() const { return index_; }
};


class IQParameterGridViewSelectorDelegate
    : public QStyledItemDelegate
{

    const IQParameterGridViewDelegateEditorWidget* delegateWidgetForIndex(const QModelIndex& idx) const;
    inline IQParameterGridViewDelegateEditorWidget* delegateWidgetForIndex(const QModelIndex& idx)
    {
        return const_cast<IQParameterGridViewDelegateEditorWidget*>(
          const_cast<const IQParameterGridViewSelectorDelegate*>(this)
                ->delegateWidgetForIndex(idx) );
    }

public:
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void destroyEditor(QWidget *editor, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const  override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};



#endif // IQPARAMETER_H
