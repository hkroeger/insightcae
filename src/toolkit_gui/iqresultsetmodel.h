#ifndef QRESULTSETMODEL_H
#define QRESULTSETMODEL_H

#include "toolkit_gui_export.h"

#include <set>

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include "base/resultsetfilter.h"

class QVBoxLayout;
class QTextEdit;
class QTreeView;

#include "base/resultset.h"
#include "base/factory.h"

#include "iqhierarchicaldatamodel.h"
#include "iqhierarchicaldataelement.h"
#include "qtextensions.h"



namespace insight
{




class TOOLKIT_GUI_EXPORT ResizeEventNotifier
    : public QObject
{
  Q_OBJECT

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;

public:
  ResizeEventNotifier(QObject* parent);

Q_SIGNALS:
  void resized(int width, int height);
};




class TOOLKIT_GUI_EXPORT IQResultElement
 : public IQHierarchicalDataElement
{
    Q_OBJECT

  IQSimpleLatexView *shortDesc_, *longDesc_;

  Qt::CheckState checkState_;

  IQHierarchicalDataElement* createForChild(
      IQHierarchicalDataModel* model,
      insight::hierarchicalData::Element *ce ) override;

public:
    declareType("IQResultElement");

    IQResultElement(
        QObject* parent,
        IQHierarchicalDataModel* hdmodel,
        insight::hierarchicalData::Element* element);


    virtual QVariant previewInformation(int role) const =0;
    virtual void createFullDisplay(QVBoxLayout* layout);

    Qt::CheckState isChecked() const;
    void setChecked( Qt::CheckState cs );
};





class TOOLKIT_GUI_EXPORT IQStaticTextResultElement
 : public IQResultElement
{
    Q_OBJECT

    QString staticText_, staticDetailText_;

public:
    declareType("IQStaticTextResultElement");

    IQStaticTextResultElement(
        QObject* parent,
        const QString& staticText,
        const QString& staticDetailText,
        IQHierarchicalDataModel* hdmodel,
        insight::hierarchicalData::Element* element );

    QVariant previewInformation(int role) const override;
    void createFullDisplay(QVBoxLayout* layout) override;
};




class IQResultSetModelBase
{
public:
    virtual IQResultElement* getResultElement(const QModelIndex& idx) =0;
};


class TOOLKIT_GUI_EXPORT IQResultSetModel
        : public IQHierarchicalDataModel,
          public IQResultSetModelBase
{
    Q_OBJECT

    bool selectableElements_;

    // void addResultElements(const ResultElementCollection& rec, IQResultElement* parent);

    void setCheckState(const QModelIndex &idx, bool checked);
    void updateParentCheckState(const QModelIndex &idx);
    void setChildrenCheckstate(const QModelIndex& idx, bool checked);
    void addUnselectedElementPaths(const QModelIndex& pidx,
                                   ResultSetFilter& filter,
                                   std::string parentPath ) const;
    void unselectElements(const QModelIndex& pidx,
                          const ResultSetFilter& filter,
                          std::string parentPath );
public:

    IQResultSetModel(std::unique_ptr<ResultSet> resultSet, bool selectableElements=false, QObject* parent=nullptr);

    QVariant headerData(int section, Qt::Orientation orient, int role) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    IQResultElement* getResultElement(const QModelIndex& idx) override;

    const insight::ResultSet& resultSet() const;

    ResultSetFilter filter() const;
    void resetFilter(const ResultSetFilter& filter);
};




class IQFilteredResultSetModel
        : public QSortFilterProxyModel,
          public IQResultSetModelBase
{
    Q_OBJECT

    ResultSetFilter filter_;

public:
    IQFilteredResultSetModel(QObject *parent = 0);

    IQResultElement* getResultElement(const QModelIndex& idx) override;

    void resetFilter(const ResultSetFilter& filter);

    void addChildren(const QModelIndex& pidx, insight::ResultElementCollection* re) const;
    ResultSetPtr filteredResultSet() const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};




void connectToCWithContentsDisplay(QTreeView* ToCView, QWidget* content);




}


#endif // QRESULTSETMODEL_H
