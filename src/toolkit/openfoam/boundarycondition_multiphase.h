#ifndef BOUNDARYCONDITION_MULTIPHASE_H
#define BOUNDARYCONDITION_MULTIPHASE_H


#include "base/boost_include.h"
#include "base/parameterset.h"
#include "base/resultset.h"
#include "openfoam/openfoamcase.h"


namespace insight
{


namespace multiphaseBC
{



class multiphaseBC
{
public:
    declareType ( "multiphaseBC" );
    declareDynamicClass(multiphaseBC);

    virtual ~multiphaseBC();


    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    // return true, if this field was handled, false otherwise
    virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const =0;
};

typedef std::shared_ptr<multiphaseBC> multiphaseBCPtr;




class uniformPhases
    : public multiphaseBC
{
public:
#include "boundarycondition_multiphase__uniformPhases__Parameters.h"
/*
PARAMETERSET>>> uniformPhases Parameters

phaseFractions = array [
    set {
    name = string "CO2" "Name of specie"
    fraction = double 0.5 "Mass fraction of specie"
    handleflowreversal = bool true
"By default, the BC turns into a Neumann boundary condition, if outflow occurs on this boundary for any reason.

If a dirichlet BC shall remain under all circumstances, uncheck this option. Then a fixedValue-BC will be used instead of inletOutlet."

} ] *0 "Mass fractions of species"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "uniformPhases" );
    uniformPhases ( const ParameterSet& p );
    inline static multiphaseBCPtr create(const ParameterSet& ps) { return multiphaseBCPtr(new uniformPhases(ps)); }
    virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const;
    static Parameters mixture( const std::map<std::string, double>& sp);
    static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
    virtual ParameterSet getParameters() const { return p_; }
};




class uniformWallTiedPhases
: public uniformPhases
{
public:
    declareType ( "uniformWallTiedPhases" );
    uniformWallTiedPhases ( const ParameterSet& p );
    inline static multiphaseBCPtr create(const ParameterSet& ps) { return multiphaseBCPtr(new uniformWallTiedPhases(ps)); }
    virtual bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const;
    static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
};

}

}


#endif // BOUNDARYCONDITION_MULTIPHASE_H
