
#include "snappyhexmesh_gui.h"
#include "occtools.h"
#include "openfoam/openfoamtools.h"

#include "cadfeatures.h"
#include "qoccviewwidget.h"
#include "qmodeltree.h"

#include "datum.h"

#include "base/boost_include.h"
#include "base/units.h"

using namespace std;
using namespace boost;

namespace insight
{



addToStaticFunctionTable2(
    CADParameterSetModelVisualizer,
    VisualizerFunctions, visualizerForOpenFOAMCaseElement,
    snappyHexMeshConfiguration, &newVisualizer<snappyHexMeshConfiguration_ParameterSet_Visualizer>);


std::shared_ptr<supplementedInputDataBase>
snappyHexMeshConfiguration_ParameterSet_Visualizer::computeSupplementedInput()
{
    return nullptr;
}

void snappyHexMeshConfiguration_ParameterSet_Visualizer::recreateVisualizationElements()
{
#warning reenable
  snappyHexMeshConfiguration::Parameters p(parameters());

  for (const auto& feat: p.features)
  {
    if ( const auto* geo = dynamic_cast<snappyHexMeshFeats::Geometry*>(feat.get()) )
    {
      const auto& gp = geo->p();

      if (gp.geometry->isValid())
      {
          cad::is_gp_Trsf tr(gp.translate, gp.rollPitchYaw, gp.scalefactor);

        addFeature(
              "geometry:"+gp.name,
              cad::Transform::create(gp.geometry->geometry(), tr)
              );
      }
    } else if ( const auto* refbox = dynamic_cast<snappyHexMeshFeats::RefinementBox*>(feat.get()) )
    {
      const auto& gp = refbox->p();
      arma::mat l=gp.max-gp.min;
      addFeature( "refinement:"+gp.name,
                  cad::Box::create(
                    cad::matconst(gp.min),
                    cad::vec3const(l(0),0,0),
                    cad::vec3const(0,l(1),0),
                    cad::vec3const(0,0,l(2))
                    )
                  );

    } else if ( const auto* refcyl = dynamic_cast<snappyHexMeshFeats::RefinementCylinder*>(feat.get()) )
    {
      const auto& gp = refcyl->p();
      addFeature( "refinement:"+gp.name,
                  cad::Cylinder::create(
                    cad::matconst(gp.point1),
                    cad::matconst(gp.point2),
                    cad::scalarconst(2.*gp.radius),
                    false, false
                    ));
    } else if ( const auto* refsph = dynamic_cast<snappyHexMeshFeats::RefinementSphere*>(feat.get()) )
    {
      const auto& gp = refsph->p();
      addFeature( "refinement:"+gp.name,
                  cad::Sphere::create(
                    cad::matconst(gp.center),
                    cad::scalarconst(2.*gp.radius)
                    ));
    } else if ( const auto* refgeo = dynamic_cast<snappyHexMeshFeats::RefinementGeometry*>(feat.get()) )
    {
      const auto& gp = refgeo->p();

      if (gp.geometry->isValid())
      {
        addFeature( "refinement:"+gp.name,
                    cad::Transform::create(
                       gp.geometry->geometry(),
                        cad::is_gp_Trsf(gp.translate, gp.rollPitchYaw, gp.scalefactor))
                    );
      }
    }
  }

  {
    int i=-1;
    for (const auto& pt: p.PiM)
    {
      i++;

      addDatum( str(format("PiM %d")%i),
                cad::DatumPtr(
                  new cad::ExplicitDatumPoint(cad::matconst(pt))
                  ) );
    }
  }

}


addToStaticFunctionTable2(
    CADParameterSetModelVisualizer,
    IconFunctions, iconForOpenFOAMCaseElement,
    snappyHexMeshConfiguration, [](){ return QIcon(":symbole/sHM-cfg.svg"); });




}
