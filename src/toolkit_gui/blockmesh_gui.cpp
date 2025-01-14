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



addToStaticFunctionTable2(
    CADParameterSetModelVisualizer, VisualizerFunctions, visualizerForOpenFOAMCaseElement,
    blockMeshDict_Box, &newVisualizer<blockMeshDict_Box_ParameterSet_Visualizer>);


std::shared_ptr<supplementedInputDataBase>
blockMeshDict_Box_ParameterSet_Visualizer::computeSupplementedInput()
{
    return nullptr;
}

void blockMeshDict_Box_ParameterSet_Visualizer::recreateVisualizationElements()
{
  CurrentExceptionContext ex("computing visualization of blockMeshDict box template");

  blockMeshDict_Box::Parameters p(parameters());

  arma::mat ey = arma::cross( p.geometry.ez, p.geometry.ex);

  addFeature( "blockMeshDict_Box",
              cad::Box::create(
                cad::matconst(p.geometry.p0),
                cad::matconst(p.geometry.ex * p.geometry.L),
                cad::matconst(           ey * p.geometry.W),
                cad::matconst(p.geometry.ez * p.geometry.H)
                ),
             { insight::Wireframe }
              );
}





addToStaticFunctionTable2(
    CADParameterSetModelVisualizer, VisualizerFunctions, visualizerForOpenFOAMCaseElement,
    blockMeshDict_Cylinder, &newVisualizer<blockMeshDict_Cylinder_ParameterSet_Visualizer>);


std::shared_ptr<supplementedInputDataBase>
blockMeshDict_Cylinder_ParameterSet_Visualizer::computeSupplementedInput()
{
    return nullptr;
}

void blockMeshDict_Cylinder_ParameterSet_Visualizer::recreateVisualizationElements()
{

  blockMeshDict_Cylinder::Parameters p(parameters());

  arma::mat ex=p.geometry.ex;
  arma::mat er=p.geometry.er;
  arma::mat ey=BlockMeshTemplate::correct_trihedron(ex, er);

  std::string label = "blockMeshDict_Cylinder";

  if (p.geometry.d<1e-10)
  {


    cad::FeaturePtr shape=cad::Cylinder::create(
               cad::matconst(p.geometry.p0),
               cad::matconst(ex * p.geometry.L),
               cad::scalarconst( p.geometry.D ),
               true,
               false
               );

    if (auto* og = boost::get<blockMeshDict_Cylinder::Parameters::mesh_type::topology_oGrid_type>(
                &p.mesh.topology))
    {
        double Lc=p.geometry.D*og->core_fraction;
        auto core=cad::Box::create(
                    cad::matconst(p.geometry.p0),
                    cad::matconst(ex * p.geometry.L),
                    cad::matconst(Lc*ey), cad::matconst(Lc*er),
                    cad::BoxCentering(false, true, true)
                    );
        shape = cad::Compound::create(cad::CompoundFeatureList({shape, core}));
    }

    addFeature( label,
                shape,
                { insight::Wireframe }
                );
  }
  else
  {
    auto cyl=cad::Cylinder::create(
               cad::matconst(p.geometry.p0),
               cad::matconst(ex * p.geometry.L),
               cad::scalarconst( p.geometry.D ),
               cad::scalarconst( p.geometry.d ),
               true,
               false
               );

    addFeature( label,
                cyl,
                { insight::Wireframe }
                );
  }
}




addToStaticFunctionTable2(
    CADParameterSetModelVisualizer, IconFunctions, iconForOpenFOAMCaseElement,
    blockMeshDict_Cylinder, [](){ return QIcon(":symbole/bmd_cyl.svg"); });




addToStaticFunctionTable2(
    CADParameterSetModelVisualizer, VisualizerFunctions, visualizerForOpenFOAMCaseElement,
    blockMeshDict_Sphere, &newVisualizer<blockMeshDict_Sphere_ParameterSet_Visualizer>);




std::shared_ptr<supplementedInputDataBase>
blockMeshDict_Sphere_ParameterSet_Visualizer::computeSupplementedInput()
{
    return nullptr;
}

void blockMeshDict_Sphere_ParameterSet_Visualizer::recreateVisualizationElements()
{
  blockMeshDict_Sphere::Parameters p(parameters());

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
              { insight::Wireframe }
              );
}



}
}
