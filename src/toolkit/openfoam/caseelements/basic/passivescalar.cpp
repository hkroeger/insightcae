#include "passivescalar.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/basic/fvoption.h"

namespace insight {

defineType(PassiveScalar);
addToOpenFOAMCaseElementFactoryTable(PassiveScalar);

PassiveScalar::PassiveScalar( OpenFOAMCase& c, ParameterSetInput ip )
: OpenFOAMCaseElement(c, ip.forward<Parameters>()),
  OpenFOAMCase(c.ofe())
{
}

void PassiveScalar::addFields( OpenFOAMCase& c ) const
{
  c.addField(
        p().fieldname,
        FieldInfo(scalarField, 	dimless, 	FieldValue({p().internal}), volField ) );
}


void PassiveScalar::addIntoDictionaries(OFdicts& dictionaries) const
{
    OFDictData::dict Fd;
    Fd["type"]="scalarTransport";
    Fd["field"]=p().fieldname;
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
    controlDict.subDict("functions")[p().fieldname+"_transport"]=Fd;


    OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
    OFDictData::dict& divSchemes = fvSchemes.subDict("divSchemes");

    if (const auto* fo = boost::get<Parameters::scheme_firstorder_type>(&p().scheme))
      {
        divSchemes["div(phi,"+p().fieldname+")"]="Gauss upwind";
      }
    else if (const auto* so = boost::get<Parameters::scheme_secondorder_type>(&p().scheme))
      {
        if (p().underrelax < 1.)
          {
            OFDictData::dict& gradSchemes = fvSchemes.subDict("gradSchemes");

            divSchemes["div(phi,"+p().fieldname+")"]="Gauss linearUpwind grad("+p().fieldname+")";
            gradSchemes["grad(T)"]="cellLimited Gauss linear 1";
          }
        else
          {
            if (so->bounded01)
              divSchemes["div(phi,"+p().fieldname+")"]="Gauss limitedLinear01 1";
            else
              divSchemes["div(phi,"+p().fieldname+")"]="Gauss limitedLinear 1";
          }
      }


    OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
    OFDictData::dict& solvers=fvSolution.subDict("solvers");
    solvers[p().fieldname]=OFcase().smoothSolverSetup(1e-6, 0.);

    OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
    if (OpenFOAMCaseElement::OFversion()<210)
    {
      relax[p().fieldname]=p().underrelax;
    }
    else
    {
      OFDictData::dict& eqnRelax=relax.subDict("equations");
      eqnRelax[p().fieldname]=p().underrelax;
    }
}


} // namespace insight
