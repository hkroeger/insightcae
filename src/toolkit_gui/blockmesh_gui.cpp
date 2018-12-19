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

void blockMeshDict_Box_ParameterSet_Visualizer::updateVisualizationElements(QoccViewWidget* vw, QModelTree* mt) const
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

void blockMeshDict_Cylinder_ParameterSet_Visualizer::updateVisualizationElements(QoccViewWidget* vw, QModelTree* mt) const
{

    Parameters p(ps_);

    insight::cad::cache.initRebuild();

    mt->onAddFeature( "blockMeshDict_Cylinder",
                        cad::Cylinder::create(
                          cad::matconst(p.geometry.p0),
                          cad::matconst(p.geometry.ex * p.geometry.L),
                          cad::scalarconst( p.geometry.D ),
                          true,
                          false
                        ),
                        true );

    insight::cad::cache.finishRebuild();
}




}
}
