#ifndef QRESULTSETMODEL_H
#define QRESULTSETMODEL_H

#include <QAbstractItemModel>

class QVBoxLayout;
class QTextEdit;
class QTreeView;

#include "base/resultset.h"
#include "base/factory.h"

namespace insight
{




class ResizeEventNotifier
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




class QResultElement
 : public QObject
{
    Q_OBJECT

  insight::ResultElementPtr resultElement_;

  QTextEdit *shortDesc_, *longDesc_;

public:
    declareFactoryTable(
        QResultElement,
        LIST(QObject* parent, const QString& label, insight::ResultElementPtr rep),
        LIST(parent, label, rep)
        );

public:
    declareType("QResultElement");

    QResultElement(QObject* parent, const QString& label, insight::ResultElementPtr rep);
    QResultElement* parentResultElement() const;
    QList<QResultElement*> children_;

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
};




class QRootResultElement
 : public QResultElement
{
    Q_OBJECT

public:
    declareType("QRootResultElement");

    QRootResultElement(QObject* parent, const QString& label);

    QVariant previewInformation(int role) const override;
    void createFullDisplay(QVBoxLayout* layout) override;
};



class QStaticTextResultElement
 : public QResultElement
{
    Q_OBJECT

    QString staticText_, staticDetailText_;

public:
    declareType("QStaticTextResultElement");

    QStaticTextResultElement(QObject* parent, const QString& label, const QString& staticText, const QString& staticDetailText);

    QVariant previewInformation(int role) const override;
    void createFullDisplay(QVBoxLayout* layout) override;
};



class QResultSetModel
        : public QAbstractItemModel
{
    Q_OBJECT

    void addResultElements(const ResultElementCollection& rec, QResultElement* parent);
    QResultElement* root_;

public:

    QResultSetModel(ResultSetPtr resultSet, QObject* parent=nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const  override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orient, int role) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};

void connectToCWithContentsDisplay(QTreeView* ToCView, QWidget* content);

}


#endif // QRESULTSETMODEL_H
