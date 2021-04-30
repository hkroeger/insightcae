#include "ofes.h"
#include "base/tools.h"
#include "rapidxml/rapidxml.hpp"

#include <iostream>
#include <fstream>
#include <iterator>

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
  using namespace rapidxml;
  using namespace std;
  using namespace boost::filesystem;

  SharedPathList spaths;
  for ( const path& p: spaths )
  {
   if ( exists(p) && is_directory(p) )
   {
    path ofesdir = p / "ofes.d";
    if ( exists(ofesdir) && is_directory (ofesdir) )
     {
       for ( directory_iterator itr(ofesdir);
              itr != directory_iterator();
              ++itr )
       {
        if ( is_regular_file(itr->status()) )
        {
         if ( itr->path().extension() == ".ofe" )
         {
          path fn= itr->path();
          try
          {   
              std::string contents;
              std::ifstream in(fn.c_str());
              istreambuf_iterator<char> fbegin(in), fend;
              std::copy(fbegin, fend, back_inserter(contents));
              xml_document<> doc;
              doc.parse<0>(&contents[0]);

              xml_node<> *rootnode = doc.first_node("root");
              for (xml_node<> *e = rootnode->first_node("ofe"); e; e = e->next_sibling("ofe"))
              {
               std::string label(e->first_attribute("label")->value());
               std::string bashrc(e->first_attribute("bashrc")->value());
               std::string version(e->first_attribute("version")->value());

               (*this).insert(label, new OFEnvironment(boost::lexical_cast<int>(version), bashrc)); 
              }
          }
          catch (const std::exception& e)
          {
              insight::Warning("Failed to read OpenFOAM environments from file "+fn.string()+".\nReason: "+e.what());
          }
         }
        }
       }
     }
   }
  }

  char *envvar=getenv("INSIGHT_OFES");
  if (envvar!=nullptr)
  {
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

  if (this->size()==0)
  {
   insight::Warning("There are no OpenFOAM environments defined!");
  }
}




OFEs::~OFEs()
{
}



} // namespace insight
