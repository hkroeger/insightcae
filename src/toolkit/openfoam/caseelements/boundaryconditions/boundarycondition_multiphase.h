#ifndef BOUNDARYCONDITION_MULTIPHASE_H
#define BOUNDARYCONDITION_MULTIPHASE_H


#include "base/boost_include.h"
#include "base/parameterset.h"
#include "base/resultset.h"
#include "openfoam/openfoamcase.h"

#include "boundarycondition_multiphase__uniformPhases__Parameters_headers.h"


namespace insight
{


namespace multiphaseBC
{



class multiphaseBC
{
public:
#include "boundarycondition_multiphase__multiphaseBC__Parameters.h"
/*
PARAMETERSET>>> multiphaseBC Parameters
createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "multiphaseBC" );
    declareDynamicClass(multiphaseBC);

    multiphaseBC(ParameterSetInput ip = Parameters() );
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
inherits multiphaseBC::Parameters

phaseFractions = labeledarray "phase%d" [
    set {
    fraction = double 0.5 "Mass fraction of specie"
    handleflowreversal = bool true
"By default, the BC turns into a Neumann boundary condition, if outflow occurs on this boundary for any reason.

If a dirichlet BC shall remain under all circumstances, uncheck this option. Then a fixedValue-BC will be used instead of inletOutlet."

} ] *0 "Mass fractions of species"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "uniformPhases" );
    uniformPhases ( ParameterSetInput ip = Parameters() );
    bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const override;

    inline static multiphaseBCPtr
    create(const ParameterSet& ps)
    { return std::make_shared<uniformPhases>(ps); }

    static Parameters
    mixture( const std::map<std::string, double>& sp);
};




class uniformWallTiedPhases
: public uniformPhases
{
public:
    declareType ( "uniformWallTiedPhases" );
    uniformWallTiedPhases ( ParameterSetInput ip = Parameters() );

    inline static multiphaseBCPtr
    create(const ParameterSet& ps)
    {
        return multiphaseBCPtr(new uniformWallTiedPhases(ps));
    }

    bool addIntoFieldDictionary ( const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC ) const override;
};

}

}


#endif // BOUNDARYCONDITION_MULTIPHASE_H
