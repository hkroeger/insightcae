#include "zonebalance.h"
#include "openfoam/ofdicts.h"

namespace insight {

namespace OutputSectionReaders {

defineType(zoneBalance);
addToStaticFunctionTable2(
    OutputSectionReader,
    Readers, createIfMatches,
    zoneBalance, &zoneBalance::createIfMatches);


boost::regex zoneBalance_intro("^(.*): balance of (phi*.*) over (.*) cells \\(V=(.*)\\) = (.*)$");




zoneBalance::zoneBalance(const boost::smatch& g)
  : label_(g[1]),
    varDesc_(g[2]),
    ncells_(toNumber<int>(g[3])),
    Vol_(toNumber<double>(g[4])),
    balance_(toNumber<double>(g[5]))
{}




std::unique_ptr<OutputSectionReader> zoneBalance::createIfMatches(
    const std::string& line )
{
    boost::smatch match;
    if (boost::regex_search( line, match, zoneBalance_intro, boost::match_default ) )
    {
        return std::unique_ptr<OutputSectionReader>(new zoneBalance(match));
    }
    else
        return nullptr;
}




void zoneBalance::addProgressVariables(std::map<std::string, double>& pv) const
{
    pv["zoneBalance_"+varDesc_+"_Vol/"+label_]=Vol_;
    pv["zoneBalance_"+varDesc_+"_ncells/"+label_]=ncells_;
    pv["zoneBalance_"+varDesc_+"/"+label_]=balance_;
}


}


defineType(zoneBalance);
addToOpenFOAMCaseElementFactoryTable(zoneBalance);

zoneBalance::zoneBalance(OpenFOAMCase &c, ParameterSetInput ip)
    : functionObject(c, ip.forward<Parameters>())
{}

OFDictData::dict zoneBalance::functionObjectDict() const
{
    OFDictData::dict fod;
    fod["type"]="zoneBalance";

    if (boost::get<Parameters::cellSelection_all_type>(&p().cellSelection))
    {
        fod["cellSelection"]="all";
    }
    else if (auto* hv=boost::get<Parameters::cellSelection_highestValueVolumeFraction_type>(&p().cellSelection))
    {
        fod["cellSelection"]="highestValueVolumeFraction";
        fod["thresholdScalarFieldName"]=hv->thresholdScalarFieldName;
        fod["volumeFraction"]=hv->volumeFraction;
    }
    else if (auto* set=boost::get<Parameters::cellSelection_cellSet_type>(&p().cellSelection))
    {
        fod["cellSelection"]="cellSet";
        fod["cellSet"]=set->name;
    }
    else if (auto* thr=boost::get<Parameters::cellSelection_threshold_type>(&p().cellSelection))
    {
        fod["cellSelection"]="threshold";
        fod["thresholdScalarFieldName"]=thr->thresholdScalarFieldName;
        fod["lowerThreshold"]=thr->lowerThreshold;
        fod["upperThreshold"]=thr->upperThreshold;
    }
    fod["factorFields"]=OFDictData::list(p().factorFields);
    return fod;
}

std::set<std::string> zoneBalance::requiredLibraries() const
{
    auto rl=functionObject::requiredLibraries();
    rl.insert("libzoneBalance.so");
    return rl;
}


arma::mat zoneBalance::readBalance(
    const OpenFOAMCase& c,
    const boost::filesystem::path& location,
    const std::string& foName
)
{
    return readAndCombineTabularFiles
        (
            c, location,
            foName, "zonebalance.dat",
            "()"
            );
}



} // namespace insight
