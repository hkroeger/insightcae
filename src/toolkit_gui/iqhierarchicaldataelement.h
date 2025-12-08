#ifndef IQHIERARCHICALDATAELEMENT_H
#define IQHIERARCHICALDATAELEMENT_H

#include "base/hierarchicalelement.h"
#include "iqhierarchicaldatamodel.h"


class TOOLKIT_GUI_EXPORT IQHierarchicalDataElement
    : public QObject,
      public std::observer_ptr<insight::hierarchicalData::Element>,
      public insight::ObjectWithBoostSignalConnections
{
    Q_OBJECT

protected:
    void invalidate() override;

public:
    declareFactoryTable
        (
            IQHierarchicalDataElement,
            LIST(
                QObject* parent,
                IQHierarchicalDataModel* model,
                insight::hierarchicalData::Element* element ),
            LIST(
                parent,
                model,
                element )
            );

    declareStaticFunctionTableWithArgs
        (
            createIQHierarchicalDataElementGridViewDelegate,
            QAbstractItemDelegate*,
            LIST(QWidget *),
            LIST(QWidget *parent)
            );


    static IQHierarchicalDataElement* create(
        QObject* parent,
        IQHierarchicalDataModel* model,
        insight::hierarchicalData::Element* e );

    virtual IQHierarchicalDataElement* createForChild(
        IQHierarchicalDataModel* model,
        insight::hierarchicalData::Element *ce );


private:
    IQHierarchicalDataModel *model_;

    Qt::CheckState checkState_;

public:
    declareType("IQHierarchicalDataElement");

    IQHierarchicalDataElement
        (
            QObject* parent,
            IQHierarchicalDataModel* hdmodel,
            insight::hierarchicalData::Element* element
            );

    virtual ~IQHierarchicalDataElement();

    virtual void removeFromViews(bool deleteLater=true);

    /**
     * @brief visibleParent
     * return element, which is displayed one hierarchy level above.
     * might not be the elements actual parent element
     * @return
     */
    IQHierarchicalDataElement* visibleParent() const;

    /**
     * @brief parentParameter
     * the elements actual parent
     * @return
     */
    IQHierarchicalDataElement* parentElement() const;

    QList<IQHierarchicalDataElement*> children() const;

    // in separate function for beeing able to call
    // virtual functions
    virtual void connectSignals();


    IQHierarchicalDataModel *model() const;

    QString name() const;

    virtual QVariant value() const;

    virtual bool canSetValue() const;
    /**
   * @brief setValue
   * @param value
   * @return
   * true, if value could be successfully interpreted
   */
    virtual bool setValue(QVariant value);

    Qt::CheckState isChecked() const;
    void setChecked( Qt::CheckState cs );

    virtual QVariant backgroundColor() const;
    virtual QVariant textColor() const;
    virtual QVariant textFont() const;

    virtual void populateContextMenu(
        QMenu* m,
        IQCADModel3DViewer *viewer );

    virtual QVBoxLayout* populateEditControls(
        QWidget* editControlsContainer,
        IQCADModel3DViewer *viewer );


    static QAbstractItemDelegate* createIQParameterGridViewDelegate(QWidget *parent);

    void checkEnabledOrDisabled();

    template<class E>
    E& elementAs()
    {
        return dynamic_cast<E&>(*get());
    }

    template<class E>
    const E& elementAs() const
    {
        return dynamic_cast<const E&>(*get());
    }

Q_SIGNALS:
    void setControlsEnabled(bool isEnabled);
};




template<class E>
class IQSpecializedHierarchicalDataElement
    : public IQHierarchicalDataElement
{
public:
    using IQHierarchicalDataElement::IQHierarchicalDataElement;

    const E& element() const
    {
        return elementAs<const E&>(*get());
    }

    E& elementRef()
    {
        return elementAs<E&>(*get());
    }
};





class IQHierarchicalDataGridViewDelegateEditorWidget
    : public QObject
{
    QPersistentModelIndex index_;
    QAbstractItemDelegate* delegate_;

public:
    declareStaticFunctionTableWithArgs(
        createDelegate,
        QAbstractItemDelegate*,
        LIST( QObject* ),
        LIST( QObject* parent )
        );

public:
    IQHierarchicalDataGridViewDelegateEditorWidget(
        QObject* parent,
        const IQHierarchicalDataElement& element,
        const QModelIndex& index );

    inline const QAbstractItemDelegate* delegate() const { return delegate_; }
    inline QAbstractItemDelegate* delegate() { return delegate_; }
    inline QModelIndex index() const { return index_; }
};




class IQHierarchicalDataGridViewSelectorDelegate
    : public QStyledItemDelegate
{

    const IQHierarchicalDataGridViewDelegateEditorWidget* delegateWidgetForIndex(
        const QModelIndex& idx ) const;

    inline IQHierarchicalDataGridViewDelegateEditorWidget* delegateWidgetForIndex(
        const QModelIndex& idx )
    {
        return const_cast<IQHierarchicalDataGridViewDelegateEditorWidget*>(
            const_cast<const IQHierarchicalDataGridViewSelectorDelegate*>(this)
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



#endif // IQHIERARCHICALDATAELEMENT_H
