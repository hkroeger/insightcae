#include "ofdicts.h"



namespace insight {



boost::filesystem::path OFdicts::insertAdditionalInputFile(std::shared_ptr<PathParameter> file)
{
  throw insight::Exception("not implemented");
}

OFDictData::dictFile& OFdicts::addFieldIfNonexistent(const std::string& key, const FieldInfo& fi)
{
  OFDictData::dictFile& d=lookupDict(key);
  std::string className;
  if (boost::fusion::get<3>(fi) == volField )
    className="vol";
  else if (boost::fusion::get<3>(fi) == pointField )
    className="point";
  else if (boost::fusion::get<3>(fi) == tetField )
    className="tetPoint";
  else
    throw insight::Exception("Don't know the geotype of field "+boost::lexical_cast<std::string>(boost::fusion::get<3>(fi)));

  if (boost::fusion::get<0>(fi)==scalarField)
    className+="ScalarField";
  else if (boost::fusion::get<0>(fi)==vectorField)
    className+="VectorField";
  else if (boost::fusion::get<0>(fi)==symmTensorField)
    className+="SymmTensorField";
  else
    throw insight::Exception("Don't know the class name of field type "+boost::lexical_cast<std::string>(boost::fusion::get<0>(fi)));

  d.className=className;

  return d;
}




OFDictData::dictFile& OFdicts::lookupDict(const std::string& key, bool createIfNonexistent)
{
  OFdicts::iterator i=find(key);

  if (createIfNonexistent)
  {
    if ( i==end() ) {
        ( *this ) [key]=OFDictData::dictFile();
    }
    return ( *this ) [key];
  }
  else
  {
    if (i==end())
    {
      throw Exception("Dictionary "+key+" not found!");
    }
    return *(i->second);
  }
}




} // namespace insight
