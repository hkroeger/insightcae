#include "iqcaditemmodel.h"
#include "base/exception.h"
#include "iqcadmodel3dviewer.h"
#include "datum.h"

#include <algorithm>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <boost/random/exponential_distribution.hpp>

boost::mt19937 boostRanGen;


void TreeNode::childEvent(QChildEvent *event)
{
    if (event->added())
    {
        // not yet fully constructed
        tba_.insert(event->child());
    }
    else if (event->removed())
    {
        tbd_.insert(event->child());
    }
    QObject::childEvent(event);
}


const std::map<std::string, QObject*> &
TreeNode::childrenList() const
{
    if (tba_.size())
    {
        std::for_each(
            tba_.begin(), tba_.end(),
            [this](QObject*o)
            {
                if (auto*n=dynamic_cast<TreeNode*>(o))
                {
                    childrenInOrder_[n->label.toStdString()]=n;
                }
            }
        );
        tba_.clear();
    }
    if (tbd_.size())
    {
        std::for_each(
            tbd_.begin(), tbd_.end(),
            [this](QObject*o)
            {
                auto i=std::find_if(
                    childrenInOrder_.begin(),
                    childrenInOrder_.end(),
                    [&](const decltype(childrenInOrder_)::value_type& v)
                    { return v.second==o; });
                childrenInOrder_.erase(i);
            }
            );
        tbd_.clear();
    }
    return childrenInOrder_;
}



TreeNode *TreeNode::parentNode() const
{
    return dynamic_cast<TreeNode*>(parent());
}


int TreeNode::childRow(TreeNode *n) const
{
    auto i=std::find_if(
        childrenList().begin(),
        childrenList().end(),
        [&](const decltype(childrenInOrder_)::value_type& v )
        { return v.second==n; }
        );
    return std::distance(childrenList().begin(), i);
}

int TreeNode::nChildNodes() const
{
    return childrenList().size();
}

TreeNode::TreeNode(QString l, TreeNode *parent)
    :QObject(parent),
    label(l)
{}


QString SectionNode::valueString() const
{
    return QString();
}

QVariant SectionNode::valueAsVariant() const
{
    return QVariant();
}

SectionNode::SectionNode(QString label, TreeNode *parent)
    :TreeNode(label, parent)
{}


QString ScalarNode::valueString() const
{
    return QString::number(value->value());
}

QVariant ScalarNode::valueAsVariant() const
{
    QVariant v;
    v.setValue(value);
    return v;
}

HideableNode::HideableNode(QString label, TreeNode *parent)
    :TreeNode(label, parent),
    visible(true)
{

}

QString PointNode::valueString() const
{
    arma::mat v=value->value();
    return QString("[%1, %2, %3]")
        .arg(v(0)).arg(v(1)).arg(v(2));
}

QVariant PointNode::valueAsVariant() const
{
    QVariant v;
    v.setValue(value);
    return v;
}

QString VectorNode::valueString() const
{
    arma::mat v=value->value();
    return QString("[%1, %2, %3]")
        .arg(v(0)).arg(v(1)).arg(v(2));
}

QVariant VectorNode::valueAsVariant() const
{
    QVariant v;
    v.setValue(value);
    return v;
}


QString DatumNode::valueString() const
{
    return QString::fromStdString(value->type());
}

QVariant DatumNode::valueAsVariant() const
{
    QVariant v;
    v.setValue(value);
    return v;
}



QString FeatureNode::valueString() const
{
    return QString::fromStdString(value->type());
}

QVariant FeatureNode::valueAsVariant() const
{
    QVariant v;
    v.setValue(value);
    return v;
}

FeatureNode::FeatureNode(QString label, TreeNode *parent)
    : HideableNode(label, parent)
{
    boost::random::exponential_distribution<> ed( 5. /* larger => lighter colors */ );
    color.setRedF(      1.-std::max(0., std::min(1., ed(boostRanGen))) );
    color.setGreenF(    1.-std::max(0., std::min(1., ed(boostRanGen))) );
    color.setBlueF(     1.-std::max(0., std::min(1., ed(boostRanGen))) );
    opacity=1.;
    visible=true;
    representation = insight::Surface;
}



void FeatureNode::operator=(
    const insight::cad::FeatureVisualizationStyle& fvs)
{
    if (auto*s=boost::get<insight::DatasetRepresentation>(&fvs.style))
    {
        representation=*s;
    }

    if (auto*o=boost::get<double>(&fvs.opacity))
    {
        opacity=*o;
    }

    if (fvs.color.n_elem==3)
    {
        color=qRgb(
            255.*fvs.color(0),
            255.*fvs.color(1),
            255.*fvs.color(2) );
    }

    visible = fvs.initiallyVisible;

    assocParamPaths = fvs.associatedParameterPaths;
}


QString PostProcNode::valueString() const
{
    return QString::fromStdString(value->type());
}

QVariant PostProcNode::valueAsVariant() const
{
    QVariant v;
    v.setValue(value);
    return v;
}



QString DatasetNode::valueString() const
{
    return QString::fromStdString(value->GetClassName());
}

QVariant DatasetNode::valueAsVariant() const
{
    QVariant v;
    v.setValue(value);
    return v;
}


IQCADItemModel::IQCADItemModel(
    insight::cad::ModelPtr m,
    QObject* parent,
    bool addSubshapesAsLeafs )
  : QAbstractItemModel(parent),
    associatedParameterSetModel_(nullptr),
    addSubshapesAsLeafs_(addSubshapesAsLeafs),
    sections{
        SectionNode("Scalars"),
        SectionNode("Points"),
        SectionNode("Vectors"),
        SectionNode("Datums"),
        SectionNode("Features"),
        SectionNode("Post Proc"),
        SectionNode("Datasets")
    }
{
    if (m)
        model_=m;
    else
        model_=std::make_shared<insight::cad::Model>();

    // add decorators for entities in model_
    std::for_each(
        model_->scalars().begin(),
        model_->scalars().end(),
        [this](const insight::cad::Model::ScalarTableContents::value_type& vt){
            addDecoration<insight::cad::ScalarPtr, ScalarNode>(
                vt.first, vt.second,
                &sections[scalarVariable],
                std::function<void(ScalarNode&)>() );
        });
    std::for_each(
        model_->points().begin(),
        model_->points().end(),
        [this](const insight::cad::Model::VectorTableContents::value_type& vt){
            addDecoration<insight::cad::VectorPtr, PointNode>(
                vt.first, vt.second,
                &sections[pointVariable],
                std::function<void(PointNode&)>() );
        });
    std::for_each(
        model_->directions().begin(),
        model_->directions().end(),
        [this](const insight::cad::Model::VectorTableContents::value_type& vt){
            addDecoration<insight::cad::VectorPtr, VectorNode>(
                vt.first, vt.second,
                &sections[vectorVariable],
                std::function<void(VectorNode&)>() );
        });
    std::for_each(
        model_->datums().begin(),
        model_->datums().end(),
        [this](const insight::cad::Model::DatumTableContents::value_type& vt){
            addDecoration<insight::cad::DatumPtr, DatumNode>(
                vt.first, vt.second,
                &sections[datum],
                std::function<void(DatumNode&)>() );
        });
    std::for_each(
        model_->modelsteps().begin(),
        model_->modelsteps().end(),
        [this](const insight::cad::Model::ModelstepTableContents::value_type& vt){
            addDecoration<insight::cad::FeaturePtr, FeatureNode>(
                vt.first, vt.second,
                &sections[feature],
                std::function<void(FeatureNode&)>() );
        });
    std::for_each(
        model_->postprocActions().begin(),
        model_->postprocActions().end(),
        [this](const insight::cad::Model::PostprocActionTableContents::value_type& vt){
            addDecoration<insight::cad::PostprocActionPtr, PostProcNode>(
                vt.first, vt.second,
                &sections[postproc],
                std::function<void(PostProcNode&)>() );
        });
    model_->checkForBuildDuringAccess();
}




IQCADItemModel::~IQCADItemModel()
{}




QVariant IQCADItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role==Qt::DisplayRole)
    {
      if (orientation==Qt::Horizontal)
      {
        if (section == labelCol)
            return QVariant("Label");
        else if (section == valueCol)
            return QVariant("Value");
      }
    }
    return QVariant();
}




QModelIndex IQCADItemModel::index(
    int row, int column,
    const QModelIndex &parent ) const
{
    if (!parent.isValid())
    {
        // top level
        if ( (row>=0) &&
             (row<CADModelSection::numberOf) )
        {
            return createIndex(
                row, column,
                static_cast<void*>(&sections[row]) );
        }
    }
    else
    {
        if ( auto *pn = static_cast<TreeNode*>(
                parent.internalPointer()) )
        {
            if ( auto *n=pn->childNode(row) )
            {
                return createIndex(
                    row, column, n );
            }
        }
    }

    return QModelIndex();
}




QModelIndex IQCADItemModel::parent(const QModelIndex &index) const
{
    if (index.isValid())
    {
        if (auto *n=static_cast<TreeNode*>(index.internalPointer()))
        {
            if (auto *pn = n->parentNode())
            {
                int row=-1;

                if (auto *ppn = pn->parentNode())
                {
                    row=ppn->childRow(pn);
                }
                else
                {
                    for (int i=0;i<numberOf; ++i)
                    {
                        if (pn==&sections[i])
                        {
                            row=i;
                            break;
                        }
                    }
                    insight::assertion(
                        row>=0,
                        "internal error: assumed parent %s of %s is a section but was not found among section nodes",
                        pn->label.toStdString().c_str(),
                        n->label.toStdString().c_str()
                        );
                }

                return createIndex(row, 0, pn);
            }
        }
    }

    return QModelIndex();
}




int IQCADItemModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        return CADModelSection::numberOf;
    }
    else if (auto *n=static_cast<TreeNode*>(parent.internalPointer()))
    {
        if (parent.column()==0) // only first column has children
        {
            return n->nChildNodes();
        }
    }
    return 0;
}




int IQCADItemModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}




QVariant IQCADItemModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid())
    {
        auto *n=static_cast<TreeNode*>(index.internalPointer());
        if (n && index.isValid())
        {
            if (role==Qt::DisplayRole)
            {
                switch (index.column())
                {
                case labelCol:
                    return n->label;

                case valueCol:
                    return n->valueString();

                case entityCol:
                    return n->valueAsVariant();
                }

                if (auto *en=dynamic_cast<FeatureNode*>(n))
                {
                    switch (index.column())
                    {
                    case entityColorCol:
                        return en->color;

                    case entityOpacityCol:
                        return en->opacity;

                    case entityRepresentationCol:
                        return en->representation;

                    case assocParamPathsCol:
                        auto paths = boost::join(en->assocParamPaths, ":");
                        return QString::fromStdString(paths);
                    }
                }


                if (auto *dn=dynamic_cast<DatasetNode*>(n))
                {
                    switch (index.column())
                    {
                    case datasetFieldNameCol:
                        return QString::fromStdString(dn->fieldName);

                    case datasetPointCellCol:
                        return int(dn->fieldSupport);

                    case datasetComponentCol:
                        return int(dn->fieldComponent);

                    case datasetMinCol:
                        if (dn->minVal) return double(*dn->minVal);
                        break;

                    case datasetMaxCol:
                        if (dn->maxVal) return double(*dn->maxVal);
                        break;

                    case datasetRepresentationCol:
                        return int(dn->representation);
                    }
                }
            }
            else if (role == Qt::CheckStateRole)
            {
                if (auto *hn=dynamic_cast<HideableNode*>(n))
                {
                    if (index.column()==visibilityCol)
                    {
                        return Qt::CheckState(
                            hn->visible?
                                Qt::Checked : Qt::Unchecked );
                    }
                }
            }
            else if (role == Qt::FontRole)
            {
                QFont font;

                if (auto *fn=dynamic_cast<FeatureNode*>(n))
                {
                    if (fn->parentNode()==&sections[feature])
                    {
                        if (model_->isComponent(n->label.toStdString()))
                        {
                            font.setBold(true);
                        }
                    }
                }

                return font;
            }
            else if (role == Qt::BackgroundRole)
            {
                QVariant bgcol;
                if (auto* fn=dynamic_cast<FeatureNode*>(n))
                {
                    return fn->color;
                }
                return bgcol;
            }
        }
    }

    return QVariant();
}




Qt::ItemFlags IQCADItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();

    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    if (auto *n=static_cast<TreeNode*>(index.internalPointer()))
    {

        if ( index.column() == visibilityCol
            && dynamic_cast<HideableNode*>(n) )
        {
            flags |= Qt::ItemIsUserCheckable;
        }
    }

    return flags;
}




bool IQCADItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    auto *n=static_cast<TreeNode*>(index.internalPointer());

    if (role == Qt::CheckStateRole)
    {
        if (auto *hn=dynamic_cast<HideableNode*>(n))
        {
            if ( index.column() == visibilityCol )
            {
                hn->visible =
                    value.value<Qt::CheckState>()==Qt::Checked;

                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
        }
    }
    if (role == Qt::EditRole)
    {
        if (auto *en = dynamic_cast<FeatureNode*>(n))
        {
            if (index.column()==entityColorCol)
            {
                en->color =
                        value.value<QColor>();
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            else if (index.column()==entityOpacityCol)
            {
                en->opacity =
                        value.toDouble();
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            else if (index.column()==entityRepresentationCol)
            {
               en->representation =
                       insight::DatasetRepresentation(value.toInt());
               Q_EMIT dataChanged(index, index, {role});
               return true;
            }
        }
        else if (auto *ds=dynamic_cast<DatasetNode*>(n))
        {
            if (index.column()==datasetFieldNameCol)
            {
                ds->fieldName =
                        value.toString().toStdString();
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            else if (index.column()==datasetPointCellCol)
            {
                ds->fieldSupport =
                        insight::FieldSupport(value.toInt());
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            else if (index.column()==datasetComponentCol)
            {
                ds->fieldComponent =
                        value.toInt();
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            else if (index.column()==datasetMinCol)
            {
                auto& minVal=ds->minVal;
                if (value.isValid())
                {
                    minVal = value.toDouble();
                }
                else
                {
                    minVal.reset();
                }
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            else if (index.column()==datasetMaxCol)
            {
                auto& maxVal=ds->maxVal;
                if (value.isValid())
                {
                    maxVal = value.toDouble();
                }
                else
                {
                    maxVal.reset();
                }
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            else if (index.column()==datasetRepresentationCol)
            {
                ds->representation=insight::DatasetRepresentation(value.toInt());
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
        }
    }

    return false;
}




const insight::cad::ModelPtr IQCADItemModel::model() const
{
    return model_;
}




void IQCADItemModel::setAssociatedParameterSetModel(QAbstractItemModel *psm)
{
    associatedParameterSetModel_=psm;
}

QAbstractItemModel *IQCADItemModel::associatedParameterSetModel() const
{
    return associatedParameterSetModel_;
}




const insight::cad::Model::ScalarTableContents& IQCADItemModel::scalars() const
{
    return model_->scalars();
}




const insight::cad::Model::VectorTableContents& IQCADItemModel::points() const
{
    return model_->points();
}




const insight::cad::Model::VectorTableContents& IQCADItemModel::directions() const
{
    return model_->directions();
}




const insight::cad::Model::DatumTableContents& IQCADItemModel::datums() const
{
    return model_->datums();
}




const insight::cad::Model::ModelstepTableContents& IQCADItemModel::modelsteps() const
{
    return model_->modelsteps();
}




const insight::cad::Model::PostprocActionTableContents& IQCADItemModel::postprocActions() const
{
    return model_->postprocActions();
}




const insight::cad::Model::DatasetTableContents &IQCADItemModel::datasets() const
{
    return model_->datasets();
}




QModelIndex IQCADItemModel::scalarIndex(
    const std::string& name ) const
{
    return sectionIndex(
                name,
                CADModelSection::scalarVariable );
}




QModelIndex IQCADItemModel::pointIndex(
    const std::string& name ) const
{
    return sectionIndex(
                name,
                CADModelSection::pointVariable);
}




QModelIndex IQCADItemModel::directionIndex(
    const std::string& name) const
{
    return sectionIndex(
                name,
                CADModelSection::vectorVariable );
}




QModelIndex IQCADItemModel::datumIndex(
    const std::string& name ) const
{
    return sectionIndex(
                name,
                CADModelSection::datum );
}




QModelIndex IQCADItemModel::modelstepIndex(
    const std::string& name ) const
{
    return sectionIndex(
                name,
                CADModelSection::feature );
}


QModelIndex IQCADItemModel::modelstepIndexFromValue(
    insight::cad::FeaturePtr feat) const
{
    auto list = modelsteps();
    auto si = std::find_if(
        list.begin(), list.end(),
        [feat](const decltype(list)::value_type& mo) {return mo.second == feat; });
    if (si == list.end())
    {
        return QModelIndex();
    }
    else
    {
        auto row = std::distance(list.begin(), si);
        return index(row, 0, index(CADModelSection::feature, 0) );
    }
}



QModelIndex IQCADItemModel::postprocActionIndex(
    const std::string& name ) const
{
    return sectionIndex(
                name,
                CADModelSection::postproc );
}




QModelIndex IQCADItemModel::datasetIndex(
    const std::string& name ) const
{
    return sectionIndex(
                name,
                CADModelSection::dataset );
}




void IQCADItemModel::addScalar(
    const std::string& name,
    insight::cad::ScalarPtr value)
{
    addEntity<insight::cad::ScalarPtr, ScalarNode>(
        name, value,
        scalarVariable,
        std::bind(&insight::cad::Model::scalars, model_.get()),
        std::bind(&insight::cad::Model::addScalar, model_.get(),
             std::placeholders::_1, std::placeholders::_2)//,
        );
}




void IQCADItemModel::addPoint(const std::string& name, insight::cad::VectorPtr value, bool initialVisibility)
{
    addEntity<insight::cad::VectorPtr, PointNode>(
        name, value,
        CADModelSection::pointVariable,
        std::bind(&insight::cad::Model::points, model_.get()),
        std::bind(&insight::cad::Model::addPoint, model_.get(), std::placeholders::_1, std::placeholders::_2),
        [&](PointNode& pn)
        {
            pn.visible=initialVisibility;
        }
        );
}




void IQCADItemModel::addDirection(const std::string& name, insight::cad::VectorPtr value, bool initialVisibility )
{
    addEntity<insight::cad::VectorPtr, VectorNode>(
        name, value,
        CADModelSection::pointVariable,
        std::bind(&insight::cad::Model::directions, model_.get()),
        std::bind(&insight::cad::Model::addDirection, model_.get(), std::placeholders::_1, std::placeholders::_2),
        [&](VectorNode& pn)
        {
            pn.visible=initialVisibility;
        }
        );
}




void IQCADItemModel::addDatum(const std::string& name, insight::cad::DatumPtr value, bool initialVisibility)
{
    addEntity<insight::cad::DatumPtr, DatumNode>(
        name, value,
        CADModelSection::datum,
        std::bind(&insight::cad::Model::datums, model_.get()),
        std::bind(&insight::cad::Model::addDatum, model_.get(), std::placeholders::_1, std::placeholders::_2),
        [&](DatumNode& pn)
        {
            pn.visible=initialVisibility;
        }
        );
}




void IQCADItemModel::addModelstep(
    const std::string& name,
    insight::cad::FeaturePtr value,
    bool isComponent,
    const std::string& featureDescription,
    const boost::variant<boost::blank,insight::cad::FeatureVisualizationStyle>& optfvs)
{
    addEntity<insight::cad::FeaturePtr, FeatureNode>(
        name, value,
        CADModelSection::feature,
        std::bind(&insight::cad::Model::modelsteps, model_.get()),
        std::bind(
            &insight::cad::Model::addModelstep, model_.get(),
            std::placeholders::_1, std::placeholders::_2,
            isComponent, std::string() ),
        [&](FeatureNode& pn)
        {
            if (auto* fvs = boost::get<insight::cad::FeatureVisualizationStyle>(&optfvs))
            {
                pn=*fvs;
            }
            if (!isComponent)
                pn.visible=false;
        }
        );

    auto newidx=modelstepIndex(name);
    auto *featnode=dynamic_cast<FeatureNode*>(
        static_cast<TreeNode*>(
            newidx.internalPointer() ) );

    if (auto* fvs = boost::get<insight::cad::FeatureVisualizationStyle>(&optfvs))
    {
        featnode
            ->assocParamPaths = fvs->associatedParameterPaths;
    }

    if (addSubshapesAsLeafs_)
    {
        for (auto& ss: value->providedSubshapes())
        {
            addDecoration<insight::cad::FeaturePtr, FeatureNode>(
                ss.first, ss.second,
                featnode,
                [&](FeatureNode& pn)
                {
                    pn.visible=false;
                }
            );
        }
    }
}




void IQCADItemModel::setStaticModelStep(const std::string &name, bool isStatic)
{
    if (isStatic)
    {
        staticFeatures_.insert(name);
    }
    else
    {
        auto sf = staticFeatures_.find(name);
        if (sf!=staticFeatures_.end()) staticFeatures_.erase(sf);
    }
}




bool IQCADItemModel::isStaticModelStep(const std::string &name)
{
    return staticFeatures_.find(name)!=staticFeatures_.end();
}




void IQCADItemModel::addPostprocAction(
        const std::string& name,
        insight::cad::PostprocActionPtr value )
{
    addEntity<insight::cad::PostprocActionPtr, PostProcNode>(
        name, value,
        CADModelSection::postproc,
        std::bind(&insight::cad::Model::postprocActions, model_.get()),
        std::bind(&insight::cad::Model::addPostprocAction, model_.get(),
                  std::placeholders::_1, std::placeholders::_2),
        [&](PostProcNode& pn)
        {
            pn.visible=true;
        }
        );
}




void IQCADItemModel::addDataset(const std::string &name, vtkSmartPointer<vtkDataObject> value)
{
    addEntity<vtkSmartPointer<vtkDataObject>, DatasetNode >(
        name, value,
        CADModelSection::dataset,
        std::bind(&insight::cad::Model::datasets, model_.get()),
        std::bind(&insight::cad::Model::addDataset, model_.get(),
                  std::placeholders::_1, std::placeholders::_2),
        [&](DatasetNode& pn)
        {
            if (auto* ds=vtkDataSet::SafeDownCast(value))
            {
                if (ds->GetNumberOfCells()==0 && ds->GetNumberOfPoints()>0)
                    pn.representation=insight::Points;
            }
        }
        );
}




void IQCADItemModel::removeScalar(const std::string& name)
{
    removeEntity<insight::cad::ScalarPtr>(
              name,
              std::bind(&IQCADItemModel::scalarIndex, this, std::placeholders::_1),
              std::bind(&insight::cad::Model::removeScalar, model_.get(), std::placeholders::_1),
              CADModelSection::scalarVariable );
}




void IQCADItemModel::removePoint(const std::string& name)
{
    removeEntity<insight::cad::VectorPtr>(
              name,
              std::bind(&IQCADItemModel::pointIndex, this, std::placeholders::_1),
              std::bind(&insight::cad::Model::removePoint, model_.get(), std::placeholders::_1),
              CADModelSection::pointVariable );
}




void IQCADItemModel::removeDirection(const std::string& name)
{
    removeEntity<insight::cad::VectorPtr>(
              name,
              std::bind(&IQCADItemModel::directionIndex, this, std::placeholders::_1),
              std::bind(&insight::cad::Model::removeDirection, model_.get(), std::placeholders::_1),
              CADModelSection::vectorVariable );
}




void IQCADItemModel::removeDatum(const std::string& name)
{
    removeEntity<insight::cad::DatumPtr>(
              name,
              std::bind(&IQCADItemModel::datumIndex, this, std::placeholders::_1),
              std::bind(&insight::cad::Model::removeDatum, model_.get(), std::placeholders::_1),
              CADModelSection::datum );
}




void IQCADItemModel::removeModelstep(const std::string& name )
{
    removeEntity<insight::cad::FeaturePtr>(
              name,
              std::bind(&IQCADItemModel::modelstepIndex, this, std::placeholders::_1),
              std::bind(
                    &insight::cad::Model::removeModelstep,
                    model_.get(), std::placeholders::_1 ),
              CADModelSection::feature );
}




void IQCADItemModel::removePostprocAction(const std::string& name)
{
    removeEntity<insight::cad::PostprocActionPtr>(
              name,
              std::bind(&IQCADItemModel::postprocActionIndex, this, std::placeholders::_1),
              std::bind(&insight::cad::Model::removePostprocAction, model_.get(), std::placeholders::_1),
                CADModelSection::postproc );
}




void IQCADItemModel::removeDataset(const std::string& name)
{
    removeEntity<vtkSmartPointer<vtkDataObject> >(
              name,
              std::bind(&IQCADItemModel::datasetIndex, this, std::placeholders::_1),
              std::bind(&insight::cad::Model::removeDataset, model_.get(), std::placeholders::_1),
                CADModelSection::dataset );
}





void IQCADItemModel::populateClipPlaneMenu(QMenu *clipplanemenu, IQCADModel3DViewer *viewer)
{
    clipplanemenu->clear();

    if (model_)
    {
        int nAdded=0;
        auto datums = model_->datums();
        for (const auto& dat: datums)
        {
            insight::cad::DatumPtr datpl = dat.second;
            if (datpl->providesPlanarReference())
            {
                QAction *act = new QAction( dat.first.c_str(), this );

                connect
                    (
                      act, &QAction::triggered,
                      std::bind(&IQCADModel3DViewer::toggleClipDatum, viewer, datpl.get())
                    );
                clipplanemenu->addAction(act);

                nAdded++;
            }
        }

        if (nAdded)
        {
            clipplanemenu->setEnabled(true);
        }
        else
        {
            clipplanemenu->setDisabled(true);
        }
    }
}







QModelIndex IQCADItemModel::sectionIndex(
    const std::string &name,
    CADModelSection entitySection ) const
{
    auto *parent=&sections[entitySection];
    if (auto n=parent->childNode(name))
    {
        auto row = parent->childRow(n);
        return index(row, 0, index(entitySection, 0) );
    }
    else
    {
        return QModelIndex();
    }
}



void IQCADItemModel::addSymbolsToSubmenu(
        const QString& name,
        QMenu *menu,
        insight::cad::FeaturePtr feat,
        bool *someSubMenu,
        bool *someHoverDisplay )
{
  if (feat->getDatumScalars().size()>0)
  {
    if (someSubMenu) *someSubMenu=true;
    QMenu *sm = new QMenu("Scalar Symbols");
    menu->addMenu(sm);
    for (auto i: feat->getDatumScalars())
    {
      QAction *a = new QAction(
                  QString::fromStdString(
                      boost::str(boost::format("%s to Notepad (= %g)") % i.first % i.second)
                      ), menu );
      sm->addAction(a);
      connect(a, &QAction::triggered, this, [=]() {
               Q_EMIT insertIntoNotebook( name+"$"+QString::fromStdString(i.first) );
              } );
    }
  }
  if (feat->getDatumPoints().size()>0)
  {
    if (someSubMenu) *someSubMenu=true;
    QMenu *sm = new QMenu("Point Symbols");
    menu->addMenu(sm);
    for (auto i: feat->getDatumPoints())
    {
      QAction *a = new QAction( QString::fromStdString(
              boost::str(boost::format("%s to Notepad (= [%g %g %g])")
                         % i.first % i.second(0) % i.second(1) % i.second(2))
              ), menu );
      sm->addAction(a);
      connect(a, &QAction::triggered, this, [=]() {
          Q_EMIT insertIntoNotebook( name+"@"+QString::fromStdString(i.first) );
        });
//      connect(a, &QAction::hovered,
//              [=]() {
//        gp_Pnt p=to_Pnt(i.second);
//        Handle_AIS_Point ip(new AIS_Point(
//           Handle_Geom_Point(new Geom_CartesianPoint(p) )
//                                             ));
//        ip->SetMarker(Aspect_TOM_O_PLUS);
//        ip->SetWidth(5);
//        focus(ip);
//      });
      if (someHoverDisplay) *someHoverDisplay=true;
    }
  }
  if (feat->getDatumVectors().size()>0)
  {
    if (someSubMenu) *someSubMenu=true;
    QMenu *sm = new QMenu("Vector Symbols");
    menu->addMenu(sm);
    for (auto i: feat->getDatumVectors())
    {
      QAction *a = new QAction(
                  QString::fromStdString(
                      boost::str(boost::format("%s to Notepad (= [%g %g %g])") % i.first % i.second(0) % i.second(1) % i.second(2))
                      ), menu );
      sm->addAction(a);
      connect(a, &QAction::triggered, this, [=]() {
          Q_EMIT insertIntoNotebook( name+"^"+QString::fromStdString(i.first) );
        });
    }
  }
  if (feat->providedSubshapes().size()>0)
  {
    if (someSubMenu) *someSubMenu=true;
    QMenu *sm = new QMenu("Subshapes");
    menu->addMenu(sm);
    for (auto i: feat->providedSubshapes())
    {
        QString subname=QString::fromStdString(i.first);
        if (subname!="PREV") // cause too deep recursion
        {

            QAction *a = new QAction(
                subname + " to Notepad",
                menu );
            sm->addAction(a);

            connect(a, &QAction::triggered, this, [=]() {
                Q_EMIT insertIntoNotebook( name+"."+QString::fromStdString(i.first) );
            });

            connect(a, &QAction::hovered, this, [=]() {
                Q_EMIT highlightInView(i.second);
            });

            bool someEntries=false;
            QMenu* submenu=new QMenu(QString::fromStdString(i.first));

            addSymbolsToSubmenu(name+"."+subname, submenu, i.second, &someEntries);
            if (someEntries)
            {
                sm->addMenu(submenu);
            }
            else
            {
                delete submenu;
            }
        }

      if (someHoverDisplay) *someHoverDisplay=true;
    }
  }
}



