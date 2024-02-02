#include "velocityinletbc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {




defineType(VelocityInletBC);
addToFactoryTable(BoundaryCondition, VelocityInletBC);
addToStaticFunctionTable(BoundaryCondition, VelocityInletBC, defaultParameters);




VelocityInletBC::VelocityInletBC
(
  OpenFOAMCase& c,
  const std::string& patchName,
  const OFDictData::dict& boundaryDict,
  const ParameterSet& ps
)
: BoundaryCondition(c, patchName, boundaryDict, ps),
  ps_(ps)
{
 BCtype_="patch";
}




void VelocityInletBC::setField_p(OFDictData::dict& BC, OFdicts&, bool isPrgh) const
{
    Parameters p ( ps_ );
    if (boost::get<Parameters::VoFWave_enabled_type>(&p.VoFWave))
    {
        BC["type"]=OFDictData::data ( "zeroGradient" );
    }
    else
    {
        if (isPrgh)
        {
            if ( OFversion() >=210 ) {
                BC["type"]=OFDictData::data ( "fixedFluxPressure" );
            } else {
                BC["type"]=OFDictData::data ( "buoyantPressure" );
            }
        }
        else
        {
            BC["type"]=OFDictData::data("zeroGradient");
        }
    }
}




void VelocityInletBC::setField_U(OFDictData::dict& BC, OFdicts& dictionaries) const
{
    Parameters p ( ps_ );
    if (boost::get<Parameters::VoFWave_enabled_type>(&p.VoFWave))
        BC["type"]=OFDictData::data ( "zeroGradient" );
    else
        FieldData(p.velocity).setDirichletBC(BC, dictionaries);
//   FieldData(ps_.get<SelectableSubsetParameter>("velocity")()).setDirichletBC(BC);
}




void VelocityInletBC::addIntoFieldDictionaries ( OFdicts& dictionaries) const
{
    Parameters p ( ps_ );

    turbulenceBC::turbulenceBCPtr turbulence =
        turbulenceBC::turbulenceBC::create ( ps_.get<SelectableSubsetParameter> ( "turbulence" ) );
    multiphaseBC::multiphaseBCPtr phasefractions =
        multiphaseBC::multiphaseBC::create ( ps_.get<SelectableSubsetParameter> ( "phasefractions" ) );

    FieldData velocity(ps_.getSubset("velocity")), T(ps_.getSubset("T")), rho(ps_.getSubset("rho"));

    BoundaryCondition::addIntoFieldDictionaries ( dictionaries );

    if ( boost::get<Parameters::VoFWave_enabled_type>(&p.VoFWave) )
    {
        phasefractions.reset(); // will trigger "zeroGradient" for alpha
    }
    else
    {
        phasefractions->addIntoDictionaries ( dictionaries );
    }

    for ( const FieldList::value_type& field: OFcase().fields() )
    {
        OFDictData::dict& BC=dictionaries.addFieldIfNonexistent (
                    "0/"+field.first, field.second )
                .subDict ( "boundaryField" )
                .subDict ( patchName_ );

        if ( ( field.first=="U" ) && ( get<0> ( field.second ) ==vectorField ) )
        {
            setField_U ( BC, dictionaries );
        }
        else if (
            ( field.first=="p" ) && ( get<0> ( field.second ) ==scalarField )
        )
        {
            setField_p ( BC, dictionaries, false );
        }
        else if (
            ( field.first=="T" )
            &&
            ( get<0> ( field.second ) ==scalarField )
        )
        {
            T.setDirichletBC ( BC, dictionaries );
        }
        else if (isPrghPressureField(field))
        {
            setField_p ( BC, dictionaries, true );
        }
        else if ( ( field.first=="rho" ) && ( get<0> ( field.second ) ==scalarField ) )
        {
            rho.setDirichletBC ( BC, dictionaries );
        }
        else if ( ( field.first=="k" ) && ( get<0> ( field.second ) ==scalarField ) )
        {
            turbulence->setDirichletBC_k ( BC, velocity.representativeValueMag() );
        }
        else if ( ( field.first=="omega" ) && ( get<0> ( field.second ) ==scalarField ) )
        {
            turbulence->setDirichletBC_omega ( BC, velocity.representativeValueMag() );
        }
        else if ( ( field.first=="epsilon" ) && ( get<0> ( field.second ) ==scalarField ) )
        {
            turbulence->setDirichletBC_epsilon ( BC, velocity.representativeValueMag() );
        }
        else if ( ( field.first=="nut" ) && ( get<0> ( field.second ) ==scalarField ) )
        {
            BC["type"]=OFDictData::data ( "calculated" );
            BC["value"]="uniform "+boost::lexical_cast<std::string> ( 1e-10 );
        }
        else if ( ( field.first=="nuTilda" ) && ( get<0> ( field.second ) ==scalarField ) )
        {
            turbulence->setDirichletBC_nuTilda ( BC, velocity.representativeValueMag() );
        }
        else if ( ( field.first=="R" ) && ( get<0> ( field.second ) ==symmTensorField ) )
        {
            turbulence->setDirichletBC_R ( BC, velocity.representativeValueMag() );
        }
        else if ( ( field.first=="nuSgs" ) && ( get<0> ( field.second ) ==scalarField ) )
        {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]="uniform 1e-10";
        }
        else
        {
            if ( !(
                     MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC )
                     ||
                     (phasefractions && phasefractions->addIntoFieldDictionary ( field.first, field.second, BC ))
                     ) )
            {
                BC["type"]=OFDictData::data ( "zeroGradient" );
            }
            //throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
        }
    }
}




void VelocityInletBC::addIntoDictionaries(OFdicts &dicts) const
{
    BoundaryCondition::addIntoDictionaries(dicts);

    Parameters p ( ps_ );

    if (const auto* w =
            boost::get<Parameters::VoFWave_enabled_type>(&p.VoFWave))
    {

        auto& wp = dicts.lookupDict("constant/waveProperties.input");
        wp["seaLevel"]=0.0;
        wp.getList("relaxationNames").insertNoDuplicate(patchName_);

        if (wp.findKeys(boost::regex("initializationName")).empty())
        {
            wp["initializationName"]=patchName_;
        }

        OFDictData::dict coeffs;
        if (w->waveType==Parameters::VoFWave_enabled_type::stokesFirst)
            coeffs["waveType"]="stokesFirst";
        coeffs["Tsoft"]=w->Tsoft;
        coeffs["depth"]=w->depth;
        coeffs["period"]=w->period;
        coeffs["phi"]=w->phi;
        coeffs["direction"]=OFDictData::vector3(normalized(w->direction));
        coeffs["height"]=w->height;


        FieldData Uf(p.velocity);
        if (Uf.maxValueMag()>SMALL)
        {
            throw insight::Exception("waves + convection velocity is not supported!");
//            arma::mat U;
//            if (!Uf.isAConstantValue(U))
//                throw insight::Exception("only constant uniform values are supported!");

//            OFDictData::dict fcs;
//            fcs["Tsoft"]=0;
//            fcs["U"]= OFDictData::to_OF(U);
//            fcs["waveType"]="potentialCurrent";

//            wp[patchName_+"FlowCoeffs"]=fcs;

//            wp[patchName_+"WavesCoeffs"]=coeffs;

//            OFDictData::dict combinedCoeffs;
//            combinedCoeffs["waveType"]="combinedWaves";
//            combinedCoeffs["combinedWaveNames"]=
//                OFDictData::list{
//                    patchName_+"Flow", patchName_+"Waves"};

//            wp[patchName_+"Coeffs"]=combinedCoeffs;
//            wp["wind"]= OFDictData::to_OF(U);
        }
        else
        {
            wp[patchName_+"Coeffs"]=coeffs;
        }

        auto& controlDict = dicts.lookupDict("system/controlDict");
        controlDict.getList("libs").insertNoDuplicate("\"libwaves2Foam.so\"");
        controlDict.getList("libs").insertNoDuplicate("\"libwaves2FoamPatchDistRelaxationShape.so\"");
    }
}



void VelocityInletBC::modifyCaseOnDisk(const OpenFOAMCase &cm, const boost::filesystem::path &location) const
{
    BoundaryCondition::modifyCaseOnDisk(cm, location);

    Parameters p ( ps_ );

    if (const auto* w =
            boost::get<Parameters::VoFWave_enabled_type>(&p.VoFWave))
    {
        cm.executeCommand(location, "setWaveParameters");

        // modify generated dict, because subdicts in the waveProperties.input would cause errors
        auto wpdpath=location/"constant"/"waveProperties";
        OFDictData::dictFile wpd;
        readOpenFOAMDict(wpdpath, wpd);
        auto& coeffs = wpd.subDict(patchName_+"Coeffs");

        OFDictData::dict relax;
        if ( const auto* rt =
                boost::get<Parameters::VoFWave_enabled_type::relaxationZone_patchDist_type>(
                    &w->relaxationZone ) )
        {
            relax["relaxationScheme"]="Spatial";
            relax["relaxationShape"]="PatchDist2";
            relax["beachType"]="Empty";
            relax["relaxationPatches"]=OFDictData::list({patchName_});
                OFDictData::dict pdm;
                pdm["method"]="meshWave";
            relax["patchDist"]=pdm;
            relax["width"]=rt->width;
        }

        coeffs["relaxationZone"]=relax;
        writeOpenFOAMDict(wpdpath, wpd);
    }
}



} // namespace insight
