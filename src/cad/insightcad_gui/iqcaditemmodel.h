#ifndef IQCADMODELCONTAINER_H
#define IQCADMODELCONTAINER_H

#include "insightcad_gui_export.h"
#include "iscadmetatyperegistrator.h"

#include "cadmodel.h"

#include "base/tools.h"

#include <QAbstractItemModel>


class INSIGHTCAD_GUI_EXPORT IQCADItemModel
    : public QAbstractItemModel
{
  Q_OBJECT

  insight::cad::ModelPtr model_;

  mutable std::map<insight::cad::DatumPtr, bool> datumVisibility_;
  mutable std::map<insight::cad::FeaturePtr, bool> featureVisibility_;



public:
  enum CADModelSection {
      scalarVariable = 0,
      pointVariable = 1,
      vectorVariable = 2,
      datum = 3,
      feature = 4,
      postproc = 5,
      numberOf =6
  };

private:
  template<class E>
  void addEntity(
          const std::string& name,
          E value,
          std::function<std::map<std::string,E>(void)> efunc,
          std::function<QModelIndex(const std::string&)> eidxfunc,
          std::function<void(const std::string&,E)> modeladdfunc,
          CADModelSection esect
          )
  {
      auto s=efunc();
      auto ss = s.find(name);
      if (ss!=s.end())
      {
          if (ss->second==value)
          {
              // is present and the same
              return;
          }
          else
          {
              // replace
              auto i = eidxfunc(name);
              modeladdfunc(name, value);
              Q_EMIT dataChanged(i, i);
          }
      }
      else
      {
          // insert new
          auto newrow = insight::predictInsertionLocation(s, name);
          beginInsertRows(index(esect, 0), newrow, newrow);
          modeladdfunc(name, value);
          endInsertRows();
      }
  }

  template<class E>
  QModelIndex sectionIndex(
          const std::string& name,
          std::function<std::map<std::string,E>(void)> efunc,
          CADModelSection esect
          ) const
  {
      auto s=efunc();
      auto si=s.find(name);
      if (si==s.end())
          return QModelIndex();
      else
      {
          auto row = std::distance(si, s.begin());
          return index(row, 0, index(esect, 0) );
      }
  }


public:
  IQCADItemModel(insight::cad::ModelPtr model=insight::cad::ModelPtr(), QObject* parent=nullptr);
  virtual ~IQCADItemModel();

  /**
   * abstract item model overrides
   */
  // Header:
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  // Basic functionality:
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &index) const override;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  Qt::ItemFlags flags(const QModelIndex &index) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;


  /**
   * read access functions
   */
  insight::cad::Model::ScalarTableContents scalars() const;
  insight::cad::Model::VectorTableContents points() const;
  insight::cad::Model::VectorTableContents directions() const;
  insight::cad::Model::DatumTableContents datums() const;
  insight::cad::Model::ModelstepTableContents modelsteps() const;
  insight::cad::Model::PostprocActionTableContents postprocActions() const;

  QModelIndex scalarIndex(const std::string& name) const;
  QModelIndex pointIndex(const std::string& name) const;
  QModelIndex directionIndex(const std::string& name) const;
  QModelIndex datumIndex(const std::string& name) const;
  QModelIndex modelstepIndex(const std::string& name) const;
  QModelIndex postprocActionIndex(const std::string& name) const;


  /**
   * modifier functions
   */
  void addScalar(const std::string& name, insight::cad::ScalarPtr value);
  void addPoint(const std::string& name, insight::cad::VectorPtr value);
  void addDirection(const std::string& name, insight::cad::VectorPtr value);
  void addDatum(const std::string& name, insight::cad::DatumPtr value);
  void addModelstep(const std::string& name, insight::cad::FeaturePtr value, const std::string& featureDescription = std::string() );
  void addComponent(const std::string& name, insight::cad::FeaturePtr value, const std::string& featureDescription = std::string() );
  void addPostprocAction(const std::string& name, insight::cad::PostprocActionPtr value);

  void removeScalar(const std::string& name);
  void removePoint(const std::string& name);
  void removeDirection(const std::string& name);
  void removeDatum(const std::string& name);
  void removeModelstep(const std::string& name );
  void removePostprocAction(const std::string& name);
};

#endif // IQCADMODELCONTAINER_H
