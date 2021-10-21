#ifndef INSIGHT_CHTMULTIREGIONNUMERICS_H
#define INSIGHT_CHTMULTIREGIONNUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"
#include "chtmultiregionnumerics__chtMultiRegionNumerics__Parameters_headers.h"

namespace insight {

class chtMultiRegionNumerics
    : public FVNumerics
{

public:
#include "chtmultiregionnumerics__chtMultiRegionNumerics__Parameters.h"

/*
PARAMETERSET>>> chtMultiRegionNumerics Parameters
inherits FVNumerics::Parameters

formulation = selectablesubset {{
 unsteady set { }
 steady set { }
}} steady "Handling of the time"

fluidRegions = array [ string "" "Name of cell zone" ] *0 "Names of the cell zones comprising the different fluid regions. One region per cell zone is defined."
solidRegions = array [ string "" "Name of cell zone" ] *0 "Names of the cell zones comprising the different solid regions. One region per cell zone is defined."

<<<PARAMETERSET
*/

protected:
    Parameters p_;

    void init();

public:
    declareType ( "chtMultiRegionNumerics" );
    chtMultiRegionNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};




class chtMultiRegionFluidNumerics
    : public FVNumerics
{

public:
#include "chtmultiregionnumerics__chtMultiRegionFluidNumerics__Parameters.h"

/*
PARAMETERSET>>> chtMultiRegionFluidNumerics Parameters
inherits FVNumerics::Parameters

nNonOrthogonalCorrectors = int 0 "Number of non-orthogonality correctors"
frozenFlow = bool false "Switch off flow solution"

rhoMin = double 0.1 "[kg/m^3] Minimum density"
rhoMax = double 10 "[kg/m^3] Maximum density"

pinternal = double 0.0 "Internal pressure field value"
Tinternal = double 300.0 "Internal temperature field value"
Uinternal = vector (0 0 0) "Internal velocity field value"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

    void init();

public:
    declareType ( "chtMultiRegionFluidNumerics" );
    chtMultiRegionFluidNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};

} // namespace insight

#endif // INSIGHT_CHTMULTIREGIONNUMERICS_H
