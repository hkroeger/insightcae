#include "boundaryconfigurationmodel.h"

#include "insertedcaseelement.h"
#include "parametereditorwidget.h"

#include "isofcasebuilderwindow.h"

using namespace insight;
using namespace rapidxml;

Patch* BoundaryConfigurationModel::patchByIndex(const QModelIndex& index)
{
  if (!index.isValid())
    return nullptr;

  int i=index.row();
  if (i==0)
    return defaultPatch_;
  else
    return patches_[i-1];
}


Patch* BoundaryConfigurationModel::patchByName(const std::string& patchName)
{
  Patch* p=nullptr;

  for(auto* cp: patches_)
  {
    if (cp->patch_name()==patchName)
    {
      p=cp;
      break;
    }
  }

  return p;
}



BoundaryConfigurationModel::BoundaryConfigurationModel(DefaultPatch* defaultPatch, QObject *parent)
  : QAbstractListModel(parent),
    defaultPatch_(defaultPatch)
{
  defaultPatch->setParent(this);
}




QVariant BoundaryConfigurationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation==Qt::Horizontal && role==Qt::DisplayRole)
  {
    switch (section)
    {
      case 0: return "Patch name";
      case 1: return "BC type";
    }
  }
  return QVariant();
}




int BoundaryConfigurationModel::rowCount(const QModelIndex &parent) const
{
  if (parent.isValid())
    return 0;

  return patches_.size() +1;
}




int BoundaryConfigurationModel::columnCount(const QModelIndex &parent) const
{
  if (parent.isValid())
    return 0;

  return 2;
}




QVariant BoundaryConfigurationModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (role==Qt::DisplayRole)
  {
    auto r=index.row();
    Patch *patch = defaultPatch_;
    if (r>0)
      patch = patches_[r-1];

    switch (index.column())
    {
      case 0:
        return QString::fromStdString( patch->patch_name() );
      case 1:
        return QString::fromStdString( patch->type_name() );
    }
  }

  return QVariant();
}


 void BoundaryConfigurationModel::clear()
 {
   auto n=patches_.size();
   beginRemoveRows(QModelIndex(), 1, n);
   for (auto* p: patches_)
   {
     p->deleteLater();
   }
   patches_.clear();
   endRemoveRows();
 }

const Patch* BoundaryConfigurationModel::patch(const QModelIndex& index) const
{
  return const_cast<BoundaryConfigurationModel*>(this)->patchByIndex(index);
}






void BoundaryConfigurationModel::addPatchConfiguration(Patch* patch)
{
  if (auto *p = patchByName(patch->patch_name()))
  {
    // exists already, replace
    int r=patches_.indexOf(p);
    patch->setParent(this);
    patches_[r]=patch;
    Q_EMIT dataChanged(
          createIndex(r+1, 0),
          createIndex(r+1, 1)
          );
    p->deleteLater();
  }
  else
  {
    int r=patches_.size();
    // not there, append
    beginInsertRows(QModelIndex(), r+1, r+1);
    patches_.append(patch);
    endInsertRows();
  }
}


void BoundaryConfigurationModel::addUnconfiguredPatchIfNonexistent(
    const std::string& patchName )
{
  if (patchByName(patchName)==nullptr)
  {
    int r=patches_.size();
    // not there, append
    beginInsertRows(QModelIndex(), r+1, r+1);
    auto *ice = new Patch(
        patchName,
        defaultPatch_->multiVisualizer(),
        dynamic_cast<MultivisualizationGenerator*>(parent()),
        this);
    patches_.append(ice);
    endInsertRows();
  }
}

void BoundaryConfigurationModel::resetBCType(const QModelIndex& index, const std::string& BCtype)
{
  if (index.isValid())
  {
    auto *p=patchByIndex(index);
    p->set_bc_type(BCtype);
    Q_EMIT dataChanged(
          createIndex(index.row(), 0),
          createIndex(index.row(), 1)
          );
  }
}


void BoundaryConfigurationModel::removePatch(const QModelIndex& index)
{
  if (index.isValid())
  {
    if (index.row()<1)
    {
      throw insight::Exception("the default patch cannot be removed!");
    }

    int r=index.row()-1;
    auto *p=patchByIndex(index);
    beginRemoveRows(QModelIndex(), r, r);
    patches_.removeAt(r);
    p->deleteLater();
    endRemoveRows();
  }
}


void BoundaryConfigurationModel::renamePatch(const QModelIndex& index, const QString& newPatchName)
{
  if (auto *p=patchByIndex(index))
  {
    if (index.row()<1)
    {
      throw insight::Exception("the default patch cannot be renamed!");
    }
    p->set_patch_name(newPatchName);
    auto r=index.row();
    Q_EMIT dataChanged(
          createIndex(r, 0),
          createIndex(r, 1)
          );
  }
}


bool BoundaryConfigurationModel::isValid() const
{
  return (
        ( patches_.size() > 0 )
        ||
        (
          ( patches_.size() == 0 )
          &&
          !defaultPatch_->type_name().empty()
        )
        );
}



const DefaultPatch *BoundaryConfigurationModel::defaultPatch() const
{
  return defaultPatch_;
}



QList<const Patch *> BoundaryConfigurationModel::allNamedPatches() const
{
  QList<const Patch *> result;
  for (const auto *p : patches_)
  {
    if (!p->type_name().empty())
      result.append(p);
  }
  return result;
}



insight::Parameter& BoundaryConfigurationModel::patchParameterRef(
    const std::string &patchName, const std::string& path)
{
  Patch* p=patchByName(patchName);

  if (!p)
  {
    throw insight::Exception(
          "Error: patch \""+patchName+"\" was not found!"
          );
  }

  if (p->type_name().empty())
  {
    throw insight::Exception(
        "Error: Requested patch \""+patchName+"\" has no valid configuration!"
        );
  }

  return p->parameterSetModel()->parameterRef(path);
}



void BoundaryConfigurationModel::appendConfigurationToNode(
    rapidxml::xml_document<>& doc, rapidxml::xml_node<> *BCnode,
    bool pack,
    const boost::filesystem::path& fileParentPath ) const
{
  xml_node<> *unassignedBCnode = doc.allocate_node ( node_element, "UnassignedPatches" );
  if (pack)
  {
    defaultPatch_->parameterSetModel()->pack();
  }
  else
  {
    defaultPatch_->parameterSetModel()->clearPackedData();
  }
  defaultPatch_->appendToNode(doc, *unassignedBCnode, fileParentPath);
  BCnode->append_node ( unassignedBCnode );

  for (int i=0; i < patches_.size(); i++)
  {
      xml_node<> *patchnode = doc.allocate_node ( node_element, "Patch" );
      auto *p = patches_[i];
      if (pack)
      {
        p->parameterSetModel()->pack();
      }
      else
      {
        p->parameterSetModel()->clearPackedData();
      }
      p->appendToNode(doc, *patchnode, fileParentPath);
      BCnode->append_node ( patchnode );
  }
}

void BoundaryConfigurationModel::readFromNode(
    rapidxml::xml_document<> &doc,
    rapidxml::xml_node<> *BCnode,
    insight::MultiCADParameterSetVisualizer::SubVisualizerList& mvl,
    MultivisualizationGenerator* visGen,
    const boost::filesystem::path &fileParentPath)
{
  clear();

  if (xml_node<> *unassignedBCnode =
      BCnode->first_node( "UnassignedPatches" ))
  {
    defaultPatch_ = new DefaultPatch(doc, *unassignedBCnode, fileParentPath, mvl, visGen);
    Q_EMIT dataChanged(
          createIndex(0, 0),
          createIndex(0, 1)
          );
  }

  for (xml_node<> *e = BCnode->first_node("Patch");
       e; e = e->next_sibling("Patch"))
  {
      auto *ice=new Patch(doc, *e, fileParentPath, mvl, visGen);
      addPatchConfiguration( ice );
  }
}


ParameterEditorWidget* BoundaryConfigurationModel::launchParameterEditor(
    const QModelIndex &index,
    QWidget* parentWidget,
    IQVTKParameterSetDisplay* display
    )
{
  if (auto *pc = dynamic_cast<Patch*>(patchByIndex(index)))
  {
    if (!pc->type_name().empty())
    {
      insight::ParameterSet_ValidatorPtr vali;

      try {
          vali = insight::OpenFOAMCaseElement::validatorFor(pc->type_name());
      } catch (const std::exception& e)
      { /* ignore, if non-existent */ }

      insight::CADParameterSetModelVisualizer
          ::CreateGUIActionsFunctions::Function cgaf;

      if (insight::CADParameterSetModelVisualizer
          ::createGUIActionsForOpenFOAMCaseElement().count(pc->type_name()))
      {
          cgaf=insight::CADParameterSetModelVisualizer
                 ::createGUIActionsForOpenFOAMCaseElement().lookup(pc->type_name());
      }

      auto ppe = new ParameterEditorWidget
             (
               parentWidget,
               PECADParameterSetVisualizerBuilder(),
               cgaf,
               vali,
               display
             );
      auto model = pc->parameterSetModel();
      ppe->setModel(model);

      // ensure that the editor is removed, when CE is deleted
      connect(pc, &QObject::destroyed,
              ppe, &ParameterEditorWidget::deleteLater );

      return ppe;
    }
  }

  return nullptr;
}
