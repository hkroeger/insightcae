#include "availablecaseelementsmodel.h"

#include "cadparametersetvisualizer.h"
#include "openfoam/caseelements/openfoamcaseelement.h"




AvailableCaseElementOrCategory::AvailableCaseElementOrCategory(
    QString name,
    AvailableCaseElementOrCategory* parent,
    Type t )
  : QObject(parent), name_(name), type_(t)
{}




AvailableCaseElementOrCategory* AvailableCaseElementOrCategory::parentCategory() const
{
  return dynamic_cast<AvailableCaseElementOrCategory*>(parent());
}




AvailableCaseElementOrCategory* AvailableCaseElementOrCategory::addCaseElement(
    QString name,
    QIcon icon,
    Type t)
{
  auto *ce = new AvailableCaseElementOrCategory(name, this, t);
  ce->icon_=icon;
  subElements_.append(ce);
//  if (t==CaseElement)
//    std::cout<<ce->fullName().toStdString()<<std::endl;
  return ce;
}




AvailableCaseElementOrCategory* AvailableCaseElementOrCategory::findChild(
    const QString& childName,
    bool createIfNonexisting )
{
  AvailableCaseElementOrCategory* c=nullptr;
  for (auto *cc: subElements_)
  {
    if (cc->name_==childName)
    {
      c=cc;
      break;
    }
  }
  if (!c && createIfNonexisting)
  {
    c=addCaseElement(childName, QIcon(), Category);
  }

  return c;
}







AvailableCaseElementsModel::AvailableCaseElementsModel(QObject *parent)
  : QAbstractItemModel(parent),
    topCategory_("", nullptr)
{
  topCategory_.findChild("Uncategorized", true);

  for (
       auto i = insight::OpenFOAMCaseElement::factories_->begin();
       i != insight::OpenFOAMCaseElement::factories_->end();
       i++
       )
  {
    std::string elemName = i->first;
    std::string category = insight::OpenFOAMCaseElement::categoryFor (elemName);
    QStringList path = QString::fromStdString(category)
                        .split ( "/", QString::SkipEmptyParts );

//    std::cout<<category<<" "<<elemName<<std::endl;

    AvailableCaseElementOrCategory *pe = &topCategory_;
    for ( auto pit = path.constBegin(); pit != path.constEnd(); ++pit )
    {
      pe = pe->findChild( *pit, true );
    }

    {
      QIcon icon;

        if (insight::CADParameterSetModelVisualizer::iconForOpenFOAMCaseElement_table().count(elemName))
      {
          icon =
              insight::CADParameterSetModelVisualizer::iconForOpenFOAMCaseElement(elemName);
      }

      pe->addCaseElement( QString::fromStdString(elemName), icon );
    }
  }

//  for (const auto& e: topCategory_.subElements_)
//  {
//    std::cout<<e->name_.toStdString()<<std::endl;
//  }
}

QVariant AvailableCaseElementsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation==Qt::Horizontal && role==Qt::DisplayRole)
  {
    switch (section)
    {
      case 0: return "Case element type name";
    }
  }
  return QVariant();
}




QModelIndex AvailableCaseElementsModel::index(int row, int column, const QModelIndex &parent) const
{
  const AvailableCaseElementOrCategory* parentItem;

  if ( !parent.isValid() )
      parentItem = &topCategory_;
  else
      parentItem =
          static_cast<const AvailableCaseElementOrCategory*>(
            parent.internalPointer() );

  if ( (row>=0) && (row < parentItem->subElements_.size()) )
  {
    AvailableCaseElementOrCategory *childItem = parentItem->subElements_[ row ];
    return createIndex( row, column, childItem );
  }
  return QModelIndex();
}




QModelIndex AvailableCaseElementsModel::parent(const QModelIndex &index) const
{
  if ( !index.isValid() )
      return QModelIndex();

  auto *childItem =
      static_cast<const AvailableCaseElementOrCategory*>(
        index.internalPointer() );

  auto *parentItem = childItem->parentCategory();

  if ( parentItem == &topCategory_ )
      return QModelIndex();

  return createIndex(
        parentItem->subElements_.indexOf(
          const_cast< AvailableCaseElementOrCategory*>(childItem) ),
        0,
        parentItem );
}




int AvailableCaseElementsModel::rowCount(const QModelIndex &parent) const
{
  const AvailableCaseElementOrCategory * parentItem;

  if ( !parent.isValid() )
      parentItem = &topCategory_;
  else
      parentItem = static_cast<const AvailableCaseElementOrCategory*>( parent.internalPointer() );

  return parentItem->subElements_.size();
}




int AvailableCaseElementsModel::columnCount(const QModelIndex &parent) const
{
  return 1;
}




QVariant AvailableCaseElementsModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  auto *item = static_cast<AvailableCaseElementOrCategory*>(
        index.internalPointer() );

  switch (role)
  {
    case Qt::DisplayRole:
      return item->name_;
    case Qt::DecorationRole:
      return item->icon_;
  }

  return QVariant();
}




std::string AvailableCaseElementsModel::selectedCaseElementTypeName(const QModelIndex &index) const
{
  if (auto *item = static_cast<AvailableCaseElementOrCategory*>(
        index.internalPointer() ))
  {
    if (item->type_ == AvailableCaseElementOrCategory::CaseElement)
    {
      return item->name_.toStdString();
    }
  }
  return std::string();
}
