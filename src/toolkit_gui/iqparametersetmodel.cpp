#include <QDebug>
#include <QLayout>
#include <QPushButton>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>

#include "iqparametersetmodel.h"
#include "iqparameter.h"
#include "iqparameters/iqarrayparameter.h"
#include "iqparameters/iqarrayelementparameter.h"

#include "cadparametersetvisualizer.h"

#include "base/rapidxml.h"
#include "rapidxml/rapidxml_print.hpp"

std::pair<QString, const insight::Parameter *>
IQParameterSetModel::getParameterAndName(const QModelIndex &index) const
{
  std::pair<QString, const insight::Parameter *> result("", nullptr);

  if (const auto* p=static_cast<const insight::Parameter*>(index.internalPointer()))
  {
    auto idx=p->searchMyParentIn(parameterSet_);
    if (const auto* sdi = boost::get<insight::Parameter::SearchResultParentInDict>(&idx))
    {
      result = std::make_pair( QString::fromStdString(sdi->myIterator->first), p );
    }
    else if (const auto* ai = boost::get<insight::Parameter::SearchResultParentInArray>(&idx))
    {
      result = std::make_pair( QString("%1").arg(ai->i), p) ;
    }
  }

  return result;
}



IQParameterSetModel::IQParameterSetModel(const insight::ParameterSet& ps, const insight::ParameterSet& defaultps, QObject *parent)
  : QAbstractItemModel(parent)
{
  resetParameters(ps, defaultps);

  connect( this, &QAbstractItemModel::dataChanged,
           [&](const QModelIndex&, const QModelIndex&, const QVector<int> &)
  {
    Q_EMIT parameterSetChanged();
  } );

  connect( this, &QAbstractItemModel::rowsInserted,
           [&](const QModelIndex &, int, int)
  {
    Q_EMIT parameterSetChanged();
  } );

  connect( this, &QAbstractItemModel::rowsRemoved,
           [&](const QModelIndex &, int, int)
  {
    Q_EMIT parameterSetChanged();
  } );
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
    s=rootParameters_.size();
  }
  else if (const auto* p=static_cast<IQParameter*>(parent.internalPointer()))
  {
      if (parent.column()==0)
      {
          s=p->nChildParameters(); // only first column has children
      }
  }
//  qDebug()<<parent<<"has rows = "<<s;
  return s;
}




QModelIndex IQParameterSetModel::index(int row, int column, const QModelIndex &parent) const
{
  QModelIndex i;

  if (!parent.isValid()) // in root dict
  {
    if (row>=0 && row<rootParameters_.size())
    {
      i=createIndex(row, column, rootParameters_[row]);
    }
  }
  else if (auto* p=static_cast<IQParameter*>(parent.internalPointer())) // down in tree
  {
    if (row>=0 && row<p->nChildParameters())
    {
      i=createIndex(row, column, (*p)[row]);
    }
  }

//  qDebug()<<"index r="<<row<<", c="<<column<<", p="<<parent<<" = "<<i;

  return i;
}




QModelIndex IQParameterSetModel::parent(const QModelIndex &index) const
{
  QModelIndex i;

  if ( index.isValid() )
  {
    if (auto* p=static_cast<IQParameter*>(index.internalPointer()))
    {
      if (auto* pp = p->parentParameter())
      {
        int parentRow;
        if (auto* ppp = pp->parentParameter())
        {
          parentRow = ppp->indexOf(pp);
        }
        else
        {
          parentRow = rootParameters_.indexOf(pp);
        }
        i = createIndex(parentRow, 0, pp);
      }
    }
  }

//  qDebug()<<"parent of "<<index<<" = "<<i;

  return i;
}



Qt::ItemFlags IQParameterSetModel::flags(const QModelIndex &index) const
{
  if (!index.isValid())
  {
    return 0;
  }

  auto flags=QAbstractItemModel::flags(index);

//  auto *iqp=static_cast<IQParameter*>(index.internalPointer());
//  if (dynamic_cast<IQArrayElementParameterBase*>(iqp))
//  {
    flags |= Qt::ItemIsDragEnabled;
//  }

//  if (dynamic_cast<IQArrayParameter*>(iqp))
//  {
    flags |= Qt::ItemIsDropEnabled;
//  }

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
        auto *iqp=static_cast<IQParameter*>(index.internalPointer());
        const auto& ip=iqp->parameter();

        ip.appendToNode(iqp->name().toStdString(), doc, *rootnode, "");
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
  if (action == Qt::IgnoreAction)
  {
    return true;
  }
  else if (action == Qt::MoveAction || action == Qt::CopyAction)
  {

    // parse, what we got
    std::string contents(data->data("application/xml").toStdString());
    using namespace rapidxml;
    xml_document<> doc;
    doc.parse<0>(&contents[0]);
    xml_node<> *rootnode = doc.first_node("root");

    std::string allTypesEqual(rootnode->first_node()->name());
    int nArgs = 0;
    for (auto *e = rootnode->first_node(); e!=nullptr; e=e->next_sibling())
    {
        std::string type(e->name());
        nArgs++;
        if (type!=allTypesEqual) allTypesEqual="";
    }

    IQArrayParameter *iqap = nullptr;
    insight::ArrayParameter *iap = nullptr;
    if (auto *iqp=static_cast<IQParameter*>(parent.internalPointer()))
    {
        if ((iqap = dynamic_cast<IQArrayParameter*>(iqp)))
        {
            iap = dynamic_cast<insight::ArrayParameter*>(
                &iqap->parameterRef());
        }
    }

    if (iqap && allTypesEqual==iap->defaultValue().type())
    {
        // Parent is Array and all given parameters are of the same and child type of array? -> inserted/append children
        std::vector<insight::ParameterPtr> args;
        for (auto *e = rootnode->first_node(); e!=nullptr; e=e->next_sibling())
        {
            insight::ParameterPtr np(
                iap->defaultValue().clone() );

            np->readFromNode(
                e->first_attribute("name")->value(),
                *rootnode,
                ""
                );

            args.push_back(np);
        }

        auto row=parent.row();
        if (row<0) // append
        {
            for (auto& arg: args)
            {
                appendArrayElement(parent, *arg);
            }
            return true;
        }
        else if (row>=0)
        {
            // insert in reverse order
            std::reverse(args.begin(), args.end());
            for (auto& arg: args)
            {
                insertArrayElement(parent, *arg);
            }
            return true;
        }
    }
    else if (nArgs==1)
    {
        // expect a single given item of same type as indexed parameter
        auto targetIndex = index(row, column, parent);
        if (!targetIndex.isValid())
            targetIndex=parent;

        auto *iqp=static_cast<IQParameter*>(targetIndex.internalPointer());
        if (allTypesEqual==iqp->parameter().type())
        {
            insight::ParameterPtr np(iqp->parameter().clone());
            np->readFromNode(
                    rootnode->first_node()->first_attribute("name")->value(),
                    *rootnode,
                    ""
                );
            iqp->parameterRef().reset(*np);
            notifyParameterChange(targetIndex, true);
            return true;
        }
    }


  }

  return false;
}




QVariant IQParameterSetModel::data(const QModelIndex &index, int role) const
{
  if (auto *p = static_cast<IQParameter*>(index.internalPointer()))
  {
    switch (role)
    {

      case Qt::DisplayRole:
        if (index.column()==0) // name
        {
          return p->name();
        }
        else if (index.column()==1) // data
        {
          return p->valueText();
        }
        else if (index.column()==2) // full path (hidden)
        {
          qDebug()<<p->path();
          return p->path();
        }

      case Qt::BackgroundRole:
        return p->backgroundColor();

      case Qt::ForegroundRole:
        return p->textColor();

      case Qt::FontRole:
        return p->textFont();

    }
  }
  return QVariant();
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
    if (auto* iqp=static_cast<IQParameter*>(index.internalPointer()))
    {
      QMenu ctxMenu;
      iqp->populateContextMenu(this, index, &ctxMenu);

      // copy/paste
      QAction *a;

      ctxMenu.addSeparator();
      a=new QAction("&Copy");
      connect(a, &QAction::triggered, a,
              [this,index]() { copy({index.siblingAtColumn(0)}); }
      );
      ctxMenu.addAction(a);
      a=new QAction("&Paste");
      if (qApp->clipboard()->mimeData()->formats().contains("application/xml"))
      {
       connect(a, &QAction::triggered, a,
                  [this,index]() { paste({index.siblingAtColumn(0)}); }
              );
      }
      else
      {
       a->setDisabled(true);
      }
      ctxMenu.addAction(a);

      ctxMenu.exec(pw->mapToGlobal(p));
    }
  }
}





void IQParameterSetModel::clearParameters()
{
  beginResetModel();
  for (auto& rp: rootParameters_)
  {
    rp->deleteLater();
  }
  rootParameters_.clear();
  endResetModel();

  parameterSet_=insight::ParameterSet();
  defaultParameterSet_=insight::ParameterSet();
}



void IQParameterSetModel::decorateChildren(QObject* parent, insight::Parameter* p)
{
  if (const auto* sdp = dynamic_cast<insight::SubParameterSet*>(p))
  {
    decorateSubdictContent(parent, sdp->subset());
  }
  else if (auto* ap = dynamic_cast<insight::ArrayParameterBase*>(p))
  {
    decorateArrayContent(parent, *ap);
  }
}

QList<IQParameter*> IQParameterSetModel::decorateSubdictContent(QObject* parent, const insight::ParameterSet& ps)
{
  QList<IQParameter*> children;
  for (const auto& p: ps)
  {
    auto& cp=*p.second;
    auto name=QString::fromStdString(p.first);
    auto iqp = IQParameter::create(parent, name, cp, defaultParameterSet_);

    decorateChildren(iqp, &cp);
    children.append(iqp);
  }
  if (IQParameter* ciqp = dynamic_cast<IQParameter*>(parent))
  {
    ciqp->append(children);
  }
  return children;
};

IQParameter* IQParameterSetModel::decorateArrayElement(QObject* parent, int i, insight::Parameter& cp)
{
  auto name=QString("%1").arg(i);
  auto iqp = IQArrayElementParameterBase::create(parent, name, cp, defaultParameterSet_);

  decorateChildren(iqp, &cp);
  return iqp;
}

QList<IQParameter*> IQParameterSetModel::decorateArrayContent(QObject* parent, insight::ArrayParameterBase& ap)
{
  QList<IQParameter*> children;
  for (int i=0; i<ap.size(); ++i)
  {
    auto iqp=decorateArrayElement(parent, i, ap.elementRef(i));
    children.append(iqp);
  }
  if (IQParameter* ciqp = dynamic_cast<IQParameter*>(parent))
  {
    ciqp->append(children);
  }
  return children;
};



void IQParameterSetModel::resetParameters(const insight::ParameterSet &ps, const insight::ParameterSet &defaultps)
{
  clearParameters();

  parameterSet_=ps;
  defaultParameterSet_=defaultps;

  // create decorators that store parent relationship

  beginInsertRows(QModelIndex(), 0, parameterSet_.size()-1);
  rootParameters_=decorateSubdictContent(this, parameterSet_);
  endInsertRows();
}




const insight::ParameterSet &IQParameterSetModel::getParameterSet() const
{
  return parameterSet_;
}

bool IQParameterSetModel::removeRows(int row, int count, const QModelIndex &parent)
{
  if (parent.column()==0)
  {
      auto* iqp=static_cast<IQParameter*>(parent.internalPointer());
      if (auto *iqap=dynamic_cast<IQArrayParameter*>(iqp))
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






insight::Parameter &IQParameterSetModel::parameterRef(const QModelIndex &index)
{
  if (index.isValid())
  {
    if (auto* p=static_cast<IQParameter*>(index.internalPointer()))
    {
      return const_cast<insight::Parameter&>(p->parameter());
    }
  }

  throw insight::Exception("cannot change parameter: invalid index provided!");
}



void IQParameterSetModel::notifyParameterChange(const QModelIndex &index, bool redecorateChildren)
{
  Q_ASSERT(index.isValid());

  if (redecorateChildren)
  {
      // remove existing child params
      auto* iqp = static_cast<IQParameter*>(index.internalPointer());
      beginRemoveRows(index, 0, iqp->size()-1);
      for (auto* c: *iqp)
      {
        c->deleteLater();
      }
      iqp->clear();
      endRemoveRows();

      if (auto* param = dynamic_cast<insight::SubParameterSet*>(&(parameterRef(index))))
      {
        auto &subset = param->subset();
        if (subset.size())
        {
          // repopulate
          beginInsertRows(index, 0, subset.size()-1);
          decorateSubdictContent(iqp, subset/*, 0*/);
          endInsertRows();
        }
      }
      else if (auto* param = dynamic_cast<insight::ArrayParameter*>(&(parameterRef(index))))
      {
        if (param->size())
        {
          // repopulate
          beginInsertRows(index, 0, param->size()-1);
          decorateArrayContent(iqp, *param);
          endInsertRows();
        }
      }
  }

  if (auto *p=static_cast<IQParameter*>(index.internalPointer()))
  {
    p->resetModificationState();
  }
  emit dataChanged(
        createIndex(index.row(), 0, index.internalPointer()),
      createIndex(index.row(), columnCount(index)-1, index.internalPointer()) );
}


void IQParameterSetModel::appendArrayElement(
    const QModelIndex &index,
    const insight::Parameter &elem )
{
  insertArrayElement(index, elem);
}


void IQParameterSetModel::insertArrayElement(const QModelIndex &index, const insight::Parameter &elem)
{
  IQArrayParameter *iqap(nullptr);
  insight::ArrayParameter *iap(nullptr);
  int iIns=0;

  auto* iqp = static_cast<IQParameter*>(index.internalPointer());
  auto parentIndex = parent(index);
  auto *iqpp = static_cast<IQParameter*>(parentIndex.internalPointer());
  if ((iqap=dynamic_cast<IQArrayParameter*>(iqp)))
  {
    iap=dynamic_cast<insight::ArrayParameter*>(&iqap->parameterRef());
    iIns=iap->size();
  }
  else if ((iqap=dynamic_cast<IQArrayParameter*>(iqpp)))
  {
    iap=dynamic_cast<insight::ArrayParameter*>(&iqap->parameterRef());
    iIns=index.row();
  }

  iap->insertValue( iIns, elem );


  beginInsertRows(index, iIns, iIns);
  auto iqnp=decorateArrayElement(iqap, iIns, iap->elementRef(iIns)/*, 0*/);
  iqap->append(iqnp);
  endInsertRows();
}


void IQParameterSetModel::removeArrayElement(const QModelIndex &index)
{
  auto parentIndex = parent(index);
  Q_ASSERT( parentIndex.isValid() );

  auto *aiqp = static_cast<IQParameter*>(parentIndex.internalPointer());
  auto &parentParameter = dynamic_cast<insight::ArrayParameter&>(aiqp->parameterRef());

  auto row = index.row();
  auto* iqp = static_cast<IQParameter*>(index.internalPointer());


  beginRemoveRows(parentIndex, row, row);
  iqp->deleteLater();
  aiqp->erase(aiqp->begin()+row);
  endRemoveRows();

  parentParameter.eraseValue(row);
  notifyParameterChange(parentIndex);

  // change name for all subsequent parameters
  for (int i=row; i<aiqp->size(); ++i)
  {
    (*aiqp)[i]->setName(QString("%1").arg(i));
    notifyParameterChange( this->index(i, 1, parentIndex) );
  }
}




QList<int> IQParameterSetModel::pathFromIndex(const QModelIndex &i) const
{
  QList<int> p;
  QModelIndex ii = i;
  while (ii.isValid())
  {
    p<<ii.row();
    ii=parent(ii);
  }
  return p;
}

QModelIndex IQParameterSetModel::indexFromParameterPath(const std::string &pp) const
{
    std::vector<std::string> splitPath;
    boost::split(splitPath, pp, boost::is_any_of("/"));

    QModelIndex ii;
    QList<IQParameter*> paramList=rootParameters_;
    for (int si=0; si<splitPath.size(); ++si)
    {
        for (int j=0; j<paramList.size(); ++j)
        {
            if (paramList[j]->name().toStdString() == splitPath[si])
            {
                paramList=*paramList[j];
                ii=index(j, 0, ii);
                break;
            }
        }
    }
    return ii;
}




QModelIndex IQParameterSetModel::indexFromPath(const QList<int> &p) const
{
  QModelIndex ii;
  if (p.size())
  {
    for (int i=p.size()-1; i>=0; --i)
    {
      ii=index(p[i], 0, ii);
    }
  }
  return ii;
}

void IQParameterSetModel::addGeometryToSpatialTransformationParameter(
        const QString &parameterPath,
        insight::cad::FeaturePtr geom )
{
    transformedGeometry_[parameterPath]=geom;
}

void IQParameterSetModel::addVectorBasePoint(const QString &parameterPath, const arma::mat &pBase)
{
    vectorBasePoints_[parameterPath]=pBase;
}

insight::cad::FeaturePtr IQParameterSetModel::getGeometryToSpatialTransformationParameter(
        const QString &parameterPath )
{
    auto i = transformedGeometry_.find(parameterPath);
    if (i!=transformedGeometry_.end())
    {
        return i->second;
    }
    return insight::cad::FeaturePtr();
}

const arma::mat * const IQParameterSetModel::getVectorBasePoint(const QString &parameterPath)
{
    auto i = vectorBasePoints_.find(parameterPath);
    if (i!=vectorBasePoints_.end())
    {
        return &i->second;
    }
    return nullptr;
}

void IQParameterSetModel::setAnalysisName(const std::string &analysisName)
{
    analysisName_=analysisName;
}

const std::string &IQParameterSetModel::getAnalysisName() const
{
    return analysisName_;
}




void IQParameterSetModel::handleClick(
        const QModelIndex &index,
        QWidget* editControlsContainer,
        IQCADModel3DViewer *vri,
        insight::CADParameterSetVisualizer *viz )
{
  if (index.isValid())
  {
    if (auto* p=static_cast<IQParameter*>(index.internalPointer()))
    {
      // remove existing controls first
      {
        // clear contents of widget with edit controls
        QList<QWidget*> widgets = editControlsContainer->findChildren<QWidget*>();
        foreach(QWidget* widget, widgets)
        {
          widget->deleteLater();
        }
        // remove old layout
        if (editControlsContainer->layout())
        {
          delete editControlsContainer->layout();
        }
      }

      // create new controls
      auto l = p->populateEditControls(this, index, editControlsContainer, vri);


      if (viz)
      {
          auto cmdl=new QHBoxLayout;
          auto acts = viz->createGUIActions(p->path().toStdString(), this, editControlsContainer, vri);
          for (auto& act: acts)
          {
              auto btn = new QPushButton(act.icon, act.label);
              connect(btn, &QPushButton::clicked, btn,
                      act.action);
              cmdl->addWidget(btn);
          }
          l->addLayout(cmdl);
      }

      l->addStretch();
    }
  }
}


IQParameterSetModel::ParameterEditor::ParameterEditor(IQParameterSetModel& psm, const std::string &parameterPath)
    : model_(psm),
      index_( psm.indexFromParameterPath(parameterPath) ),
      parameter( psm.parameterRef(index_) )
{}

IQParameterSetModel::ParameterEditor::~ParameterEditor()
{
    model_.notifyParameterChange(index_);
}




void IQFilteredParameterSetModel::searchRootSourceIndices(QAbstractItemModel *sourceModel, const QModelIndex& parent)
{
    for (int r=0; r<sourceModel->rowCount(parent); ++r)
    {
        auto i=sourceModel->index(r, 2, parent);
        auto curPath=sourceModel->data(i).toString();
        for (const auto& sourceParam: qAsConst(sourceRootParameterPaths_))
        {
          if (boost::starts_with(sourceParam, curPath))
          {
              if (curPath.size()==sourceParam.size())
              {
                  qDebug()<<"added "<<curPath;
                  rootSourceIndices.append(i.siblingAtColumn(0));
              }
              else if (curPath.size()<sourceParam.size())
              {
                  searchRootSourceIndices(sourceModel, i.siblingAtColumn(0));
              }
          }
        }
    }
}

void IQFilteredParameterSetModel::storeAllChildSourceIndices(QAbstractItemModel *sourceModel, const QModelIndex &sourceIndex)
{
    if (!mappedIndices_.contains(sourceIndex))
        mappedIndices_.append(sourceIndex);

    for (int r=0; r<sourceModel->rowCount(sourceIndex); ++r)
    {
        storeAllChildSourceIndices(
            sourceModel,
            sourceModel->index(r, 0, sourceIndex) );
    }
}

bool IQFilteredParameterSetModel::isBelowRootParameter(const QModelIndex &sourceIndex, int* topRow) const
{
    if (!sourceIndex.isValid())
        return false;

    if (rootSourceIndices.contains(sourceIndex))
    {
        if (topRow)
            *topRow=rootSourceIndices.indexOf(sourceIndex);
        return true;
    }

    auto sourceIndexParent = sourceModel()->parent(sourceIndex);

    if (!sourceIndexParent.isValid())
    {
        return false;
    }
    else
    {
        if (rootSourceIndices.contains(sourceIndexParent))
        {
            if (topRow) *topRow=-1;
            return true;
        }
        else
            return isBelowRootParameter(sourceIndexParent, topRow);
    }
}

IQFilteredParameterSetModel::IQFilteredParameterSetModel(
    const QList<std::string>& sourceParameterPaths,
    QObject *parent )
  : QAbstractProxyModel(parent),
    sourceRootParameterPaths_(sourceParameterPaths)
{}

void IQFilteredParameterSetModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    searchRootSourceIndices(sourceModel, QModelIndex());
    for (const auto& pi: qAsConst(rootSourceIndices))
    {
        storeAllChildSourceIndices(sourceModel, pi);
    }
    QAbstractProxyModel::setSourceModel(sourceModel);
}

QModelIndex IQFilteredParameterSetModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    qDebug()<<"mapFromSource"<<sourceIndex;

    if (!sourceIndex.isValid())
        return QModelIndex();

    int topRow=-1;
    if (isBelowRootParameter(sourceIndex, &topRow))
    {
        qDebug()<<topRow;
        if (topRow>=0)
        {
            return index(topRow, sourceIndex.column());
        }
        else
        {
            return index(sourceIndex.row(), sourceIndex.column(), mapFromSource(sourceIndex.parent()));
        }
    }
    return QModelIndex();
}

QModelIndex IQFilteredParameterSetModel::mapToSource(const QModelIndex &proxyIndex) const
{
    qDebug()<<"mapToSource"<<proxyIndex;

    if (!proxyIndex.isValid())
    {
        return QModelIndex();
    }

    if (!proxyIndex.internalPointer())  // top level
    {
        if (proxyIndex.row()>=0 && proxyIndex.row()<rootSourceIndices.size())
        {
            return QModelIndex(rootSourceIndices[proxyIndex.row()]).siblingAtColumn(proxyIndex.column());
        }
    }
    else
    {
        auto& sppi = *reinterpret_cast<QPersistentModelIndex*>(
            proxyIndex.internalPointer() );
        return sourceModel()->index(proxyIndex.row(), proxyIndex.column(), sppi);
    }
    return QModelIndex();
}

//bool IQFilteredParameterSetModel::hasChildren(const QModelIndex &parent) const
//{
//    return rowCount(parent)>0 && columnCount(parent)>0;
//}

int IQFilteredParameterSetModel::columnCount(const QModelIndex &parent) const
{
    qDebug()<<"colcount";
    return sourceModel()->columnCount(mapToSource(parent));
}

int IQFilteredParameterSetModel::rowCount(const QModelIndex &parent) const
{
    qDebug()<<"rouwcount"<<parent;
    if (!parent.isValid())
    {
        return rootSourceIndices.size();
    }
    else
    {
        auto r= sourceModel()->rowCount(mapToSource(parent));
        qDebug()<<r;
        return r;
    }

    return 0;
}

QModelIndex IQFilteredParameterSetModel::index(int row, int column, const QModelIndex &parent) const
{
    qDebug()<<"index"<<row<<column<<parent;
    if (!parent.isValid())
    {
        // top rows
        if (row>=0 && row<rootSourceIndices.size())
        {
            return createIndex(
                row, column, nullptr
            );
        }
    }
    else
    {
        auto pp=parent.parent();
        if (!pp.isValid())
        {
            // one below top rows
            auto mi=mappedIndices_.indexOf(rootSourceIndices[parent.row()]);
            return createIndex(
                row, column,
                const_cast<void*>(reinterpret_cast<const void*>(&mappedIndices_[mi]))
                );
        }
        else
        {
            auto mi=mappedIndices_.indexOf(mapToSource(parent));
            // more than one level below top rows
            return createIndex(
                row, column,
                //parent.internalPointer()
                const_cast<void*>(reinterpret_cast<const void*>(&mappedIndices_[mi]))
                );
        }
    }

    return QModelIndex();
}

QModelIndex IQFilteredParameterSetModel::parent(const QModelIndex &index) const
{
    qDebug()<<"parent"<<index;

    if (!index.isValid())
    {
        // top node
        return QModelIndex();
    }

    if (index.internalPointer())
    {
        // some index below top level
        auto& sppi = *reinterpret_cast<QPersistentModelIndex*>(
            index.internalPointer() );

        auto mi1=rootSourceIndices.indexOf(sppi);
        if (mi1>=0)
        {
            // top level row
            return createIndex(
                mi1, 0,
                nullptr
                );
        }
        else
        {
            // below top level row
            auto mi=mappedIndices_.indexOf(sppi);
            return createIndex(
                sppi.row(), 0,
                const_cast<void*>(reinterpret_cast<const void*>(&mappedIndices_[mi]))
                );
        }
    }

    return QModelIndex();
}

