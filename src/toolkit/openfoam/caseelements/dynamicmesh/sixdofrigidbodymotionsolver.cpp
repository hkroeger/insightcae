#include "sixdofrigidbodymotionsolver.h"

#include "base/exception.h"
#include "openfoam/ofdicts.h"

namespace insight {



relativeMovingBody::relativeMovingBody(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{}




void relativeMovingBody::addIntoDictionary(OFDictData::dict& d) const
{
    if (auto *cz =
        boost::get<Parameters::region_cellZone_type>(&p().region))
    {
        d["cellZone"]=cz->zoneName;
    }
    else if (auto *cs =
               boost::get<Parameters::region_cellSet_type>(&p().region))
    {
        d["cellSet"]=cs->setName;
    }
    else
        throw insight::UnhandledSelection();

    solidBodyMotionFunction(p()).addIntoDictionary(d);
}




defineType(SixDOFRigidBodyMotionSolver);




SixDOFRigidBodyMotionSolver::SixDOFRigidBodyMotionSolver(
    ParameterSetInput ip )
    : p_(ip.forward<Parameters>())
{}




void SixDOFRigidBodyMotionSolver::addIntoDict(OFDictData::dict& rbmc) const
{
    rbmc["report"]=true;


    OFDictData::dict sc;
    sc["type"]="Newmark";
    rbmc["solver"]=sc;


    if (auto* rhof =
        boost::get<Parameters::rho_field_type>(&p().rho))
      {
        rbmc["rho"]=rhof->fieldname;
      }
    else if (auto* rhoc =
               boost::get<Parameters::rho_constant_type>(&p().rho))
      {
        rbmc["rho"]="rhoInf";
        rbmc["rhoInf"]=rhoc->rhoInf;
      }

    rbmc["accelerationRelaxation"]=p().accelerationRelaxation;
    rbmc["accelerationDamping"]=p().accelerationDamping;

    OFDictData::dict bl;
    for (auto& ib: p().bodies)
    {
      auto& body=ib.second;

      OFDictData::dict bc;

      bc["type"]="rigidBody";
      bc["parent"]="root";
      bc["centreOfMass"]=OFDictData::vector3(body.centreOfMass);
      bc["mass"]=body.mass;

      bc["inertia"]=
          boost::str(boost::format(
                "(%g 0 0 %g 0 %g)"
              )
              % body.Ixx % body.Iyy % body.Izz
            );


      {
          arma::mat r=body.transform;
          bc["transform"]=str(
              boost::format(
                  "(%g %g %g %g %g %g %g %g %g) (0 0 0)"
                )
              % r(0,0)%r(0,1)%r(0,2)
              % r(1,0)%r(1,1)%r(1,2)
              % r(2,0)%r(2,1)%r(2,2)
            );
      }

      OFDictData::list jl;
      for (const Parameters::bodies_default_type::joints_default_type& tc:
                    body.joints)
      {
        std::string code;
        if (tc==Parameters::bodies_default_type::joints_default_type::Px) code="Px";
        else if (tc==Parameters::bodies_default_type::joints_default_type::Py) code="Py";
        else if (tc==Parameters::bodies_default_type::joints_default_type::Pz) code="Pz";
        else if (tc==Parameters::bodies_default_type::joints_default_type::Pxyz) code="Pxyz";
        else if (tc==Parameters::bodies_default_type::joints_default_type::Rx) code="Rx";
        else if (tc==Parameters::bodies_default_type::joints_default_type::Ry) code="Ry";
        else if (tc==Parameters::bodies_default_type::joints_default_type::Rz) code="Rz";
        else if (tc==Parameters::bodies_default_type::joints_default_type::Rxyz) code="Rxyz";
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

      bl[ib.first]=bc;
    }
    rbmc["bodies"]=bl;



    OFDictData::dict rcs;
    for (const auto& rc: p().restraints)
    {
        OFDictData::dict fsd;
        if (const auto* pv =
                boost::get<Parameters::restraints_default_prescribedVelocity_type>(
                 &rc.second))
        {
            fsd["type"]="prescribedVelocity";
            fsd["body"]=pv->body;
            fsd["velocity"]="table ( ( 0 "+
                    toString(arma::norm(pv->velocity,2))
                    +" ) )";
            fsd["direction"]=OFDictData::vector3(
                        pv->velocity
                        / arma::norm(pv->velocity,2) );
            fsd["relax"]=0.22;
            fsd["p"]=0.1;
            fsd["i"]=0.01;
            fsd["d"]=0.;
        }
        else if (const auto* ld =
            boost::get<Parameters::restraints_default_linearDamper_type>(
                &rc.second))
        {
            fsd["type"]="linearDamper";
            fsd["body"]="hull";
            fsd["coeff"]=ld->coeff;
        }
        else if (const auto* sd =
                 boost::get<Parameters::restraints_default_sphericalAngularDamper_type>(
                     &rc.second))
        {
            fsd["type"]="sphericalAngularDamper";
            fsd["body"]="hull";
            fsd["coeff"]=sd->coeff;
        }
        else
            throw insight::UnhandledSelection();

        rcs[rc.first]=fsd;
    }

    rbmc["restraints"]=rcs;


    // after bodies have been added
    if (auto* impl =
        boost::get<Parameters::implementation_extended_type>(
            &p().implementation))
    {
        insertExtendedMotionParameters(rbmc, p(), *impl);
    }
    else if (auto* impl =
               boost::get<Parameters::implementation_maneuvering_type>(
                   &p().implementation))
    {
        insertExtendedMotionParameters(rbmc, p(), *impl);

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




void SixDOFRigidBodyMotionSolver::insertExtendedMotionParameters(
    OFDictData::dict& rbmc,
    const Parameters& p,
    const ExtendedMotion::Parameters& emp) const
{
    rbmc["rampDuration"]=emp.rampDuration;
    rbmc["referenceSystemSpeed"]=
        OFDictData::vector3(emp.referenceSystemSpeed);

    auto& bodyList = rbmc.subDict("bodies");

    for (auto& b: bodyList)
    {
        bodyList.subDict(b.first)["directThrustForces"]=
            OFDictData::list();
        // bodyList.subDict(b.first)["relativeMovingBodies"]=
        //     OFDictData::list();
    }

    for (const auto& df: emp.directForces)
    {
        auto ib=p.bodies.find(df.bodyName);
        int iforce = &df-&emp.directForces.front();
        insight::assertion(
            ib!=p.bodies.end(),
            str(boost::format("direct force #%d references non-existing body %s!")
                % iforce % df.bodyName) );

        OFDictData::dict dfp;
        dfp["PoA"]=OFDictData::vector3(df.PoA);
        dfp["forceSource"]=df.forceSource;
        dfp["localDirection"]=OFDictData::vector3(df.localDirection);
        dfp["coordinateSystemName"]=df.coordinateSystemName;
        bodyList.subDict(df.bodyName).getList("directThrustForces").push_back(dfp);
    }

    auto &rmbs=rbmc.getList("relativeMovingBodies");
    for (auto& rmb: emp.relativeMovingBodies)
    {
        OFDictData::dict rmbd;
        relativeMovingBody(rmb).addIntoDictionary(rmbd);
        rmbs.push_back(rmbd);
    }
}


std::string SixDOFRigidBodyMotionSolver::motionSolverName() const
{
    std::string name="rigidBodyMotion";
    if (boost::get<Parameters::implementation_extended_type>(
            &p().implementation))
    {
      name="extendedRigidBodyMotion";
    }
    else if (boost::get<Parameters::implementation_maneuvering_type>(
                   &p().implementation))
    {
      name="maneuveringMotion";
    }
    return name;
}

} // namespace insight
