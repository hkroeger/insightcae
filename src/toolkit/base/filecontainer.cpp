
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


namespace boost
{
namespace filesystem
{




template < >
// path& path::append< typename path::iterator >( typename path::iterator begin, typename path::iterator end, const codecvt_type& cvt)
path&
path::append< typename path::iterator >(
    typename path::iterator begin,
    typename path::iterator end )
{
  for( ; begin != end ; ++begin )
    *this /= *begin;
  return *this;
}



// Return path when appended to a_From will resolve to same as a_To
path
make_relative(
    path a_From,
    path a_To )
{
  a_From = absolute( a_From ); a_To = absolute( a_To );
  path ret;
  path::const_iterator itrFrom( a_From.begin() ), itrTo( a_To.begin() );

  // Find common base
  for(
      path::const_iterator toEnd( a_To.end() ), fromEnd( a_From.end() ) ;
      itrFrom != fromEnd && itrTo != toEnd && *itrFrom == *itrTo;
      ++itrFrom, ++itrTo );

  // Navigate backwards in directory to reach previously found base
  for( path::const_iterator fromEnd( a_From.end() ); itrFrom != fromEnd; ++itrFrom )
  {
    if( (*itrFrom) != "." )
      ret /= "..";
  }

  // Now navigate down the directory branch
  ret.append( itrTo, a_To.end() );
  return ret;
}


// from Rob Kennedy
// https://stackoverflow.com/users/33732/rob-kennedy
bool path_contains_file(path dir, path file)
{
    // If dir ends with "/" and isn't the root directory, then the final
    // component returned by iterators will include "." and will interfere
    // with the std::equal check below, so we strip it before proceeding.
    if (dir.filename() == ".")
        dir.remove_filename();
    // We're also not interested in the file's name.
    assert(file.has_filename());
    file.remove_filename();

    // If dir has more components than file, then file can't possibly
    // reside in dir.
    auto dir_len = std::distance(dir.begin(), dir.end());
    auto file_len = std::distance(file.begin(), file.end());
    if (dir_len > file_len)
        return false;

    // This stops checking when it reaches dir.end(), so it's OK if file
    // has more directory components afterward. They won't be checked.
    return std::equal(dir.begin(), dir.end(), file.begin());
}


bool weakly_equivalent(path first, path second)
{
    auto fullpath1=weakly_canonical(absolute(first));
    auto fullpath2=weakly_canonical(absolute(second));

    return fullpath1 == fullpath2;
}


}
}




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
    : fileName_(other.fileName_),
    baseDirectory_(other.baseDirectory_),
    file_content_(other.file_content_), // get reference to the same content for performance reasons
    fileContentTimestamp_(other.fileContentTimestamp_)/*,
    fileContentHash_(other.fileContentHash_)*/
{
}




FileContainer::FileContainer(
    const boost::filesystem::path& fileName )
  : fileName_(fileName)
{
    clock_gettime(CLOCK_REALTIME, &fileContentTimestamp_);
}




FileContainer::FileContainer(
    const TemporaryFile& tf,
    const boost::filesystem::path &fileName )
    : fileName_(fileName)
{
    if (!fileName_.has_extension())
    {
        fileName_.replace_extension(tf.path().filename().extension());
    }
    replaceContent(tf.path());
}




FileContainer::FileContainer(
    const char *binaryFileContent, size_t size,
    const boost::filesystem::path &fileName )
    : fileName_(fileName)
{
    replaceContentBuffer(
        std::make_shared<std::string>(
            binaryFileContent, size ) );
}




FileContainer::FileContainer(
    std::shared_ptr<std::string> content,
    const boost::filesystem::path& fileName )
    : fileName_(fileName)
{
    replaceContentBuffer(content);
}




FileContainer::~FileContainer()
{}



bool FileContainer::hasFileContent() const
{
  return bool(file_content_);
}



bool FileContainer::isValid() const
{
  return  !fileName_.empty();
}





const boost::filesystem::path& FileContainer::fileName() const
{
    return fileName_;
}

void FileContainer::setFileName(const boost::filesystem::path &fn)
{
    fileName_=fn;
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
      auto lfp=localFilePath();

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
    const boost::filesystem::path &baseDirectory)
{
    baseDirectory_=baseDirectory;
}

boost::filesystem::path
FileContainer::localFilePath() const
{
    if (fileName_.is_relative())
    {
        if (baseDirectory_)
        {
            return (*baseDirectory_)/fileName_;
        }
        else
        {
            throw insight::UnsetBaseDirectory(
                str(boost::format("only a relative path (%s) was given"
                " but no base directory for full path resolution has been set")
                    % fileName_.string())
                );
        }
    }
    else
        return fileName_;
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
        auto lfp = localFilePath();

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
  fileName_ = oc.fileName_;
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
  auto fn=fileName_;
  if (baseDirectory_)
  {
      if (!fn.is_relative() && path_contains_file(*baseDirectory_, fileName_))
      {
          fn=make_relative(fileName_, *baseDirectory_);
      }
  }

  appendAttribute(doc, node, fileNameAttribName, fn.string());

  if (hasFileContent())
  {

    // ===========================================================================================
    // 1.) do base64 encode
    // typedefs
    using namespace boost::archive::iterators;
    typedef
//          insert_linebreaks<         // insert line breaks every 72 characters
            base64_from_binary<    // convert binary values to base64 characters
                transform_width<   // retrieve 6 bit integers from a sequence of 8 bit bytes
                    const char*, 6, 8
                >
            >
//              ,72 >
        base64_enc; // compose all the above operations in to a new iterator

    char tail[3] = {0,0,0};
    size_t len=file_content_->size();
    unsigned int one_third_len = len/3;
    unsigned int len_rounded_down = one_third_len*3;
    unsigned int j = len_rounded_down + one_third_len;
    unsigned int base64length = ((4 * file_content_->size() / 3) + 3) & ~3;

    auto *xml_content = doc.allocate_string(0, base64length+1);
    std::copy(
          base64_enc(file_content_->c_str()),
          base64_enc(file_content_->c_str()+len_rounded_down),
          xml_content
          );

    if (len_rounded_down != len)
    {
        unsigned int i=0;
        for(; i < len - len_rounded_down; ++i)
        {
            tail[i] = (*file_content_)[len_rounded_down+i];
        }

        std::copy(base64_enc(tail), base64_enc(tail + 3), xml_content + j);

        for(i=len + one_third_len + 1; i < j+4; ++i)
        {
            xml_content[i] = '=';
        }
    }

    xml_content[base64length]=0;


    // ===========================================================================================
    // 2.) append to node
    node.append_attribute(doc.allocate_attribute
    (
      doc.allocate_string(contentAttribName.c_str()),
      //os.str().c_str()
      xml_content
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

  fileName_ =
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
    if (!boost::filesystem::weakly_equivalent(fileName_, o.fileName_))
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
