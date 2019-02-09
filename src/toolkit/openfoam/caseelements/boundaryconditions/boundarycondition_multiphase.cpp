
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

multiphaseBC::~multiphaseBC()
{
}

void multiphaseBC::addIntoDictionaries(OFdicts&) const
{
}




defineType(uniformPhases);
addToFactoryTable(multiphaseBC, uniformPhases);
addToStaticFunctionTable(multiphaseBC, uniformPhases, defaultParameters);

uniformPhases::uniformPhases(const ParameterSet& ps)
: p_(ps)
{}

bool uniformPhases::addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const
{
    const Parameters::phaseFractions_default_type* pf =NULL;
    for ( const Parameters::phaseFractions_default_type& c: p_.phaseFractions ) {
        if ( c.name == fieldname ) {
            pf=&c;
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
        std::ostringstream entry;
        entry << "uniform " << pf->fraction;
        if (pf->handleflowreversal)
          {
            BC["type"]="inletOutlet";
            BC["inletValue"]=entry.str();
            BC["value"]=entry.str();
          } else
          {
            BC["type"]="fixedValue";
            BC["value"]=entry.str();
          }
        return true;
    } else {
        return false;
    }

}


uniformPhases::Parameters uniformPhases::mixture( const std::map<std::string, double>& sps )
{
    Parameters pf;
    typedef std::map<std::string, double> MixList;
    for (const MixList::value_type& sp: sps)
    {
        Parameters::phaseFractions_default_type s;
        s.name=sp.first;
        s.fraction=sp.second;
        pf.phaseFractions.push_back(s);
    }
    return pf;
}




defineType(uniformWallTiedPhases);
addToFactoryTable(multiphaseBC, uniformWallTiedPhases);
addToStaticFunctionTable(multiphaseBC, uniformWallTiedPhases, defaultParameters);

uniformWallTiedPhases::uniformWallTiedPhases(const ParameterSet& ps)
: uniformPhases(ps)
{}

bool uniformWallTiedPhases::addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const
{
    const Parameters::phaseFractions_default_type* pf =NULL;
    for ( const Parameters::phaseFractions_default_type& c: p_.phaseFractions ) {
        if ( c.name == fieldname ) {
            pf=&c;
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
        std::ostringstream entry;
        entry << "uniform " << pf->fraction;
//     BC["type"]="fixedValue";
//     BC["value"]=entry.str();
        BC["type"]="fixedValue";
        BC["value"]=entry.str();
        return true;
    } else {
        return false;
    }

}


}

}
