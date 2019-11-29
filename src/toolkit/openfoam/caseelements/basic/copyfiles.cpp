#include "copyfiles.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


defineType(copyFiles);
addToOpenFOAMCaseElementFactoryTable(copyFiles);

copyFiles::copyFiles( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "", ps),
  p_(ps)
{
    name_="copyFiles";
}

namespace fs = boost::filesystem;

void copyDirectoryRecursively(const fs::path& sourceDir, const fs::path& destinationDir)
{
    if (!fs::exists(sourceDir) || !fs::is_directory(sourceDir))
    {
        throw std::runtime_error("Source directory " + sourceDir.string() + " does not exist or is not a directory");
    }
    if (fs::exists(destinationDir))
    {
        throw std::runtime_error("Destination directory " + destinationDir.string() + " already exists");
    }
    if (!fs::create_directory(destinationDir))
    {
        throw std::runtime_error("Cannot create destination directory " + destinationDir.string());
    }

    for (const auto& dirEnt : boost::make_iterator_range(fs::recursive_directory_iterator{sourceDir}, {}))
    {
        const auto& path = dirEnt.path();
        auto relativePathStr = path.string();
        boost::replace_first(relativePathStr, sourceDir.string(), "");
        fs::copy(path, destinationDir / relativePathStr);
    }
}

void copyFiles::addIntoDictionaries(OFdicts&) const
{
}

void copyFiles::modifyFilesOnDiskBeforeDictCreation ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const
{
  for (const auto& f: p_.files)
    {
      boost::filesystem::path src = f.source->filePath(location);
      boost::filesystem::path targ = location / f.target;

      if (!exists(src))
        throw insight::Exception("Source file/directory "+src.string()+" does not exist!");

      if (!exists(targ.parent_path()))
        create_directories(targ.parent_path());

      if (boost::filesystem::is_directory(src))
        copyDirectoryRecursively(src, targ);
      else
        boost::filesystem::copy_file(src, targ);

    }
}


} // namespace insight
