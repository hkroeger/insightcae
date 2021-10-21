#ifndef CASECONFIGURATIONMODEL_H
#define CASECONFIGURATIONMODEL_H

#include <QAbstractListModel>

#include "insertedcaseelement.h"

#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/caseelements/numerics/fvnumerics.h"

#include "rapidxml/rapidxml.hpp"

class ParameterSetDisplay;
class ParameterEditorWidget;

class CaseConfigurationModel : public QAbstractListModel
{
  Q_OBJECT

  QList<CaseElementData*> caseElements_;

  CaseElementData* caseElementByIndex(const QModelIndex& index);

public:
  explicit CaseConfigurationModel(QObject *parent = nullptr);

  // Header:
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  // Basic functionality:
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  const CaseElementData *caseElement(const QModelIndex& index) const;
  QList<const InsertedCaseElement*> allCaseElements() const;
  int nCaseElements() const;

  void clear();
  void addCaseElement(CaseElementData *ce);
  void removeElement(const QModelIndex& index);
  QString applicationName(const QString& OFEname) const;

  insight::ParameterSet& caseElementParametersRef(int id);

  void appendConfigurationToNode(
      rapidxml::xml_document<>& doc,
      rapidxml::xml_node<> *rootnode,
      bool pack,
      const boost::filesystem::path& fileParentPath ) const;

  void readFromNode(
      rapidxml::xml_document<>& doc,
      rapidxml::xml_node<> *rootnode,
      insight::Multi_CAD_ParameterSet_Visualizer* mv,
      const boost::filesystem::path& fileParentPath );

  template<class T>
  bool containsCE(const QString& OFEname) const
  {
    insight::OpenFOAMCase ofc(insight::OFEs::get(OFEname.toStdString()));
    for ( int i=0; i < caseElements_.count(); i++ )
      {
        auto* cur =
          dynamic_cast<InsertedCaseElement*> ( caseElements_[i] );
        if ( cur )
          {
            std::unique_ptr<insight::OpenFOAMCaseElement> ce( cur->createElement(ofc) );
            if ( dynamic_cast<T*>(ce.get()) ) return true;
          }
      }
    return false;
  }

  ParameterEditorWidget* launchParameterEditor(
      const QModelIndex& index,
      QWidget* parentWidget,
      ParameterSetDisplay* display );

};

#endif // CASECONFIGURATIONMODEL_H
