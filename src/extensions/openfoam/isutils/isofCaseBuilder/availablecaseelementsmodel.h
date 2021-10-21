#ifndef AVAILABLECASEELEMENTSMODEL_H
#define AVAILABLECASEELEMENTSMODEL_H

#include <QAbstractItemModel>
#include <QIcon>



class AvailableCaseElementOrCategory : public QObject
{
  Q_OBJECT

public:
  enum Type {
    Category, CaseElement
  };

  QString name_;
  QIcon icon_;
  Type type_;
  QList<AvailableCaseElementOrCategory*> subElements_;

  AvailableCaseElementOrCategory(QString name, AvailableCaseElementOrCategory* parent, Type t = Category);
  AvailableCaseElementOrCategory* parentCategory() const;
  AvailableCaseElementOrCategory* addCaseElement(QString name, QIcon icon, Type t = CaseElement);
  AvailableCaseElementOrCategory* findChild(const QString& childName, bool createIfNonexisting);

  inline QString fullName() const {
    if (const auto* pe=parentCategory())
    {
      return pe->fullName()+"/"+name_;
    }
    else
      return name_;
  }
};




class AvailableCaseElementsModel : public QAbstractItemModel
{
  Q_OBJECT

  AvailableCaseElementOrCategory topCategory_;

public:
  explicit AvailableCaseElementsModel(QObject *parent = nullptr);

  // Header:
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  // Basic functionality:
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &index) const override;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  std::string selectedCaseElementTypeName(const QModelIndex& index) const;

private:
};

#endif // AVAILABLECASEELEMENTSMODEL_H
