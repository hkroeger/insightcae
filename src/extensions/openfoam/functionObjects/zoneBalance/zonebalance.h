#ifndef ZONEBALANCE_H
#define ZONEBALANCE_H


#include "functionObject.H"
#include "fvMesh.H"
#include "OFstream.H"

#include "uniof.h"
#include "uniof_tools.h"
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

    struct AboveFractionOfMinimum {
        word thresholdScalarFieldName;
        scalar minimumFraction;
    };

private:
    const fvMesh& mesh_;

    boost::variant<
        boost::blank,
        cellSetSelection,
        ThresholdSelection,
        HighestValueVolumeFraction,
        AboveFractionOfMinimum> cellSelection_;

    word fluxFieldName_;
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
