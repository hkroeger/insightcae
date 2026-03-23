

#include "base/parameter.h"
#include "base/parameters/labeledarrayparameter.h"
#include "base/parameters/selectablesubsetparameter.h"
#include "base/parameters/subsetparameter.h"
#include "iqparametersetmodel.h"

#include "iqparameters/iqlabeledarrayparameter.h"

#include <QMimeData>

bool IQParameterSetModel::isInsertableContainer(const QModelIndex &index) const
{
    auto *ip=elementOfIndex(index);
    return
        dynamic_cast<const insight::ArrayParameter*>(ip)
        || dynamic_cast<const insight::LabeledArrayParameter*>(ip)
        ;
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
                if (auto *lap =
                    dynamic_cast<const insight::LabeledArrayParameter*>(
                        &p->parent() ) )
                {
                    if (!lap->keysAreLocked())
                        flags |= Qt::ItemIsEditable;
                }
            }

            if (index.isValid())
            {
                flags |= Qt::ItemIsDragEnabled;
            // if (isInsertableContainer(index)) // only insertable containers as drop targets
                flags |= Qt::ItemIsDropEnabled;
            }
        }
    }

    return flags;
}



Qt::DropActions IQParameterSetModel::supportedDragActions() const
{
    if (editingIsEnabled())
    {
        return Qt::CopyAction | Qt::MoveAction;
    }
    else
        return IQHierarchicalDataModel::supportedDragActions();
}


Qt::DropActions IQParameterSetModel::supportedDropActions() const
{
    if (editingIsEnabled())
    {
        return Qt::CopyAction | Qt::MoveAction;
    }
    else
        return IQHierarchicalDataModel::supportedDropActions();
}




QStringList IQParameterSetModel::mimeTypes() const
{
    return { "application/xml" };
}




QMimeData *IQParameterSetModel::mimeData(const QModelIndexList &indexes) const
{
    auto* mimeData = new QMimeData();


    insight::XMLDocument doc;

    // store each selected element into XML by its name
    for (const auto& index: indexes)
    {
        if (index.column()==0) // one index per col is issued
        {
            auto *ip=elementOfIndex(index);
            ip->appendToNode(
                ip->name(),
                doc, *doc.rootNode,
                insight::hierarchicalData::Element::OutputProperties());
        }
    }

    std::ostringstream os;
    doc.saveToStream(os);

    insight::dbg()<<os.str();

    mimeData->setData(
        "application/xml",
        QByteArray::fromStdString(os.str()) );

    return mimeData;
}



bool IQParameterSetModel::canDropMimeData(
    const QMimeData *data,
    Qt::DropAction action, int row, int col,
    const QModelIndex &parent) const
{
    if (!data->hasFormat("application/xml"))
        return false;

    // if (action == Qt::MoveAction && !isInsertableContainer(parent))
    //     return false;

    return true;
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
        if ( auto *ip = dynamic_cast<insight::Parameter*>(
                elementOfIndex(parent)) )
        {
            std::string iptype=ip->type();
            // parse, what we got
            std::string contents(data->data("application/xml").toStdString());
            insight::XMLDocument doc(contents.begin(), contents.end());

            std::string allTypesEqual(doc.rootNode->first_node()->name());
            int nArgs = 0;
            for (auto *e = doc.rootNode->first_node(); e!=nullptr; e=e->next_sibling())
            {
                std::string type(e->name());
                nArgs++;
                if (type!=allTypesEqual) allTypesEqual="";
            }

            auto setIntoParameter = [](
                const rapidxml::xml_node<>& node, insight::Parameter& target)
            {
                target.readFromNode(std::string(), node);
            };

            auto setIntoSubset = [&](insight::ParameterSet& target)
            {
                for (auto& e: target)
                {
                    e.readFromNode(e.name(), *doc.rootNode);
                }
            };

            auto iap = dynamic_cast<insight::ArrayParameter*>(ip);
            auto ilap = dynamic_cast<insight::LabeledArrayParameter*>(ip);
            const insight::Parameter *deflParam{nullptr};
            if (iap)
            {
                deflParam = &iap->defaultValue();
            }
            else if (ilap)
            {
                deflParam = &ilap->defaultValue();
            }

            //1. target is a container
            if ( (iap || ilap) && (allTypesEqual==deflParam->type()) )
            {
            //    1. a list of array elements was supplied, sources may be deleted
                for (auto *e = doc.rootNode->first_node();
                     e!=nullptr; e=e->next_sibling())
                {
                    insight::dbg() << "append dropped to array"<<std::endl;

                    auto np = deflParam->cloneAs<insight::Parameter>();

                    setIntoParameter( *e, *np );

                    if (row<0) // append
                    {
                        if (iap)
                        {
                            iap->appendValue(std::move(np));
                        }
                        else if (ilap) // labeled array
                        {
                            ilap->appendValue(std::move(np));
                        }
                    }
                    else if (row>=0) //insert
                    {
                        if (iap) // array
                        {
                            iap->insertValue(row, std::move(np));
                        }
                        else if (ilap) // labeled array
                        {
                            auto lbl=ilap->childElementName(row);

                            int attempts=0;
                            while (ilap->hasKey(lbl) && (attempts++)<20)
                                lbl+="_2";

                            if (!ilap->hasKey(lbl))
                            {
                                ilap->insertValue(lbl, std::move(np));
                            }
                        }
                    }
                }

                return true;
            }
            //2. target is not a container
            else
            {
            //    1. target is a set, insert matching elements, no source deletion
                if (auto *ss =
                    dynamic_cast<insight::ParameterSet*>(
                        ip ) )
                {
                    insight::dbg() << "set into subset"<<std::endl;
                    setIntoSubset(*ss);
                }
            //    2. target is a subset, insert matching elements into selected set, no source deletion
                else if (auto *sss =
                         dynamic_cast<insight::SelectableSubsetParameter*>(
                             ip ) )
                {
                    insight::dbg() << "set into selectable subset"<<std::endl;
                    setIntoSubset((*sss)());
                }
            //    3. target is a single element of matching type, no source deletion
                else if (ip->type()==allTypesEqual)
                {
                    insight::dbg() << "set into parameter"<<std::endl;
                    setIntoParameter(
                        *doc.rootNode
                            ->first_node(),
                        *ip);
                }
                return false;
            }
        }
    }

    return false;
}
