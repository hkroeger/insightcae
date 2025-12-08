#include "base/filestorageinfo.h"

namespace insight {


FileStorageInfo::FileStorageInfo()
{}


FileStorageInfo::FileStorageInfo(
    const boost::filesystem::path& parentDirectory)
    : boost::optional<boost::filesystem::path>(parentDirectory)
{}


FileStorageInfo::FileStorageInfo(
    const boost::filesystem::path& parentDirectory,
    boost::optional<boost::filesystem::path> additionalFilesDirectory )
    : boost::optional<boost::filesystem::path>(parentDirectory)
{
    if (additionalFilesDirectory)
    {
        additionalFiles = AdditionalFiles{
            boost::filesystem::absolute(*additionalFilesDirectory),
            boost::filesystem::make_relative(
                parentDirectory, *additionalFilesDirectory )
        };
    }
}




} // namespace insight
