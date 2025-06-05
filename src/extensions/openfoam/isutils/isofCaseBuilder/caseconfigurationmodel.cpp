#include "caseconfigurationmodel.h"

#include "parametereditorwidget.h"
#include "isofcasebuilderwindow.h"

using namespace insight;
using namespace rapidxml;




CaseElementData *CaseConfigurationModel::caseElementByIndex(const QModelIndex &index)
{
  if (!index.isValid())
    return nullptr;

  return caseElements_[index.row()];
}




CaseConfigurationModel::CaseConfigurationModel(QObject *parent)
  : QAbstractListModel(parent)
{}




QVariant CaseConfigurationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation==Qt::Horizontal && role==Qt::DisplayRole)
  {
    switch (section)
    {
      case 0: return "Case element type";
    }
  }
  return QVariant();
}




int CaseConfigurationModel::rowCount(const QModelIndex &parent) const
{
  // For list models only the root node (an invalid parent) should return the list's size. For all
  // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
  if (parent.isValid())
    return 0;

  return caseElements_.size();
}




int CaseConfigurationModel::columnCount(const QModelIndex &parent) const
{
  if (parent.isValid())
    return 0;

  return 1;
}




QVariant CaseConfigurationModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (role==Qt::DisplayRole)
  {
    auto ce = caseElements_[index.row()];
    return QString::fromStdString( ce->type_name() );
  }

  return QVariant();
}




const CaseElementData *CaseConfigurationModel::caseElement(const QModelIndex &index) const
{
  return const_cast<CaseConfigurationModel*>(this)->caseElementByIndex(index);
}




QList<const InsertedCaseElement *> CaseConfigurationModel::allCaseElements() const
{
  QList<const InsertedCaseElement *> constCaseElements_;
  for (int i=0; i<caseElements_.size(); ++i)
    if (auto *ce = dynamic_cast<InsertedCaseElement*>(caseElements_[i]))
      constCaseElements_.append(ce);
  return constCaseElements_;
}




int CaseConfigurationModel::nCaseElements() const
{
  return caseElements_.size();
}




void CaseConfigurationModel::clear()
{
  beginRemoveRows(QModelIndex(), 0, caseElements_.size()-1);
  for (auto* ce: caseElements_)
    ce->deleteLater();
  caseElements_.clear();
  endRemoveRows();
}




QModelIndex CaseConfigurationModel::addCaseElement(CaseElementData *ce)
{
  int r = caseElements_.size();
  beginInsertRows(QModelIndex(), r, r);
  ce->setParent(this);
  caseElements_.append(ce);
  endInsertRows();

  return createIndex(caseElements_.size()-1, 0);
}




void CaseConfigurationModel::removeElement(const QModelIndex& index)
{
  if (!index.isValid())
    return;
  auto r=index.row();
  beginRemoveRows(QModelIndex(), r, r);
  caseElements_[r]->deleteLater();
  caseElements_.removeAt(r);
  endRemoveRows();
}




QString CaseConfigurationModel::applicationName(const QString& OFEname) const
{
  insight::OpenFOAMCase ofc(
        insight::OFEs::get(OFEname.toStdString()) );

  for ( int i=0; i < caseElements_.count(); i++ )
    {
      auto* cur = dynamic_cast<InsertedCaseElement*> ( caseElements_[i] );
      if ( cur )
        {
          std::unique_ptr<insight::OpenFOAMCaseElement> ce( cur->createElement(ofc) );
          if ( const auto* fvn = dynamic_cast<const insight::FVNumerics*>(ce.get()) )
          {
            insight::OFdicts dicts;
            fvn->addIntoDictionaries(dicts);
            try
            {
              OFDictData::dict cd = dicts.lookupDict("system/controlDict");
              std::string appname = cd.getString("application");
              return QString(appname.c_str());
            }
            catch (const std::exception& e)
            {
              // continue
            }
          }
        }
    }
  return QString();
}




insight::Parameter &CaseConfigurationModel::caseElementParameterRef(int id, const std::string& path)
{
  if (auto *ce = caseElementByIndex(index(id,0)))
  {
    return ce->parameterSetModel()->parameterRef(path);
  }
  else
  {
    throw insight::Exception(
        boost::str(boost::format("Error: Requested case element #%d is not valid!")%id)
    );
  }
}




void CaseConfigurationModel::appendConfigurationToNode(
    xml_document<>& doc,
    xml_node<> *rootnode,
    bool pack,
    const boost::filesystem::path& fileParentPath ) const
{
  for (int i=0; i < caseElements_.count(); i++)
  {
      auto* elem = caseElements_[i];
      if (elem)
      {
          xml_node<> *elemnode = doc.allocate_node ( node_element, "OpenFOAMCaseElement" );
          elemnode->append_attribute(doc.allocate_attribute("type", elem->type_name().c_str()));
          rootnode->append_node ( elemnode );

          if (pack)
          {
            elem->parameterSetModel()->pack();
          }
          else
          {
            elem->parameterSetModel()->clearPackedData();
          }
          elem->parameterSetModel()->getParameterSet()
              .appendToNode(std::string(), doc, *elemnode, fileParentPath);
      }
  }
}

void CaseConfigurationModel::readFromNode(
    const rapidxml::xml_node<> &rootnode,
    insight::MultiCADParameterSetVisualizer::SubVisualizerList& mvl,
    MultivisualizationGenerator* visGen,
    const boost::filesystem::path& fileParentPath )
{
  clear();

  for (auto *e = rootnode.first_node("OpenFOAMCaseElement");
       e;  e = e->next_sibling("OpenFOAMCaseElement"))
  {
    auto typeattr = e->first_attribute("type");
    insight::assertion(typeattr, "Missing \"type\" attribute!");
    std::string type_name( typeattr->value() );

    auto* ice = new InsertedCaseElement(
          type_name, mvl, visGen
          );

    auto np = ice->parameterSetModel()->getParameterSet().cloneParameterSet();
    np->readFromNode(std::string(), *e, fileParentPath);
    ice->parameterSetModel()->resetParameterValues(*np);

    addCaseElement( ice );
  }

}

ParameterEditorWidget *CaseConfigurationModel::launchParameterEditor(
    const QModelIndex &index,
    QWidget *parentWidget,
    IQVTKParameterSetDisplay *display )
{
  auto* ce = caseElementByIndex(index);

  insight::ParameterSet_ValidatorPtr vali;

  try {
      vali = insight::OpenFOAMCaseElement::validatorFor(ce->type_name());
  } catch (const std::exception& e)
  { /* ignore, if non-existent */ }

  insight::CADParameterSetModelVisualizer
      ::CreateGUIActionsFunctions::Function cgaf;

  if (insight::CADParameterSetModelVisualizer
      ::createGUIActionsForOpenFOAMCaseElement().count(ce->type_name()))
  {
      cgaf=insight::CADParameterSetModelVisualizer
             ::createGUIActionsForOpenFOAMCaseElement().lookup(ce->type_name());
  }

  auto cepe = new ParameterEditorWidget
         (
           parentWidget,
           PECADParameterSetVisualizerBuilder(),
           cgaf,
           vali,
           display
         );

  auto model = ce->parameterSetModel();        
  cepe->setModel(model);

  // ensure that the editor is removed, when CE is deleted
  connect(ce, &QObject::destroyed,
          cepe, &ParameterEditorWidget::deleteLater );


  return cepe;
}
