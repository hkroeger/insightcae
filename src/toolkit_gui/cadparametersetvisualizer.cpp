#include <QDebug>

#include "cadparametersetvisualizer.h"
#include "base/exception.h"
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

void CADParameterSetVisualizerGenerator::addPoint(
    const std::string& name,
    const arma::mat& p,
    bool initialVisibility)
{
    CurrentExceptionContext ec("adding visualizer point "+name);
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
  CurrentExceptionContext ec("adding visualizer datum "+name);
  Q_EMIT createdDatum(QString::fromStdString(name), dat, initialVisibility);
}




void CADParameterSetVisualizerGenerator::addFeature(
    const std::string& name,
    insight::cad::FeaturePtr feat,
    const insight::cad::FeatureVisualizationStyle& fvs )
{
  CurrentExceptionContext ec("adding visualizer feature "+name);
  Q_EMIT createdFeature( QString::fromStdString(name), feat, true, fvs );
}




void CADParameterSetVisualizerGenerator::addDataset(
    const std::string &name,
    vtkSmartPointer<vtkDataObject> ds )
{
  CurrentExceptionContext ec("adding visualizer dataset "+name);
  Q_EMIT createdDataset( QString::fromStdString(name), ds, true );
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
    status_(BeforeLaunch), success_(false)
{}

void CADParameterSetModelVisualizer::addGeometryToSpatialTransformationParameter(
        const std::string &parameterPath,
        cad::FeaturePtr geom )
{
    psmodel_->addGeometryToSpatialTransformationParameter(
                    parameterPath,
                    geom );
}




void CADParameterSetModelVisualizer::addVectorBasePoint(
        const std::string &parameterPath,
        const arma::mat &pBase )
{
    psmodel_->addVectorBasePoint(
                    parameterPath,
                    pBase );
}






void CADParameterSetModelVisualizer::recreateVisualizationElements()
{}






const ParameterSet &CADParameterSetModelVisualizer::parameters() const
{
    return psmodel_->getParameterSet();
}









void CADParameterSetModelVisualizer::launch(IQCADItemModel *model)
{
    timerToUpdate_.setInterval(100);
    timerToUpdate_.setSingleShot(true);

    timerToUpdate_.callOnTimeout(
        [this,model]{
            // need to launch visualization recomputation through signal,
            // because we want to be able to join multiple visualizers
            // into a single CADModel in another class
            rebuildThread_=std::make_unique<boost::thread>(
                [&,model]()
                {
                    status_=Running;

                    CurrentExceptionContext ex("computing visualization of scheduled parameter set");
                    try
                    {
                        CurrentExceptionContext ex("recreate visualization elements");

                        std::unique_ptr<IQISCADModelRebuilder> rb;
                        if (model)
                        {
                            rb=std::make_unique<IQISCADModelRebuilder>(
                                model, QList<IQISCADModelGenerator*>{this});
                        }
                        // else connections to model will set up by caller

                        if ((sid_=computeSupplementedInput()))
                        {
                            Q_EMIT updateSupplementedInputData( sid_ );
                        }
                        recreateVisualizationElements();
                        success_=true;
                    }
                    catch (insight::Exception& e)
                    {
                        status_=Finished;
                        insight::dbg()
                        << "Could not rebuild visualization.\n"
                        << " Error was:\n" << e <<endl;
                        Q_EMIT visualizationComputationError(e);
                    }

                    status_=Finished;
                    Q_EMIT visualizationCalculationFinished(success_);
                });
        });

    timerToUpdate_.start();
}




CADParameterSetModelVisualizer::~CADParameterSetModelVisualizer()
{
    stopVisualizationComputation();
}


bool CADParameterSetModelVisualizer::isFinished() const
{
    return status_==Finished;
}



void CADParameterSetModelVisualizer::stopVisualizationComputation()
{
    timerToUpdate_.stop();

    if (rebuildThread_)
    {
        rebuildThread_->interrupt();

        dbg()<<"waiting for visualizer thread to finish..."<<std::endl;

        rebuildThread_->join();

        rebuildThread_.reset();
    }

    insight::assertion(
        rebuildThread_==nullptr,
        "internal error: visualization computation could not be cancelled!");
}



IQParameterSetModel* CADParameterSetModelVisualizer::parameterSetModel() const
{
    return psmodel_;
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
    visualizers_(visualizers)
{}


void MultiCADParameterSetVisualizer::launch(IQCADItemModel *model)
{
    QList<IQISCADModelGenerator*> gens;

    for (auto& v: visualizers_)
    {
        auto src=v.first;

        auto vis = v.second.vizLookup(
            this,
            v.second.parameterSource,
            workDir_, progress_ );

        gens.append(vis);

        connect(
            vis,
            &CADParameterSetModelVisualizer::visualizationCalculationFinished,
            this,
            [this,src](bool success)
            { onSubVisualizationCalculationFinished(src, success); }
            );

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

    }

    rb_=std::make_unique<IQISCADModelRebuilder>(model, gens);

    for (auto& vis: gens)
    {
        dynamic_cast<CADParameterSetModelVisualizer*>(vis)
            ->launch(nullptr);
    }

    if (gens.size()==0)
    {
        allFinished(true);
    }

}


bool MultiCADParameterSetVisualizer::isFinished() const
{
    if (finishedVisualizers_.size()==visualizers_.size())
    {
        bool allFinished=true;
        for (auto& fv: finishedVisualizers_)
        {
            allFinished=allFinished && fv.second.first->isFinished();
        }
        return allFinished;
    }
    else
        return false;
}


void MultiCADParameterSetVisualizer::stopVisualizationComputation()
{
    for (auto c: children())
    {
        if (auto *sc = dynamic_cast<CADParameterSetModelVisualizer*>(c))
        {
            sc->stopVisualizationComputation();
        }
    }
}


void MultiCADParameterSetVisualizer::onSubVisualizationCalculationFinished(QObject* source, bool s)
{
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
