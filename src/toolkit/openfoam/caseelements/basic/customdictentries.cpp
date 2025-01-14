#include "customdictentries.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


defineType(customDictEntries);
addToOpenFOAMCaseElementFactoryTable(customDictEntries);

customDictEntries::customDictEntries( OpenFOAMCase& c, ParameterSetInput ip )
: OpenFOAMCaseElement(c, ip.forward<Parameters>())
{
    // name_="customDictEntries";
}

OFDictData::dict& getOrCreateSubDict(OFDictData::dict& d, std::string path)
{
  if (path.empty())
    {
      return d;
    }
  else
    {
      auto i = path.find('/');
      if (i==std::string::npos)
        {
          return d.subDict(path);
        }
      else
        {
          std::string k = path.substr(0, i);
          std::string p = path.substr(i+1);
          return getOrCreateSubDict( d.subDict(k), p );
        }
    }
}

void customDictEntries::addIntoDictionaries(OFdicts& dictionaries) const
{
    for (const auto& e: p().entries)
    {
      OFDictData::dict& dict
        = dictionaries.lookupDict(e.dict);

      std::string path, key;
      auto i = e.path.rfind('/');
      if (i==std::string::npos)
        {
          path="";
          key=e.path;
        }
      else
        {
          path = e.path.substr(0, i);
          key = e.path.substr(i+1);
        }

      OFDictData::dict& parent = getOrCreateSubDict(dict, path);
      parent[key]=e.value;
    }

    for (const auto& e: p().appendList)
    {
      OFDictData::dict& dict
        = dictionaries.lookupDict(e.dict);

      std::string path, key;
      auto i = e.path.rfind('/');
      if (i==std::string::npos)
        {
          path="";
          key=e.path;
        }
      else
        {
          path = e.path.substr(0, i);
          key = e.path.substr(i+1);
        }

      OFDictData::dict& parent = getOrCreateSubDict(dict, path);
      parent.getList(key).push_back(e.value);
    }


}



} // namespace insight
