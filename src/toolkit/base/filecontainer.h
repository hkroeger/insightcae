#ifndef INSIGHT_FILECONTAINER_H
#define INSIGHT_FILECONTAINER_H

#include <ostream>
#include "base/filestorageinfo.h"
#include "base/exception.h"
#include <openssl/md5.h>
#include <string>

#include "rapidxml/rapidxml.hpp"


namespace boost
{
namespace filesystem
{

template < >
path& path::append< typename path::iterator >( typename path::iterator begin, typename path::iterator end, const codecvt_type& cvt);

//boost::filesystem::path make_relative( boost::filesystem::path a_From, boost::filesystem::path a_To );

}
}



namespace insight
{


class TemporaryFile;
class PathParameter;


typedef std::array<unsigned char, MD5_DIGEST_LENGTH> MD5Hash;
typedef std::shared_ptr<MD5Hash> MD5HashPtr;


bool operator<(const timespec& lhs, const timespec& rhs);
bool operator==(const timespec& lhs, const timespec& rhs);
std::ostream& operator<<(std::ostream& os, const timespec& ts);




class UnsetBaseDirectory
    : public Exception
{
public:
    UnsetBaseDirectory(const std::string& msg);
};



/**
 * @brief The FileContainer class
 * stores a file path and optionally its content.
 */
class FileContainer
{
    /**
   * @brief originalFileName_
   * the path to the file
   * a relative path or an absolute path.
   * Relative paths are resolved with the baseDirectory,
   * which might change later.
   */
  boost::filesystem::path fileName_;


  /**
   * @brief baseDirectory_
   * for relative paths: base directory for resolution
   */
  boost::optional<boost::filesystem::path> baseDirectory_;


  /**
   * @brief file_content_
   * Store content of file, if packed.
   * Contains plain file content, not encoded.
   */
    std::shared_ptr<std::string> file_content_;
    timespec fileContentTimestamp_;
    //  MD5HashPtr fileContentHash_;

protected:
    virtual void signalContentChange();

    inline const boost::optional<boost::filesystem::path> baseDirectory() const
    {
        return baseDirectory_;
    }

public:
  FileContainer();

    /**
   * @brief FileContainer
   * copy contents from other
   * @param other
   */
  FileContainer(
      const FileContainer& other );

  /**
   * @brief FileContainer
   * create from local file
   * The contents will only be read in later,
   * if packing is requested.
   * @param fileName
   */
  FileContainer(
      const boost::filesystem::path& fileName,
      const boost::optional<boost::filesystem::path>& baseDir
        = boost::optional<boost::filesystem::path>() );

  /**
   * @brief FileContainer
   * copy contents from temporary file and use with given name
   * @param originalFileName
   * @param tf
   */
  FileContainer(
      const TemporaryFile& tf,
      const boost::filesystem::path& fileName );

  /**
   * @brief FileContainer
   * create from given buffer
   * @param binaryFileContent
   * @param size
   * @param fileName
   */
  FileContainer(
      const char *binaryFileContent, size_t size,
      const boost::filesystem::path& fileName );

  /**
   * @brief FileContainer
   * create from content
   * @param content
   * @param fileName
   */
  FileContainer(
      std::shared_ptr<std::string> content,
      const boost::filesystem::path& fileName );


  virtual ~FileContainer();

  bool hasFileContent() const;

  bool isValid() const;

  /**
   * @brief fileName
   * @return returns the file name component only
   */
  const boost::filesystem::path& fileName() const;

  virtual void setFileName(const boost::filesystem::path& fn);


  std::unique_ptr<std::istream> stream() const;
  const char* binaryFileContent() const;

  void resolveRelativePath(const boost::filesystem::path& baseDirectory);

  boost::filesystem::path localFilePath() const;

  /**
   * @brief copyTo
   * Unpack or copy file to given location
   * @param filePath
   */
  void copyTo(const boost::filesystem::path& filePath, bool createParentPath = false) const;

  /**
   * @brief replaceContent
   * reads the content from specified file. The original file path is retained.
   * @param filePath
   */
  void replaceContent(const boost::filesystem::path& filePath);

  void replaceContentBuffer(std::shared_ptr<std::string> newContent);

  size_t contentBufferSize() const;

  void clearPackedData();

  void operator=(const FileContainer& oc);


  void appendToNode (
      rapidxml::xml_document<>& doc,
      rapidxml::xml_node<>& node,
      const std::string& fileNameAttribName = "value",
      const std::string& contentAttribName = "content" ) const;

  void readFromNode (
      const rapidxml::xml_node<>& node,
      const std::string& fileNameAttribName = "value",
      const std::string& contentAttribName = "content" );

  const timespec& contentModificationTime() const;

  bool operator==(const FileContainer&) const;

};




} // namespace insight

#endif // INSIGHT_FILECONTAINER_H
