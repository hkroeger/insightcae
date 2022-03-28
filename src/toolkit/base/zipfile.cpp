#include "zipfile.h"

#include "unzip.h"

#include "base/cppextensions.h"


namespace insight {


const char dir_delimter = '/';
const int MAX_FILENAME=1024;
const int READ_SIZE=8192;


struct ZipFileImpl
{
    unzFile handle;
    ZipFileImpl(const boost::filesystem::path& fp)
    {
        handle = unzOpen64( reinterpret_cast<const void*>(fp.c_str()) );
        if ( !handle )
        {
            throw std::logic_error( "Could not open ZIP file "+fp.string() );
        }
    };

    ~ZipFileImpl()
    {
        unzClose( handle );
    }
};


class CurrentArchiveFile
{
    ZipFileImpl& zipfile_;

    void readCurrentInfo()
    {
        char filenameBuffer[ MAX_FILENAME ];
        if ( unzGetCurrentFileInfo(
            zipfile_.handle,
            &fileInfo,
            filenameBuffer,
            MAX_FILENAME,
            NULL, 0, NULL, 0 ) != UNZ_OK )
        {
            throw std::logic_error( "could not read file info" );
        }
        fileName = filenameBuffer;
    }

public:
    unz_file_info fileInfo;
    boost::filesystem::path fileName;

    CurrentArchiveFile(ZipFileImpl& zipfile)
        : zipfile_(zipfile)
    {
        if (unzGoToFirstFile(zipfile_.handle)!=UNZ_OK)
        {
            throw std::logic_error("cannot seek to first file in archive");
        }
        readCurrentInfo();
    }

    ~CurrentArchiveFile()
    {
        unzCloseCurrentFile( zipfile_.handle );
    }

    bool gotoNextFile()
    {
        unzCloseCurrentFile( zipfile_.handle );

        auto r = unzGoToNextFile( zipfile_.handle );

        if (r == UNZ_OK)
        {
            readCurrentInfo();
            return true;
        }
        if (r == UNZ_END_OF_LIST_OF_FILE)
        {
            return false;
        }
        else
        {
            throw std::logic_error( "cound not read next file" );
            return false;
        }
    }

    bool isDirectory() const
    {
        auto fn=fileName.string();
        return fn[fn.size()-1]==dir_delimter;
    }
};


ZipFile::ZipFile(const boost::filesystem::path& zipFilePath)
    : zipfile_(std::make_shared<ZipFileImpl>(zipFilePath))
{}



std::set<boost::filesystem::path> ZipFile::files(bool includeDirectories) const
{
    std::set<boost::filesystem::path> files;

    // Loop to find all contained files
    CurrentArchiveFile cf(*zipfile_);
    do
    {
        if ( !cf.isDirectory() || includeDirectories )
        {
            files.insert(cf.fileName);
        }
    } while (cf.gotoNextFile());

    return files;
}

std::map<boost::filesystem::path, std::shared_ptr<std::string> >
ZipFile::uncompressFiles(
        const std::set<boost::filesystem::path>& filesToInclude ) const
{
    std::map<boost::filesystem::path, std::shared_ptr<std::string> > result;

    // Loop to find all contained files
    CurrentArchiveFile cf(*zipfile_);
    do
    {
        if (
                !cf.isDirectory()
                &&
                (
                    ( filesToInclude.find(cf.fileName)!=filesToInclude.end() )
                    ||
                    filesToInclude.empty()
                )
           )
        {
            if ( unzOpenCurrentFile( zipfile_->handle ) != UNZ_OK )
            {
                throw std::logic_error( "could not open file "+cf.fileName.string() );
            }

            auto readBuffer = std::make_shared<std::string>(cf.fileInfo.uncompressed_size, ' ');

            auto res = unzReadCurrentFile(
                        zipfile_->handle,
                        const_cast<void*>(static_cast<const void*>(readBuffer->data())),
                        cf.fileInfo.uncompressed_size );

            if ( res != cf.fileInfo.uncompressed_size )
            {
                throw std::logic_error("error in reading file "+cf.fileName.string());
            }

            result[cf.fileName]=readBuffer;
        }
    } while (cf.gotoNextFile());

    return result;
}

} // namespace insight
