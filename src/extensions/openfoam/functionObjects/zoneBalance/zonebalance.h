#ifndef ZONEBALANCE_H
#define ZONEBALANCE_H


#include "functionObject.H"
#include "fvMesh.H"
#include "OFstream.H"

#include "uniof.h"
#include "uniof_helpers.h"
#include "boost/variant.hpp"

namespace Foam {


class zoneBalance
: public UniFunctionObject
{
public:

    struct cellSetSelection {
       word cellSetName;
    };

    struct ThresholdSelection {
        word thresholdScalarFieldName;
        scalar lowerThreshold, upperThreshold;
    };

    struct HighestValueVolumeFraction {
        word thresholdScalarFieldName;
        scalar volumeFraction;
    };

private:
    const fvMesh& mesh_;
    boost::variant<boost::blank,cellSetSelection,ThresholdSelection,HighestValueVolumeFraction> cellSelection_;
    wordList factorFields_;

    scalar balanceSum_;

    static constexpr char
        outfname[]{"zonebalance.dat"};

    FileOnDemand<outfname> outputFile_;


public:
    TypeName("zoneBalance");

    zoneBalance(
        const word& name,
        const objectRegistry& obr,
        const dictionary& dict );

    bool read(const dictionary&) override;
    bool perform() override;
    bool write() override;
};


}

#endif
