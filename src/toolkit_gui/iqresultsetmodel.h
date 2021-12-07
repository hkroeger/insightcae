#ifndef QRESULTSETMODEL_H
#define QRESULTSETMODEL_H

#include "toolkit_gui_export.h"


#include <QAbstractItemModel>

class QVBoxLayout;
class QTextEdit;
class QTreeView;

#include "base/resultset.h"
#include "base/factory.h"

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
 : public QObject
{
    Q_OBJECT

  insight::ResultElementPtr resultElement_;

  QTextEdit *shortDesc_, *longDesc_;

  Qt::CheckState checkState_;

public:
    declareFactoryTable(
        IQResultElement,
        LIST(QObject* parent, const QString& label, insight::ResultElementPtr rep),
        LIST(parent, label, rep)
        );

public:
    declareType("IQResultElement");

    IQResultElement(QObject* parent, const QString& label, insight::ResultElementPtr rep);
    IQResultElement* parentResultElement() const;
    QList<IQResultElement*> children_;

    QString label_;

    insight::ResultElement* resultElement() const;

    template<class T>
    T* resultElementAs() const
    {
      return dynamic_cast<T*>(resultElement_.get());
    }


    virtual QVariant previewInformation(int role) const =0;
    virtual void createFullDisplay(QVBoxLayout* layout);
    virtual void resetContents(int width, int height);

    Qt::CheckState isChecked() const;
    void setChecked( Qt::CheckState cs );
};




class TOOLKIT_GUI_EXPORT IQRootResultElement
 : public IQResultElement
{
    Q_OBJECT

public:
    declareType("IQRootResultElement");

    IQRootResultElement(QObject* parent, const QString& label);

    QVariant previewInformation(int role) const override;
    void createFullDisplay(QVBoxLayout* layout) override;
};




class TOOLKIT_GUI_EXPORT IQStaticTextResultElement
 : public IQResultElement
{
    Q_OBJECT

    QString staticText_, staticDetailText_;

public:
    declareType("IQStaticTextResultElement");

    IQStaticTextResultElement(QObject* parent, const QString& label, const QString& staticText, const QString& staticDetailText);

    QVariant previewInformation(int role) const override;
    void createFullDisplay(QVBoxLayout* layout) override;
};




class TOOLKIT_GUI_EXPORT IQResultSetModel
        : public QAbstractItemModel
{
    Q_OBJECT

    void addResultElements(const ResultElementCollection& rec, IQResultElement* parent);
    ResultSetPtr orgResultSet_;
    IQResultElement* root_;
    bool selectableElements_;


    void updateParentCheckState(const QModelIndex &idx);
    void setChildrenCheckstate(const QModelIndex& idx, bool checked);

public:

    IQResultSetModel(ResultSetPtr resultSet, bool selectableElements=false, QObject* parent=nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const  override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orient, int role) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    void addChildren(const QModelIndex& pidx, insight::ResultElementCollection* re) const;
    ResultSetPtr filteredResultSet() const;

    insight::ResultSetPtr resultSet() const;
};




void connectToCWithContentsDisplay(QTreeView* ToCView, QWidget* content);




}


#endif // QRESULTSETMODEL_H
