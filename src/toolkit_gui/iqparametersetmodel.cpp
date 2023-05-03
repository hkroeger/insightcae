#include <QDebug>
#include <QLayout>
#include <QPushButton>

#include "iqparametersetmodel.h"
#include "iqparameter.h"
#include "iqparameters/iqarrayelementparameter.h"

#include "cadparametersetvisualizer.h"


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




void IQParameterSetModel::contextMenu(QWidget *pw, const QModelIndex& index, const QPoint &p)
{
  if (index.isValid())
  {
    if (auto* iqp=static_cast<IQParameter*>(index.internalPointer()))
    {
      QMenu ctxMenu;
      iqp->populateContextMenu(this, index, &ctxMenu);
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



void IQParameterSetModel::decorateChildren(QObject* parent, insight::Parameter* p, int level)
{
  if (const auto* sdp = dynamic_cast<insight::SubParameterSet*>(p))
  {
    decorateSubdictContent(parent, sdp->subset(), level+1);
  }
  else if (auto* ap = dynamic_cast<insight::ArrayParameterBase*>(p))
  {
    decorateArrayContent(parent, *ap, level+1);
  }
}

QList<IQParameter*> IQParameterSetModel::decorateSubdictContent(QObject* parent, const insight::ParameterSet& ps, int level)
{
  QList<IQParameter*> children;
  for (const auto& p: ps)
  {
    auto& cp=*p.second;
    auto name=QString::fromStdString(p.first);
    auto iqp = IQParameter::create(parent, name, cp, defaultParameterSet_);

    decorateChildren(iqp, &cp, level);
    children.append(iqp);
  }
  if (IQParameter* ciqp = dynamic_cast<IQParameter*>(parent))
  {
    ciqp->append(children);
  }
  return children;
};

IQParameter* IQParameterSetModel::decorateArrayElement(QObject* parent, int i, insight::Parameter& cp, int level)
{
  auto name=QString("%1").arg(i);
  auto iqp = IQArrayElementParameterBase::create(parent, name, cp, defaultParameterSet_);

  decorateChildren(iqp, &cp, level);
  return iqp;
}

QList<IQParameter*> IQParameterSetModel::decorateArrayContent(QObject* parent, insight::ArrayParameterBase& ap, int level)
{
  QList<IQParameter*> children;
  for (int i=0; i<ap.size(); ++i)
  {
    auto iqp=decorateArrayElement(parent, i, ap.elementRef(i), level);
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
  rootParameters_=decorateSubdictContent(this, parameterSet_, 0);
  endInsertRows();
}




const insight::ParameterSet &IQParameterSetModel::getParameterSet() const
{
  return parameterSet_;
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



void IQParameterSetModel::notifyParameterChange(const QModelIndex &index)
{
  if (auto *p=static_cast<IQParameter*>(index.internalPointer()))
  {
    p->resetModificationState();
  }
  emit dataChanged(
        createIndex(index.row(), 0, index.internalPointer()),
        createIndex(index.row(), columnCount(index)-1, index.internalPointer()) );
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
