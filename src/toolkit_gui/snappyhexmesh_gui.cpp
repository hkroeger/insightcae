
#include "snappyhexmesh_gui.h"
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


ParameterSet_VisualizerPtr snappyHexMeshConfiguration_visualizer()
{
    return ParameterSet_VisualizerPtr( new snappyHexMeshConfiguration_ParameterSet_Visualizer );
}

addStandaloneFunctionToStaticFunctionTable(OpenFOAMCaseElement, snappyHexMeshConfiguration, visualizer, snappyHexMeshConfiguration_visualizer);


void snappyHexMeshConfiguration_ParameterSet_Visualizer::recreateVisualizationElements(UsageTracker* ut)
{
  CAD_ParameterSet_Visualizer::recreateVisualizationElements(ut);

  snappyHexMeshConfiguration::Parameters p(ps_);

  for (const auto& feat: p.features)
  {
    if ( const auto* geo = dynamic_cast<snappyHexMeshFeats::Geometry*>(feat.get()) )
    {
      const auto& gp = geo->parameters();

      if (boost::filesystem::exists(gp.fileName))
      {
        gp_Trsf trans;
        trans.SetTranslation(to_Vec(gp.translate));

        gp_Trsf t2a;
        t2a.SetRotation(gp::OX(), gp.rollPitchYaw(0)*SI::deg);
        gp_Trsf t2b;
        t2b.SetRotation(gp::OY(), gp.rollPitchYaw(1)*SI::deg);
        gp_Trsf t2c;
        t2c.SetRotation(gp::OZ(), gp.rollPitchYaw(2)*SI::deg);

        gp_Trsf scale;
        scale.SetScale(gp::Origin(), gp.scale[0]);

        addFeature(
              "geometry:"+gp.name,
              cad::STL::create_trsf(gp.fileName,
                                    scale*t2c*t2b*t2a*trans)
              );
      }
    } else if ( const auto* refbox = dynamic_cast<snappyHexMeshFeats::RefinementBox*>(feat.get()) )
    {
      const auto& gp = refbox->parameters();
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
      const auto& gp = refcyl->parameters();
      addFeature( "refinement:"+gp.name,
                  cad::Cylinder::create(
                    cad::matconst(gp.point1),
                    cad::matconst(gp.point2),
                    cad::scalarconst(2.*gp.radius),
                    false, false
                    ));
    } else if ( const auto* refsph = dynamic_cast<snappyHexMeshFeats::RefinementSphere*>(feat.get()) )
    {
      const auto& gp = refsph->parameters();
      addFeature( "refinement:"+gp.name,
                  cad::Sphere::create(
                    cad::matconst(gp.center),
                    cad::scalarconst(2.*gp.radius)
                    ));
    } else if ( const auto* refgeo = dynamic_cast<snappyHexMeshFeats::RefinementGeometry*>(feat.get()) )
    {
      const auto& rgp = refgeo->parameters();
      const auto& gp = rgp.geometry;

      if (boost::filesystem::exists(gp.fileName))
      {
        gp_Trsf trans;
        trans.SetTranslation(to_Vec(gp.translate));

        gp_Trsf scale;
        scale.SetScale(gp::Origin(), gp.scale[0]);

        addFeature( "refinement:"+rgp.name,
                    cad::STL::create_trsf(gp.fileName, trans*scale)
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


void snappyHexMeshConfiguration_ParameterSet_Visualizer::setIcon(QIcon *i)
{
  *i=QIcon(":symbole/sHM-cfg.svg");
}

}
