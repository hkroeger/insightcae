#include "turbulentvelocityinletbc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

#include <algorithm>

namespace insight {


defineType(TurbulentVelocityInletBC);
addToFactoryTable(BoundaryCondition, TurbulentVelocityInletBC);
addToStaticFunctionTable(BoundaryCondition, TurbulentVelocityInletBC, defaultParameters);

TurbulentVelocityInletBC::TurbulentVelocityInletBC
(
  OpenFOAMCase& c,
  const std::string& patchName,
  const OFDictData::dict& boundaryDict,
  ParameterSetInput ip
)
: BoundaryCondition(
          c, patchName,
          boundaryDict,
          ip.forward<Parameters>())
{
 BCtype_="patch";
}

const std::vector<std::string> TurbulentVelocityInletBC::inflowGenerator_types = boost::assign::list_of
   ("inflowGenerator<hatSpot>")
   ("inflowGenerator<gaussianSpot>")
   ("inflowGenerator<decayingTurbulenceSpot>")
   ("inflowGenerator<decayingTurbulenceVorton>")
   ("inflowGenerator<anisotropicVorton_Analytic>")
   ("inflowGenerator<anisotropicVorton_PseudoInv>")
   ("inflowGenerator<anisotropicVorton_NumOpt>")
   ("inflowGenerator<anisotropicVorton2>")
   ("inflowGenerator<combinedVorton>")
   ("modalTurbulence")
   .convert_to_container<std::vector<std::string> >();

void TurbulentVelocityInletBC::setField_U(OFDictData::dict& BC, OFdicts& dictionaries) const
{
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu
        = boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(
            &p().turbulence))
  {
    FieldData(p().umean).setDirichletBC(BC, dictionaries);
  }
  else if (auto* tu
             = boost::get<Parameters::turbulence_inflowGenerator_type>(
                 &p().turbulence))
  {
    BC["type"]= inflowGenerator_types[tu->type];
    BC["Umean"]=FieldData(p().umean).sourceEntry(dictionaries);
    BC["c"]=FieldData(tu->volexcess).sourceEntry(dictionaries);
    BC["uniformConvection"]=tu->uniformConvection;
    BC["R"]=FieldData(tu->R).sourceEntry(dictionaries);
    BC["L"]=FieldData(tu->L).sourceEntry(dictionaries);
    BC["value"]=OFDictData::toUniformField(vec3Zero());
  }
}

void TurbulentVelocityInletBC::setField_p(OFDictData::dict& BC, OFdicts&) const
{
  BC["type"]=OFDictData::data("zeroGradient");
}

void TurbulentVelocityInletBC::setField_k(OFDictData::dict& BC, OFdicts&) const
{
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu
        = boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(
            &p().turbulence))
  {

    double U=FieldData( p().umean ).representativeValueMag();

    double uprime=tu->intensity*U;
    double k=std::max(1e-6, 3.*pow(uprime, 2)/2.);
    BC["type"]="fixedValue";
    BC["value"]=OFDictData::toUniformField(k);

  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu
        = boost::get<Parameters::turbulence_inflowGenerator_type>(
                 &p().turbulence))
  {
    // set some small sgs energy
    BC["type"]="fixedValue";
    BC["value"]=OFDictData::toUniformField(1e-5);
  }
}

void TurbulentVelocityInletBC::setField_omega(OFDictData::dict& BC, OFdicts&) const
{
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu
        = boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(
            &p().turbulence))
  {

    double U=FieldData( p().umean ).representativeValueMag();

    double uprime = tu->intensity*U;
    double k = std::max(1e-6, 3.*pow(uprime, 2)/2.);
    double omega = sqrt(k) / tu->lengthScale;
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]=OFDictData::toUniformField(omega);

  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu
        = boost::get<Parameters::turbulence_inflowGenerator_type>(
                 &p().turbulence))
  {
    throw insight::Exception("Requested BC for field omega while inflow generator was selected!");
  }
}

void TurbulentVelocityInletBC::setField_epsilon(OFDictData::dict& BC, OFdicts&) const
{

  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu
        = boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(
            &p().turbulence))
  {

    double U=FieldData( p().umean ).representativeValueMag();

    double uprime = tu->intensity*U;
    double k=3.*pow(uprime, 2)/2.;
    double epsilon=0.09*pow(k, 1.5)/tu->lengthScale;
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]=OFDictData::toUniformField(epsilon);

  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu
        = boost::get<Parameters::turbulence_inflowGenerator_type>(
                 &p().turbulence))
  {
    throw insight::Exception("Requested BC for field epsilon while inflow generator was selected!");
  }

}

void TurbulentVelocityInletBC::setField_nuTilda(OFDictData::dict& BC, OFdicts&) const
{
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu
        = boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(
            &p().turbulence))
  {

    double U=FieldData( p().umean ).representativeValueMag();

    double uprime = tu->intensity*U;
    double nutilda=sqrt(1.5)* uprime * tu->lengthScale;
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]=OFDictData::toUniformField(nutilda);

  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu
        = boost::get<Parameters::turbulence_inflowGenerator_type>(
                 &p().turbulence))
  {
    throw insight::Exception("Requested BC for field nuTilda while inflow generator was selected!");
  }

}

void TurbulentVelocityInletBC::setField_R(OFDictData::dict& BC, OFdicts&) const
{
  if (const Parameters::turbulence_uniformIntensityAndLengthScale_type* tu
        = boost::get<Parameters::turbulence_uniformIntensityAndLengthScale_type>(
            &p().turbulence))
  {

    double U=FieldData( p().umean ).representativeValueMag();

    double uprime=tu->intensity*U;
    double kBy3=std::max(1e-6, pow(uprime, 2)/2.);
    BC["type"]="fixedValue";
    arma::mat R;
    R << kBy3 << 0. << 0. << kBy3 << 0. << kBy3 << arma::endr;
    BC["value"]=OFDictData::toUniformField(R);

  }
  else if (const Parameters::turbulence_inflowGenerator_type* tu
        = boost::get<Parameters::turbulence_inflowGenerator_type>(
                 &p().turbulence))
  {
    throw insight::Exception("Requested BC for field R while inflow generator was selected!");
  }
}


void TurbulentVelocityInletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  multiphaseBC::multiphaseBCPtr phasefractions =p().phasefractions;

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");

  if (boost::get<Parameters::turbulence_inflowGenerator_type>(&p().turbulence))
    controlDict.getList("libs").push_back( OFDictData::data("\"libinflowGeneratorBC.so\"") );

  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  phasefractions->addIntoDictionaries ( dictionaries );
//   p_.phasefractions()->addIntoDictionaries(dictionaries);

  for (const FieldList::value_type& field: OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      setField_U(BC, dictionaries);
    }

    else if (
      (field.first=="p") && (get<0>(field.second)==scalarField)
    )
    {
      setField_p(BC, dictionaries);
    }
    else if ( (field.first=="k") && (get<0>(field.second)==scalarField) )
    {
      setField_k(BC, dictionaries);
    }
    else if ( (field.first=="omega") && (get<0>(field.second)==scalarField) )
    {
      setField_omega(BC, dictionaries);
    }
    else if ( (field.first=="epsilon") && (get<0>(field.second)==scalarField) )
    {
      setField_epsilon(BC, dictionaries);
    }
    else if ( (field.first=="nut") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("calculated");
      BC["value"]=OFDictData::toUniformField(1e-10);
    }
    else if ( (field.first=="nuTilda") && (get<0>(field.second)==scalarField) )
    {
      setField_nuTilda(BC, dictionaries);
    }
    else if ( (field.first=="R") && (get<0>(field.second)==symmTensorField) )
    {
      setField_R(BC, dictionaries);
    }
    else if ( (field.first=="nuSgs") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]="zeroGradient";
    }
    else
    {
      if (!(
          MeshMotionBC::noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
          ||
          phasefractions->addIntoFieldDictionary(field.first, field.second, BC)
          ))
        {
          BC["type"]=OFDictData::data("zeroGradient");
        }
        //throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
    }
  }
}



// ParameterSet TurbulentVelocityInletBC::defaultParameters()
// {
//   return ParameterSet
//   (
//     boost::assign::list_of<ParameterSet::SingleEntry>
//     ("uniformConvection", new BoolParameter(false, "Whether to use a uniform convection velocity instead of the local mean velocity"))
//     ("volexcess", new DoubleParameter(16.0, "Volumetric overlapping of spots"))
//     (
//       "type", new SelectionParameter(0,
// 	list_of<string>
// 	("inflowGenerator<hatSpot>")
// 	("inflowGenerator<gaussianSpot>")
// 	("inflowGenerator<decayingTurbulenceSpot>")
// 	("inflowGenerator<decayingTurbulenceVorton>")
// 	("inflowGenerator<anisotropicVorton>")
// 	("modalTurbulence")
//       ,
//       "Type of inflow generator")
//     )
//     ("L", FieldData::defaultParameter(vec3(1,1,1), "Origin of the prescribed integral length scale field"))
//     ("R", FieldData::defaultParameter(arma::zeros(6), "Origin of the prescribed reynolds stress field"))
//     .convert_to_container<ParameterSet::EntryList>()
//   );
// }


// void TurbulentVelocityInletBC::initInflowBC(const boost::filesystem::path& location, const ParameterSet& iniparams) const
// {
//   if (p_.initializer())
//   {
//     OFDictData::dictFile inflowProperties;
//
//     OFDictData::list& initializers = inflowProperties.getList("initializers");
//
//     initializers.push_back( p_.initializer()->type() );
//     OFDictData::dict d;
//     p_.initializer()->addToInitializerList(d, patchName_, p_.velocity().representativeValue(), iniparams);
//     initializers.push_back(d);
//
//     // then write to file
//     inflowProperties.write( location / "constant" / "inflowProperties" );
//
//     OFcase().executeCommand(location, "initInflowGenerator");
//   }
// }

} // namespace insight
