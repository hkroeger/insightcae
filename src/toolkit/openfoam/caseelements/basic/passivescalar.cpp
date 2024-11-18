#include "passivescalar.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/basic/fvoption.h"

namespace insight {

defineType(PassiveScalar);
addToOpenFOAMCaseElementFactoryTable(PassiveScalar);

PassiveScalar::PassiveScalar( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "PassiveScalar", ps),
  OpenFOAMCase(c.ofe()),
  p_(ps)
{
}

void PassiveScalar::addFields( OpenFOAMCase& c ) const
{
  c.addField(p_.fieldname, 	FieldInfo(scalarField, 	dimless, 	FieldValue({p_.internal}), volField ) );
}


void PassiveScalar::addIntoDictionaries(OFdicts& dictionaries) const
{
    OFDictData::dict Fd;
    Fd["type"]="scalarTransport";
    Fd["field"]=p_.fieldname;
    Fd["resetOnStartUp"]=false;
    Fd["autoSchemes"]=false;

    OFDictData::dict fvo;
    auto opts = findElements<fvOption>();
    for (auto& o: opts)
    {
        o->addIntoCustomFvOptionDictionary(fvo, dictionaries);
    }
    Fd["fvOptions"]=fvo;

    OFDictData::list fol;
    fol.push_back("\"libutilityFunctionObjects.so\"");
    Fd["functionObjectLibs"]=fol;


    OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
    controlDict.subDict("functions")[p_.fieldname+"_transport"]=Fd;


    OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
    OFDictData::dict& divSchemes = fvSchemes.subDict("divSchemes");

    if (const auto* fo = boost::get<Parameters::scheme_firstorder_type>(&p_.scheme))
      {
        divSchemes["div(phi,"+p_.fieldname+")"]="Gauss upwind";
      }
    else if (const auto* so = boost::get<Parameters::scheme_secondorder_type>(&p_.scheme))
      {
        if (p_.underrelax < 1.)
          {
            OFDictData::dict& gradSchemes = fvSchemes.subDict("gradSchemes");

            divSchemes["div(phi,"+p_.fieldname+")"]="Gauss linearUpwind grad("+p_.fieldname+")";
            gradSchemes["grad(T)"]="cellLimited Gauss linear 1";
          }
        else
          {
            if (so->bounded01)
              divSchemes["div(phi,"+p_.fieldname+")"]="Gauss limitedLinear01 1";
            else
              divSchemes["div(phi,"+p_.fieldname+")"]="Gauss limitedLinear 1";
          }
      }


    OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
    OFDictData::dict& solvers=fvSolution.subDict("solvers");
    solvers[p_.fieldname]=OFcase().smoothSolverSetup(1e-6, 0.);

    OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
    if (OpenFOAMCaseElement::OFversion()<210)
    {
      relax[p_.fieldname]=p_.underrelax;
    }
    else
    {
      OFDictData::dict& eqnRelax=relax.subDict("equations");
      eqnRelax[p_.fieldname]=p_.underrelax;
    }
}


} // namespace insight
