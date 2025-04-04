#include "exptdatainletbc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {

defineType(ExptDataInletBC);
addToFactoryTable(BoundaryCondition, ExptDataInletBC);
addToStaticFunctionTable(BoundaryCondition, ExptDataInletBC, defaultParameters);


ExptDataInletBC::ExptDataInletBC
(
  OpenFOAMCase& c,
  const std::string& patchName,
  const OFDictData::dict& boundaryDict,
  ParameterSetInput ip
)
: BoundaryCondition(
          c, patchName, boundaryDict,
          ip.forward<Parameters>() )
{
 BCtype_="patch";
}

void ExptDataInletBC::addDataDict(OFdicts& dictionaries, const std::string& prefix, const std::string& fieldname, const arma::mat& data) const
{
  OFDictData::dictFile& Udict=dictionaries.lookupDict(prefix+"/0/"+fieldname);
  Udict.isSequential=true;

  if (OFversion()<170)
  {
      if (data.n_cols==1)
          Udict["a"]=0.0;
      else if (data.n_cols==3)
          Udict["a"]=OFDictData::vector3(vec3(0,0,0));
      else
          throw insight::Exception("Unhandled number of components: "+boost::lexical_cast<std::string>(data.n_cols));
  }
  else
  {
      Udict.no_header=true;
  }

  OFDictData::list vals;
  for (size_t r=0; r<data.n_rows; r++)
  {
    if (data.n_cols==1)
      vals.push_back(data(r));
    else if (data.n_cols==3)
      vals.push_back(OFDictData::vector3(data.row(r).t()));
  }
  Udict["b"]=vals;
}

void ExptDataInletBC::addIntoFieldDictionaries ( OFdicts& dictionaries ) const
{
    multiphaseBC::multiphaseBCPtr phasefractions =
        p().phasefractions;

    BoundaryCondition::addIntoFieldDictionaries ( dictionaries );
    phasefractions->addIntoDictionaries ( dictionaries );

    std::string prefix="constant/boundaryData/"+patchName_;

    size_t np=p().data.size();
    arma::mat ptdat = arma::zeros ( np, 3 );
    arma::mat velocity = arma::zeros ( np, 3 );
    arma::mat TKE = arma::zeros ( np );
    arma::mat epsilon = arma::zeros ( np );
    size_t j=0;
    for ( const Parameters::data_default_type& pt: p().data ) {
        ptdat.row ( j ) =pt.point.t();
        velocity.row ( j ) =pt.velocity.t();
        TKE ( j ) =pt.k;
        epsilon ( j ) =pt.epsilon;
        j++;
    }

    OFDictData::dictFile& ptsdict=dictionaries.lookupDict ( prefix+"/points" );
    if (OFversion()>=170)
    {
        ptsdict.no_header=true;
    }
    ptsdict.isSequential=true;
    OFDictData::list pts;
    for ( size_t r=0; r<ptdat.n_rows; r++ ) {
        pts.push_back ( OFDictData::vector3 ( ptdat.row ( r ).t() ) );
    }
    ptsdict["a"]=pts;


    for ( const FieldList::value_type& field: OFcase().fields() ) {
        OFDictData::dict& BC=dictionaries.addFieldIfNonexistent ( "0/"+field.first, field.second )
                             .subDict ( "boundaryField" ).subDict ( patchName_ );

        if ( ( field.first=="U" ) && ( get<0> ( field.second ) ==vectorField ) ) {
            BC["type"]=OFDictData::data ( "timeVaryingMappedFixedValue" );
            BC["offset"]=OFDictData::vector3 ( vec3 ( 0,0,0 ) );
            BC["setAverage"]=false;
            BC["perturb"]=1e-3;

//       OFDictData::dictFile& Udict=dictionaries.lookupDict(prefix+"/0/U");
//       Udict.isSequential=true;
//       Udict["a"]=OFDictData::vector3(vec3(0,0,0));
//
//       OFDictData::list vals;
//       const arma::mat& Udat=p_.velocity();
//       cout<<Udat<<endl;
//       for (int r=0; r<Udat.n_rows; r++)
// 	vals.push_back(OFDictData::vector3(Udat.row(r).t()));
//       Udict["b"]=vals;
            addDataDict ( dictionaries, prefix, "U", velocity );
        }

        else if (
            ( field.first=="p" ) && ( get<0> ( field.second ) ==scalarField )
        ) {
            BC["type"]=OFDictData::data ( "zeroGradient" );
        }
//     else if (
//       (field.first=="T")
//       &&
//       (get<0>(field.second)==scalarField)
//     )
//     {
//       BC["type"]=OFDictData::data("fixedValue");
//       BC["value"]="uniform "+lexical_cast<string>(p_.T());
//     }
        else if (isPrghPressureField(field)) {
            if ( OFversion() >=210 ) {
                BC["type"]=OFDictData::data ( "fixedFluxPressure" );
            } else {
                BC["type"]=OFDictData::data ( "buoyantPressure" );
            }
//       BC["type"]=OFDictData::data("calculated");
//       BC["value"]=OFDictData::data("uniform 0");
        }

//     else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
//     {
//       BC["type"]=OFDictData::data("fixedValue");
//       BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
//     }
        else if ( ( field.first=="k" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "timeVaryingMappedFixedValue" );
            BC["offset"]=0.0;
            BC["setAverage"]=false;
            BC["perturb"]=1e-3;
            addDataDict ( dictionaries, prefix, "k", TKE );
        } else if ( ( field.first=="omega" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "timeVaryingMappedFixedValue" );
            BC["offset"]=0.0;
            BC["setAverage"]=false;
            BC["perturb"]=1e-3;
            addDataDict ( dictionaries, prefix, "omega", epsilon/ ( 0.09*TKE ) );
        } else if ( ( field.first=="epsilon" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "timeVaryingMappedFixedValue" );
            BC["offset"]=0.0;
            BC["setAverage"]=false;
            BC["perturb"]=1e-3;
            addDataDict ( dictionaries, prefix, "epsilon", epsilon );
        } else if ( ( field.first=="nut" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            double nutilda=1e-10; //sqrt(1.5)*p_.turbulenceIntensity() * arma::norm(p_.velocity(), 2) * p_.mixingLength();
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]="uniform "+boost::lexical_cast<std::string> ( nutilda );
        }
//     else if ( (field.first=="nuTilda") && (get<0>(field.second)==scalarField) )
//     {
//       BC["type"]=OFDictData::data("timeVaryingMappedFixedValue");
//       BC["offset"]=0.0;
//       BC["setAverage"]=false;
// //       addDataDict(dictionaries, prefix, "nuTilda", p_.epsilon());
//     }
        else if ( ( field.first=="nuSgs" ) && ( get<0> ( field.second ) ==scalarField ) ) {
            BC["type"]=OFDictData::data ( "fixedValue" );
            BC["value"]="uniform 1e-10";
        } else {
            if ( ! (
                        MeshMotionBC::noMeshMotion.addIntoFieldDictionary ( field.first, field.second, BC )
                        ||
                        phasefractions->addIntoFieldDictionary ( field.first, field.second, BC )
                    ) ) {
                BC["type"]=OFDictData::data ( "zeroGradient" );
            }
            //throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
        }
    }
}


} // namespace insight
