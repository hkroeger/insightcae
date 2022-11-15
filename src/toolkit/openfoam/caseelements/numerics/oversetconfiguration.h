#ifndef INSIGHT_OVERSETCONFIGURATION_H
#define INSIGHT_OVERSETCONFIGURATION_H

#include "openfoam/openfoamcase.h"
#include "openfoam/ofdicts.h"

#include "oversetconfiguration__OversetConfiguration__Parameters_headers.h"

namespace insight {

class OversetConfiguration
{
public:
#include "oversetconfiguration__OversetConfiguration__Parameters.h"
/*
PARAMETERSET>>> OversetConfiguration Parameters

oversetInterpolation = selection (cellVolumeWeight inverseDistance trackingInverseDistance leastSquares) cellVolumeWeight "type of overset interpolation"

oversetAdjustPhi = bool false ""

skipPoissonWallDist = bool false "For overset parallel runs, a inter-zone method of wall distance determination is required. Set this to true to skip it and stick with the incorrect but stable default method."

<<<PARAMETERSET
*/

protected:
    OpenFOAMCase& cm_;
    Parameters p_;

public:
    OversetConfiguration(
            OpenFOAMCase& c,
            const ParameterSet& ps = Parameters::makeDefault() );

    void addFields() const;

    void addIntoDictionaries (
            OFdicts& dictionaries,
            const std::string& pName,
            const std::string& fluxName ) const;
};

} // namespace insight

#endif // INSIGHT_OVERSETCONFIGURATION_H
