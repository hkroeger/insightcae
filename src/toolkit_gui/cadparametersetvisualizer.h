#ifndef PARAMETERSETVISUALIZER_H
#define PARAMETERSETVISUALIZER_H

#include "base/cppextensions.h"
#include "base/factory.h"
#include "toolkit_gui_export.h"


#include <QObject>
#include <QThread>
#include <QIcon>
#include <QColor>
#include <QTimer>

#include "base/analysis.h"
#include "base/supplementedinputdata.h"
#include "cadtypes.h"
#include "iqiscadmodelgenerator.h"
#include "iqiscadmodelrebuilder.h"

#include "AIS_DisplayMode.hxx"


class IQCADItemModel;
class IQParameterSetModel;
class IQCADModel3DViewer;

namespace insight
{


arma::mat vec3(const QColor& s);

std::string visPath(const std::vector<std::string>& path);

std::string visPath();

template<typename T, typename... Args>
std::string visPath(T t, Args... args) // recursive variadic function
{
    std::string head = t;
    std::string tail = visPath(args...);
    return head + ((head.size()&&tail.size())?"/":"") + tail;
}

struct CameraState
{
    arma::mat focalPosition;
    arma::mat cameraPosition;
    arma::mat viewUp;
    double parallelScale;
    bool isParallelProjection;


    CameraState(
        arma::mat focalPosition = insight::vec3Zero(),
        arma::mat cameraPosition = insight::vec3(1,1,1),
        arma::mat viewUp = insight::vec3Z(1),
        double parallelScale = 1.,
        bool isParallelProjection = true
        );
};

class CADParameterSetModelVisualizer;


struct GUIAction
{
    QIcon icon;
    QString label, tooltip;
    std::function<void(void)> action;
};
typedef std::vector<GUIAction> GUIActionList;



class TOOLKIT_GUI_EXPORT CADParameterSetVisualizerGenerator
    : public IQISCADModelGenerator
{

    Q_OBJECT

protected:
    boost::filesystem::path workDir_;
    ProgressDisplayer& progress_;


public:
    /**
   * @brief CADParameterSetVisualizer
   * sets up the model rebuild and launches background rebuild
   */
    CADParameterSetVisualizerGenerator(
        QObject* parent,
        const boost::filesystem::path& workDir,
        ProgressDisplayer& progress
        );

    virtual void launch(IQCADItemModel *model) =0;

    // stop
    virtual void stopVisualizationComputation() =0;
    virtual bool isFinished() const =0;

    virtual void addPoint(
        const std::string& name,
        const arma::mat& p,
        bool initialVisibility=true);

    virtual void addDatum(
        const std::string& name,
        insight::cad::DatumPtr dat,
        bool initialVisibility=false);

    virtual void addFeature(
        const std::string& name,
        insight::cad::FeaturePtr feat,
        const insight::cad::FeatureVisualizationStyle& fvs =
        insight::cad::FeatureVisualizationStyle::componentStyle() );

    virtual void addDataset(
        const std::string& name,
        vtkSmartPointer<vtkDataObject> ds );


Q_SIGNALS:
    void visualizationCalculationFinished(bool success);
    void updateSupplementedInputData(insight::supplementedInputDataBasePtr sid);
    void visualizationComputationError(insight::Exception ex);
};




class TOOLKIT_GUI_EXPORT CADParameterSetModelVisualizer
: public CADParameterSetVisualizerGenerator
{

  Q_OBJECT

public:
  declareType("CADParameterSetModelVisualizer");

    enum Status { BeforeLaunch, Running, Finished };


  typedef
      insight::StaticFunctionTable<
          &CADParameterSetModelVisualizer::typeName,
          CADParameterSetModelVisualizer*,
              QObject*,
              IQParameterSetModel *,
              const boost::filesystem::path&,
              ProgressDisplayer&
          >
          VisualizerFunctions;

  declareStaticFunctionTable2(VisualizerFunctions, visualizerForAnalysis);
  declareStaticFunctionTable2(VisualizerFunctions, visualizerForOpenFOAMCaseElement);


  typedef
      insight::StaticFunctionTable<
          &CADParameterSetModelVisualizer::typeName,
          QIcon
          >
          IconFunctions;


  declareStaticFunctionTable2(IconFunctions, iconForAnalysis);
  declareStaticFunctionTable2(IconFunctions, iconForOpenFOAMCaseElement);


  typedef
      insight::StaticFunctionTable<
          &CADParameterSetModelVisualizer::typeName,
          CameraState
          >
          CameraStateFunctions;

  declareStaticFunctionTable2(CameraStateFunctions, defaultCameraStateForAnalysis);
  declareStaticFunctionTable2(CameraStateFunctions, defaultCameraStateForOpenFOAMCaseElement);


  typedef insight::StaticFunctionTable<
      &CADParameterSetModelVisualizer::typeName,
      GUIActionList,
      const std::string&,
      QObject *,
      IQCADModel3DViewer *,
      IQParameterSetModel *
      > CreateGUIActionsFunctions;

  declareStaticFunctionTable2(CreateGUIActionsFunctions, createGUIActionsForAnalysis);
  declareStaticFunctionTable2(CreateGUIActionsFunctions, createGUIActionsForOpenFOAMCaseElement);


protected:
  std::unique_ptr<boost::thread> rebuildThread_;
  IQParameterSetModel *psmodel_;

  QTimer timerToUpdate_;

  std::shared_ptr<supplementedInputDataBase> sid_;

  mutable boost::atomic<Status> status_;
  mutable boost::atomic<bool> success_;

  virtual const ParameterSet& parameters() const;

public:

  /**
   * @brief CADParameterSetModelVisualizer
   * sets up the model rebuild and launches background rebuild
   */
  CADParameterSetModelVisualizer(
        QObject* parent,
        IQParameterSetModel *psm,
        const boost::filesystem::path& workDir,
        ProgressDisplayer& progress
        );

  void launch(IQCADItemModel *model) override;

  /**
   * triggers stopping of rebuild thread,
   * return immediately
   */
  ~CADParameterSetModelVisualizer();

  // access state and results
  bool isFinished() const override;

  // stop
  void stopVisualizationComputation() override;

  IQParameterSetModel* parameterSetModel() const;

  void addGeometryToSpatialTransformationParameter(
      const std::string& parameterPath,
      insight::cad::FeaturePtr geom );

  void addVectorBasePoint(
      const std::string& parameterPath,
      const arma::mat& pBase );

  virtual std::shared_ptr<supplementedInputDataBase> computeSupplementedInput() = 0;

  virtual void recreateVisualizationElements();


Q_SIGNALS:
  void visualizationCalculationFinished(bool success);
  void updateSupplementedInputData(insight::supplementedInputDataBasePtr sid);
  void visualizationComputationError(insight::Exception ex);
};


template<class VisualizerClass>
VisualizerClass *newVisualizer(
    QObject* parent,
    IQParameterSetModel *psm,
    const boost::filesystem::path& workDir,
    ProgressDisplayer& progress )
{
    return new VisualizerClass(
        parent, psm, workDir, progress );
}


/**
 * @brief The IncompleteAnalysisVisualizer class
 * is intended to be used as intermediate stage in inheritance hierarchy.
 * Skips the instantiation of the supplementedInputData object.
 * This needs to be done further down in the hierarchy by some class that inherits this.
 */
template<
    class AnalysisInstance,
    class BaseVisualizer = CADParameterSetModelVisualizer >
class IncompleteAnalysisVisualizer
    : public BaseVisualizer
{
public:
    typedef typename AnalysisInstance::Parameters Parameters;

    using BaseVisualizer::BaseVisualizer;

    const typename AnalysisInstance::supplementedInputData& sp() const
    {
        return dynamic_cast<
            const typename AnalysisInstance::supplementedInputData&>(
            *this->sid_ );
    }

    const Parameters& p() const
    {
        return sp().p();
    }
};



template<
    class AnalysisInstance,
    class BaseVisualizer = CADParameterSetModelVisualizer >
class AnalysisVisualizer
: public BaseVisualizer
{
public:
    typedef typename AnalysisInstance::Parameters Parameters;

    using BaseVisualizer::BaseVisualizer;

    virtual std::shared_ptr<supplementedInputDataBase>
    computeSupplementedInput() override
    {
        return std::make_shared<typename AnalysisInstance::supplementedInputData>(
            ParameterSetInput(this->parameters()), this->workDir_, this->progress_);
    }

    const typename AnalysisInstance::supplementedInputData& sp() const
    {
        return dynamic_cast<
            const typename AnalysisInstance::supplementedInputData&>(
                *this->sid_ );
    }

    const Parameters& p() const
    {
        return sp().p();
    }
};




class TOOLKIT_GUI_EXPORT MultiCADParameterSetVisualizer
: public CADParameterSetVisualizerGenerator
{
public:
    struct IndividualVisualizer {
        IQParameterSetModel* parameterSource;
        CADParameterSetModelVisualizer::VisualizerFunctions::Function vizLookup; // individual visualizer dynamicLookup
    };
    typedef std::map<
        QObject*, // source
        IndividualVisualizer
        > SubVisualizerList;

private:
  Q_OBJECT

  SubVisualizerList visualizers_;
  mutable
      std::map<QObject*, std::pair<CADParameterSetModelVisualizer*,bool> >
      finishedVisualizers_;

  std::unique_ptr<IQISCADModelRebuilder> rb_;

  void allFinished(bool success);

public:
  MultiCADParameterSetVisualizer(
      QObject* parent,
      const SubVisualizerList& visualizers,
      const boost::filesystem::path& workDir,
      ProgressDisplayer& progress );

  void launch(IQCADItemModel *model) override;
  bool isFinished() const override;
  void stopVisualizationComputation() override;

public Q_SLOTS:
  void onSubVisualizationCalculationFinished(QObject* source, bool success);

};


}

#endif // PARAMETERSETVISUALIZER_H
