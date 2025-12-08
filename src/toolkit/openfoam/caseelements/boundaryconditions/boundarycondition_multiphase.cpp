
#include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;

namespace insight
{

namespace multiphaseBC
{




defineType(multiphaseBC);
defineDynamicClass(multiphaseBC);

multiphaseBC::multiphaseBC(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{}

multiphaseBC::~multiphaseBC()
{}

void multiphaseBC::addIntoDictionaries(OFdicts&) const
{
}



defineType(uniformPhases);
addToFactoryTable(multiphaseBC, uniformPhases);
addToStaticFunctionTable(multiphaseBC, uniformPhases, defaultParameters);

uniformPhases::uniformPhases(ParameterSetInput ip)
    : multiphaseBC(ip.forward<Parameters>())
{}

bool uniformPhases::addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const
{
    const Parameters::phaseFractions_default_type* pf =NULL;
    for ( const auto& c: p().phaseFractions ) {
        if ( c.first == fieldname ) {
            pf=&c.second;
            break;
        }
    }
    if
    (
//     (f.find(fieldname)!=f.end())
        ( pf!=NULL )
        &&
        ( get<0> ( fieldinfo ) ==scalarField )
    ) {
        auto entry = OFDictData::toUniformField(pf->fraction);
        if (pf->handleflowreversal)
          {
            BC["type"]="inletOutlet";
            BC["inletValue"]=entry;
            BC["value"]=entry;
          } else
          {
            BC["type"]="fixedValue";
            BC["value"]=entry;
          }
        return true;
    } else {
        return false;
    }

}


uniformPhases::Parameters
uniformPhases::mixture( const std::map<std::string, double>& sps )
{
    Parameters pf;
    typedef std::map<std::string, double> MixList;
    for (const MixList::value_type& sp: sps)
    {
        Parameters::phaseFractions_default_type s;
        s.fraction=sp.second;
        pf.phaseFractions[sp.first]=s;
    }
    return pf;
}




defineType(uniformWallTiedPhases);
addToFactoryTable(multiphaseBC, uniformWallTiedPhases);
addToStaticFunctionTable(multiphaseBC, uniformWallTiedPhases, defaultParameters);

uniformWallTiedPhases::uniformWallTiedPhases(ParameterSetInput ip)
: uniformPhases(ip.forward<Parameters>())
{}

bool uniformWallTiedPhases::addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const
{
    const Parameters::phaseFractions_default_type* pf
        = nullptr;

    for ( const auto& c: p().phaseFractions )
    {
        if ( c.first == fieldname ) {
            pf=&c.second;
            break;
        }
    }

    if
    (
//     (f.find(fieldname)!=f.end())
        ( pf!=NULL )
        &&
        ( get<0> ( fieldinfo ) ==scalarField )
    ) {
        auto entry=OFDictData::toUniformField(pf->fraction);
//     BC["type"]="fixedValue";
//     BC["value"]=entry.str();
        BC["type"]="fixedValue";
        BC["value"]=entry;
        return true;
    } else {
        return false;
    }

}


}

}
