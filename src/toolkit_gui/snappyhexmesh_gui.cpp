
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


void snappyHexMeshConfiguration_ParameterSet_Visualizer::update(const ParameterSet& ps)
{
    ParameterSet_Visualizer::update(ps);
}

void snappyHexMeshConfiguration_ParameterSet_Visualizer::updateVisualizationElements(QoccViewWidget* vw, QModelTree* mt)
{

    Parameters p(ps_);

    cad::cache.initRebuild();

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

                mt->onAddFeature(
                      "geometry:"+QString::fromStdString(gp.name),
                      cad::STL::create_trsf(gp.fileName,
                                            scale*t2c*t2b*t2a*trans),
                          true );
        }
      } else if ( const auto* refbox = dynamic_cast<snappyHexMeshFeats::RefinementBox*>(feat.get()) )
      {
        const auto& gp = refbox->parameters();
        arma::mat l=gp.max-gp.min;
        mt->onAddFeature( "refinement:"+QString::fromStdString(gp.name),
                          cad::Box::create(
                            cad::matconst(gp.min),
                            cad::vec3const(l(0),0,0),
                            cad::vec3const(0,l(1),0),
                            cad::vec3const(0,0,l(2))
                          ),
                  true );

      } else if ( const auto* refcyl = dynamic_cast<snappyHexMeshFeats::RefinementCylinder*>(feat.get()) )
      {
        const auto& gp = refcyl->parameters();
        mt->onAddFeature( "refinement:"+QString::fromStdString(gp.name),
                          cad::Cylinder::create(
                             cad::matconst(gp.point1),
                             cad::matconst(gp.point2),
                             cad::scalarconst(2.*gp.radius),
                             false, false
                            ), true );
      } else if ( const auto* refsph = dynamic_cast<snappyHexMeshFeats::RefinementSphere*>(feat.get()) )
      {
        const auto& gp = refsph->parameters();
        mt->onAddFeature( "refinement:"+QString::fromStdString(gp.name),
                          cad::Sphere::create(
                             cad::matconst(gp.center),
                             cad::scalarconst(2.*gp.radius)
                            ), true );
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

                mt->onAddFeature( "refinement:"+QString::fromStdString(rgp.name),
                          cad::STL::create_trsf(gp.fileName, trans*scale),
                          true );
        }
      }
    }

    {
      int i=-1;
      for (const auto& pt: p.PiM)
      {
          i++;

          mt->onAddDatum( QString::fromStdString(str(format("PiM %d")%i)), cad::DatumPtr(
                              new cad::ExplicitDatumPoint(cad::matconst(pt))
                              ) );
      }
    }



//    double xBow, xStern, zKeel; // always in mm

//    if (Parameters::geometry_type::size_fixed_type* fixed
//                 = boost::get<Parameters::geometry_type::size_fixed_type>(&p.geometry.size) )
//    {
//      xBow = fixed->xBow;
//      xStern = fixed->xStern;
//      zKeel = fixed->zKeel;

//      mt->onAddDatum( "xBow", cad::DatumPtr(
//                          new cad::DatumPlane(cad::vec3const((xBow-xStern)*SI::mm,0,0), cad::vec3const(1,0,0))
//                          ) );
//      mt->onAddDatum( "xStern", cad::DatumPtr(
//                          new cad::DatumPlane(cad::vec3const(0,0,0), cad::vec3const(-1,0,0))
//                          ) );
//    }

//    if (boost::filesystem::exists(p.geometry.hullfile))
//    {

//        if (const Parameters::geometry_type::size_detect_type* detect
//                = boost::get<Parameters::geometry_type::size_detect_type>(&p.geometry.size) )
//        {
//          arma::mat bb=STLBndBox(p.geometry.hullfile);
//          xBow = bb(0,1); // max x
//          xStern = bb(0,0); // min x
//          zKeel = bb(2,0);
//          if (p.geometry.hullfile_units == Parameters::geometry_type::hullfile_units_type::meters)
//          {
//              xBow /= SI::mm; // max x
//              xStern /= SI::mm; // min x
//              zKeel /= SI::mm;
//          }
//          mt->onAddDatum( "xBow", cad::DatumPtr(
//                              new cad::DatumPlane(cad::vec3const((xBow-xStern)*SI::mm,0,0), cad::vec3const(1,0,0))
//                              ) );
//          mt->onAddDatum( "xStern", cad::DatumPtr(
//                              new cad::DatumPlane(cad::vec3const(0,0,0), cad::vec3const(-1,0,0))
//                              ) );
//        }

//        gp_Trsf trans;
////        if (p.geometry.hullfile_units == Parameters::geometry_type::hullfile_units_type::millimeters)
////        {
////            trans.SetTranslation(gp_Vec(-xStern, 0, -zKeel/*-p.geometry.hWater*/));
////        }
////        else
////        {
//            trans.SetTranslation(gp_Vec(-xStern*SI::mm, 0, -zKeel*SI::mm/*-p.geometry.hWater*/));
////        }

//        gp_Trsf scale;
//        if (p.geometry.hullfile_units == Parameters::geometry_type::hullfile_units_type::millimeters)
//        {
//            scale.SetScale(gp::Origin(), SI::mm);
//        }


//        mt->onAddFeature( "geometry/hullfile",
//                          cad::STL::create_trsf(p.geometry.hullfile, trans*scale),
//                          true );
//    }


//    mt->onAddDatum( "geometry/hWater", cad::DatumPtr(
//                        new cad::DatumPlane(cad::vec3const(0.5*(xBow-xStern)*SI::mm,0,p.geometry.hWater*SI::mm), cad::vec3const(0,0,1))
//                        ) );




//    int i=0;
//    BOOST_FOREACH(const Parameters::geometry_type::fins_default_type& fin, p.geometry.fins)
//    {
//        PDStripFin f(fin);

//        mt->onAddFeature
//        (
//            QString::fromStdString(boost::str(boost::format("fin %d")%i)),
//            cad::Cylinder::create
//                    (
//                      cad::matconst( f.startPoint() ),
//                      cad::matconst( f.endPoint() ),
//                      cad::scalarconst( 1. ),
//                      false, false
//                    ),
//            true
//        );

//        i++;
//    }

    insight::cad::cache.finishRebuild();
}

}
