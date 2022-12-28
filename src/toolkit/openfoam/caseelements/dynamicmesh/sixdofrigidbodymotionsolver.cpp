#include "sixdofrigidbodymotionsolver.h"

#include "openfoam/ofdicts.h"

namespace insight {

defineType(SixDOFRigidBodyMotionSolver);

SixDOFRigidBodyMotionSolver::SixDOFRigidBodyMotionSolver(const ParameterSet& ps )
    : ps_(ps)
{}

void SixDOFRigidBodyMotionSolver::addIntoDict(OFDictData::dict& rbmc) const
{
    Parameters p(ps_);

    rbmc["report"]=true;


    OFDictData::dict sc;
     sc["type"]="Newmark";
    rbmc["solver"]=sc;


    if (const Parameters::rho_field_type* rhof = boost::get<Parameters::rho_field_type>(&p.rho))
      {
        rbmc["rho"]=rhof->fieldname;
      }
    else if (const Parameters::rho_constant_type* rhoc = boost::get<Parameters::rho_constant_type>(&p.rho))
      {
        rbmc["rho"]="rhoInf";
        rbmc["rhoInf"]=rhoc->rhoInf;
      }

    rbmc["accelerationRelaxation"]=0.4;

    OFDictData::dict bl;
    for (const Parameters::bodies_default_type& body: p.bodies)
    {
      OFDictData::dict bc;

      bc["type"]="rigidBody";
      bc["parent"]="root";
      bc["centreOfMass"]=OFDictData::vector3(body.centreOfMass);
      bc["mass"]=body.mass;
      bc["inertia"]=boost::str(boost::format("(%g 0 0 %g 0 %g)") % body.Ixx % body.Iyy % body.Izz);
      bc["transform"]="(1 0 0 0 1 0 0 0 1) (0 0 0)";

      OFDictData::list jl;
      for (const Parameters::bodies_default_type::translationConstraint_default_type& tc:
                    body.translationConstraint)
      {
        std::string code;
        if (tc==Parameters::bodies_default_type::translationConstraint_default_type::Px) code="Px";
        else if (tc==Parameters::bodies_default_type::translationConstraint_default_type::Py) code="Py";
        else if (tc==Parameters::bodies_default_type::translationConstraint_default_type::Pz) code="Pz";
        else if (tc==Parameters::bodies_default_type::translationConstraint_default_type::Pxyz) code="Pxyz";
        else throw insight::Exception("internal error: unhandled value!");
        OFDictData::dict d;
         d["type"]=code;
        jl.push_back(d);
      }
      for (const Parameters::bodies_default_type::rotationConstraint_default_type& rc:
                    body.rotationConstraint)
      {
        std::string code;
        if (rc==Parameters::bodies_default_type::rotationConstraint_default_type::Rx) code="Rx";
        else if (rc==Parameters::bodies_default_type::rotationConstraint_default_type::Ry) code="Ry";
        else if (rc==Parameters::bodies_default_type::rotationConstraint_default_type::Rz) code="Rz";
        else if (rc==Parameters::bodies_default_type::rotationConstraint_default_type::Rxyz) code="Rxyz";
        else throw insight::Exception("internal error: unhandled value!");
        OFDictData::dict d;
         d["type"]=code;
        jl.push_back(d);
      }
      OFDictData::dict jc;
       jc["type"]="composite";
       jc["joints"]=jl;
      bc["joint"]=jc;

      OFDictData::list pl;
      std::copy(body.patches.begin(), body.patches.end(), std::back_inserter(pl));
      bc["patches"]=pl;

      bc["innerDistance"]=body.innerDistance;
      bc["outerDistance"]=body.outerDistance;

      bl[body.name]=bc;
    }
    rbmc["bodies"]=bl;



    OFDictData::dict rcs;
    for (const auto& rc: p.restraints)
    {
        if (const auto* pv =
                boost::get<Parameters::restraints_default_prescribedVelocity_type>(&rc))
        {
            OFDictData::dict fsd;
            fsd["type"]="prescribedVelocity";
            fsd["body"]=pv->body;
            fsd["velocity"]="table ( ( 0 "+
                    boost::lexical_cast<std::string>(arma::norm(pv->velocity,2))
                    +" ) )";
            fsd["direction"]=OFDictData::vector3(
                        pv->velocity
                        / arma::norm(pv->velocity,2) );
            fsd["relax"]=0.22;
            fsd["p"]=0.1;
            fsd["i"]=0.01;
            fsd["d"]=0.;
            rcs[pv->label]=fsd;
        }
    }

    rbmc["restraints"]=rcs;


    // after bodies have been added
    if (const auto* impl = boost::get<Parameters::implementation_extended_type>(&p.implementation))
    {
        insertExtendedMotionParameters(rbmc, p, *impl);
    }
    else if (const auto* impl = boost::get<Parameters::implementation_maneuvering_type>(&p.implementation))
    {
        insertExtendedMotionParameters(rbmc, p, *impl);

        rbmc["relativeMovingBodies"]=OFDictData::list();

        OFDictData::list rudders;
        for (const auto& r: impl->rudders)
        {
            OFDictData::dict rd;
            rd["cellZone"]=r.cellZone;
            rd["origin"]=OFDictData::vector3(r.origin);
            rd["axis"]=OFDictData::vector3(r.axis);
            rd["maxRudderSpeed"]=r.maxRudderSpeed;
            rudders.push_back(rd);
        }
        rbmc["rudders"]=rudders;
    }

}

std::string SixDOFRigidBodyMotionSolver::motionSolverName() const
{
    Parameters p(ps_);

    std::string name="rigidBodyMotion";
    if (boost::get<Parameters::implementation_extended_type>(&p.implementation))
    {
      name="extendedRigidBodyMotion";
    }
    else if (boost::get<Parameters::implementation_maneuvering_type>(&p.implementation))
    {
      name="maneuveringMotion";
    }
    return name;
}

} // namespace insight
