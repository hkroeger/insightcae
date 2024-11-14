#include "wallheatflux.h"

#include "openfoam/openfoamtools.h"
#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {

defineType(wallHeatFlux);
addToOpenFOAMCaseElementFactoryTable(wallHeatFlux);

wallHeatFlux::wallHeatFlux ( OpenFOAMCase& c, const ParameterSet& ps )
  : outputFilterFunctionObject(c, ps),
    p_(ps)
{
}

OFDictData::dict wallHeatFlux::functionObjectDict() const
{
  OFDictData::dict fod;

  fod["type"]="wallHeatFlux";
  fod["region"]=p_.region;
  fod["libs"]=OFDictData::list({ "\"libfieldFunctionObjects.so\"" });

  OFDictData::list p;
  std::copy(p_.patches.begin(), p_.patches.end(), std::back_inserter(p));
  fod["patches"]=p;

  if (const auto* qr = boost::get<Parameters::qr_field_type>(&p_.qr))
  {
    fod["qr"] = qr->fieldName;
  }

  return fod;
}


std::map<std::string,arma::mat> wallHeatFlux::readWallHeatFlux(
    const OpenFOAMCase& /*c*/,
    const boost::filesystem::path& location,
    const std::string& regionName,
    const std::string& foName )
{
  CurrentExceptionContext ex("reading output of wallHeatFlux function object "+foName+" in case \""+location.string()+"\"");

  using namespace boost;
  using namespace boost::filesystem;

  std::vector<std::vector<double> >  fl;

  path fp=absolute ( location ) /"postProcessing"/ regionName / foName;

  TimeDirectoryList tdl = listTimeDirectories ( fp );

  std::map<std::string,std::vector<arma::mat> > lineData;
  for ( const TimeDirectoryList::value_type& td: tdl )
  {
    path fname=( td.second/"wallHeatFlux.dat" );
    std::ifstream f(fname.string());
    std::string line;
    while (getline(f, line))
    {
      arma::mat t_mi_ma_int=arma::zeros(4);
      std::string patchName;
      std::istringstream is(line);
      is >> t_mi_ma_int(0) >> patchName >> t_mi_ma_int(1) >> t_mi_ma_int(2) >> t_mi_ma_int(3);
      if (!is.fail())
        lineData[patchName].push_back(t_mi_ma_int);
    }
  }

  std::map<std::string, arma::mat> result;
  for (auto i=lineData.begin(); i!=lineData.end(); ++i)
  {
    // sort by time
    std::sort(i->second.begin(), i->second.end(),
              [&](const arma::mat& l, const arma::mat& r)
              {
                return l(0)<r(0);
              }
    );
    arma::mat r = arma::zeros(i->second.size(), 4);
    for (arma::uword j=0; j<i->second.size(); ++j)
    {
      r.row(j)=i->second[j].t();
    }
    result[i->first]=r;
  }
  return result;
}



} // namespace insight
