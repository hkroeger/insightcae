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
    insight::assertion(
        !bashrc_.empty(),
        "requested OpenFOAM environment is not installed"
        );
  return bashrc_;
}




} // namespace insight
