#include "blockmesh_gui.h"
#include "cadfeatures.h"
#include "qoccviewwidget.h"
#include "qmodeltree.h"

#include "datum.h"
#include "box.h"
#include "cylinder.h"

#include "base/boost_include.h"
#include "base/units.h"


using namespace std;
using namespace boost;

namespace insight
{
namespace bmd
{



ParameterSet_VisualizerPtr blockMeshDict_Box_visualizer()
{
    return ParameterSet_VisualizerPtr( new blockMeshDict_Box_ParameterSet_Visualizer );
}

addStandaloneFunctionToStaticFunctionTable(OpenFOAMCaseElement, blockMeshDict_Box, visualizer, blockMeshDict_Box_visualizer);


void blockMeshDict_Box_ParameterSet_Visualizer::update(const ParameterSet& ps)
{
    ParameterSet_Visualizer::update(ps);
}

void blockMeshDict_Box_ParameterSet_Visualizer::updateVisualizationElements(QoccViewWidget* vw, QModelTree* mt)
{

    Parameters p(ps_);

    insight::cad::cache.initRebuild();

    arma::mat ey = arma::cross( p.geometry.ez, p.geometry.ex);

    mt->onAddFeature( "blockMeshDict_Box",
                        cad::Box::create(
                          cad::matconst(p.geometry.p0),
                          cad::matconst(p.geometry.ex * p.geometry.L),
                          cad::matconst(           ey * p.geometry.W),
                          cad::matconst(p.geometry.ez * p.geometry.H)
                        ),
                        true );

    insight::cad::cache.finishRebuild();
}





ParameterSet_VisualizerPtr blockMeshDict_Cylinder_visualizer()
{
    return ParameterSet_VisualizerPtr( new blockMeshDict_Cylinder_ParameterSet_Visualizer );
}

addStandaloneFunctionToStaticFunctionTable(OpenFOAMCaseElement, blockMeshDict_Cylinder, visualizer, blockMeshDict_Cylinder_visualizer);


void blockMeshDict_Cylinder_ParameterSet_Visualizer::update(const ParameterSet& ps)
{
    ParameterSet_Visualizer::update(ps);
}

void blockMeshDict_Cylinder_ParameterSet_Visualizer::updateVisualizationElements(QoccViewWidget* vw, QModelTree* mt)
{

    Parameters p(ps_);

    insight::cad::cache.initRebuild();

    arma::mat ex=p.geometry.ex;
    arma::mat er=p.geometry.er;
    arma::mat ey=BlockMeshTemplate::correct_trihedron(ex, er);
    double Lc=p.geometry.D*p.mesh.core_fraction;
//    double al = M_PI/2.;
//    double lc = ::cos ( al/2. ) *Lc;

    auto cyl=cad::Cylinder::create(
          cad::matconst(p.geometry.p0),
          cad::matconst(ex * p.geometry.L),
          cad::scalarconst( p.geometry.D ),
      true,
      false
    );

    auto core=cad::Box::create(
          cad::matconst(p.geometry.p0),
          cad::matconst(ex * p.geometry.L),
          cad::matconst(Lc*ey), cad::matconst(Lc*er),
          cad::BoxCentering(false, true, true)
          );

    mt->onAddFeature( "blockMeshDict_Cylinder",
                        cad::Compound::create(cad::CompoundFeatureList({cyl, core})),
                        true );

    insight::cad::cache.finishRebuild();
}





ParameterSet_VisualizerPtr blockMeshDict_Sphere_visualizer()
{
    return ParameterSet_VisualizerPtr( new blockMeshDict_Sphere_ParameterSet_Visualizer );
}

addStandaloneFunctionToStaticFunctionTable(OpenFOAMCaseElement, blockMeshDict_Sphere, visualizer, blockMeshDict_Sphere_visualizer);


void blockMeshDict_Sphere_ParameterSet_Visualizer::update(const ParameterSet& ps)
{
    ParameterSet_Visualizer::update(ps);
}

void blockMeshDict_Sphere_ParameterSet_Visualizer::updateVisualizationElements(QoccViewWidget* vw, QModelTree* mt)
{

    Parameters p(ps_);

    insight::cad::cache.initRebuild();

    arma::mat ex=p.geometry.ex;
    arma::mat ez=p.geometry.ez;
    arma::mat ey=BlockMeshTemplate::correct_trihedron(ex, ez);
    double Lc=p.geometry.D*p.geometry.core_fraction;

    auto sph=cad::Sphere::create(
          cad::matconst(p.geometry.center),
          cad::scalarconst( p.geometry.D )
    );

    auto core=cad::Box::create(
          cad::matconst(p.geometry.center),
          cad::matconst(Lc*ex), cad::matconst(Lc*ey), cad::matconst(Lc*ez),
          cad::BoxCentering(true, true, true)
          );

    mt->onAddFeature( "blockMeshDict_Sphere",
                        cad::Compound::create(cad::CompoundFeatureList({sph, core})),
                        true );

    insight::cad::cache.finishRebuild();
}



}
}
