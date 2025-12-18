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
#include <qnamespace.h>

#include "iqparametersetmodel.h"
#include "base/cppextensions.h"
#include "base/parameter.h"
#include "base/parameters/arrayparameter.h"
#include "base/parameters/selectablesubsetparameter.h"
#include "base/parameters/subsetparameter.h"
#include "base/parameterset.h"
#include "iqhierarchicaldataelement.h"
#include "iqhierarchicaldatamodel.h"
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






IQParameterSetModel::IQParameterSetModel(
    std::unique_ptr<insight::ParameterSet>&& ps,
    boost::optional<const insight::ParameterSet&> defaultps,
    QObject *parent
    )
  : IQHierarchicalDataModel( std::move(ps), parent )
{
    if (defaultps)
    {
        if (defaultParameterSet_)
        {
            defaultParameterSet_->assignFrom( *defaultps );
        }
        else
        {
            defaultParameterSet_=
                 defaultps->cloneAs<insight::ParameterSet>();
        }
    }
    else
    {
        defaultParameterSet_=
            getHierarchicalData()
                .cloneAs<insight::ParameterSet>();
    }
}










Qt::ItemFlags IQParameterSetModel::flags(const QModelIndex &index) const
{
    auto flags = IQHierarchicalDataModel::flags(index);

    if (index.isValid())
    {
        auto *iqp=iqElementOfIndex(index);
        auto *p=iqp->get();


        if (editingIsEnabled())
        {
            if (index.column()==labelCol)
            {
                if (auto *lap = dynamic_cast<const insight::LabeledArrayParameter*>(&p->parent())
                    )
                {
                    if (!lap->keysAreLocked())
                        flags |= Qt::ItemIsEditable;
                }
            }
        }
    }

    return flags;
}




Qt::DropActions IQParameterSetModel::supportedDropActions() const
{
    if (editingIsEnabled())
    {
        return Qt::CopyAction | Qt::MoveAction;
    }
    else
        return 0;
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
        auto *ip=elementOfIndex(index);

        ip->appendToNode(
            ip->name(), doc, *rootnode,
            insight::hierarchicalData::Element::OutputProperties());
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

// QVariant IQParameterSetModel::data(const QModelIndex &index, int role) const
// {

// }






bool IQParameterSetModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (IQHierarchicalDataModel::setData(index, value, role))
        return true;

    if (editingIsEnabled())
    {
      if (auto *p = elementOfIndex(index))
      {
        switch (role)
        {

        case Qt::EditRole:
            if (index.column()==labelCol) // data
            {
                auto *me = iqElementOfIndex(index);
                auto *iqp = dynamic_cast<IQLabeledArrayParameter*>(me->parentElement());
                auto &pp = dynamic_cast<insight::LabeledArrayParameter&>(iqp->parameterRef());
                auto label = p->name();
                auto newl = value.toString();
                if (!newl.isEmpty())
                {
                    pp.changeLabel(label, newl.toStdString());
                }
            }
            break;
        }
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

    if (editingIsEnabled())
    {
      const QMimeData *mimeData = qApp->clipboard()->mimeData();

      auto index=indexes.first();

      if (canDropMimeData(mimeData, Qt::CopyAction,
                          index.row(), 0, index.parent()))
      {
        dropMimeData(mimeData, Qt::CopyAction,
                     index.row(), 0, index.parent());
      }
    }
}




void IQParameterSetModel::contextMenu(
    QWidget *pw,
    const QModelIndex& index,
    const QPoint &p,
    IQCADModel3DViewer *viewer )
{
  if (index.isValid())
  {
    if (auto* iqp=parameterFromIndex(index))
    {
      QMenu ctxMenu;
      iqp->populateContextMenu(&ctxMenu, viewer);

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








void IQParameterSetModel::resetParameterValues(
    const insight::ParameterSet &ps,
    boost::optional<const insight::ParameterSet&> defaultps )
{

  if (defaultps)
  {
      defaultParameterSet_->assignFrom( *defaultps );
  }
  else
  {
      defaultParameterSet_->assignFrom( ps );
  }

  resetValue(ps);
}



const insight::ParameterSet &IQParameterSetModel::getParameterSet() const
{
  return dynamic_cast<const insight::ParameterSet&>(
        getHierarchicalData() );
}

bool IQParameterSetModel::hasDefaultParameterSet() const
{
    return bool(defaultParameterSet_);
}


const insight::ParameterSet *IQParameterSetModel::defaultParameterSet() const
{
    return defaultParameterSet_.get();
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
        auto *ip = elementOfIndex(parent);
        if (auto *iap=dynamic_cast<insight::ArrayParameter*>(ip))
        {
            for (int i=row+count-1; i>=row; --i)
            {
                removeArrayElement(index(i, 0, parent));
            }
            return true;
        }
        else if (auto *iap=dynamic_cast<insight::LabeledArrayParameter*>(ip))
        {
            for (int i=row+count-1; i>=row; --i)
            {
                removeLabeledArrayElement(index(i, 0, parent));
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
    if (auto* p = dynamic_cast<insight::Parameter*>(elementOfIndex(index)) )
    {
      return *p;
    }
  }

  throw insight::Exception("cannot change parameter: invalid index provided!");
}




insight::Parameter& IQParameterSetModel::parameterRef(const std::string &path)
{
    return const_cast<insight::Parameter&>(
        getHierarchicalData()
            .get<insight::Parameter>(path) );
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
           elementOfIndex(index))))
  {
    iIns=iap->size();
  }
  else if ((iap=dynamic_cast<insight::ArrayParameter*>(
                  elementOfIndex(parent(index)))))
  {
    iIns=index.row();
  }



  beginInsertRows(index, iIns, iIns);
  iap->insertValue( iIns, elem.cloneAs<insight::Parameter>() );
  // auto iqnp=decorateArrayElement(iqap, iIns, iap->elementRef(iIns)/*, 0*/);
  // iqp->append(iqnp);
  endInsertRows();
}




void IQParameterSetModel::removeArrayElement(const QModelIndex &index)
{
  auto parentIndex = parent(index);
  Q_ASSERT( parentIndex.isValid() );

  if (auto *ap = dynamic_cast<insight::ArrayParameter*>(
          elementOfIndex(parentIndex)))
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
        notifyElementChange( this->index(i, 1, parentIndex) );
      }
  }
}




void IQParameterSetModel::removeLabeledArrayElement(const QModelIndex &index)
{
    auto *p = elementOfIndex(index);
    auto parentIndex = parent(index);
    Q_ASSERT( parentIndex.isValid() );

    if (auto *ap = dynamic_cast<insight::LabeledArrayParameter*>(
            elementOfIndex(parentIndex)))
    {
        auto row = index.row();


        beginRemoveRows(parentIndex, row, row);
        ap->eraseValue(p->name());
        endRemoveRows();
        // notifyParameterChange(parentIndex);

        // change name for all subsequent parameters
        for (int i=row; i<ap->size(); ++i)
        {
            // (*aiqp)[i]->setName(QString("%1").arg(i));
            notifyElementChange( this->index(i, 1, parentIndex) );
        }
    }
}




insight::ParameterSetGUIContext &IQParameterSetModel::GUIContext()
{
    return dynamic_cast<insight::ParameterSetGUIContext &>(
        const_cast<insight::hierarchicalData::Element&>(
            getHierarchicalData()) );
}






void IQParameterSetModel::pack()
{
    const_cast<insight::ParameterSet&>(
        getParameterSet()).pack();
}


void IQParameterSetModel::clearPackedData()
{
    const_cast<insight::ParameterSet&>(
        getParameterSet()).clearPackedData();
}




std::string IQParameterSetModel::getAnalysisName() const
{
    if (auto *aps=dynamic_cast<const insight::AnalysisParameterSet*>(
            &getHierarchicalData() ) )
    {
        return aps->analysisTypeName();
    }
    return std::string();
}






// IQParameterSetModel::ParameterEditor::ParameterEditor(
//     IQParameterSetModel& psm,
//     const std::string &parameterPath
//     )
//     : model_(psm),
//       index_( psm.indexFromParameterPath(parameterPath, 0) ),
//       parameter( psm.parameterRef(index_) )
// {}

// IQParameterSetModel::ParameterEditor::~ParameterEditor()
// {
//     model_.notifyParameterChange(index_);
// }




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


