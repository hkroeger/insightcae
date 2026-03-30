#ifndef IQHIERARCHICALDATAMODEL_H
#define IQHIERARCHICALDATAMODEL_H

#include "toolkit_gui_export.h"
#include "base/hierarchicalelement.h"

#include "iqundoredostack.h"

#include <QObject>
#include <QAbstractItemModel>

#include <QMenu>
#include <QAbstractItemDelegate>
#include <QVBoxLayout>
#include <QStyledItemDelegate>



class IQCADModel3DViewer;
class IQHierarchicalDataElement;
class IQHierarchicalDataModel;

/**
 * @brief The ParameterErrorState class
 * as long as an object of this class exists,
 * an error is signalled in all connected views.
 */
class TOOLKIT_GUI_EXPORT ParameterErrorState
    : public boost::noncopyable,
      public std::observable
{
public:
    enum Severity
    {
        Yellow, Red
    };
private:
    IQHierarchicalDataModel& model_;
    std::string parameterPath_;
    Severity severity_;
    QString explanation_;

public:
    ParameterErrorState(
        IQHierarchicalDataModel& model,
        const std::string& parameterPath,
        const QString& explanation,
        Severity s = Severity::Yellow );
    ~ParameterErrorState();

    QVariant data(const QModelIndex &index, int role) const;
};



class IQHierarchicalDataModel
: public QAbstractItemModel,
      public IQUndoRedoStack
{
    Q_OBJECT

public:
    static const int
        labelCol=0,
        valueCol=1,
        stringPathCol=2,
        iqParamCol=3;

private:
    friend class IQHierarchicalDataElement;
    friend class ParameterErrorState;
    friend class BulkUpdateGuard;

    std::unique_ptr<insight::hierarchicalData::Element> data_;
    mutable std::unique_ptr<insight::hierarchicalData::Element> dataBeforeLastChange_;

    mutable std::key_observer_map<IQHierarchicalDataElement, int>
        wrappers_;

    IQHierarchicalDataElement* findWrapper(
        const insight::hierarchicalData::Element& e) const;

    int countDisplayedChildren(const QModelIndex& index) const;

    std::atomic<bool>
        editingIsDisabled_;

    bool bulkUpdateInProgress_ = false;

    void editingOff();
    void editingOn();

    bool selectableElements_;

    std::set<ParameterErrorState*> errors_;


public:
    bool editingIsEnabled() const;


    class UndoState
        : public IQUndoRedoStackState,
          public std::unique_ptr<insight::hierarchicalData::Element>
    {
    public:
        UndoState(
            const QString& description,
            std::unique_ptr<insight::hierarchicalData::Element>&& sk );
    };

protected:
    void applyUndoState(const IQUndoRedoStackState& state) override;
    IQUndoRedoStackStatePtr createUndoState(const QString& description) const override;

    void setCheckState(const QModelIndex &idx, bool checked);
    void updateParentCheckState(const QModelIndex &idx);
    void setChildrenCheckstate(const QModelIndex& idx, bool checked);

protected Q_SLOTS:
    void handleDataChangeForUndo(
        const QModelIndex &topLeft,
        const QModelIndex &bottomRight,
        const QVector<int> &roles = QVector<int>() );

public:
    class UndoRecordingBlocker
    {
        friend class IQHierarchicalDataModel;

        IQHierarchicalDataModel* model_;
    protected:
        UndoRecordingBlocker(IQHierarchicalDataModel* model)
            : model_(model)
        {
            disconnect(
                model_, &IQHierarchicalDataModel::dataChanged,
                model_, &IQHierarchicalDataModel::handleDataChangeForUndo
                );
        }
    public:
        ~UndoRecordingBlocker()
        {
            connect(
                model_, &IQHierarchicalDataModel::dataChanged,
                model_, &IQHierarchicalDataModel::handleDataChangeForUndo
                );
        }
    };

    inline std::shared_ptr<UndoRecordingBlocker> blockUndoRecording()
    {
        return std::shared_ptr<UndoRecordingBlocker>(
            new UndoRecordingBlocker(this) );
    }

    static insight::hierarchicalData::Element* elementOfIndex(const QModelIndex& idx);
    IQHierarchicalDataElement* iqElementOfIndex(const QModelIndex& idx);
    const IQHierarchicalDataElement* iqElementOfIndex(const QModelIndex& idx) const;

    QModelIndex indexOfElement(const insight::hierarchicalData::Element& e, int col) const;
    QModelIndex indexOfPath(const std::string& pp, int col) const;

    void resetData(
        std::unique_ptr<insight::hierarchicalData::Element>&& data );

    void resetValue(
        const insight::hierarchicalData::Element& data );

    IQHierarchicalDataModel(
        std::unique_ptr<insight::hierarchicalData::Element>&& data,
        QObject *parent = nullptr,
        bool selectableElements=false,
        bool editingIsDisabled=false
    );

    int	columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    int	rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex	index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex	parent(const QModelIndex &index) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    class EditingDisabler
    {
        IQHierarchicalDataModel& psm_;
    public:
        std::function<void()> additionalCleanup;

        EditingDisabler(IQHierarchicalDataModel& psm);
        ~EditingDisabler();
    };
    std::shared_ptr<EditingDisabler> disableEditing();

    class BulkUpdateGuard
    {
        IQHierarchicalDataModel& m_;
    public:
        BulkUpdateGuard(IQHierarchicalDataModel& m);
        ~BulkUpdateGuard();
    };
    std::shared_ptr<BulkUpdateGuard> beginBulkUpdate();
    bool isBulkUpdateInProgress() const;

    /**
   * @brief notifyParameterChange
   * update parameter and redecorate all children, if necessary
   * @param path
   * path (slash separated) to changed parameter
   */
    void notifyElementChange(const insight::hierarchicalData::Element& e);

    /**
   * @brief notifyParameterChange
   * update parameter and redecorate all children, if necessary
   * @param index
   */
    void notifyElementChange(const QModelIndex &index);

    const insight::hierarchicalData::Element& getHierarchicalData() const;
    inline bool hasData() const
    {
        return bool(data_);
    }

    void issueEphemeralError(
        const std::string& parameterPath,
        const QString& explanation,
        ParameterErrorState::Severity s =
            ParameterErrorState::Severity::Yellow );

Q_SIGNALS:
    void bulkUpdateFinished();
};







#endif // IQHIERARCHICALDATAMODEL_H
