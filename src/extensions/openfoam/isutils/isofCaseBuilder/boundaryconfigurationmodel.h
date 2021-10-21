#ifndef BOUNDARYCONFIGURATIONMODEL_H
#define BOUNDARYCONFIGURATIONMODEL_H

#include <QAbstractListModel>

#include "patch.h"

class ParameterEditorWidget;
class ParameterSetDisplay;

class BoundaryConfigurationModel : public QAbstractListModel
{
  Q_OBJECT

  DefaultPatch* defaultPatch_;
  QList<Patch*> patches_;

  Patch* patchByIndex(const QModelIndex& index);
  Patch* patchByName(const std::string& patchName);

public:
  BoundaryConfigurationModel(DefaultPatch* defaultPatch, QObject *parent = nullptr);

  // Header:
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  const Patch* patch(const QModelIndex& index) const;

  void clear();
  void addPatchConfiguration(Patch* patch);
  void addUnconfiguredPatchIfNonexistent(const std::string& patchName);
  void resetBCType(const QModelIndex& index, const std::string& BCtype);
  void removePatch(const QModelIndex& index);
  void renamePatch(const QModelIndex& index, const QString& newPatchName);

  bool isValid() const;
  const DefaultPatch* defaultPatch() const;
  QList<const Patch *> allNamedPatches() const;

  insight::ParameterSet& patchParametersRef(const std::string& patchName);


  void appendConfigurationToNode(
      rapidxml::xml_document<>& doc, rapidxml::xml_node<> *rootnode,
      bool pack,
      const boost::filesystem::path& fileParentPath ) const;

  void readFromNode(
      rapidxml::xml_document<>& doc,
      rapidxml::xml_node<> *rootnode,
      insight::Multi_CAD_ParameterSet_Visualizer* mv,
      const boost::filesystem::path& fileParentPath );

  ParameterEditorWidget* launchParameterEditor(
      const QModelIndex& index,
      QWidget* parentWidget,
      ParameterSetDisplay* display );
};

#endif // BOUNDARYCONFIGURATIONMODEL_H
