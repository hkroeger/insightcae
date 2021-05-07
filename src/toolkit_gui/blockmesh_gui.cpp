#include "blockmesh_gui.h"
#include "cadfeatures.h"
#include "qoccviewwidget.h"
#include "qmodeltree.h"

#include "datum.h"
#include "cadfeatures/box.h"
#include "cadfeatures/cylinder.h"

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



void blockMeshDict_Box_ParameterSet_Visualizer::recreateVisualizationElements()
{
  CAD_ParameterSet_Visualizer::recreateVisualizationElements();

  blockMeshDict_Box::Parameters p(currentParameters());

  arma::mat ey = arma::cross( p.geometry.ez, p.geometry.ex);

  addFeature( "blockMeshDict_Box",
              cad::Box::create(
                cad::matconst(p.geometry.p0),
                cad::matconst(p.geometry.ex * p.geometry.L),
                cad::matconst(           ey * p.geometry.W),
                cad::matconst(p.geometry.ez * p.geometry.H)
                ),
              AIS_WireFrame
              );
}





ParameterSet_VisualizerPtr blockMeshDict_Cylinder_visualizer()
{
    return ParameterSet_VisualizerPtr( new blockMeshDict_Cylinder_ParameterSet_Visualizer );
}

addStandaloneFunctionToStaticFunctionTable(OpenFOAMCaseElement, blockMeshDict_Cylinder, visualizer, blockMeshDict_Cylinder_visualizer);


void blockMeshDict_Cylinder_ParameterSet_Visualizer::recreateVisualizationElements()
{
  CAD_ParameterSet_Visualizer::recreateVisualizationElements();

  blockMeshDict_Cylinder::Parameters p(currentParameters());

  arma::mat ex=p.geometry.ex;
  arma::mat er=p.geometry.er;
  arma::mat ey=BlockMeshTemplate::correct_trihedron(ex, er);

  std::string label = "blockMeshDict_Cylinder";

  if (p.geometry.d<1e-10)
  {
    double Lc=p.geometry.D*p.mesh.core_fraction;


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

    addFeature( label,
                cad::Compound::create(cad::CompoundFeatureList({cyl, core})),
                AIS_WireFrame
                );
  }
  else
  {
    auto cyl=cad::Cylinder::create_hollow(
               cad::matconst(p.geometry.p0),
               cad::matconst(ex * p.geometry.L),
               cad::scalarconst( p.geometry.D ),
               cad::scalarconst( p.geometry.d ),
               true,
               false
               );

    addFeature( label,
                cyl,
                AIS_WireFrame
                );
  }
}



void blockMeshDict_Cylinder_ParameterSet_Visualizer::setIcon(QIcon* i)
{
  *i=QIcon(":symbole/bmd_cyl.svg");
}




ParameterSet_VisualizerPtr blockMeshDict_Sphere_visualizer()
{
    return ParameterSet_VisualizerPtr( new blockMeshDict_Sphere_ParameterSet_Visualizer );
}

addStandaloneFunctionToStaticFunctionTable(OpenFOAMCaseElement, blockMeshDict_Sphere, visualizer, blockMeshDict_Sphere_visualizer);


void blockMeshDict_Sphere_ParameterSet_Visualizer::recreateVisualizationElements()
{
  CAD_ParameterSet_Visualizer::recreateVisualizationElements();

  blockMeshDict_Sphere::Parameters p(currentParameters());

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

  addFeature( "blockMeshDict_Sphere",
              cad::Compound::create(cad::CompoundFeatureList({sph, core})),
              AIS_WireFrame
              );
}



}
}
