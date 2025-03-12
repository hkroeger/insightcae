#include "copyfiles.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

#include "base/tools.h"

namespace insight {


defineType(copyFiles);
addToOpenFOAMCaseElementFactoryTable(copyFiles);

copyFiles::copyFiles( OpenFOAMCase& c, ParameterSetInput ip )
: OpenFOAMCaseElement(c, ip.forward<Parameters>())
{
    // name_="copyFiles";
}


void copyFiles::addIntoDictionaries(OFdicts&) const
{
}

void copyFiles::modifyFilesOnDiskBeforeDictCreation ( const OpenFOAMCase& /*cm*/, const boost::filesystem::path& location ) const
{
  for (const auto& f: p().files)
  {
    boost::filesystem::path src = f.source->filePath(location);
    boost::filesystem::path targ = location / static_cast<const std::string&>(f.target);

    if (!exists(src))
      throw insight::Exception("Source file/directory "+src.string()+" does not exist!");

    if (!exists(targ.parent_path()))
      create_directories(targ.parent_path());

    if (boost::filesystem::is_directory(src))
      copyDirectoryRecursively(src, targ);
    else
      boost::filesystem::copy_file(src, targ, boost::filesystem::copy_option::overwrite_if_exists);

  }
}


} // namespace insight
