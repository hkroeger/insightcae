#include "iqresultsetmodel.h"
#include "base/resultelement.h"


#include <QLabel>
#include <QVBoxLayout>
#include <QEvent>
#include <QResizeEvent>
#include <QTextEdit>
#include <QTreeView>
#include <QSpacerItem>
#include <QDebug>
#include <qnamespace.h>

namespace insight
{




ResizeEventNotifier::ResizeEventNotifier(QObject* parent)
  : QObject(parent)
{
}




bool ResizeEventNotifier::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::Resize && obj==parent())
  {
      QResizeEvent *ev = static_cast<QResizeEvent*>(event);
      auto s=ev->size();
      emit resized(s.width(), s.height());
  }
  return QObject::eventFilter(obj, event);
}






defineType(IQResultElement);


IQHierarchicalDataElement *
IQResultElement::createForChild(
    IQHierarchicalDataModel *model,
    hierarchicalData::Element *e )
{
    IQHierarchicalDataElement *ne{ nullptr };

    if (IQHierarchicalDataElement::has_factory(e->type()))
    {
        ne = IQHierarchicalDataElement::lookup(e->type(), this, model, e);
    }
    else
    {
        ne = new IQStaticTextResultElement(
            this,
            "",
            "(There is currently no viewer implemented for this result element.)",
            model, e);
    }

    ne->connectSignals();

    return ne;
}

IQResultElement::IQResultElement(
    QObject* parent,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element )
    : IQHierarchicalDataElement(parent, hdmodel, element),
      checkState_(Qt::Checked)
{}




void IQResultElement::createFullDisplay(QVBoxLayout* layout)
{
    auto &resElem=elementAs<ResultElement>();
    layout->addWidget(new QLabel("<b>"+name()+"</b>"));


    if (!resElem.shortDescription().empty())
    {
        shortDesc_=new IQSimpleLatexView(
            resElem.shortDescription() );
        layout->addWidget(shortDesc_);
    }
    if (!resElem.longDescription().empty())
    {
        longDesc_=new IQSimpleLatexView(
            resElem.longDescription() );
        layout->addWidget(longDesc_);
    }
}




Qt::CheckState IQResultElement::isChecked() const
{
    return checkState_;
}

void IQResultElement::setChecked( Qt::CheckState cs )
{
    checkState_=cs;
}









defineType(IQStaticTextResultElement);




IQStaticTextResultElement::IQStaticTextResultElement
(
    QObject *parent,
    const QString &staticText,
    const QString& staticDetailText,
    IQHierarchicalDataModel* hdmodel,
    insight::hierarchicalData::Element* element
)
  : IQResultElement(parent, hdmodel, element),
    staticText_(staticText),
    staticDetailText_(staticDetailText)
{}




QVariant IQStaticTextResultElement::previewInformation(int role) const
{
  if (role==Qt::DisplayRole) return staticText_;

  return QVariant();
}




void IQStaticTextResultElement::createFullDisplay(QVBoxLayout* layout)
{
  layout->addWidget(new QLabel(name()));
  layout->addWidget(new QLabel(staticDetailText_));
}








// void IQResultSetModel::addResultElements(const ResultElementCollection &rec, IQResultElement *parent)
// {

//   // sort according to stored "order" field
//   std::vector<std::pair<ResultElementCollection::key_type,ResultElementCollection::mapped_type> > sortedrec;
//   std::copy(rec.ResultElementMap::begin(), rec.ResultElementMap::end(),
//             back_inserter(sortedrec));

//   std::sort
//   (
//       sortedrec.begin(), sortedrec.end(),
//       [] ( const ResultElementCollection::value_type &left, const ResultElementCollection::value_type &right ) {
//             return left.second->order() < right.second->order();
//         }
//   );

//   for ( const auto& re: sortedrec )
//   {
//       IQResultElement *curnode;
//       QString label=QString::fromStdString(re.first);

//       try
//       {
//         curnode = IQResultElement::lookup
//             (
//               re.second->type(),
//               parent,
//               label,
//               re.second
//             );
//       }
//       catch (const std::exception& e) {
//         curnode=new IQStaticTextResultElement(
//               parent, label,
//               "",
//               "(There is currently no viewer implemented for this result element.)",
//               re.second
//               //QString::fromStdString(e.what())
//               );
//       }

//       parent->children_.append(curnode);

//       if (const auto *sec = dynamic_cast<ResultElementCollection*>(re.second.get()))
//       {
//           addResultElements(*sec, curnode);
//       }
//   }
// }




void IQResultSetModel::setCheckState(const QModelIndex &idx, bool checked)
{
    if (auto *e = dynamic_cast<IQResultElement*>(
                static_cast<QObject*>(idx.internalPointer())))
    {
        e->setChecked( checked?Qt::Checked:Qt::Unchecked );
        emit dataChanged(idx, idx);
        setChildrenCheckstate( idx, checked );
        updateParentCheckState( idx );
    }
}



void IQResultSetModel::updateParentCheckState(const QModelIndex &idx)
{
    auto pidx=parent(idx);
    if (pidx.isValid())
    {
        if (auto *e = dynamic_cast<IQResultElement*>(
                    static_cast<QObject*>( pidx.internalPointer())))
        {
            int nChecked=0, nUnchecked=0, nPartChecked=0;
            for (int row=0; row<rowCount(pidx); ++row)
            {
                auto cidx=index(row, 0, pidx);
                if (auto *c = dynamic_cast<IQResultElement*>(
                            static_cast<QObject*>( cidx.internalPointer())))
                {
                    switch(c->isChecked())
                    {
                        case Qt::Checked: nChecked++; break;
                        case Qt::Unchecked: nUnchecked++; break;
                        case Qt::PartiallyChecked: nPartChecked++; break;
                    }
                }
            }

            auto oldcs=e->isChecked();
            Qt::CheckState newcs=oldcs;
            if (nChecked>0 && nUnchecked==0 && nPartChecked==0)
                newcs=Qt::Checked;
            else if (nUnchecked>0 && nChecked==0 && nPartChecked==0)
                newcs=Qt::Unchecked;
            else
                newcs=Qt::PartiallyChecked;
            insight::dbg()<<oldcs<<" "<<nChecked<<" "<<nUnchecked<<" "<<nPartChecked<<" "<<newcs<<std::endl;
            if (newcs!=oldcs)
            {
                e->setChecked(newcs);
                emit dataChanged(pidx, pidx);
                updateParentCheckState(pidx);
            }
        }
    }
}




void IQResultSetModel::setChildrenCheckstate(const QModelIndex& idx, bool checked)
{
    for (int row=0; row<rowCount(idx); ++row)
    {
        auto cidx=index(row, 0, idx);
        if (auto *c = dynamic_cast<IQResultElement*>(
                    static_cast<QObject*>( cidx.internalPointer())))
        {
            c->setChecked( checked?Qt::Checked:Qt::Unchecked );
            emit dataChanged(cidx, cidx);
            setChildrenCheckstate(cidx, checked);
        }
    }
}



IQResultSetModel::IQResultSetModel(
    std::unique_ptr<ResultSet> resultSet,
    bool selectableElements,
    QObject* parent )
    : IQHierarchicalDataModel(std::move(resultSet), parent),
    selectableElements_(selectableElements)
{
    // root_=new IQRootResultElement(this, QString::fromStdString(resultSet->title()));
    // addResultElements(*resultSet, root_);
}




// QModelIndex IQResultSetModel::index(int row, int column, const QModelIndex &parent) const
// {
//   if (!parent.isValid())
//   {
//     if (row==0)
//     {
//       return createIndex( row, column, root_ );
//     }
//   }
//   else
//   {
//     auto *e=static_cast<IQResultElement*>(parent.internalPointer());

//     if (row>=0 && row < e->children_.size())
//     {
//       return createIndex( row, column, e->children_.at(row) );
//     }
//   }

//   return QModelIndex();
// }




// QModelIndex IQResultSetModel::parent(const QModelIndex &index) const
// {
//   if (auto *e=static_cast<IQResultElement*>(index.internalPointer()))
//   {
//       if (auto pe=e->parentResultElement())
//       {
//         if (auto ppe=pe->parentResultElement())
//         {
//           return createIndex(ppe->children_.indexOf(pe), 0, pe);
//         }
//         else
//         {
//           return createIndex(0, 0, root_);
//         }
//       }
//   }
//   return QModelIndex();
// }





QVariant IQResultSetModel::headerData(int section, Qt::Orientation orient, int role) const
{
    if (role==Qt::DisplayRole)
    {
        if (orient==Qt::Horizontal)
        {
            switch (section)
            {
            case 0: return QVariant("Result element label");
            case 1: return QVariant("Summary");
            }
        }
    }
    return QVariant();
}




QVariant IQResultSetModel::data(const QModelIndex &index, int role) const
{
    if (auto *e = dynamic_cast<const IQResultElement*>(iqElementOfIndex(index)))
    {
        if (role == Qt::CheckStateRole)
        {
            if ( index.column()==0 && selectableElements_)
            {
                return QVariant( e->isChecked() );
            }
        }
        else
        {
            if (index.column()==1)
            {
                auto pe=e->previewInformation(role);
                return pe;
            }
        }
    }

    return IQHierarchicalDataModel::data(index, role);
}




Qt::ItemFlags IQResultSetModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    Qt::ItemFlags flags = IQHierarchicalDataModel::flags(index);

    if ( index.column() == 0 && selectableElements_ )
        flags |= Qt::ItemIsUserCheckable;

    return flags;
}




bool IQResultSetModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole && index.column()==0)
     {
         setCheckState(index, value.toBool()?Qt::Checked:Qt::Unchecked);
         return true;
     }

     return false;
}

IQResultElement *IQResultSetModel::getResultElement(const QModelIndex &idx)
{
    return
        dynamic_cast<IQResultElement*>(
         iqElementOfIndex(idx) );
}




// std::string IQResultSetModel::path(const QModelIndex &idx) const
// {
//     std::string p;
//     if (auto e = dynamic_cast<IQResultElement*>(
//                 static_cast<QObject*>(idx.internalPointer())))
//     {
//         QModelIndex pidx = parent(idx);
//         if (pidx.isValid() && pidx.internalPointer()!=root_ ) p = path(pidx);
//         p = ( p.empty() ? "" : (p+"/") ) + e->label_.toStdString();
//     }
//     return p;
// }




//void IQResultSetModel::addChildren(const QModelIndex& pidx, ResultElementCollection* re) const
//{
//    for (int row=0; row<rowCount(pidx); ++row)
//    {
//        auto cidx = index(row, 0, pidx);
//        if (auto e = dynamic_cast<IQResultElement*>(
//                    static_cast<QObject*>(cidx.internalPointer())))
//        {
//            if (e->isChecked()==Qt::Checked || e->isChecked()==Qt::PartiallyChecked)
//            {
//                auto toBeInserted = e->resultElement()->clone();
//                if ( auto rec =
//                     dynamic_cast<insight::ResultElementCollection*>(toBeInserted.get()) )
//                {
//                    rec->clear();
//                    addChildren(cidx, rec);
//                }
//                re->insert( e->label_.toStdString(),  toBeInserted);
//            }
//        }
//    }
//}

//ResultSetPtr IQResultSetModel::filteredResultSet() const
//{
//    std::string author = orgResultSet_->author();
//    std::string date = orgResultSet_->date();
//    auto fr = std::make_shared<ResultSet>(
//                orgResultSet_->parameters(),
//                orgResultSet_->title(),
//                orgResultSet_->subtitle(),
//                &author, &date
//                );
//    addChildren( index(0,0), fr.get() );
//    return fr;
//}




const insight::ResultSet& IQResultSetModel::resultSet() const
{
    return dynamic_cast<const insight::ResultSet&>(
        getHierarchicalData() );
}



void IQResultSetModel::addUnselectedElementPaths(
        const QModelIndex& pidx,
        ResultSetFilter& filter,
        std::string parentPath) const
{
    for (int row=0; row<rowCount(pidx); ++row)
    {
        auto cidx = index(row, 0, pidx);
        if (auto e = iqElementOfIndex(cidx))
        {
            auto path = (*e)->path();
            if ( data(cidx.siblingAtColumn(0), Qt::CheckStateRole)
                    .value<Qt::CheckState>() == Qt::Unchecked )
            {
                filter.insert(path);
            }
            else
            {
                if ( rowCount(cidx)>0 )
                {
                    addUnselectedElementPaths(cidx, filter, path);
                }
            }
        }
    }
}


ResultSetFilter IQResultSetModel::filter() const
{
    ResultSetFilter f;
    addUnselectedElementPaths(index(0,0), f, "");
    return f;
}




void IQResultSetModel::unselectElements(const QModelIndex& pidx,
                                        const ResultSetFilter& filter,
                                        std::string parentPath)
{
    for (int row=0; row<rowCount(pidx); ++row)
    {
        auto cidx = index(row, 0, pidx);
        if (auto e = iqElementOfIndex(cidx))
        {
            auto path = (*e)->path();
            if ( filter.matches(path) )
            {
                setCheckState(cidx, false);
            }
            else
            {
                if ( rowCount(cidx)>0 )
                {
                    unselectElements(cidx, filter, path);
                }
            }
        }
    }
}


void IQResultSetModel::resetFilter(const ResultSetFilter& filter)
{
    unselectElements(index(0,0), filter, "");
}


void connectToCWithContentsDisplay(QTreeView* ToCView, QWidget* contentDisplayWidget)
{

  QObject::connect(ToCView, &QTreeView::clicked,
          [contentDisplayWidget,ToCView](const QModelIndex& i)
  {
    if (auto *rm = dynamic_cast<IQResultSetModelBase*>(ToCView->model()))
    {
        if (auto re = rm->getResultElement(i) )
        {
          qDeleteAll( contentDisplayWidget->children() );
          if (auto *l = contentDisplayWidget->layout())
          {
              delete l;
          }

          QVBoxLayout* layout=new QVBoxLayout(contentDisplayWidget);
          contentDisplayWidget->setLayout(layout);

          re->createFullDisplay(layout);

          layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        }
    }
  }
  );
}





IQFilteredResultSetModel::IQFilteredResultSetModel(QObject *parent)
{}




IQResultElement *IQFilteredResultSetModel::getResultElement(const QModelIndex &idx)
{
    if ( auto *rsm =
            dynamic_cast<IQResultSetModel*>(sourceModel()) )
    {
        return rsm->getResultElement( mapToSource(idx) );
    }
    return nullptr;
}



void IQFilteredResultSetModel::resetFilter(const ResultSetFilter &filter)
{
    filter_=filter;
    invalidateFilter();
}


void IQFilteredResultSetModel::addChildren(
        const QModelIndex& pidx,
        insight::ResultElementCollection* re) const
{
//     for (int row=0; row<rowCount(pidx); ++row)
//     {
//         auto cidx = index(row, 0, pidx);
//         auto scidx = mapToSource(cidx);
//         if (auto e = dynamic_cast<IQResultElement*>(
//                     static_cast<QObject*>(scidx.internalPointer())))
//         {
// //            if (e->isChecked()==Qt::Checked || e->isChecked()==Qt::PartiallyChecked)
//             {
//                 auto toBeInserted = (*e)->cloneAs<ResultElement>();
//                 if ( auto rec =
//                      dynamic_cast<insight::ResultElementCollection*>(toBeInserted.get()) )
//                 {
//                     rec->clear();
//                     addChildren(cidx, rec);
//                 }
//                 re->insert( e->name().toStdString(),  std::move(toBeInserted));
//             }
//         }
//     }
}

ResultSetPtr IQFilteredResultSetModel::filteredResultSet() const
{
    auto *orgResultModel =
               dynamic_cast<IQResultSetModel*>(sourceModel());
    auto &orgResultSet = orgResultModel->resultSet();

    std::string author = orgResultSet.author();
    std::string date = orgResultSet.date();
    auto fr = std::make_unique<ResultSet>(
                orgResultSet.parameters().cloneAs<ParameterSet>(),
                orgResultSet.title(),
                orgResultSet.subtitle(),
                &author, &date
                );
    addChildren( index(0,0), fr.get() );
    return fr;
}


bool IQFilteredResultSetModel::filterAcceptsRow(
        int sourceRow,
        const QModelIndex &sourceParent ) const
{
    if ( auto *rsm =
            dynamic_cast<IQResultSetModel*>(sourceModel()) )
    {
        QModelIndex index0 = rsm->index(sourceRow, 0, sourceParent);

        bool isDisplayed = !filter_.matches(
            rsm->elementOfIndex(index0)->path() );

        if ( sourceParent.isValid() )
        {
            QModelIndex pindex0 = rsm->parent(index0);
            isDisplayed = isDisplayed
                    && filterAcceptsRow(pindex0.row(), rsm->parent(pindex0));
        }
        return isDisplayed;
    }
    return true;
}




}
