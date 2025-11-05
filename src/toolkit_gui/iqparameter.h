#ifndef IQPARAMETER_H
#define IQPARAMETER_H

#include "toolkit_gui_export.h"

#include <functional>

#include <QObject>
#include <QVariant>
#include <QMenu>
#include <QStyledItemDelegate>


#include "iqhierarchicaldataelement.h"
#include "base/factory.h"
#include "base/parameter.h"
#include "base/parameterset.h"
#include "base/cppextensions.h"



class QVBoxLayout;
class IQParameterSetModel;
class IQCADModel3DViewer;

QString mat2Str(const arma::mat& m);

class TOOLKIT_GUI_EXPORT IQParameter
    : public IQHierarchicalDataElement
{

private:
  // cache state
  mutable boost::optional<bool> markedAsModified_;

public:
  declareType("IQParameter");

  IQParameter
  (
      QObject* parent,
      IQHierarchicalDataModel* hdmodel,
      insight::hierarchicalData::Element* parameter
  );


  void resetModificationState();
  virtual bool isModified() const;

  QVariant backgroundColor() const override;
  QVariant textColor() const override;
  QVariant textFont() const override;

  QVBoxLayout* populateEditControls(
          QWidget* editControlsContainer,
          IQCADModel3DViewer *viewer ) override;

  virtual void applyProposition(
      const insight::ParameterSet& propositions,
      const std::string& selectProposition );

  const insight::ParameterSet* defaultParameterSet() const;

  static QAbstractItemDelegate* createIQParameterGridViewDelegate(QWidget *parent);

  IQParameterSetModel* psModel() const;

};




template<class P>
class IQSpecializedParameter
: public IQParameter
{
public:
    using IQParameter::IQParameter;

    const P& parameter() const
    {
        return dynamic_cast<const P&>(*get());
    }

    P& parameterRef()
    {
        return dynamic_cast<P&>(*get());
    }
};





// class IQParameterGridViewDelegateEditorWidget
//     : public QObject
// {
//     QPersistentModelIndex index_;
//     QAbstractItemDelegate* delegate_;

// public:
//     declareStaticFunctionTableWithArgs
//         (
//             createDelegate, QAbstractItemDelegate*,
//             LIST(
//                 QObject* ),
//             LIST(
//                 QObject* parent )
//             );

// public:
//     IQParameterGridViewDelegateEditorWidget(
//         QObject* parent,
//         const IQParameter& parameter,
//         const QModelIndex& index );

//     inline const QAbstractItemDelegate* delegate() const { return delegate_; }
//     inline QAbstractItemDelegate* delegate() { return delegate_; }
//     inline QModelIndex index() const { return index_; }
// };


// class IQParameterGridViewSelectorDelegate
//     : public QStyledItemDelegate
// {

//     const IQParameterGridViewDelegateEditorWidget* delegateWidgetForIndex(const QModelIndex& idx) const;
//     inline IQParameterGridViewDelegateEditorWidget* delegateWidgetForIndex(const QModelIndex& idx)
//     {
//         return const_cast<IQParameterGridViewDelegateEditorWidget*>(
//           const_cast<const IQParameterGridViewSelectorDelegate*>(this)
//                 ->delegateWidgetForIndex(idx) );
//     }

// public:
//     QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
//     void destroyEditor(QWidget *editor, const QModelIndex &index) const override;
//     bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
//     bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;
//     void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
//     void setEditorData(QWidget *editor, const QModelIndex &index) const override;
//     void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
//     QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const  override;
//     void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
// };



#endif // IQPARAMETER_H
