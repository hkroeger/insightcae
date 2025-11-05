#ifndef FILESTORAGEINFO_H
#define FILESTORAGEINFO_H

#include "base/boost_include.h"

namespace insight {


struct FileStorageInfo
    : public boost::optional<boost::filesystem::path>
{
    struct AdditionalFiles
    {
        /**
             * @brief additionalFilesDirectory
             * absolute path to put the additional files into
             */
        boost::filesystem::path directory;

        /**
             * @brief additionalFilesDirectoryRelativePath
             * relative path to use inside document
             */
        boost::filesystem::path directoryRelativePath;
    };

    boost::optional<AdditionalFiles> additionalFiles;

    FileStorageInfo();

    FileStorageInfo(
        const boost::filesystem::path& parentDirectory) ;

    FileStorageInfo(
        const boost::filesystem::path& parentDirectory,
        boost::optional<boost::filesystem::path> additionalFilesDirectory =
        boost::optional<boost::filesystem::path>() );
};

} // namespace insight

#endif // FILESTORAGEINFO_H
