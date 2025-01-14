#include <QObject>
#include <QDebug>
#include <QLayout>
#include <QPushButton>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <iterator>

#include "iqparametersetmodel.h"
#include "base/parameter.h"
#include "base/parameters/arrayparameter.h"
#include "base/parameters/selectablesubsetparameter.h"
#include "base/parameters/subsetparameter.h"
#include "base/parameterset.h"
#include "iqparameter.h"
#include "iqparameters/iqarrayparameter.h"
#include "iqparameters/iqarrayelementparameter.h"
#include "iqparameters/iqlabeledarrayparameter.h"
#include "iqparameters/iqlabeledarrayelementparameter.h"

#include "base/parameters/simpleparameter.h"
#include "base/parameters/selectionparameter.h"

#include "cadparametersetvisualizer.h"

#include "base/rapidxml.h"
#include "rapidxml/rapidxml_print.hpp"



IQParameter *
IQParameterSetModel::findWrapper(const insight::Parameter &p) const
{
    auto i = std::find_if(
        wrappers_.begin(), wrappers_.end(),
        [&](decltype(wrappers_)::const_reference_type v)
        {
            return v.first.valid() && (v.first.get()==&p);
        }
        );
    if (i==wrappers_.end())
        return nullptr;
    else
        return const_cast<IQParameter*>(&(*i).first);
}




int IQParameterSetModel::countDisplayedChildren(const QModelIndex &index) const
{
    auto iqp=iqIndexData(index);
    int n=0;
    for (auto& c: iqp->children())
    {
        if (dynamic_cast<IQParameter*>(c))
            ++n;
    }
    return n;
}



insight::Parameter* IQParameterSetModel::indexData(const QModelIndex& idx)
{
    return static_cast<insight::Parameter*>(idx.internalPointer());
}



IQParameter *IQParameterSetModel::iqIndexData(const QModelIndex &idx)
{
    // create wrapper on the fly
    if (auto *p=indexData(idx))
    {
        IQParameter *wrapper = findWrapper(*p);
        if (!wrapper)
        {
            auto *model = const_cast<IQParameterSetModel*>(this);

            // use other wrapper, if its an array element
            if (p->hasParent() && dynamic_cast<insight::ArrayParameter*>(&p->parent()))
            {
                wrapper = IQArrayElementParameterBase::create(
                    model, model,
                    p, *defaultParameterSet_ );
            }
            else
            {
                wrapper = IQParameter::create(
                    model, model,
                    p, *defaultParameterSet_ );
            }

            wrappers_.insert( wrapper, 0 );
        }

        return wrapper;
    }

    return nullptr;
}



const IQParameter *IQParameterSetModel::iqIndexData(const QModelIndex &idx) const
{
    return const_cast<IQParameterSetModel*>(this)
        ->iqIndexData(idx);
}



IQParameterSetModel::IQParameterSetModel(
    const insight::ParameterSet& ps,
    boost::optional<const insight::ParameterSet&> defaultps,
    QObject *parent
    )
  : QAbstractItemModel(parent)
{
  resetParameters( ps, defaultps ? *defaultps : ps );
}



const insight::Parameter *IQParameterSetModel::visibleParent(
    const insight::Parameter &p, int& row ) const
{
    const insight::Parameter *pp = nullptr;
    if (p.hasParent())
    {
        pp=&p.parent();
        row=pp->childParameterIndex(&p);

        // is parent is subset and parents parent is selectablesubet,
        // then redirect
        if (dynamic_cast<const insight::ParameterSet*>(pp))
        {
            if (pp->hasParent())
            {
                if (dynamic_cast<const insight::SelectableSubsetParameter*>(&pp->parent()))
                {
                    pp=&pp->parent();
                }
            }
        }
    }
    else
    {
        row=parameterSet_->childParameterIndex(&p);
    }
    return pp;
}



QModelIndex IQParameterSetModel::indexFromParameter(const insight::Parameter &p, int col) const
{
    auto *model = const_cast<IQParameterSetModel*>(this);
    auto *parameter = const_cast<insight::Parameter*>(&p);

    const insight::Parameter *pp = parameterSet_.get();

    if (parameter==pp) return QModelIndex();

    int row=-1;
    if (p.hasParent())
    {
        pp=visibleParent(p, row);
    }

    return createIndex(
        row, col, parameter );
}




QModelIndex IQParameterSetModel::indexFromParameterPath(const std::string &path, int col) const
{
    return indexFromParameter(
        parameterSet_->get<insight::Parameter>(path), col );
}



int IQParameterSetModel::columnCount(const QModelIndex &/*parent*/) const
{
  return 2;
}

QVariant IQParameterSetModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role==Qt::DisplayRole)
  {
    if (orientation==Qt::Horizontal)
    {
      if (section==0)
        return QString("Name");
      else if (section==1)
        return QString("Value");
    }
  }

  return QVariant();
}




int IQParameterSetModel::rowCount(const QModelIndex &parent) const
{
  int s=0;
  if (!parent.isValid())
  {
    s=parameterSet_->nChildren();
  }
  else if (const auto* p=indexData(parent))
  {
      if (parent.column()==0)
      {
          s = p->nChildren(); // only first column has children
      }
  }

  return s;
}




QModelIndex IQParameterSetModel::index(int row, int column, const QModelIndex &parent) const
{
    QModelIndex i;

    if (!parent.isValid()) // in root dict
    {
        if (row>=0 && row<parameterSet_->nChildren())
        {
            i=indexFromParameter(
                parameterSet_->childParameter(row), column );
        }
    }
    else if (auto* p=indexData(parent)) // down in tree
    {
        if ( (row>=0) && (row<p->nChildren()) )
        {
            i=indexFromParameter(
                p->childParameter(row), column );
        }
    }

    return i;
}




QModelIndex IQParameterSetModel::parent(const QModelIndex &index) const
{
    QModelIndex i;
    int row;

    if ( index.isValid() )
    {
        if (auto* p = indexData(index))
        {
            if (auto *pp=visibleParent(*p, row))
            {
                i = indexFromParameter(*pp, 0);
            }
        }
    }

    return i;
}




Qt::ItemFlags IQParameterSetModel::flags(const QModelIndex &index) const
{
  if (!index.isValid())
  {
    return 0;
  }

  auto flags=QAbstractItemModel::flags(index);

  auto *iqp=iqIndexData(index);
  auto *p=iqp->get();


  flags |= Qt::ItemIsDragEnabled;
  flags |= Qt::ItemIsDropEnabled;

  if (index.column()==valueCol)
  {
    if (dynamic_cast<const insight::DoubleParameter*>(p)
        ||dynamic_cast<const insight::IntParameter*>(p)
        ||dynamic_cast<const insight::StringParameter*>(p)
        )
        flags |= Qt::ItemIsEditable;

    if (dynamic_cast<const insight::BoolParameter*>(p))
        flags |= Qt::ItemIsUserCheckable;

    if (IQParameterGridViewDelegateEditorWidget::has_createDelegate(iqp->type()))
    {
        flags |= Qt::ItemIsEditable;
    }
  }

  return flags;
}




Qt::DropActions IQParameterSetModel::supportedDropActions() const
{
  return Qt::CopyAction | Qt::MoveAction;
}




QStringList IQParameterSetModel::mimeTypes() const
{
  return { "application/xml" };
}




QMimeData *IQParameterSetModel::mimeData(const QModelIndexList &indexes) const
{
  auto* mimeData = new QMimeData();

  std::ostringstream os;
  using namespace rapidxml;
  xml_document<> doc;
  xml_node<>* decl = doc.allocate_node(node_declaration);
  decl->append_attribute(doc.allocate_attribute("version", "1.0"));
  decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
  doc.append_node(decl);
  xml_node<> *rootnode = doc.allocate_node(node_element, "root");
  doc.append_node(rootnode);
  for (const auto& index: indexes)
  {
    if (index.column()==0) // one index per col is issued
    {
        auto *ip=indexData(index);

        ip->appendToNode(ip->name(), doc, *rootnode, "");
    }
  }
  os << doc;

  mimeData->setData(
      "application/xml",
      QByteArray::fromStdString(os.str()) );

  return mimeData;
}




bool IQParameterSetModel::dropMimeData(
    const QMimeData *data,
    Qt::DropAction action,
    int row, int column,
    const QModelIndex &parent )
{
  // if (action == Qt::IgnoreAction)
  // {
  //   return true;
  // }
  // else if (action == Qt::MoveAction || action == Qt::CopyAction)
  // {

  //   // parse, what we got
  //   std::string contents(data->data("application/xml").toStdString());
  //   using namespace rapidxml;
  //   xml_document<> doc;
  //   doc.parse<0>(&contents[0]);
  //   xml_node<> *rootnode = doc.first_node("root");

  //   std::string allTypesEqual(rootnode->first_node()->name());
  //   int nArgs = 0;
  //   for (auto *e = rootnode->first_node(); e!=nullptr; e=e->next_sibling())
  //   {
  //       std::string type(e->name());
  //       nArgs++;
  //       if (type!=allTypesEqual) allTypesEqual="";
  //   }

  //   insight::ArrayParameter *iap = nullptr;
  //   if (auto *ip=indexData(parent))
  //   {
  //       iap = dynamic_cast<insight::ArrayParameter*>(ip);
  //   }

  //   if (iap && allTypesEqual==iap->defaultValue().type())
  //   {
  //       // Parent is Array and all given parameters are of the same and child type of array? -> inserted/append children
  //       std::vector<insight::ParameterPtr> args;
  //       for (auto *e = rootnode->first_node(); e!=nullptr; e=e->next_sibling())
  //       {
  //           insight::ParameterPtr np(
  //               iap->defaultValue().clone() );

  //           np->readFromNode(
  //               e->first_attribute("name")->value(),
  //               *rootnode,
  //               ""
  //               );

  //           args.push_back(np);
  //       }

  //       auto row=parent.row();
  //       if (row<0) // append
  //       {
  //           for (auto& arg: args)
  //           {
  //               appendArrayElement(parent, *arg);
  //           }
  //           return true;
  //       }
  //       else if (row>=0)
  //       {
  //           // insert in reverse order
  //           std::reverse(args.begin(), args.end());
  //           for (auto& arg: args)
  //           {
  //               insertArrayElement(parent, *arg);
  //           }
  //           return true;
  //       }
  //   }
  //   else if (nArgs==1)
  //   {
  //       // expect a single given item of same type as indexed parameter
  //       auto targetIndex = index(row, column, parent);
  //       if (!targetIndex.isValid())
  //           targetIndex=parent;

  //       auto *ip=indexData(targetIndex);
  //       if (allTypesEqual==ip->type())
  //       {
  //           insight::ParameterPtr np(iqp->parameter().clone());
  //           np->readFromNode(
  //                   rootnode->first_node()->first_attribute("name")->value(),
  //                   *rootnode,
  //                   ""
  //               );
  //           iqp->parameterRef().copyFrom( *np );
  //           return true;
  //       }
  //   }


  // }

  return false;
}




QVariant IQParameterSetModel::data(const QModelIndex &index, int role) const
{
  if (auto *iqp = iqIndexData(index))
  {
    auto *p=iqp->get();

    switch (role)
    {

      case Qt::DisplayRole:
      case Qt::EditRole:
        if (index.column()==labelCol) // name
        {
          return QString::fromStdString(p->name());
        }
        else if (index.column()==valueCol) // data
        {
            if (auto *sp = dynamic_cast<const insight::SelectionParameter*>(p))
                return QString::fromStdString(sp->selection());
          else if (auto *dp = dynamic_cast<const insight::DoubleParameter*>(p))
                return dp->operator()();
          else if (auto *ip = dynamic_cast<const insight::IntParameter*>(p))
                return ip->operator()();
          else if (auto *sp = dynamic_cast<const insight::StringParameter*>(p))
                return QString::fromStdString(sp->operator()());
          else if (auto *bp = dynamic_cast<const insight::BoolParameter*>(p))
                return QVariant(); //handled in checkstaterole below //dp->operator()();
          else
                return iqp->valueText();
        }
        else if (index.column()==stringPathCol) // full path (hidden)
        {
          return QString::fromStdString(p->path());
        }
        else if (index.column()==iqParamCol) // pointer to decorator (hidden)
        {
          return QVariant::fromValue(static_cast<void*>(const_cast<IQParameter*>(iqp)));
        }
        break;

      case Qt::CheckStateRole:
        if (index.column()==valueCol) // data
        {
          if (auto *bp = dynamic_cast<const insight::BoolParameter*>(p))
                return bp->operator()() ? Qt::Checked : Qt::Unchecked;
        }
        break;

      case Qt::BackgroundRole:
        return iqp->backgroundColor();
        break;

      case Qt::ForegroundRole:
        return iqp->textColor();
        break;

      case Qt::FontRole:
        return iqp->textFont();
        break;

    }
  }
  return QVariant();
}



bool IQParameterSetModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (auto *p = indexData(index))
  {
    switch (role)
    {

    case Qt::EditRole:
        if (index.column()==valueCol) // data
        {
            auto *iqp=iqIndexData(index);
            bool ok = iqp->setValue(value);
            if (ok)
            {
                Q_EMIT dataChanged(index, index, {role});
            }
            return ok;
        }
        break;

    case Qt::CheckStateRole:
        if (index.column()==valueCol) // data
        {
          if (auto *bp = dynamic_cast<insight::BoolParameter*>(p))
          {
            auto *iqp=iqIndexData(index);
            if (iqp->setValue(value))
            {
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
          }
        }
        break;
    }
  }
  return false;
}




void IQParameterSetModel::copy(const QModelIndexList &indexes) const
{
  QMimeData *mimeData = this->mimeData(indexes);
  qApp->clipboard()->setMimeData(mimeData);
}




void IQParameterSetModel::paste(const QModelIndexList &indexes)
{
  insight::assertion(indexes.size()==1, "only single paste op supported");

  const QMimeData *mimeData = qApp->clipboard()->mimeData();

  auto index=indexes.first();

  if (canDropMimeData(mimeData, Qt::CopyAction,
                      index.row(), 0, index.parent()))
  {
    dropMimeData(mimeData, Qt::CopyAction,
                 index.row(), 0, index.parent());
  }
}




void IQParameterSetModel::contextMenu(QWidget *pw, const QModelIndex& index, const QPoint &p)
{
  if (index.isValid())
  {
    if (auto* iqp=parameterFromIndex(index))
    {
      QMenu ctxMenu;
      iqp->populateContextMenu(&ctxMenu);

#warning reenable
//      // copy/paste
//      QAction *a;

//      ctxMenu.addSeparator();
//      a=new QAction("&Copy");
//      connect(a, &QAction::triggered, a,
//              [this,index]() { copy({index.siblingAtColumn(0)}); }
//      );
//      ctxMenu.addAction(a);
//      a=new QAction("&Paste");
//      if (qApp->clipboard()->mimeData()->formats().contains("application/xml"))
//      {
//       connect(a, &QAction::triggered, a,
//                  [this,index]() { paste({index.siblingAtColumn(0)}); }
//              );
//      }
//      else
//      {
//       a->setDisabled(true);
//      }
//      ctxMenu.addAction(a);

      ctxMenu.exec(pw->mapToGlobal(p));
    }
  }
}





// QList<IQParameter*> IQParameterSetModel::decorateChildren(QObject* parent, insight::Parameter& curParam)
// {
//   if (auto* ap = dynamic_cast<insight::ArrayParameterBase*>(&curParam))
//   {
//     return decorateArrayContent(parent, *ap);
//   }
//   else if (auto* ap = dynamic_cast<insight::LabeledArrayParameter*>(&curParam))
//   {
//       return decorateLabeledArrayContent(parent, *ap);
//   }
//   else
//   {
//     QList<IQParameter*> children;
//     for (auto childParam=curParam.begin(); childParam!=curParam.end(); ++childParam)
//     {
//       auto name = QString::fromStdString(childParam.name());
//       auto iqp = IQParameter::create(parent, this, name, *childParam, defaultParameterSet_);

//       decorateChildren(iqp, *childParam);
//       children.append(iqp);
//     }
//     if (auto* ciqp = dynamic_cast<IQParameter*>(parent))
//     {
//       ciqp->append(children);
//     }
//     return children;
//   }
// }





// IQParameter* IQParameterSetModel::decorateArrayElement(QObject* parent, int i, insight::Parameter& cp)
// {
//   auto name = QString("%1").arg(i);
//   auto iqp = IQArrayElementParameterBase::create(parent, this, name, cp, defaultParameterSet_);

//   decorateChildren(iqp, cp);
//   return iqp;
// }




// QList<IQParameter*> IQParameterSetModel::decorateArrayContent(QObject* parent, insight::ArrayParameterBase& ap)
// {
//   QList<IQParameter*> children;
//   for (int i=0; i<ap.size(); ++i)
//   {
//     auto iqp=decorateArrayElement(parent, i, ap.elementRef(i));
//     children.append(iqp);
//   }
//   if (IQParameter* ciqp = dynamic_cast<IQParameter*>(parent))
//   {
//     ciqp->append(children);
//   }
//   return children;
// };



// IQParameter *IQParameterSetModel::decorateLabeledArrayElement(QObject *parent, const std::string& name, insight::Parameter &cp)
// {
//     auto iqp = IQLabeledArrayElementParameterBase::create(parent, this, QString::fromStdString(name), cp, defaultParameterSet_);

//     decorateChildren(iqp, cp);
//     return iqp;
// }

// QList<IQParameter *> IQParameterSetModel::decorateLabeledArrayContent(QObject *parent, insight::LabeledArrayParameter &ap)
// {
//     QList<IQParameter*> children;
//     for (int i=0; i<ap.size(); ++i)
//     {
//         auto iqp=decorateLabeledArrayElement(parent, ap.childParameterName(i), ap.childParameterRef(i));
//         children.append(iqp);
//     }
//     if (IQParameter* ciqp = dynamic_cast<IQParameter*>(parent))
//     {
//         ciqp->append(children);
//     }
//     return children;
// }




void IQParameterSetModel::clearParameters()
{
  beginResetModel();
  // for (auto& rp: rootParameters_)
  // {
  //   //rp->deleteLater();
  //   delete rp; // needs to be deleted before PS is reassigned (valueChanged handler needs to be disconnected)
  // }
  // rootParameters_.clear();

  parameterSet_=insight::ParameterSet::create();
  defaultParameterSet_=insight::ParameterSet::create();
  endResetModel();
}




void IQParameterSetModel::resetParameters(
    const insight::ParameterSet &ps )
{
    beginResetModel();
    parameterSet_=insight::ParameterSet::create();
    endResetModel();

    // create decorators that store parent relationship

    beginInsertRows(QModelIndex(), 0, parameterSet_->size()-1);
    *parameterSet_ = ps;
    endInsertRows();
}



void IQParameterSetModel::resetParameters(
    const insight::ParameterSet &ps,
    const insight::ParameterSet &defaultps )
{
  clearParameters();

  *defaultParameterSet_ = defaultps;

  // create decorators that store parent relationship
  beginInsertRows(QModelIndex(), 0, parameterSet_->size()-1);
  *parameterSet_ = ps;
  endInsertRows();
}




const insight::ParameterSet &IQParameterSetModel::getParameterSet() const
{
  return *parameterSet_;
}




IQParameter *IQParameterSetModel::parameterFromIndex(const QModelIndex &index)
{
  return static_cast<IQParameter*>(
      index.siblingAtColumn(IQParameterSetModel::iqParamCol)
          .data()
          .value<void*>() );
}




bool IQParameterSetModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.column()==0)
    {
        auto *ip = indexData(parent);
        if (auto *iap=dynamic_cast<insight::ArrayParameter*>(ip))
        {
            for (int i=row+count-1; i>=row; --i)
            {
                removeArrayElement(index(i, 0, parent));
            }
            return true;
        }
    }
    return false;
}






insight::Parameter &
IQParameterSetModel::parameterRef(const QModelIndex &index)
{
  if (index.isValid())
  {
    if (auto* p = indexData(index) )
    {
      return *p;
    }
  }

  throw insight::Exception("cannot change parameter: invalid index provided!");
}


insight::Parameter& IQParameterSetModel::parameterRef(const std::string &path)
{
    return parameterSet_->get<insight::Parameter>(path);
}


void IQParameterSetModel::notifyParameterChange(const insight::Parameter& p)
{
    auto idx=indexFromParameter(p, 0);
    if (idx.isValid())
    {
        notifyParameterChange( idx );
    }
}




void IQParameterSetModel::notifyParameterChange(const QModelIndex &index)
{
  Q_ASSERT(index.isValid());

  Q_EMIT dataChanged(
      index.siblingAtColumn(0),
      index.siblingAtColumn(columnCount(index)-1)
      );
}




void IQParameterSetModel::appendArrayElement(
    const QModelIndex &index,
    const insight::Parameter &elem )
{
    insertArrayElement(index, elem);
}




void IQParameterSetModel::insertArrayElement(const QModelIndex &index, const insight::Parameter &elem)
{
  int iIns=0;

  insight::ArrayParameter *iap(nullptr);
  // if index points to array, insert there,
  // otherwise parent is assumed to be array and insert into it
  if ((iap=dynamic_cast<insight::ArrayParameter*>(
           indexData(index))))
  {
    iIns=iap->size();
  }
  else if ((iap=dynamic_cast<insight::ArrayParameter*>(
                  indexData(parent(index)))))
  {
    iIns=index.row();
  }



  beginInsertRows(index, iIns, iIns);
  iap->insertValue( iIns, elem );
  // auto iqnp=decorateArrayElement(iqap, iIns, iap->elementRef(iIns)/*, 0*/);
  // iqp->append(iqnp);
  endInsertRows();
}




void IQParameterSetModel::removeArrayElement(const QModelIndex &index)
{
  auto parentIndex = parent(index);
  Q_ASSERT( parentIndex.isValid() );

  if (auto *ap = dynamic_cast<insight::ArrayParameter*>(indexData(parentIndex)))
  {
      auto row = index.row();


      beginRemoveRows(parentIndex, row, row);
      ap->eraseValue(row);
      endRemoveRows();
      // notifyParameterChange(parentIndex);

      // change name for all subsequent parameters
      for (int i=row; i<ap->size(); ++i)
      {
        // (*aiqp)[i]->setName(QString("%1").arg(i));
        notifyParameterChange( this->index(i, 1, parentIndex) );
      }
  }
}










void IQParameterSetModel::addGeometryToSpatialTransformationParameter(
        const std::string &parameterPath,
        insight::cad::FeaturePtr geom )
{
    transformedGeometry_[parameterPath]=geom;
}




void IQParameterSetModel::addVectorBasePoint(const std::string &parameterPath, const arma::mat &pBase)
{
    vectorBasePoints_[parameterPath]=pBase;
}

insight::cad::FeaturePtr IQParameterSetModel::getGeometryToSpatialTransformationParameter(
        const std::string &parameterPath )
{
    auto i = transformedGeometry_.find(parameterPath);
    if (i!=transformedGeometry_.end())
    {
        return i->second;
    }
    return insight::cad::FeaturePtr();
}




const arma::mat * const IQParameterSetModel::getVectorBasePoint(const std::string &parameterPath)
{
    auto i = vectorBasePoints_.find(parameterPath);
    if (i!=vectorBasePoints_.end())
    {
        return &i->second;
    }
    return nullptr;
}


void IQParameterSetModel::pack()
{
    parameterSet_->pack();
}


void IQParameterSetModel::clearPackedData()
{
    parameterSet_->clearPackedData();
}



void IQParameterSetModel::setAnalysisName(const std::string &analysisName)
{
    analysisName_=analysisName;
}




const std::string &IQParameterSetModel::getAnalysisName() const
{
    return analysisName_;
}






IQParameterSetModel::ParameterEditor::ParameterEditor(
    IQParameterSetModel& psm,
    const std::string &parameterPath
    )
    : model_(psm),
      index_( psm.indexFromParameterPath(parameterPath, 0) ),
      parameter( psm.parameterRef(index_) )
{}

IQParameterSetModel::ParameterEditor::~ParameterEditor()
{
    model_.notifyParameterChange(index_);
}




void disconnectParameterSetChanged(QAbstractItemModel *source, QObject *target)
{
    QObject::disconnect(source, &QAbstractItemModel::dataChanged, target, 0);
    QObject::disconnect(source, &QAbstractItemModel::rowsInserted, target, 0);
    QObject::disconnect(source, &QAbstractItemModel::rowsRemoved, target, 0);
}

IQParameterSetModel *parameterSetModel(QAbstractItemModel *model)
{
    if (auto *iqpsm = dynamic_cast<IQParameterSetModel*>(model))
    {
        return iqpsm;
    }
    else if (auto *pm = dynamic_cast<QAbstractProxyModel*>(model))
    {
        return parameterSetModel(pm->sourceModel());
    }
    else
        throw insight::Exception("Item model must be derived from IQParameterSetModel!");
}


const insight::ParameterSet &getParameterSet(QAbstractItemModel *model)
{
    return parameterSetModel(model)->getParameterSet();
}

const std::string &getAnalysisName(QAbstractItemModel *model)
{
    return parameterSetModel(model)->getAnalysisName();
}

