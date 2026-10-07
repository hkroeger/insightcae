#include "zipfile.h"

#include "unzip.h"
#include "zip.h"

#include "base/cppextensions.h"
#include "base/exception.h"

#include <fstream>


namespace insight {


const char dir_delimter = '/';
const int MAX_FILENAME=1024;
const int READ_SIZE=8192;


struct ZipFileImpl
{
    unzFile handle;
    ZipFileImpl(const boost::filesystem::path& fp)
    {
        handle = unzOpen64( reinterpret_cast<const void*>(
                                fp.string().c_str()
                                ) );
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


void ZipFile::uncompressTo(const boost::filesystem::path& targetDirectory) const
{
    std::vector<char> buf(READ_SIZE);

    // reject entries which would be written outside the target directory
    for (const auto& f: files(true))
    {
        bool unsafe = f.has_root_path();
        for (const auto& c: f)
        {
            if (c=="..") unsafe=true;
        }
        if (unsafe)
        {
            throw insight::Exception(
                "refusing to extract archive: entry %s points outside the target directory",
                f.string().c_str() );
        }
    }

    CurrentArchiveFile cf(*zipfile_);
    do
    {
        auto target = targetDirectory / cf.fileName;

        if (cf.isDirectory())
        {
            boost::filesystem::create_directories(target);
        }
        else
        {
            if (target.has_parent_path())
                boost::filesystem::create_directories(target.parent_path());

            if ( unzOpenCurrentFile( zipfile_->handle ) != UNZ_OK )
            {
                throw insight::Exception(
                    "could not open file %s in archive",
                    cf.fileName.string().c_str() );
            }

            std::ofstream f(target.string(), std::ios::binary);
            if (!f.good())
            {
                throw insight::Exception(
                    "could not create file %s",
                    target.string().c_str() );
            }

            int n;
            while ( (n = unzReadCurrentFile(zipfile_->handle, buf.data(), buf.size())) > 0 )
            {
                f.write(buf.data(), n);
            }
            if (n<0)
            {
                throw insight::Exception(
                    "error in reading file %s from archive",
                    cf.fileName.string().c_str() );
            }
        }
    } while (cf.gotoNextFile());
}




void writeZipFile(
    const boost::filesystem::path& zipFilePath,
    const std::map<boost::filesystem::path, boost::filesystem::path>& entries )
{
    zipFile zf = zipOpen64(zipFilePath.string().c_str(), APPEND_STATUS_CREATE);
    if (!zf)
    {
        throw insight::Exception(
            "could not create ZIP file %s",
            zipFilePath.string().c_str() );
    }

    std::vector<char> buf(READ_SIZE);

    try
    {
        for (const auto& e: entries)
        {
            const auto& src = e.second;
            auto entryName = e.first.generic_string();

            std::ifstream f(src.string(), std::ios::binary);
            if (!f.good())
            {
                throw insight::Exception(
                    "could not read file %s",
                    src.string().c_str() );
            }

            zip_fileinfo zi{};
            if ( zipOpenNewFileInZip64(
                    zf, entryName.c_str(), &zi,
                    nullptr, 0, nullptr, 0, nullptr,
                    Z_DEFLATED, Z_DEFAULT_COMPRESSION,
                    1 /* zip64 */ ) != ZIP_OK )
            {
                throw insight::Exception(
                    "could not add entry %s to ZIP file",
                    entryName.c_str() );
            }

            while (f)
            {
                f.read(buf.data(), buf.size());
                auto n = f.gcount();
                if (n>0)
                {
                    if (zipWriteInFileInZip(zf, buf.data(), n) != ZIP_OK)
                    {
                        throw insight::Exception(
                            "error writing entry %s to ZIP file",
                            entryName.c_str() );
                    }
                }
            }

            if (zipCloseFileInZip(zf) != ZIP_OK)
            {
                throw insight::Exception(
                    "could not close entry %s in ZIP file",
                    entryName.c_str() );
            }
        }
    }
    catch (...)
    {
        zipClose(zf, nullptr);
        throw;
    }

    if (zipClose(zf, nullptr) != ZIP_OK)
    {
        throw insight::Exception(
            "could not finalize ZIP file %s",
            zipFilePath.string().c_str() );
    }
}

} // namespace insight
