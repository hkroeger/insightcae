
#include "filecontainer.h"


#include "base/exception.h"
#include "base/rapidxml.h"
#include "boost/archive/iterators/base64_from_binary.hpp"
#include <boost/archive/iterators/transform_width.hpp>


#include "base/tools.h"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <sys/mman.h>
#endif
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;




namespace insight {






MD5HashPtr calcBufferHash(const std::string& buffer)
{
  insight::CurrentExceptionContext ex(
        str(boost::format("computing MD5 hash of buffer (size %d bytes)")%buffer.size()) );

  auto hash=std::make_shared<MD5Hash>();

  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, buffer.data(), buffer.size());
  MD5_Final(hash->data(), &ctx);

  return hash;
}


#ifdef WIN32
#warning hash calculation routine not working
#else
MD5HashPtr calcFileHash(const boost::filesystem::path& filePath)
{
  insight::CurrentExceptionContext ex("computing MD5 hash of file "+filePath.string());

  auto hash=std::make_shared<MD5Hash>();

  int file_descript;
  unsigned long file_size;
  char* file_buffer;

  file_descript = open(filePath.string().c_str(), O_RDONLY);
  if(file_descript < 0) exit(-1);

  struct stat statbuf;
  if(fstat(file_descript, &statbuf) < 0)
    throw insight::Exception("Failed to get file attributes of file "+filePath.string());
  file_size=statbuf.st_size;

  file_buffer = static_cast<char*>(
        mmap(
          0, file_size,
          PROT_READ, MAP_SHARED,
          file_descript, 0 )
        );

  MD5( reinterpret_cast<unsigned char*>(file_buffer),
       file_size,
       hash->data() );

  munmap(file_buffer, file_size);

  return hash;
}
#endif



bool operator<(const timespec& lhs, const timespec& rhs)
{
    if (lhs.tv_sec == rhs.tv_sec)
        return lhs.tv_nsec < rhs.tv_nsec;
    else
        return lhs.tv_sec < rhs.tv_sec;
}


bool operator==(const timespec& lhs, const timespec& rhs)
{
    return
        (lhs.tv_sec == rhs.tv_sec) &&
        (lhs.tv_nsec == rhs.tv_nsec);
}


std::ostream& operator<<(std::ostream& os, const timespec& ts)
{
  os<<ts.tv_sec<<"."<< std::setfill('0') << std::setw(9)<<ts.tv_nsec;
  return os;
}



// boost::filesystem::path FileContainer::unpackFilePath(
//     boost::filesystem::path baseDir ) const
// {
//   if (!hasFileContent()
//         || boost::filesystem::exists(originalFilePath()))
//   {
//     return originalFilePath();
//   }
//   else
//   {
//     if (baseDir.empty())
//     {
//       baseDir=GlobalTemporaryDirectory::path();
//     }
//     if (!boost::filesystem::exists(baseDir))
//     {
//       throw insight::Exception("Error while trying to unpack embedded file: target path "+baseDir.string()+"does not exist!");
//     }

//     auto dirHash = std::hash<std::string>()(
//         originalFilePath().parent_path().string());
//     auto rp = baseDir / "embeddedFiles" / toString(dirHash) / fileName();

//     return rp;
//   }
// }

UnsetBaseDirectory::UnsetBaseDirectory(const std::string& msg)
    : Exception(msg)
{
}


FileContainer::FileContainer()
{}




FileContainer::FileContainer(const FileContainer& other)
    : filePath_(other.filePath_),
    baseDirectory_(other.baseDirectory_),
    file_content_(other.file_content_), // get reference to the same content for performance reasons
    fileContentTimestamp_(other.fileContentTimestamp_)/*,
    fileContentHash_(other.fileContentHash_)*/
{
}




FileContainer::FileContainer(
    const boost::filesystem::path& fileName,
    const boost::optional<boost::filesystem::path>& baseDir )
  : filePath_(fileName),
    baseDirectory_(baseDir)
{
    clock_gettime(CLOCK_REALTIME, &fileContentTimestamp_);
}




FileContainer::FileContainer(
    const TemporaryFile& tf,
    const boost::filesystem::path &fileName )
    : filePath_(fileName)
{
    if (!filePath_.has_extension())
    {
        filePath_.replace_extension(tf.path().filename().extension());
    }
    replaceContent(tf.path());
}




FileContainer::FileContainer(
    const char *binaryFileContent, size_t size,
    const boost::filesystem::path &fileName )
    : filePath_(fileName)
{
    replaceContentBuffer(
        std::make_shared<std::string>(
            binaryFileContent, size ) );
}




FileContainer::FileContainer(
    std::shared_ptr<std::string> content,
    const boost::filesystem::path& fileName )
    : filePath_(fileName)
{
    replaceContentBuffer(content);
}

bool FileContainer::isDifferent(const FileContainer &o) const
{
    if (o.filePath()!=filePath())
        return true;

    if (hasFileContent())
    {
        if (!o.hasFileContent())
            return true;

        if (!(o.contentModificationTime()==contentModificationTime()))
            return true;

        if (o.contentBufferSize()!=contentBufferSize())
            return true;
    }

    return false;
}




FileContainer::~FileContainer()
{}



bool FileContainer::hasFileContent() const
{
  return bool(file_content_);
}



bool FileContainer::isValid() const
{
  return  !filePath_.empty();
}





boost::filesystem::path FileContainer::fileName() const
{
    return filePath_.filename();
}


std::string FileContainer::fileNameStem() const
{
    return fileName().stem().string();
}

/**
   * @brief fileExtension
   * @return
   * file extension in lower case with leading dot
   */
std::string FileContainer::fileExtension() const
{
    return boost::to_lower_copy(
        fileName().extension().string() );
}


boost::filesystem::path FileContainer::filePath() const
{
    return filePath_;
}




boost::filesystem::path
FileContainer::accessibleFilePath(
    bool unpackIfNoLocalCopy,
    boost::optional<boost::filesystem::path> overrideBaseDirectory ) const
{
    boost::filesystem::path fp = expandedFilePath();

    if (!boost::filesystem::exists(fp) && unpackIfNoLocalCopy && hasFileContent())
    {
        auto baseDir = baseDirectory();

        if (overrideBaseDirectory)
            baseDir=*overrideBaseDirectory;

        if (!baseDir)
        {
            fp = GlobalTemporaryDirectory::path()/fileName();
        }
        else
        {
            fp = *baseDir / "embeddedFiles";
            if (!fileName().parent_path().empty())
            {
                auto dirHash = std::hash<std::string>()(
                    fileName().parent_path().string());
                fp /= toString(dirHash);
            }
            fp /= fileName();
        }
    }

    if (unpackIfNoLocalCopy && hasFileContent())
    {
        if (!boost::filesystem::exists(fp))
        {
            copyTo(fp, true);
        }
    }

    return fp;
}




void FileContainer::setFilePath(const boost::filesystem::path &fn)
{
    auto val=fn;
    if (!fn.empty() && baseDirectory_)
    {
        val=boost::filesystem::make_relative(
            *baseDirectory_, fn);
    }
    filePath_=val;
}






std::unique_ptr<std::istream> FileContainer::stream() const
{
  std::unique_ptr<std::istream> file_content_stream_;

  if (hasFileContent())
  {
      file_content_stream_.reset(new std::istringstream(*file_content_));
  }
  else
  {
      auto lfp=expandedFilePath();

      if (boost::filesystem::exists(lfp))
      {
        file_content_stream_.reset(new std::ifstream(lfp.string()));
        if (! (*file_content_stream_) )
        {
          throw insight::Exception(
                "Could not open file %s for reading",
                lfp.c_str());
        }
      }
      else
      {
          // empty
          file_content_stream_.reset(new std::istringstream());
      }
  }


  return file_content_stream_;
}




const char *FileContainer::binaryFileContent() const
{
  insight::assertion(bool(file_content_), "There is no file content in memory");
  return file_content_->c_str();
}


void FileContainer::resolveRelativePath(
    const boost::filesystem::path &newBaseDirectory)
{
    if (!filePath_.empty() && (baseDirectory_||filePath_.is_absolute()))
    {
        filePath_=boost::filesystem::make_relative(
            newBaseDirectory, expandedFilePath());
    }
    baseDirectory_=newBaseDirectory;
}

boost::filesystem::path
FileContainer::expandedFilePath(bool dontThrow) const
{
    if (filePath_.is_relative())
    {
        if (baseDirectory_)
        {
            return (*baseDirectory_)/filePath_;
        }
        else
        {
            if (!dontThrow)
            {
                throw insight::UnsetBaseDirectory(
                    str(boost::format("only a relative path (%s) was given"
                    " but no base directory for full path resolution has been set")
                        % filePath_.string())
                    );
            }
            return boost::filesystem::path();
        }
    }
    else
        return filePath_;
}




timespec highres_last_write_time(const boost::filesystem::path& file)
{
  int file_descript = open(file.string().c_str(), O_RDONLY);

  struct stat statbuf;
  if(fstat(file_descript, &statbuf) < 0)
    throw insight::Exception("Failed to get file attributes of file "+file.string());

#ifdef WIN32
#warning limited timestamp resolution in windows
  timespec result;
  result.tv_sec=statbuf.st_mtime;
  result.tv_nsec=0;
  return result;
#else
  return statbuf.st_mtim;
#endif

}



// bool FileContainer::needsUnpack(const boost::filesystem::path& unpackPath) const
// {
//   insight::CurrentExceptionContext ex("checking, if file content needs to be unpacked into "+unpackPath.string());
//   if (!originalFilePath().empty())
//   {
//     bool needUnpack = false;

//     if (file_content_) // unpack, if we have content
//     {
//       if (!exists(unpackPath)) // unpack only, if not already done
//       {
//         insight::dbg() << "needUnpack: target file does not exist"<< std::endl;
//         needUnpack = true;
//       }
//       else
//       {
//         // only consider unpacking, if the data we have is newer than what is on disk
//         auto lastWriteTime = highres_last_write_time(unpackPath);

//         if (lastWriteTime < fileContentTimestamp_)
//         {
//             insight::dbg() << "needUnpack: target file time stamp is older than buffer time stamp"<< std::endl;
//             insight::dbg() << "  "<<lastWriteTime<<" < "<<fileContentTimestamp_<<std::endl;

// //          // check if the file on disk is actually different
// //          auto cmd5 = calcFileHash(unpackPath);
// //          if (*cmd5 != *fileContentHash_)
// //          {
//             needUnpack = true;
// //          }
//         }
//       }
//     }

//     insight::dbg() << "needUnpack = " << needUnpack << std::endl;
//     return needUnpack;
//   }
//   else
//   {
//     insight::dbg() << " original file path is empty" << std::endl;
//     insight::dbg() << "needUnpack = " << false << std::endl;
//     return false;
//   }
// }





void FileContainer::signalContentChange()
{}



//void FileContainer::unpack(const boost::filesystem::path& basePath)
//{
//  if (needsUnpack(basePath))
//  {

//    // extract file, create parent path.
//    if (!exists(unpackPath.parent_path()) )
//    {
//      boost::filesystem::create_directories( unpackPath.parent_path() );
//    }

//    std::ofstream file( unpackPath.c_str(), std::ios::out | std::ios::binary);
//    if (file.good())
//    {
//        file.write(file_content_->c_str(), long(file_content_->size()) );
//        file.close();
//    }

//  }
//}


void FileContainer::copyTo(
    const boost::filesystem::path &filePath,
    bool createParentPath ) const
{
  insight::CurrentExceptionContext ex(
        boost::str(boost::format(
            "writing file content into %s (create parent path %d)"
        ) % filePath.string() % createParentPath ) );

    if (!exists(filePath.parent_path()) )
    {
      if (createParentPath)
      {
        boost::filesystem::create_directories( filePath.parent_path() );
      }
    }
    if (!exists(filePath.parent_path()) )
    {
        throw insight::Exception(
            "parent path %s for unpacking of file %s does not exist!",
            filePath.parent_path().c_str(), fileName().c_str());
    }

    if (file_content_)
    {
        writeStringIntoFile(file_content_, filePath);
    }
    else
    {
        auto lfp = expandedFilePath();

        if (boost::filesystem::exists(lfp))
        {
            if (!boost::filesystem::weakly_equivalent(lfp, filePath))
            {
                boost::filesystem::copy_file(
                    lfp, filePath,
                    boost::filesystem::copy_option::overwrite_if_exists);
            }
        }
        else
        {
            throw insight::Exception(
                "cannot copy file %s to %s."
                " There is no content available and also no local copy",
                fileName().c_str(), filePath.c_str() );
        }
    }
}


void FileContainer::replaceContent(const boost::filesystem::path& filePath)
{
  if (exists(filePath) && is_regular_file(filePath) )
  {
    insight::CurrentExceptionContext ex(
          "reading file %s content into buffer",
          filePath.c_str() );

    // read raw file into buffer
    auto newContent = std::make_shared<std::string>();

    readFileIntoString(filePath, *newContent);
    replaceContentBuffer(newContent);
  }
}


void FileContainer::replaceContentBuffer(std::shared_ptr<std::string> newContent)
{
  std::string msg;
  if (newContent)
    msg = boost::str(boost::format(
           "replacing content buffer with data of size %d"
          ) % newContent->size() );
  else
    msg = "resetting content buffer";

  insight::CurrentExceptionContext ex( msg );
  file_content_=newContent;
  clock_gettime(CLOCK_REALTIME, &fileContentTimestamp_);
  //  fileContentHash_ = calcBufferHash(*file_content_);

  signalContentChange();
}

size_t FileContainer::contentBufferSize() const
{
  if (file_content_)
  {
    return file_content_->size();
  }
  return 0;
}




void FileContainer::clearPackedData()
{
  insight::CurrentExceptionContext ex("clearing content buffer");
  file_content_.reset();
  fileContentTimestamp_={0,0};
}


void FileContainer::operator=(const FileContainer& oc)
{
  filePath_ = oc.filePath_;
  baseDirectory_ = oc.baseDirectory_;
  file_content_ = oc.file_content_;
  fileContentTimestamp_ = oc.fileContentTimestamp_;
  signalContentChange();
}


void FileContainer::appendToNode
(
    rapidxml::xml_document<> &doc,
    rapidxml::xml_node<> &node,
    const std::string& fileNameAttribName,
    const std::string& contentAttribName
) const
{
  auto fn=filePath_;
  if (baseDirectory_)
  {
      if (!fn.is_relative() && path_contains_file(*baseDirectory_, filePath_))
      {
          fn=make_relative(filePath_, *baseDirectory_);
      }
  }

  appendAttribute(doc, node, fileNameAttribName, fn.string());

  if (hasFileContent())
  {
    node.append_attribute(doc.allocate_attribute
    (
        doc.allocate_string(contentAttribName.c_str()),
        base64_encode(doc, *file_content_)
    ));

  }
}


void FileContainer::readFromNode
(
  const rapidxml::xml_node<>& node,
  const std::string& fileNameAttribName,
  const std::string& contentAttribName
)
{

  filePath_ =
      getMandatoryAttribute(node, fileNameAttribName);

  if (auto* a = node.first_attribute(contentAttribName.c_str()))
  {
    base64_decode(a->value(), a->value_size(), file_content_);
    clock_gettime(CLOCK_REALTIME, &fileContentTimestamp_);
  }

}

const timespec &FileContainer::contentModificationTime() const
{
  return fileContentTimestamp_;
}


bool FileContainer::operator==(const FileContainer& o) const
{
    if (!boost::filesystem::weakly_equivalent(filePath_, o.filePath_))
        return false;

    if (file_content_ && o.file_content_)
    {
        if (file_content_->size()
            != o.file_content_->size())
            return false;
    }

    //timespec fileContentTimestamp_;  // ignore for now

    return true;
}



} // namespace insight
