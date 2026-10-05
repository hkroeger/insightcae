#ifndef PARAMETERSETVISUALIZER_H
#define PARAMETERSETVISUALIZER_H

#include "base/cppextensions.h"
#include "base/factory.h"
#include "toolkit_gui_export.h"


#include <QObject>
#include <QThread>
#include <QIcon>
#include <QTimer>
#include <QColor>

#include "base/analysis.h"
#include "base/supplementedinputdata.h"
#include "openfoam/caseelements/openfoamcaseelement.h"
#include "boost/atomic.hpp"
#include "cadtypes.h"
#include "datum.h"
#include "cadfeature.h"
#include "iqiscadmodelgenerator.h"
#include "iqiscadmodelrebuilder.h"
#include "parametersetwithguicontext.h"

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

    typedef std::vector<std::pair<std::string,std::exception_ptr> > ErrorList;

private:
    mutable boost::mutex errorsMutex_;
    /**
     * @brief errors_
     * errors in individual visualization steps or entities,
     * which did not stop the remaining visualization
     */
    ErrorList errors_;

protected:
    void recordError(const std::string& label, std::exception_ptr ex);
    void clearErrors();
    ErrorList errors() const;

    /**
     * @brief aggregatedError
     * @return
     * a single exception, which describes all recorded errors.
     * Null, if there were no errors.
     */
    std::exception_ptr aggregatedError(
        const ErrorList& inputDataErrors = ErrorList() ) const;


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

    /**
     * @brief launch
     * starts the visualization computation immediately.
     * Must be called from the GUI thread and only once per object.
     * @param model
     * the model, into which the visualization entities are inserted.
     * If null, the caller has to connect to the created* signals.
     */
    virtual void launch(IQCADItemModel *model) =0;

    /**
     * @brief stopVisualizationComputation
     * request cancellation of a running computation and return immediately.
     * All output of the cancelled computation is discarded.
     * computationThreadEnded() is emitted, once the background thread has actually ended.
     */
    virtual void stopVisualizationComputation() =0;

    /**
     * @brief stopAndWait
     * cancel and block until the background thread has ended.
     * Intended for use in destructors of owning objects.
     */
    virtual void stopAndWait() =0;

    /**
     * @brief isComputationRunning
     * true between launch and the end of the background thread
     */
    virtual bool isComputationRunning() const =0;

    virtual bool isFinished() const =0;

    /**
     * @brief cancelAndDeleteLater
     * cancel without blocking and delete this object, once the background thread has ended
     */
    void cancelAndDeleteLater();

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

    virtual void addEvaluation(
        const std::string& name,
        insight::cad::PostprocActionPtr ppa,
        bool visible = true );

    /**
     * @brief step
     * executes a part of the visualization.
     * If it fails, the error is recorded and reported at the end of the visualization,
     * but the visualization continues with the next step.
     * Should be used in recreateVisualizationElements to separate independent parts, e.g.
     *
     *   step("domain", [&]() { ... addFeature(...); } );
     *
     * @param label
     * name of the step for error reporting
     */
    void step(const std::string& label, std::function<void()> f);


Q_SIGNALS:
    void visualizationCalculationFinished(bool success);
    void updateSupplementedInputData(insight::supplementedInputDataBasePtr sid);
    void visualizationComputationError(std::exception_ptr ex);

    /**
     * @brief computationThreadEnded
     * emitted in any case (success, error, cancellation),
     * when the background computation has ended
     */
    void computationThreadEnded();
};




class TOOLKIT_GUI_EXPORT CADParameterSetModelVisualizer
: public CADParameterSetVisualizerGenerator
{

  Q_OBJECT

public:
  declareType("CADParameterSetModelVisualizer");

    enum Status { BeforeLaunch, Running, Finished, Cancelled };


  typedef
      insight::StaticFunctionTable<
          CADParameterSetModelVisualizer*,
              QObject*,
              IQParameterSetModel *,
              const boost::filesystem::path&,
              ProgressDisplayer&
          >
          VisualizerFunctions;

  declareStaticFunctionTableAccessFunction2(
      VisualizerFunctions, visualizerForAnalysis);

  declareStaticFunctionTableAccessFunction2(
      VisualizerFunctions, visualizerForOpenFOAMCaseElement);


  typedef
      insight::StaticFunctionTable<
          QIcon
          >
          IconFunctions;

  declareStaticFunctionTableAccessFunction2(
      IconFunctions, iconForAnalysis);

  declareStaticFunctionTableAccessFunction2(
      IconFunctions, iconForOpenFOAMCaseElement);


  typedef
      insight::StaticFunctionTable<
        CameraState
      >
      DefaultCameraStateFunctions;

  declareStaticFunctionTableAccessFunction2(
      DefaultCameraStateFunctions, defaultCameraStateForAnalysis);

  declareStaticFunctionTableAccessFunction2(
      DefaultCameraStateFunctions, defaultCameraStateForOpenFOAMCaseElement);


  typedef insight::StaticFunctionTable<
      GUIActionList,
      const std::string&,
      QObject *,
      IQCADModel3DViewer *,
      IQParameterSetModel *
      > CreateGUIActionsFunctions;

  declareStaticFunctionTableAccessFunction2(
      CreateGUIActionsFunctions, createGUIActionsForAnalysis);

  declareStaticFunctionTableAccessFunction2(
      CreateGUIActionsFunctions, createGUIActionsForOpenFOAMCaseElement);


  /**
   * wizards, which provide an organized view on a subset of the analysis parameters.
   * Arguments: the parameter set model under edit and the 3D viewer (may be null).
   * See IQParameterSetWizard for a base class.
   */
  declareStaticFunctionTable2(
      GUIWizardFunctions, createGUIWizardForAnalysis,
      QWidget*,
      IQParameterSetModel *,
      IQCADModel3DViewer * );


protected:
  std::unique_ptr<boost::thread> rebuildThread_;
  IQParameterSetModel *psmodel_;

  /**
   * @brief paramSnapshot_
   * copy of the parameter set, taken in the GUI thread at launch.
   * The background thread reads only from this copy,
   * never from the (concurrently edited) parameter set model.
   */
  std::shared_ptr<ParameterSet> paramSnapshot_;

  /**
   * @brief rb_
   * lives in the GUI thread. Receives the created entities through queued connections.
   * Is deleted on cancellation, which also drops all pending (stale) entities.
   */
  std::unique_ptr<IQISCADModelRebuilder> rb_;

  std::shared_ptr<supplementedInputDataBase> sid_;

  /**
   * @brief sidError_
   * the error, which prevented the creation of the supplemented input data, if any
   */
  std::exception_ptr sidError_;

  /**
   * @brief sidBase
   * access the supplemented input data.
   * Throws, if the supplemented input data could not be created.
   */
  const supplementedInputDataBase& sidBase() const;

  mutable boost::mutex typedParametersMutex_;
  mutable std::unique_ptr<ParametersBase> typedParameters_;

  mutable boost::atomic<Status> status_;
  mutable boost::atomic<bool> success_;
  boost::atomic<bool> cancelled_;
  boost::atomic<bool> threadActive_;

  virtual const ParameterSet& parameters() const;

  void onComputationThreadEnded();

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
   * Owners should have stopped the computation before (cancelAndDeleteLater or stopAndWait).
   * If the thread is still running here, it is joined as a last resort.
   */
  ~CADParameterSetModelVisualizer();

  // access state and results
  bool isFinished() const override;
  bool isComputationRunning() const override;
  bool isCancelled() const;

  // stop
  void stopVisualizationComputation() override;
  void stopAndWait() override;

  IQParameterSetModel* parameterSetModel() const;

  insight::ParameterSetGUIContext* GUIContext();

  virtual std::shared_ptr<supplementedInputDataBase> computeSupplementedInput() = 0;

  /**
   * @brief parametersAs
   * the parameters in their typed (PDL generated) form.
   * Taken from the supplemented input data, if available.
   * Otherwise created from the parameter snapshot.
   * Hence also available, if the supplemented input data could not be computed.
   */
  template<class P>
  const P& parametersAs() const
  {
      if (auto *sidp = dynamic_cast<const supplementedInputDataFromParameters*>(sid_.get()))
      {
          if (auto *pp = dynamic_cast<const P*>(sidp->baseParametersPtr()))
              return *pp;
      }
      boost::mutex::scoped_lock lck(typedParametersMutex_);
      if (!typedParameters_)
          typedParameters_ = std::make_unique<P>(parameters());
      return dynamic_cast<const P&>(*typedParameters_);
  }

  virtual void recreateVisualizationElements();
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
            this->sidBase() );
    }

    const Parameters& p() const
    {
        return this->template parametersAs<Parameters>();
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
            ParameterSetInput(this->parameters()), this->workDir_,
            *this->progress_.forkNewAction(99, "Processing input data"));
    }

    const typename AnalysisInstance::supplementedInputData& sp() const
    {
        return dynamic_cast<
            const typename AnalysisInstance::supplementedInputData&>(
                this->sidBase() );
    }

    /**
     * @brief p
     * the parameters. Available also, if the supplemented input data could not be created.
     */
    const Parameters& p() const
    {
        return this->template parametersAs<Parameters>();
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
  QList<CADParameterSetModelVisualizer*> subVisualizers_;
  bool launched_, cancelled_, threadsEnded_;

  void allFinished(bool success);

public:
  MultiCADParameterSetVisualizer(
      QObject* parent,
      const SubVisualizerList& visualizers,
      const boost::filesystem::path& workDir,
      ProgressDisplayer& progress );

  ~MultiCADParameterSetVisualizer();

  void launch(IQCADItemModel *model) override;
  bool isFinished() const override;
  bool isComputationRunning() const override;
  void stopVisualizationComputation() override;
  void stopAndWait() override;

public Q_SLOTS:
  void onSubVisualizationCalculationFinished(QObject* source, bool success);
  void onSubComputationThreadEnded();

};


}

#endif // PARAMETERSETVISUALIZER_H
