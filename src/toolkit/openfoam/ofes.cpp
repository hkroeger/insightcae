#include "ofes.h"

namespace insight {




OFEs OFEs::list;




std::vector<std::string> OFEs::all()
{
    std::vector<std::string> entries;
    for (value_type vr: OFEs::list)
    {
//         std::cout<<vr.first<<std::endl;
        entries.push_back(vr.first);
    }
    return entries;
}




const OFEnvironment& OFEs::get(const std::string& name)
{
  const_iterator it=list.find(name);
  if (it==list.end())
    throw insight::Exception
    (
      "OFEs::get(): Requested OpenFOAM environment "+name+" is undefined.\n"
      "(Check environment variable INSIGHT_OFES)"
    );

  return *(it->second);
}




std::string OFEs::detectCurrentOFE()
{
  const char *envvar=getenv("CURRENT_OFE");
  if (!envvar)
  {
    throw insight::Exception("Environment variable CURRENT_OFE not set. Check, if OpenFOAM environment is loaded.");
  }
  for (OFEs::value_type ofe: list)
  {
    if (ofe.first==envvar)
    {
      return ofe.first;
    }
  }

  throw insight::Exception("Environment \""+std::string(envvar)+"\" is unknown, i.e. not contained in INSIGHT_OFES. Please check validity of installation.");
}




std::string OFEs::currentOrPreferredOFE()
{
  try {
    return detectCurrentOFE();
  }
  catch (const std::exception& /*e*/)
  {
    return "OFesi1806";
  }
}




const OFEnvironment& OFEs::getCurrent()
{
  return get( detectCurrentOFE() );
}

const OFEnvironment& OFEs::getCurrentOrPreferred()
{
  return get( currentOrPreferredOFE() );
}




OFEs::OFEs()
{
  const char *envvar=getenv("INSIGHT_OFES");
  if (!envvar)
  {
    std::cout<<"Warning: No OpenFOAM installations defined! (environment variable INSIGHT_OFES)"<<std::endl;
    return;
  }
  std::string cfgvar(envvar);
  std::vector<std::string> ofestrs;
  boost::split(ofestrs, cfgvar, boost::is_any_of(":"));
  for (const std::string& ofe: ofestrs)
  {
    std::vector<std::string> strs;
    boost::split(strs, ofe, boost::is_any_of("@#"));
    if (strs.size()==3)
    {
        try
        {
            (*this).insert(strs[0], new OFEnvironment(boost::lexical_cast<int>(strs[2]), strs[1]));
        }
        catch (...)
        {
            insight::Warning("Error while parsing INSIGHT_OFES environment variable: OFE specification is invalid: \""+ofe+"\"");
        }
    }
  }
}




OFEs::~OFEs()
{
}



} // namespace insight
