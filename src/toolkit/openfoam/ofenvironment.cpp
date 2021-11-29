#include "ofenvironment.h"

namespace insight {




OFEnvironment::OFEnvironment(int version, const boost::filesystem::path& bashrc)
: version_(version),
  bashrc_(bashrc)
{
}




int OFEnvironment::version() const
{
  return version_;
}




const boost::filesystem::path& OFEnvironment::bashrc() const
{
  return bashrc_;
}




} // namespace insight
