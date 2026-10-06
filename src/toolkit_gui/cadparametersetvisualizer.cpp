#include <QDebug>
#include <exception>

#include "cadparametersetvisualizer.h"
#include "base/exception.h"
#include "base/exceptionhandling.h"
#include "base/warningdispatcher.h"
#include "boost/thread/exceptions.hpp"
#include "boost/thread.hpp"
#include "qmodeltree.h"

#include "iqiscadmodelrebuilder.h"
#include "iqcaditemmodel.h"
#include "iqparametersetmodel.h"


namespace insight
{


arma::mat vec3(const QColor &c)
{
    return vec3(c.redF(), c.greenF(), c.blueF());
}


std::string visPath(const std::vector<std::string> &path)
{
    return boost::join(path, "/");
}

std::string visPath()
{
    return std::string();
}


CameraState::CameraState(
    arma::mat _focalPosition,
    arma::mat _cameraPosition,
    arma::mat _viewUp,
    double _parallelScale,
    bool _isParallelProjection
    )
   : focalPosition(_focalPosition),
     cameraPosition(_cameraPosition),
     viewUp(_viewUp),
     parallelScale(_parallelScale),
     isParallelProjection(_isParallelProjection)
{}




CADParameterSetVisualizerGenerator::CADParameterSetVisualizerGenerator(
    QObject *parent, const boost::filesystem::path &workDir, ProgressDisplayer &progress)
    : IQISCADModelGenerator(parent),
    workDir_(workDir), progress_(progress)
{}


void CADParameterSetVisualizerGenerator::cancelAndDeleteLater()
{
    stopVisualizationComputation();

    // connect before checking: the thread might end in between.
    // Calling deleteLater more than once is safe.
    connect(this, &CADParameterSetVisualizerGenerator::computationThreadEnded,
            this, &QObject::deleteLater );

    if (!isComputationRunning())
    {
        deleteLater();
    }
}

namespace {

/**
 * the current exception as exception_ptr.
 * Exceptions, which are not derived from std::exception, are converted.
 */
std::exception_ptr currentExceptionPtr()
{
    try { throw; }
    catch (std::exception&)
    {
        return std::current_exception();
    }
    catch (...)
    {
        auto errdesc = insight::describeCurrentException();

        std::ostringstream os;
        os
            << *(errdesc) << "\n"
            << errdesc->errorDetails_;
        return std::make_exception_ptr(
            insight::Exception("%s", os.str().c_str()));
    }
}

}




void CADParameterSetVisualizerGenerator::recordError(
    const std::string& label, std::exception_ptr ex)
{
    boost::mutex::scoped_lock lck(errorsMutex_);
    errors_.push_back({label, ex});
}




void CADParameterSetVisualizerGenerator::clearErrors()
{
    boost::mutex::scoped_lock lck(errorsMutex_);
    errors_.clear();
}




CADParameterSetVisualizerGenerator::ErrorList
CADParameterSetVisualizerGenerator::errors() const
{
    boost::mutex::scoped_lock lck(errorsMutex_);
    return errors_;
}




std::exception_ptr CADParameterSetVisualizerGenerator::aggregatedError(
    const ErrorList& inputDataErrors ) const
{
    auto visErrors=errors();

    if (inputDataErrors.empty() && visErrors.empty())
        return nullptr;

    std::ostringstream os;
    os << "The visualization is incomplete.\n";

    if (!inputDataErrors.empty())
    {
        os << "\nThe input data could not be processed completely:\n";
        for (const auto& e: inputDataErrors)
            os << " * " << e.first << ": " << describeException(e.second) << "\n";
    }

    if (!visErrors.empty())
    {
        os << "\nThe following parts could not be visualized:\n";
        for (const auto& e: visErrors)
        {
            os << " * " << e.first << ": ";
            try
            {
                std::rethrow_exception(e.second);
            }
            catch (const SupplementedQuantityError& sqe)
            {
                os << "requires \"" << sqe.rootQuantity() << "\", which is unavailable";
            }
            catch (...)
            {
                os << describeException(e.second);
            }
            os << "\n";
        }
    }

    return std::make_exception_ptr(
        insight::Exception("%s", os.str().c_str()) );
}




void CADParameterSetVisualizerGenerator::step(
    const std::string& label,
    std::function<void()> f )
{
    try
    {
        CurrentExceptionContext ec("visualization step "+label);
        f();
    }
    catch (const boost::thread_interrupted&)
    {
        throw;
    }
    catch (...)
    {
        recordError(label, currentExceptionPtr());
    }
}




void CADParameterSetVisualizerGenerator::addPoint(
    const std::string& name,
    const arma::mat& p,
    bool initialVisibility)
{
    // cancellation point: visualizers call this frequently from the background thread
    boost::this_thread::interruption_point();
    CurrentExceptionContext ec(GUIEvents, "adding visualizer point "+name);
    Q_EMIT createdVariable(
        QString::fromStdString(name),
        cad::matconst(p),
        cad::Point,
        initialVisibility );
}


void CADParameterSetVisualizerGenerator::addDatum(
    const std::string& name,
    insight::cad::DatumPtr dat,
    bool initialVisibility )
{
  // cancellation point: visualizers call this frequently from the background thread
  boost::this_thread::interruption_point();
  CurrentExceptionContext ec(GUIEvents, "adding visualizer datum "+name);
  Q_EMIT createdDatum(QString::fromStdString(name), dat, initialVisibility);
}




void CADParameterSetVisualizerGenerator::addFeature(
    const std::string& name,
    insight::cad::FeaturePtr feat,
    const insight::cad::FeatureVisualizationStyle& fvs )
{
  // cancellation point: visualizers call this frequently from the background thread
  boost::this_thread::interruption_point();
  CurrentExceptionContext ec(GUIEvents, "adding visualizer feature "+name);
  try
  {
      if (!feat->hasTriangulation()) // tesselate here, if needed. Will happen in GUI thread otherwise
      {
          insight::CurrentExceptionContext ex(
              str(boost::format("pre-tesselating feature %s")%name));
          feat->createTriangulation();
      }
  }
  catch (const boost::thread_interrupted&)
  {
      throw;
  }
  catch (...)
  {
      // skip this feature, but continue with the remaining visualization
      recordError(name, currentExceptionPtr());
      return;
  }
  Q_EMIT createdFeature( QString::fromStdString(name), feat, true, fvs );
}




void CADParameterSetVisualizerGenerator::addDataset(
    const std::string &name,
    vtkSmartPointer<vtkDataObject> ds )
{
  // cancellation point: visualizers call this frequently from the background thread
  boost::this_thread::interruption_point();
  CurrentExceptionContext ec(GUIEvents, "adding visualizer dataset "+name);
  Q_EMIT createdDataset( QString::fromStdString(name), ds, true );
}




void CADParameterSetVisualizerGenerator::addEvaluation(
    const std::string& name,
    insight::cad::PostprocActionPtr ppa,
    bool visible )
{
  // cancellation point: visualizers call this frequently from the background thread
  boost::this_thread::interruption_point();
  CurrentExceptionContext ec(GUIEvents, "adding visualizer evaluation "+name);
  Q_EMIT createdEvaluation( QString::fromStdString(name), ppa, visible );
}




defineType(CADParameterSetModelVisualizer);

defineStaticFunctionTableAccessFunction2(
    "analysis visualizers",
    CADParameterSetModelVisualizer, VisualizerFunctions,
    visualizerForAnalysis);

defineStaticFunctionTableAccessFunction2(
    "OpenFOAM case element visualizers",
    CADParameterSetModelVisualizer, VisualizerFunctions,
    visualizerForOpenFOAMCaseElement);

defineStaticFunctionTableAccessFunction2(
    "analysis icons",
    CADParameterSetModelVisualizer, IconFunctions,
    iconForAnalysis);

defineStaticFunctionTableAccessFunction2(
    "OpenFOAM case element icons",
    CADParameterSetModelVisualizer, IconFunctions,
    iconForOpenFOAMCaseElement);

defineStaticFunctionTableAccessFunction2(
    "analysis default camera states",
    CADParameterSetModelVisualizer, DefaultCameraStateFunctions,
    defaultCameraStateForAnalysis);

defineStaticFunctionTableAccessFunction2(
    "OpenFOAM case element default camera states",
    CADParameterSetModelVisualizer, DefaultCameraStateFunctions,
    defaultCameraStateForOpenFOAMCaseElement);

defineStaticFunctionTableAccessFunction2(
    "analysis GUI actions",
    CADParameterSetModelVisualizer, CreateGUIActionsFunctions,
    createGUIActionsForAnalysis);

defineStaticFunctionTableAccessFunction2(
    "OpenFOAM case element GUI actions",
    CADParameterSetModelVisualizer, CreateGUIActionsFunctions,
    createGUIActionsForOpenFOAMCaseElement);

defineStaticFunctionTable2(
    "wizards for analysis",
    CADParameterSetModelVisualizer, GUIWizardFunctions,
    createGUIWizardForAnalysis );



CADParameterSetModelVisualizer::CADParameterSetModelVisualizer(
    QObject* parent,
    IQParameterSetModel *psm,
    const boost::filesystem::path& workDir,
    ProgressDisplayer& progress )
    : CADParameterSetVisualizerGenerator(parent, workDir, progress),
    psmodel_(psm),
    status_(BeforeLaunch), success_(false),
    cancelled_(false), threadActive_(false)
{
    // queued, since emitted from the background thread
    connect(this, &CADParameterSetVisualizerGenerator::computationThreadEnded,
            this, &CADParameterSetModelVisualizer::onComputationThreadEnded );
}






void CADParameterSetModelVisualizer::recreateVisualizationElements()
{}






const supplementedInputDataBase& CADParameterSetModelVisualizer::sidBase() const
{
    if (!sid_)
    {
        if (sidError_)
            throw insight::Exception(
                "the supplemented input data is unavailable: %s",
                describeException(sidError_).c_str() );
        else
            throw insight::Exception(
                "the supplemented input data is unavailable" );
    }
    return *sid_;
}




const ParameterSet &CADParameterSetModelVisualizer::parameters() const
{
    if (paramSnapshot_)
        return *paramSnapshot_;

    // before launch (GUI thread only)
    return psmodel_->getParameterSet();
}




void CADParameterSetModelVisualizer::onComputationThreadEnded()
{
    // executed in GUI thread, after all entities emitted by the thread have been
    // delivered to the rebuilder (queued events are processed in order)
    if (rebuildThread_)
    {
        rebuildThread_->join(); // thread is about to return
        rebuildThread_.reset();
    }

    if (rb_)
    {
        if (cancelled_) rb_->abandon();
        rb_.reset(); // removes symbols, which were not recreated
    }
}




void CADParameterSetModelVisualizer::launch(IQCADItemModel *model)
{
    CurrentExceptionContext ex("launching visualization computation");

    if (cancelled_)
    {
        // cancelled before launch: no thread involved,
        // but report asynchronously like with thread
        QMetaObject::invokeMethod(
            this,
            [this]() { Q_EMIT computationThreadEnded(); },
            Qt::QueuedConnection );
        return;
    }

    insight::assertion(
        status_==BeforeLaunch && !rebuildThread_,
        "internal error: a visualizer can only be launched once" );

    // copy the parameters in the GUI thread.
    // The thread must never access the parameter set model, which may be modified concurrently.
    paramSnapshot_ = std::shared_ptr<ParameterSet>(
        psmodel_->getParameterSet().cloneAs<ParameterSet>() );

    if (model)
    {
        // created in GUI thread: the created* signals get queued into the GUI thread
        rb_=std::make_unique<IQISCADModelRebuilder>(
            model, QList<IQISCADModelGenerator*>{this});
    }
    // else connections to model will set up by caller

    status_=Running;
    threadActive_=true;

    auto* mainThreadWD = &WarningDispatcher::getCurrent();
    rebuildThread_=std::make_unique<boost::thread>(
        [this,mainThreadWD]()
        {
            WarningDispatcher::getCurrent().setSuperDispatcher(mainThreadWD);

            CurrentExceptionContext ex("computing visualization of scheduled parameter set");
            clearErrors();

            // warnings of this computation (only accessed from this thread)
            std::vector<std::string> sidConstructionWarnings, visualizationWarnings;

            auto collectIssues = [&]()
            {
                InputDataIssueList issues;
                if (sid_)
                {
                    issues=sid_->issues();
                }
                else
                {
                    for (const auto& w: sidConstructionWarnings)
                        issues.push_back({InputDataIssue::Warning, "input data", w});
                    if (sidError_)
                        issues.push_back({InputDataIssue::Error, "input data", describeException(sidError_)});
                }
                for (const auto& w: visualizationWarnings)
                    issues.push_back({InputDataIssue::Warning, "visualization", w});
                return issues;
            };

            try
            {
                // 1. supplemented input data.
                // Errors in here do not prevent the (partial) visualization.
                std::shared_ptr<supplementedInputDataBase> sid;
                try
                {
                    CurrentExceptionContext ex("computing supplemented input data");
                    ScopedWarningRecorder rec(
                        [&](const insight::Exception& w)
                        { sidConstructionWarnings.push_back(w.message()); } );
                    sid=computeSupplementedInput();
                }
                catch (const boost::thread_interrupted&)
                {
                    throw;
                }
                catch (...)
                {
                    if (cancelled_) throw boost::thread_interrupted();
                    sidError_=currentExceptionPtr();
                }

                if (sid)
                {
                    // the supplemented input data refers to the parameter snapshot
                    // by raw pointer: keep the snapshot alive as long as the sid
                    auto snap=paramSnapshot_;
                    sid_=supplementedInputDataBasePtr(
                        sid.get(), [sid,snap](supplementedInputDataBase*) {} );

                    for (const auto& w: sidConstructionWarnings)
                        sid_->recordWarning("input data", w);

                    // compute all deferred quantities in parallel.
                    // The visualization waits only for those, which it needs.
                    sid_->launchAll(
                        progress_.forkNewAction(1, "Computing input data") );

                    boost::this_thread::interruption_point();
                    if (!cancelled_)
                        Q_EMIT updateSupplementedInputData( sid_ );
                }

                // 2. visualization. Parts which fail are skipped.
                boost::this_thread::interruption_point();
                step("visualization", [&]()
                {
                    ScopedWarningRecorder rec(
                        [&](const insight::Exception& w)
                        { visualizationWarnings.push_back(w.message()); } );
                    recreateVisualizationElements();
                } );
                boost::this_thread::interruption_point();

                // 3. complete the deferred quantities, which are not needed for the visualization
                ErrorList inputDataErrors;
                if (sid_)
                {
                    sid_->computeAll();
                    boost::this_thread::interruption_point();

                    inputDataErrors=sid_->rootErrors();
                    if (inputDataErrors.empty())
                        inputDataErrors=sid_->errors();

                    // update the table: now including the results of all quantities
                    if (!cancelled_)
                        Q_EMIT updateSupplementedInputData( sid_ );
                }
                else if (sidError_)
                {
                    inputDataErrors.push_back({"input data", sidError_});
                }

                auto err=aggregatedError(inputDataErrors);
                success_=!err;
                status_=Finished;
                if (!cancelled_)
                {
                    Q_EMIT inputDataIssuesChanged(collectIssues());
                    if (err)
                        Q_EMIT visualizationComputationError(err);
                    else
                        Q_EMIT visualizationCalculationFinished(success_);
                }
            }
            catch (const boost::thread_interrupted&)
            {
                status_=Cancelled;
            }
            catch (...)
            {
                if (cancelled_)
                {
                    // errors of discarded computations are of no interest
                    // (might also be a translated thread_interrupted)
                    status_=Cancelled;
                }
                else
                {
                    status_=Finished;
                    auto exptr=currentExceptionPtr();
                    Q_EMIT inputDataIssuesChanged(collectIssues());
                    Q_EMIT visualizationComputationError(exptr);
                }
            }

            // never leave background computations behind:
            // they access the sid, which might be deleted after the thread has ended
            if (sid_)
                sid_->cancelDeferredComputations();

            threadActive_=false;
            Q_EMIT computationThreadEnded();
        });
}




CADParameterSetModelVisualizer::~CADParameterSetModelVisualizer()
{
    if (threadActive_)
    {
        // derived class is already destroyed at this point but the thread
        // might still execute its virtual functions. Owners have to stop
        // the computation before deletion.
        std::cerr
            << "Warning: visualizer deleted while background computation was still running. "
               "Owner should call cancelAndDeleteLater() or stopAndWait() before deletion."
            << std::endl;
    }
    stopAndWait();
}


bool CADParameterSetModelVisualizer::isFinished() const
{
    return status_==Finished;
}


bool CADParameterSetModelVisualizer::isComputationRunning() const
{
    return threadActive_;
}


bool CADParameterSetModelVisualizer::isCancelled() const
{
    return cancelled_;
}



void CADParameterSetModelVisualizer::stopVisualizationComputation()
{
    cancelled_=true;

    if (status_==BeforeLaunch)
        status_=Cancelled;

    if (rebuildThread_)
    {
        rebuildThread_->interrupt();
    }

    if (rb_)
    {
        // discard all entities of this computation,
        // including those, which are already queued
        rb_->abandon();
        rb_.reset();
    }
}


void CADParameterSetModelVisualizer::stopAndWait()
{
    stopVisualizationComputation();

    if (rebuildThread_)
    {
        dbg(DeepDetail)<<"waiting for visualizer thread to finish..."<<std::endl;
        rebuildThread_->join();
        rebuildThread_.reset();
    }
}



IQParameterSetModel* CADParameterSetModelVisualizer::parameterSetModel() const
{
    return psmodel_;
}

insight::ParameterSetGUIContext* CADParameterSetModelVisualizer::GUIContext()
{
    return parameterSetModel()->GUIContext();
}




void MultiCADParameterSetVisualizer::allFinished(bool sucess)
{
    rb_.reset();
    Q_EMIT visualizationCalculationFinished(sucess);
}

MultiCADParameterSetVisualizer::MultiCADParameterSetVisualizer(
    QObject* parent,
    const SubVisualizerList& visualizers,
    const boost::filesystem::path& workDir,
    ProgressDisplayer& progress )
 : CADParameterSetVisualizerGenerator(parent, workDir, progress),
    visualizers_(visualizers),
    launched_(false), cancelled_(false), threadsEnded_(false)
{}


MultiCADParameterSetVisualizer::~MultiCADParameterSetVisualizer()
{
    // sub visualizers are children and still intact here
    stopAndWait();
}


void MultiCADParameterSetVisualizer::launch(IQCADItemModel *model)
{
    insight::assertion(
        !launched_,
        "internal error: a visualizer can only be launched once" );
    launched_=true;

    finishedVisualizers_.clear();

    QList<IQISCADModelGenerator*> gens;

    if (!cancelled_)
    {
        for (auto& v: visualizers_)
        {
            auto src=v.first;

            auto vis = v.second.vizLookup(
                this,
                v.second.parameterSource,
                workDir_, progress_ );

            gens.append(vis);
            subVisualizers_.append(vis);

            connect(
                vis,
                &CADParameterSetModelVisualizer::visualizationCalculationFinished,
                this,
                [this,src](bool success)
                { onSubVisualizationCalculationFinished(src, success); }
                );


            connect(
                vis,
                &CADParameterSetModelVisualizer::visualizationComputationError,
                this,
                [this,src](std::exception_ptr ex)
                {
                    if (!cancelled_)
                    {
                        Q_EMIT visualizationComputationError(ex);
                        onSubVisualizationCalculationFinished(src, false);
                    }
                }
                );

            connect(
                vis,
                &CADParameterSetModelVisualizer::computationThreadEnded,
                this,
                &MultiCADParameterSetVisualizer::onSubComputationThreadEnded );

            connect(vis, QOverload<const QString&,insight::cad::ScalarPtr>::of(&IQISCADModelGenerator::createdVariable),
                    this, QOverload<const QString&,insight::cad::ScalarPtr>::of(&IQISCADModelGenerator::createdVariable) );
            connect(vis, QOverload<const QString&,insight::cad::VectorPtr,insight::cad::VectorVariableType,bool>::of(&IQISCADModelGenerator::createdVariable),
                    this, QOverload<const QString&,insight::cad::VectorPtr,insight::cad::VectorVariableType,bool>::of(&IQISCADModelGenerator::createdVariable) );
            connect(vis, &IQISCADModelGenerator::createdFeature,
                    this, &IQISCADModelGenerator::createdFeature);
            connect(vis, &IQISCADModelGenerator::createdDatum,
                    this, &IQISCADModelGenerator::createdDatum);
            connect(vis, &IQISCADModelGenerator::createdEvaluation,
                    this, &IQISCADModelGenerator::createdEvaluation );
            connect(vis, &IQISCADModelGenerator::createdDataset,
                    this, &IQISCADModelGenerator::createdDataset );

        }

        if (model)
        {
            // receives the entities, which are forwarded by this object in the GUI thread
            rb_=std::make_unique<IQISCADModelRebuilder>(
                model, QList<IQISCADModelGenerator*>{this} );
        }

        for (auto* vis: subVisualizers_)
        {
            vis->launch(nullptr);
        }
    }

    if (subVisualizers_.size()==0)
    {
        if (!cancelled_)
            allFinished(true);

        // no thread involved: report asynchronously, like with threads
        QMetaObject::invokeMethod(
            this,
            [this]()
            {
                threadsEnded_=true;
                Q_EMIT computationThreadEnded();
            },
            Qt::QueuedConnection );
    }
}


bool MultiCADParameterSetVisualizer::isFinished() const
{
    if (!launched_ || cancelled_)
        return false;

    for (auto* vis: subVisualizers_)
    {
        if (!vis->isFinished())
            return false;
    }
    return true;
}


bool MultiCADParameterSetVisualizer::isComputationRunning() const
{
    return launched_ && !threadsEnded_;
}


void MultiCADParameterSetVisualizer::stopVisualizationComputation()
{
    cancelled_=true;

    for (auto* vis: subVisualizers_)
    {
        vis->stopVisualizationComputation();
    }

    if (rb_)
    {
        rb_->abandon();
        rb_.reset();
    }
}


void MultiCADParameterSetVisualizer::stopAndWait()
{
    stopVisualizationComputation();

    for (auto* vis: subVisualizers_)
    {
        vis->stopAndWait();
    }
}


void MultiCADParameterSetVisualizer::onSubVisualizationCalculationFinished(QObject* source, bool s)
{
  if (cancelled_) return;

  finishedVisualizers_.insert({
        source,
        { dynamic_cast<CADParameterSetModelVisualizer*>(sender()), s} });

  if (finishedVisualizers_.size()==visualizers_.size())
  {
      bool alls=true;
      for (const auto& fv: finishedVisualizers_)
          alls=alls&&fv.second.second;
      allFinished(alls);
  }
}


void MultiCADParameterSetVisualizer::onSubComputationThreadEnded()
{
    if (threadsEnded_) return;

    for (auto* vis: subVisualizers_)
    {
        if (vis->isComputationRunning())
            return;
    }

    threadsEnded_=true;
    Q_EMIT computationThreadEnded();
}

// Need explicit template instantiation. Otherwise unexpected behaviour:
// https://www.reddit.com/r/cpp_questions/comments/11pnhaq/comment/jixenmc
template class insight::StaticFunctionTable<
    CADParameterSetModelVisualizer*,
    QObject*,
    IQParameterSetModel *,
    const boost::filesystem::path&,
    ProgressDisplayer&
    >;


template class
    insight::StaticFunctionTable<
        QIcon
        >;


template class
    insight::StaticFunctionTable<
        CameraState
        >;

template class insight::StaticFunctionTable<
    GUIActionList,
    const std::string&,
    QObject *,
    IQCADModel3DViewer *,
    IQParameterSetModel *
    >;



template class insight::StaticFunctionTable<
    QWidget*,
    IQParameterSetModel *
    >;


void blubb()
{
    // produce some calls to inline functions to trigger static object init
    int s;
    s=insight::CADParameterSetModelVisualizer::visualizerForAnalysis().size();
    s=insight::CADParameterSetModelVisualizer::visualizerForOpenFOAMCaseElement().size();
    s=insight::CADParameterSetModelVisualizer::iconForAnalysis().size();
    s=insight::CADParameterSetModelVisualizer::iconForOpenFOAMCaseElement().size();
}

}
