#ifndef INSIGHT_FVNUMERICS_H
#define INSIGHT_FVNUMERICS_H


#include "openfoam/caseelements/basic/decomposepardict.h"


namespace insight {



class OpenFOAMCase;
class OFdicts;


/**
 Manages basic settings in controlDict, fvSchemes, fvSolution, list of fields
 */

class FVNumerics
    : public decomposeParDict
{
public:
    declareType ( "FVNumerics" );

public:
#include "fvnumerics__FVNumerics__Parameters.h"

/*
PARAMETERSET>>> FVNumerics Parameters

inherits decomposeParDict::Parameters

writeControl = selection (
    adjustableRunTime
    clockTime
    cpuTime
    runTime
    timeStep
) timeStep "Type of write control"

writeInterval = double 100.0 "Write interval"

writeFormat = selection ( ascii binary ) ascii "Write format"

purgeWrite = int 10 "Purge write interval, set to 0 to disable"

deltaT = double 1.0 "Time step size. If the time step is selected to be adjustable, this is the initial time step size."

endTime = double 1000.0 "Maximum end time of simulation"


mapFieldsConfig = selectablesubset {{
 none set {}
 map set {
  patchMap = array [ set {
     targetPatch = string "lid" "Name of patch in target mesh"
     sourcePatch = string "movingWall" "Name of patch in source mesh"
  }] *0 "Pairs of patches for mapping"

  cuttingPatches = array [
    string "fixedWalls" "Name of patch in target mesh"
  ] *0 "Patches whose values shall be interpolated from source interior"
 }
}} map "Mapfield configuration"

<<<PARAMETERSET
*/

protected:
    Parameters p_;
    std::string pName_;

public:
    FVNumerics ( OpenFOAMCase& c, const ParameterSet& ps, const std::string& pName );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    virtual bool isCompressible() const =0;
    virtual bool isLES() const;
    virtual bool isGAMGOk() const;

    void setApplicationName(OFdicts& dictionaries, const std::string& appname) const;
    void setRelaxationFactors
    (
        OFdicts& dictionaries,
        const std::map<std::string, double>& eqnRelax,
        const std::map<std::string, double>& fieldRelax
    ) const;

    std::string lqGradSchemeIfPossible() const;
    void insertStandardGradientConfig(OFdicts& dictionaries) const;

    std::string gradNameOrScheme(OFdicts& dictionaries, const std::string& key) const;

    static std::string category() { return "Numerics"; }

    virtual bool isUnique() const;
};

} // namespace insight

#endif // INSIGHT_FVNUMERICS_H
