#ifndef INSIGHT_CHTMULTIREGIONNUMERICS_H
#define INSIGHT_CHTMULTIREGIONNUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_heat.h"
#include "openfoam/caseelements/numerics/pimplesettings.h"

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

 unsteady set {
    time_integration = includedset "insight::CompressiblePIMPLESettings::Parameters" "Settings for time integration"
      modifyDefaults {
         selectablesubset timestep_control = adjust;
         double timestep_control/maxCo = 5.0;
      }
 }
 steady set { }

}} steady "Handling of the time"

fluidRegions = array [ string "" "Name of cell zone" ] *0 "Names of the cell zones comprising the different fluid regions. One region per cell zone is defined."
solidRegions = array [ string "" "Name of cell zone" ] *0 "Names of the cell zones comprising the different solid regions. One region per cell zone is defined."

createGetters
<<<PARAMETERSET
*/

protected:
    void init();

public:
    declareType ( "chtMultiRegionNumerics" );
    chtMultiRegionNumerics ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;

    static bool insertCoupledWall
    (
        OpenFOAMCase& chc,
        const OFDictData::dict& boundaryDict,
        const std::string& patchName,
        const std::string& otherPatchName,
        const std::string& otherZoneName,
        double Tinitial,
        HeatBC::CHTCoupledWall::Parameters::offset_type offset
            = HeatBC::CHTCoupledWall::Parameters::offset_none_type());

    const Parameters::formulation_unsteady_type* unsteadyFormulation() const;
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

createGetters
<<<PARAMETERSET
*/

protected:
    void init();

public:
    declareType ( "chtMultiRegionFluidNumerics" );
    chtMultiRegionFluidNumerics ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};





class chtMultiRegionSolidNumerics
    : public FVNumerics
{

public:
#include "chtmultiregionnumerics__chtMultiRegionSolidNumerics__Parameters.h"

/*
PARAMETERSET>>> chtMultiRegionSolidNumerics Parameters
inherits FVNumerics::Parameters

nNonOrthogonalCorrectors = int 0 "Number of non-orthogonality correctors"

Tinternal = double 300.0 "Internal temperature field value"

createGetters
<<<PARAMETERSET
*/

protected:
    void init();

public:
    declareType ( "chtMultiRegionSolidNumerics" );
    chtMultiRegionSolidNumerics ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    std::string lqGradSchemeIfPossible() const override;
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};



} // namespace insight

#endif // INSIGHT_CHTMULTIREGIONNUMERICS_H
