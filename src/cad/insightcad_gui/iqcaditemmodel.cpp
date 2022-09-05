#include "iqcaditemmodel.h"

#include <QMenu>
#include <QInputDialog>
#include <QColorDialog>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <boost/random/exponential_distribution.hpp>

boost::mt19937 boostRanGen;

IQCADItemModel::FeatureVisibility::FeatureVisibility()
{
    boost::random::exponential_distribution<> ed(2);
    color.setRedF(      std::max(0., std::min(1., ed(boostRanGen))) );
    color.setGreenF(    std::max(0., std::min(1., ed(boostRanGen))) );
    color.setBlueF(     std::max(0., std::min(1., ed(boostRanGen))) );
    opacity=1.;
    visible=true;
}

IQCADItemModel::IQCADItemModel(insight::cad::ModelPtr m, QObject* parent)
  : QAbstractItemModel(parent)
{
    if (m)
        model_=m;
    else
        model_=std::make_shared<insight::cad::Model>();

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
        if (section == visibilityCol)
            return QVariant("Show");
        else if (section == labelCol)
            return QVariant("Name");
        else if (section == valueCol)
            return QVariant("Value");
      }
    }
    return QVariant();
}

QModelIndex IQCADItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        // top level
        if (row>=0 && row<CADModelSection::numberOf)
        {
            return createIndex(row, column, quintptr(INT_MAX));
        }
    }
    else
    {
        switch (parent.row())
        {
        case CADModelSection::scalarVariable:
            if (row>=0 && row<model_->scalars().size())
            {
                return createIndex(row, column, quintptr(parent.row()));
            }
            break;
        case CADModelSection::pointVariable:
            if (row>=0 && row<model_->points().size())
            {
                return createIndex(row, column, quintptr(parent.row()));
            }
            break;
        case CADModelSection::vectorVariable:
            if (row>=0 && row<model_->directions().size())
            {
                return createIndex(row, column, quintptr(parent.row()));
            }
            break;
        case CADModelSection::datum:
            if (row>=0 && row<model_->datums().size())
            {
                return createIndex(row, column, quintptr(parent.row()));
            }
            break;
        case CADModelSection::feature:
            if (row>=0 && row<model_->modelsteps().size())
            {
                return createIndex(row, column, quintptr(parent.row()));
            }
            break;
        case CADModelSection::postproc:
            if (row>=0 && row<model_->postprocActions().size())
            {
                return createIndex(row, column, quintptr(parent.row()));
            }
            break;
        case CADModelSection::dataset:
            if (row>=0 && row<model_->datasets().size())
            {
                return createIndex(row, column, quintptr(parent.row()));
            }
            break;
        }
    }

    return QModelIndex();
}

QModelIndex IQCADItemModel::parent(const QModelIndex &index) const
{
    if (index.isValid() && index.internalId()>=0 && index.internalId()<CADModelSection::numberOf)
    {
        return createIndex(index.internalId(), 0, quintptr(INT_MAX));
    }
    return QModelIndex();
}

int IQCADItemModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        return CADModelSection::numberOf;
    }
    else if (!parent.parent().isValid())
    {
        if (parent.column()==0)
        {
            switch (parent.row())
            {
            case CADModelSection::scalarVariable:
                return model_->scalars().size();
                break;
            case CADModelSection::pointVariable:
                return model_->points().size();
                break;
            case CADModelSection::vectorVariable:
                return model_->directions().size();
                break;
            case CADModelSection::datum:
                return model_->datums().size();
                break;
            case CADModelSection::feature:
                return model_->modelsteps().size();
                break;
            case CADModelSection::postproc:
                return model_->postprocActions().size();
                break;
            case CADModelSection::dataset:
                return model_->datasets().size();
                break;
            }
        }
    }
    return 0;
}

int IQCADItemModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

QVariant IQCADItemModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid())
    {
        if (role==Qt::DisplayRole)
        {
            if (!index.parent().isValid() && index.column()==labelCol)
            {
                switch (index.row())
                {
                case CADModelSection::scalarVariable:
                    return "Scalars";
                    break;
                case CADModelSection::pointVariable:
                    return "Points";
                    break;
                case CADModelSection::vectorVariable:
                    return "Vectors";
                    break;
                case CADModelSection::datum:
                    return "Datums";
                    break;
                case CADModelSection::feature:
                    return "Features";
                    break;
                case CADModelSection::postproc:
                    return "Post Proc";
                    break;
                case CADModelSection::dataset:
                    return "Datasets";
                    break;
                }
            }
            else if (index.internalId()>=0 && index.internalId()<CADModelSection::numberOf)
            {
                switch (index.internalId())
                {
                case CADModelSection::scalarVariable:
                    {
                        auto scalars = model_->scalars();                        
                        auto i = scalars.begin();
                        std::advance(i, index.row());

                        if (index.column()==labelCol)
                            return QString::fromStdString(i->first);
                        else if (index.column()==valueCol)
                            return QString::number(i->second->value());
                    }
                    break;
                case CADModelSection::pointVariable:
                    {
                        auto points = model_->points();
                        auto i = points.begin();
                        std::advance(i, index.row());

                        if (index.column()==labelCol)
                            return QString::fromStdString(i->first);
                        else if (index.column()==valueCol)
                        {
                            auto v=i->second->value();
                            return QString("[%1 %2 %3]").arg(v[0]).arg(v[1]).arg(v[2]);
                        }
                    }
                    break;
                case CADModelSection::vectorVariable:
                    {
                        auto directions = model_->directions();
                        auto i = directions.begin();
                        std::advance(i, index.row());

                        if (index.column()==labelCol)
                            return QString::fromStdString(i->first);
                        else if (index.column()==valueCol)
                        {
                            auto v=i->second->value();
                            return QString("[%1 %2 %3]").arg(v[0]).arg(v[1]).arg(v[2]);
                        }
                    }
                    break;
                case CADModelSection::datum:
                    {
                        auto datums = model_->datums();
                        auto i = datums.begin();
                        std::advance(i, index.row());
                        if (index.column()==labelCol)
                        {
                            return QString::fromStdString(i->first);
                        }
                        else if (index.column()==entityCol)
                        {
                            QVariant r;
                            r.setValue(i->second);
                            return r;
                        }
                    }
                    break;
                case CADModelSection::feature:
                    {
                        auto features = model_->modelsteps();
                        auto i = features.begin();
                        std::advance(i, index.row());
                        if (index.column()==labelCol)
                        {
                            return QString::fromStdString(i->first);
                        }
                        else if (index.column()==valueCol)
                        {
                            return QString::fromStdString(i->second->type());
                        }
                        else if (index.column()==entityCol)
                        {
                            QVariant r;
                            r.setValue(i->second);
                            return r;
                        }
                        else if (index.column()==entityColorCol)
                        {
                            return featureVisibility_[i->first].color;
                        }
                        else if (index.column()==entityOpacityCol)
                        {
                            return featureVisibility_[i->first].opacity;
                        }
                    }
                    break;
                case CADModelSection::postproc:
                    {
                        auto postprocacts = model_->postprocActions();
                        auto i = postprocacts.begin();
                        std::advance(i, index.row());
                        if (index.column()==labelCol)
                        {
                            return QString::fromStdString(i->first);
                        }
                        else if (index.column()==valueCol)
                        {
                            return QString::fromStdString(i->second->type());
                        }
                        else if (index.column()==entityCol)
                        {
                            QVariant r;
                            r.setValue(i->second);
                            return r;
                        }
                    }
                    break;
                case CADModelSection::dataset:
                    {
                        auto ds = model_->datasets();
                        auto i = ds.begin();
                        std::advance(i, index.row());
                        if (index.column()==labelCol)
                        {
                            return QString::fromStdString(i->first);
                        }
                        else if (index.column()==valueCol)
                        {
                            return QString::fromStdString(i->second->GetClassName());
                        }
                        else if (index.column()==entityCol)
                        {
                            QVariant r;
                            r.setValue(i->second);
                            return r;
                        }
                        else if (index.column()==datasetFieldNameCol)
                        {
                            return QString::fromStdString(
                                        datasetVisibility_[i->first].fieldName
                                        );
                        }
                        else if (index.column()==datasetPointCellCol)
                        {
                            return int(
                                        datasetVisibility_[i->first].fieldSupport
                                        );
                        }
                        else if (index.column()==datasetComponentCol)
                        {
                            return int(
                                        datasetVisibility_[i->first].fieldComponent
                                        );
                        }
                        else if (index.column()==datasetMinCol)
                        {
                            auto mi = datasetVisibility_[i->first];
                            if (mi.minVal) return double(*mi.minVal);
                        }
                        else if (index.column()==datasetMaxCol)
                        {
                            auto mi = datasetVisibility_[i->first];
                            if (mi.maxVal) return double(*mi.maxVal);
                        }
                        else if (index.column()==datasetRepresentationCol)
                        {
                            auto mi = datasetVisibility_[i->first];
                            return int(mi.representation);
                        }
                    }
                    break;
                }
            }
        }
        else if (role == Qt::CheckStateRole)
        {
            if (index.internalId()>=0 && index.internalId()<CADModelSection::numberOf)
            {
                switch (index.internalId())
                {
                    case CADModelSection::datum:
                        {
                            auto datums = model_->datums();
                            auto i = datums.begin();
                            std::advance(i, index.row());
                            if (index.column()==visibilityCol)
                            {
                                return Qt::CheckState( datumVisibility_[i->first]?
                                            Qt::Checked : Qt::Unchecked );
                            }
                        }
                        break;
                    case CADModelSection::feature:
                        {
                            auto features = model_->modelsteps();
                            auto i = features.begin();
                            std::advance(i, index.row());
                            if (index.column()==visibilityCol)
                            {
                                return Qt::CheckState(
                                            featureVisibility_[i->first].visible?
                                            Qt::Checked:Qt::Unchecked );
                            }
                        }
                        break;
                    case CADModelSection::dataset:
                        {
                            auto ds = model_->datasets();
                            auto i = ds.begin();
                            std::advance(i, index.row());
                            if (index.column()==visibilityCol)
                            {
                                return Qt::CheckState(
                                            datasetVisibility_[i->first].visible?
                                            Qt::Checked:Qt::Unchecked
                                            );
                            }
                        }
                        break;
                }
            }

        }
    }

    return QVariant();
}




Qt::ItemFlags IQCADItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    if ( index.column() == visibilityCol && (
             index.internalId()==CADModelSection::datum
             || index.internalId()==CADModelSection::feature
             || index.internalId()==CADModelSection::dataset
             ) )
    {
        flags |= Qt::ItemIsUserCheckable;
    }

    return flags;
}




bool IQCADItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole)
    {
        if ( index.column() == visibilityCol )
        {
            if (index.internalId()==CADModelSection::datum)
            {
                auto datums = model_->datums();
                auto i=datums.begin();
                std::advance(i, index.row());
                datumVisibility_[i->first] =
                        ( value.value<Qt::CheckState>()==Qt::Checked );
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            else if (index.internalId()==CADModelSection::feature)
            {
                auto features = model_->modelsteps();
                auto i=features.begin();
                std::advance(i, index.row());
                featureVisibility_[i->first].visible =
                        ( value.value<Qt::CheckState>()==Qt::Checked );
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            else if (index.internalId()==CADModelSection::dataset)
            {
                auto ds = model_->datasets();
                auto i=ds.begin();
                std::advance(i, index.row());
                datasetVisibility_[i->first].visible =
                        ( value.value<Qt::CheckState>()==Qt::Checked );
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
        }
    }
    if (role == Qt::EditRole)
    {
        if (index.internalId()==CADModelSection::feature)
        {
            auto feat = model_->modelsteps();
            auto i=feat.begin();
            std::advance(i, index.row());

            if (index.column()==entityColorCol)
            {
                featureVisibility_[i->first].color =
                        value.value<QColor>();
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            else if (index.column()==entityOpacityCol)
            {
                featureVisibility_[i->first].opacity =
                        value.toDouble();
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
        }
        else if (index.internalId()==CADModelSection::dataset)
        {
            auto ds = model_->datasets();
            auto i=ds.begin();
            std::advance(i, index.row());

            if (index.column()==datasetFieldNameCol)
            {
                datasetVisibility_[i->first].fieldName =
                        value.toString().toStdString();
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            else if (index.column()==datasetPointCellCol)
            {
                datasetVisibility_[i->first].fieldSupport =
                        insight::FieldSupport(value.toInt());
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            else if (index.column()==datasetComponentCol)
            {
                datasetVisibility_[i->first].fieldComponent =
                        value.toInt();
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
            else if (index.column()==datasetMinCol)
            {
                auto& minVal=datasetVisibility_[i->first].minVal;
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
                auto& maxVal=datasetVisibility_[i->first].maxVal;
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
                auto& dv=datasetVisibility_[i->first];
                dv.representation=insight::DatasetRepresentation(value.toInt());
                Q_EMIT dataChanged(index, index, {role});
                return true;
            }
        }
    }

    return false;
}


insight::cad::Model::ScalarTableContents IQCADItemModel::scalars() const
{
    return model_->scalars();
}

insight::cad::Model::VectorTableContents IQCADItemModel::points() const
{
    return model_->points();
}

insight::cad::Model::VectorTableContents IQCADItemModel::directions() const
{
    return model_->directions();
}

insight::cad::Model::DatumTableContents IQCADItemModel::datums() const
{
    return model_->datums();
}

insight::cad::Model::ModelstepTableContents IQCADItemModel::modelsteps() const
{
    return model_->modelsteps();
}

insight::cad::Model::PostprocActionTableContents IQCADItemModel::postprocActions() const
{
    return model_->postprocActions();
}

const insight::cad::Model::DatasetTableContents &IQCADItemModel::datasets() const
{
    return model_->datasets();
}

QModelIndex IQCADItemModel::scalarIndex(const std::string& name) const
{
    return sectionIndex<insight::cad::ScalarPtr>(
                name,
                std::bind(&insight::cad::Model::scalars, model_),
                CADModelSection::scalarVariable);
}

QModelIndex IQCADItemModel::pointIndex(const std::string& name) const
{
    return sectionIndex<insight::cad::VectorPtr>(
                name,
                std::bind(&insight::cad::Model::points, model_),
                CADModelSection::pointVariable);
}

QModelIndex IQCADItemModel::directionIndex(const std::string& name) const
{
    return sectionIndex<insight::cad::VectorPtr>(
                name,
                std::bind(&insight::cad::Model::directions, model_),
                CADModelSection::vectorVariable);
}

QModelIndex IQCADItemModel::datumIndex(const std::string& name) const
{
    return sectionIndex<insight::cad::DatumPtr>(
                name,
                std::bind(&insight::cad::Model::datums, model_),
                CADModelSection::datum);
}

QModelIndex IQCADItemModel::modelstepIndex(const std::string& name) const
{
    return sectionIndex<insight::cad::FeaturePtr>(
                name,
                std::bind(&insight::cad::Model::modelsteps, model_),
                CADModelSection::feature);
}

QModelIndex IQCADItemModel::postprocActionIndex(const std::string& name) const
{
    return sectionIndex<insight::cad::PostprocActionPtr>(
                name,
                std::bind(&insight::cad::Model::postprocActions, model_),
                CADModelSection::postproc);
}

QModelIndex IQCADItemModel::datasetIndex(const std::string& name) const
{
    return sectionIndex<vtkSmartPointer<vtkDataObject> >(
                name,
                std::bind(&insight::cad::Model::datasets, model_),
                CADModelSection::dataset);
}



void IQCADItemModel::addScalar(const std::string& name, insight::cad::ScalarPtr value)
{
    addEntity<insight::cad::ScalarPtr>(
              name, value,
              std::bind(&insight::cad::Model::scalars, model_.get()),
              std::bind(&IQCADItemModel::scalarIndex, this, std::placeholders::_1),
              std::bind(&insight::cad::Model::addScalar, model_.get(), std::placeholders::_1, std::placeholders::_2),
              CADModelSection::scalarVariable );
}

void IQCADItemModel::addPoint(const std::string& name, insight::cad::VectorPtr value)
{
    addEntity<insight::cad::VectorPtr>(
              name, value,
              std::bind(&insight::cad::Model::points, model_.get()),
              std::bind(&IQCADItemModel::pointIndex, this, std::placeholders::_1),
              std::bind(&insight::cad::Model::addPoint, model_.get(), std::placeholders::_1, std::placeholders::_2),
              CADModelSection::pointVariable );
}

void IQCADItemModel::addDirection(const std::string& name, insight::cad::VectorPtr value)
{
    addEntity<insight::cad::VectorPtr>(
              name, value,
              std::bind(&insight::cad::Model::directions, model_.get()),
              std::bind(&IQCADItemModel::directionIndex, this, std::placeholders::_1),
              std::bind(&insight::cad::Model::addDirection, model_.get(), std::placeholders::_1, std::placeholders::_2),
              CADModelSection::pointVariable );
}


void IQCADItemModel::addDatum(const std::string& name, insight::cad::DatumPtr value)
{
    addEntity<insight::cad::DatumPtr>(
              name, value,
              std::bind(&insight::cad::Model::datums, model_.get()),
              std::bind(&IQCADItemModel::datumIndex, this, std::placeholders::_1),
              std::bind(&insight::cad::Model::addDatum, model_.get(), std::placeholders::_1, std::placeholders::_2),
              CADModelSection::datum );
}


void IQCADItemModel::addModelstep(
        const std::string& name,
        insight::cad::FeaturePtr value,
        const std::string& featureDescription )
{
    addEntity<insight::cad::FeaturePtr>(
              name, value,
              std::bind(&insight::cad::Model::modelsteps, model_.get()),
              std::bind(&IQCADItemModel::modelstepIndex, this, std::placeholders::_1),
              std::bind(
                    &insight::cad::Model::addModelstep,
                    model_.get(), std::placeholders::_1, std::placeholders::_2, std::string() ),
              CADModelSection::feature );
}



void IQCADItemModel::addComponent(
        const std::string& name,
        insight::cad::FeaturePtr value,
        const std::string& featureDescription )
{
    addEntity<insight::cad::FeaturePtr>(
              name, value,
              std::bind(&insight::cad::Model::modelsteps, model_.get()),
              std::bind(&IQCADItemModel::modelstepIndex, this, std::placeholders::_1),
              std::bind(
                    &insight::cad::Model::addComponent,
                    model_.get(), std::placeholders::_1, std::placeholders::_2, std::string()),
              CADModelSection::feature );
}


void IQCADItemModel::addPostprocAction(
        const std::string& name,
        insight::cad::PostprocActionPtr value )
{
    addEntity<insight::cad::PostprocActionPtr>(
              name, value,
              std::bind(&insight::cad::Model::postprocActions, model_.get()),
              std::bind(&IQCADItemModel::postprocActionIndex, this, std::placeholders::_1),
              std::bind(&insight::cad::Model::addPostprocAction, model_.get(), std::placeholders::_1, std::placeholders::_2),
                CADModelSection::postproc );
}

void IQCADItemModel::addDataset(const std::string &name, vtkSmartPointer<vtkDataObject> value)
{
    // set *before* addEntity
    auto&dvv = datasetVisibility_[name];
    if (auto* ds=vtkDataSet::SafeDownCast(value))
    {
        if (ds->GetNumberOfCells()==0 && ds->GetNumberOfPoints()>0)
            dvv.representation=insight::Points;
    }

    addEntity<vtkSmartPointer<vtkDataObject> >(
              name, value,
              std::bind(&insight::cad::Model::datasets, model_.get()),
              std::bind(&IQCADItemModel::datasetIndex, this, std::placeholders::_1),
              std::bind(&insight::cad::Model::addDataset, model_.get(), std::placeholders::_1, std::placeholders::_2),
                CADModelSection::dataset );
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
              CADModelSection::pointVariable );
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



void IQCADItemModel::showContextMenu(const QModelIndex &idx, const QPoint &pos)
{
    QMenu cm;

    QAction *a;

    a=new QAction("Show", &cm);
    connect(a, &QAction::triggered,
            [this,idx]() {
                QModelIndex visi=index(idx.row(), IQCADItemModel::visibilityCol, idx.parent());
                if (flags(visi)&Qt::ItemIsUserCheckable)
                {
                    setData(visi, Qt::Checked, Qt::CheckStateRole);
                }
            });
    cm.addAction(a);

    a=new QAction("Hide", &cm);
    connect(a, &QAction::triggered,
            [this,idx]() {
                QModelIndex visi=index(idx.row(), IQCADItemModel::visibilityCol, idx.parent());
                if (flags(visi)&Qt::ItemIsUserCheckable)
                {
                    setData(visi, Qt::Unchecked, Qt::CheckStateRole);
                }
            });
    cm.addAction(a);

    if (idx.internalId()==CADModelSection::datum)
    {
//        auto datums = model_->datums();
//        auto i=datums.begin();
//        std::advance(i, index.row());
//        datumVisibility_[i->second] =
//                ( value.value<Qt::CheckState>()==Qt::Checked );
//        Q_EMIT dataChanged(index, index, {role});
//        return true;
    }
    else if (idx.internalId()==CADModelSection::feature)
    {
        a=new QAction("Set opacity...", &cm);
        connect(a, &QAction::triggered, this,
                [this,idx]() {
                    bool ok=false;
                    double val=QInputDialog::getDouble(nullptr, "opacity", "opacity", 1, 0, 1, 2, &ok);
                    if (ok)
                    {
                        QModelIndex visi=index(idx.row(), IQCADItemModel::entityOpacityCol, idx.parent());
                        setData(visi, val, Qt::EditRole);
                    }
                });
        cm.addAction(a);

        a=new QAction("Set color...", &cm);
        connect(a, &QAction::triggered, this,
                [this,idx]() {
                    bool ok=false;
                    QModelIndex ci=index(idx.row(), IQCADItemModel::entityColorCol, idx.parent());
                    auto val=QColorDialog::getColor(
                                data(ci).value<QColor>(),
                                nullptr,
                                "Set Color");
                    if (val.isValid())
                    {
                        setData(ci, val, Qt::EditRole);
                    }
                });
        cm.addAction(a);
    }
    else if (idx.internalId()==CADModelSection::dataset)
    {
        a=new QAction("Set representation...", &cm);
        connect(a, &QAction::triggered, this,
                [this,idx]() {
                    bool ok=false;
                    auto n=QInputDialog::getItem(nullptr,
                                                 "repr", "representation",
                                                 {"Points", "Wireframe", "Surface"},
                                                 2, false, &ok);
                    if (ok)
                    {
                        int i;
                        if (n=="Points")
                            i=VTK_POINTS;
                        else if (n=="Wireframe")
                            i=VTK_WIREFRAME;
                        else if (n=="Surface")
                            i=VTK_SURFACE;

                             QModelIndex visi=index(idx.row(), IQCADItemModel::datasetRepresentationCol, idx.parent());
                             setData(visi, i, Qt::EditRole);
                    }
                });
        cm.addAction(a);

        a=new QAction("Auto scale set data range", &cm);
        connect(a, &QAction::triggered, this,
                [this,idx]() {
                     QModelIndex visi=index(idx.row(), IQCADItemModel::datasetMinCol, idx.parent());
                     setData(visi, QVariant(), Qt::EditRole);
                     visi=index(idx.row(), IQCADItemModel::datasetMaxCol, idx.parent());
                     setData(visi, QVariant(), Qt::EditRole);
                });
        cm.addAction(a);
        a=new QAction("Manual set data range minimum", &cm);
        connect(a, &QAction::triggered, this,
                [this,idx]() {
                    bool ok=false;
                    double val=QInputDialog::getDouble(nullptr, "minimum", "minimum", 0, -DBL_MAX, DBL_MAX, 1, &ok);
                    if (ok)
                    {
                        QModelIndex visi=index(idx.row(), IQCADItemModel::datasetMinCol, idx.parent());
                        setData(visi, val, Qt::EditRole);
                    }
                });
        cm.addAction(a);
        a=new QAction("Manual set data range maximum", &cm);
        connect(a, &QAction::triggered, this,
                [this,idx]() {
                    bool ok=false;
                    double val=QInputDialog::getDouble(nullptr, "maximum", "maximum", 0, -DBL_MAX, DBL_MAX, 1, &ok);
                    if (ok)
                    {
                        QModelIndex visi=index(idx.row(), IQCADItemModel::datasetMaxCol, idx.parent());
                        setData(visi, val, Qt::EditRole);
                    }
                });
        cm.addAction(a);

//        auto ds = model_->datasets();
//        auto i=ds.begin();
//        std::advance(i, index.row());
//        datasetVisibility_[i->second] =
//                ( value.value<Qt::CheckState>()==Qt::Checked );
//        Q_EMIT dataChanged(index, index, {role});
//        return true;
    }

    cm.exec(pos);
}
