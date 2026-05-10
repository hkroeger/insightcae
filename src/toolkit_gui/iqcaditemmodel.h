#ifndef IQCADMODELCONTAINER_H
#define IQCADMODELCONTAINER_H

#include "cadtypes.h"
#include "toolkit_gui_export.h"
#include "iscadmetatyperegistrator.h"

#include "cadmodel.h"

#include "base/tools.h"
#include "base/vtkrendering.h"

#include <QAbstractItemModel>
#include <QDebug>
#include <QApplication>
#include <QColor>
#include <QMenu>
#include <iterator>
#include <qnamespace.h>


class IQCADModel3DViewer;
class IQParameterSetModel;


struct TreeNode
: public QObject
{
    Q_OBJECT

protected:
    mutable std::set<QObject*> tba_, tbd_;
    mutable std::map<std::string, QObject*> childrenInOrder_;

    void childEvent(QChildEvent *event) override;

public:
    const std::map<std::string, QObject*>&
    childrenList() const;

    QString label;
    TreeNode* parentNode() const;


    template<class TN=TreeNode>
    TN* childNode(const std::string& label) const
    {
        auto i=childrenList().find(label);
        if (i!=childrenList().end())
        {
            return dynamic_cast<TN*>(i->second);
        }
        return nullptr;
    }

    template<class TN=TreeNode>
    TN* childNode(int row) const
    {
        if ( (childrenList().size()>0) &&
             (row>=0) &&
             (row<=childrenList().size()) )
        {
            auto i=childrenList().begin();
            std::advance(i, row);
            return dynamic_cast<TN*>(i->second);
        }
        else
            return nullptr;
    }

    int childRow(TreeNode* n) const;

    int nChildNodes() const;

    virtual QString valueString() const =0;
    virtual QVariant valueAsVariant() const =0;

    TreeNode(QString label, TreeNode *parent=nullptr);
};




struct SectionNode
    : public TreeNode
{
    QString valueString() const override;
    QVariant valueAsVariant() const override;

    SectionNode(QString label, TreeNode *parent=nullptr);
};


struct ScalarNode
    : public TreeNode
{
    insight::cad::ScalarPtr value;

    QString valueString() const override;
    QVariant valueAsVariant() const override;

    using TreeNode::TreeNode;
};

struct HideableNode
    : public TreeNode
{
    bool visible;

    HideableNode(QString label, TreeNode *parent);
};

struct PointNode
    : public HideableNode
{
    insight::cad::VectorPtr value;

    QString valueString() const override;
    QVariant valueAsVariant() const override;

    using HideableNode::HideableNode;
};

struct VectorNode
    : public HideableNode
{
    insight::cad::VectorPtr value;

    QString valueString() const override;
    QVariant valueAsVariant() const override;

    using HideableNode::HideableNode;
};

struct DatumNode
    : public HideableNode
{
    insight::cad::DatumPtr value;

    QString valueString() const override;
    QVariant valueAsVariant() const override;

    using HideableNode::HideableNode;
};



struct FeatureNode
    : public HideableNode
{
    double opacity;
    QColor color;
    insight::DatasetRepresentation representation;
    std::vector<std::string> assocParamPaths;

    insight::cad::FeaturePtr value;

    QString valueString() const override;
    QVariant valueAsVariant() const override;

    void operator=(const insight::cad::FeatureVisualizationStyle& fvs);

    FeatureNode(QString label, TreeNode *parent);
};

struct PostProcNode
    : public HideableNode
{
    insight::cad::PostprocActionPtr value;

    QString valueString() const override;
    QVariant valueAsVariant() const override;

    using HideableNode::HideableNode;
};


struct DatasetNode
    : public HideableNode
{
    vtkSmartPointer<vtkDataObject> value;

    /**
       * @brief fieldName
       * empty string: first field
       */
    std::string fieldName = "";

    insight::FieldSupport fieldSupport = insight::OnPoint;

    insight::DatasetRepresentation representation = insight::Surface;

    /**
       * @brief fieldComponent
       * -1 for mag
       */
    int fieldComponent = -1;

    boost::optional<double> minVal;
    boost::optional<double> maxVal;

    QString valueString() const override;
    QVariant valueAsVariant() const override;

    using HideableNode::HideableNode;
};




class TOOLKIT_GUI_EXPORT IQCADItemModel
    : public QAbstractItemModel
{
  Q_OBJECT


  bool addSubshapesAsLeafs_;
    /**
   * @brief model_
   * The underlying CAD model.
   * Should be modified only through access functions of this class to ensure consistency of the views.
   */
  insight::cad::ModelPtr model_;

  QAbstractItemModel* associatedParameterSetModel_;

  std::set<std::string>
    staticFeatures_;


public:
  enum CADModelSection {
      scalarVariable = 0,
      pointVariable = 1,
      vectorVariable = 2,
      datum = 3,
      feature = 4,
      postproc = 5,
      dataset = 6,
      numberOf =7
  };

  static const int
      visibilityCol=0,
      labelCol=0,
      valueCol=1,

      datasetFieldNameCol = 2,
      datasetPointCellCol = 3,
      datasetComponentCol = 4,
      datasetMinCol = 5,
      datasetMaxCol = 6,
      datasetRepresentationCol = 7,

      entityColorCol = 2,
      entityOpacityCol = 3,
      entityRepresentationCol = 4, // insight::DatasetRepresentation
      entityCol=99,

      assocParamPathsCol=98;

private:
  mutable std::array<SectionNode, numberOf> sections;

    QModelIndex indexOf(TreeNode *n) const
    {
        int row;
        if (auto *pn=n->parentNode())
        {
            row=pn->childRow(n);
        }
        else
        {
            for (row=0; row<numberOf; ++row)
            { if (&sections[row]==n) break; }
        }
        return createIndex(row, 0, n);
    }

    template<class E, class TN>
    void addDecoration(
        const std::string& name,
        E value,
        TreeNode* parentNode,
        std::function<void(TN&)> modifyNode
        )
    {
        if (auto n=parentNode->childNode<TN>(name) )
        {
            if (n->value==value)
            {
                // is present and the same
                // Q_EMIT dataChanged(ie, ie.siblingAtColumn(entityCol), {Qt::EditRole});
            }
            else
            {
                // content has changed
                // replace
                n->value=value;
                auto ie=indexOf(n);
                Q_EMIT dataChanged(ie, ie.siblingAtColumn(entityCol), {Qt::EditRole});
            }
        }
        else
        {
            // insert new
            auto newrow = insight::predictInsertionLocation(
                parentNode->childrenList(), name);
            beginInsertRows(indexOf(parentNode), newrow, newrow);
            auto *ntn=new TN(QString::fromStdString(name), parentNode);
            if (modifyNode) modifyNode(*ntn);
            ntn->value=value;
            endInsertRows();
        }
    }


    template<class E, class TN>
    void addEntity(
        const std::string& name,
        E value,
        CADModelSection sc,
        std::function<const std::map<std::string,E>&(void)>
            entityAccessFunction,
        std::function<void(const std::string&,E)>
            modeladdfunc,
        std::function<void(TN&)> modifyNode =
        std::function<void(TN&)>()
        )
    {
        TreeNode* parentNode=&sections[sc];
        auto s = entityAccessFunction();
        auto ss = s.find(name);
        if (ss!=s.end())
        {
            if (ss->second==value)
            {
                // is present and the same
                // Q_EMIT dataChanged(ie, ie.siblingAtColumn(entityCol), {Qt::EditRole});
            }
            else
            {
                // content has changed
                // replace
                modeladdfunc(name, value);
                addDecoration<E, TN>(name, value, parentNode, modifyNode);
            }
        }
        else
        {
            // insert new
            modeladdfunc(name, value);
            addDecoration<E, TN>(name, value, parentNode, modifyNode);
        }
    }

  template<class E>
  void removeEntity(
          const std::string& name,
          std::function<QModelIndex(const std::string&)> eidxfunc,
          std::function<void(const std::string&)> modelremovefunc,
          CADModelSection esect
          )
  {
      auto idx = eidxfunc(name);
      if (idx.isValid())
      {
          beginRemoveRows(index(esect, 0), idx.row(), idx.row());
          if (auto *n=static_cast<TreeNode*>(idx.internalPointer()))
          {
              delete n;
          }
          modelremovefunc(name);
          endRemoveRows();
      }
  }


  QModelIndex sectionIndex(
          const std::string& name,
          CADModelSection entitySection
      ) const;

  void addSymbolsToSubmenu(
          const QString& name,
          QMenu *menu,
          insight::cad::FeaturePtr feat,
          bool *someSubMenu = nullptr,
          bool *someHoverDisplay = nullptr );

public:
  IQCADItemModel(
        insight::cad::ModelPtr model=insight::cad::ModelPtr(),
        QObject* parent=nullptr,
        bool addSubshapesAsLeafs = false );
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


  const insight::cad::ModelPtr model() const;

  void setAssociatedParameterSetModel(QAbstractItemModel* psm);
  QAbstractItemModel *associatedParameterSetModel() const;

  /**
   * read access functions
   */
  const insight::cad::Model::ScalarTableContents& scalars() const;
  const insight::cad::Model::VectorTableContents& points() const;
  const insight::cad::Model::VectorTableContents& directions() const;
  const insight::cad::Model::DatumTableContents& datums() const;
  const insight::cad::Model::ModelstepTableContents& modelsteps() const;
  const insight::cad::Model::PostprocActionTableContents& postprocActions() const;
  const insight::cad::Model::DatasetTableContents& datasets() const;

  QModelIndex scalarIndex(const std::string& name) const;
  QModelIndex pointIndex(const std::string& name) const;
  QModelIndex directionIndex(const std::string& name) const;
  QModelIndex datumIndex(const std::string& name) const;
  QModelIndex modelstepIndex(const std::string& name) const;
  QModelIndex modelstepIndexFromValue(insight::cad::FeaturePtr feat) const;
  QModelIndex postprocActionIndex(const std::string& name) const;
  QModelIndex datasetIndex(const std::string& name) const;


  /**
   * modifier functions
   */
  void addScalar(const std::string& name, insight::cad::ScalarPtr value);
  void addPoint(const std::string& name, insight::cad::VectorPtr value, bool initialVisibility = false);
  void addDirection(const std::string& name, insight::cad::VectorPtr value, bool initialVisibility = false);
  void addDatum(const std::string& name, insight::cad::DatumPtr value, bool initialVisibility=false);
  void addModelstep(
      const std::string& name,
      insight::cad::FeaturePtr value,
      bool isComponent,
      const std::string& featureDescription = std::string(),
      const boost::variant<boost::blank,insight::cad::FeatureVisualizationStyle>& fvs =
        boost::blank() );

  void setStaticModelStep(const std::string& name, bool isStatic);
  bool isStaticModelStep(const std::string& name);
  // void addComponent(
  //     const std::string& name,
  //     insight::cad::FeaturePtr value,
  //     const std::string& featureDescription = std::string(),
  //     const insight::cad::FeatureVisualizationStyle& fvs =
  //       insight::cad::FeatureVisualizationStyle::componentStyle() );

  void addPostprocAction(const std::string& name, insight::cad::PostprocActionPtr value);
  void addDataset(const std::string& name, vtkSmartPointer<vtkDataObject> value);

  void removeScalar(const std::string& name);
  void removePoint(const std::string& name);
  void removeDirection(const std::string& name);
  void removeDatum(const std::string& name);
  void removeModelstep(const std::string& name );
  void removePostprocAction(const std::string& name);
  void removeDataset(const std::string& name);

  void populateClipPlaneMenu(QMenu* clipplanemenu, IQCADModel3DViewer* v);

public Q_SLOTS:
  void showContextMenu(const QModelIndex& idx, const QPoint &pos, IQCADModel3DViewer* viewer);
  void showMultiSelectionContextMenu(const QModelIndexList& idxs, const QPoint &pos, IQCADModel3DViewer* viewer);

  void addPlane();
  void addImportedFeature();
  void addImportedSketch(insight::cad::DatumPtr plane);

Q_SIGNALS:
  void insertIntoNotebook(const QString& text);

  // no QModelIndex as parameter, since non-indexed shape may be highlighted
  void highlightInView(insight::cad::FeaturePtr feat);
  void undoHighlightInView();

  void jumpToDefinition(const QString& name);
  void insertParserStatementAtCursor(const QString& name);
};

#endif // IQCADMODELCONTAINER_H
