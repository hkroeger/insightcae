#ifndef INSIGHT_FILECONTAINER_H
#define INSIGHT_FILECONTAINER_H

#include <ostream>
#include "base/boost_include.h"
#include <openssl/md5.h>

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

/**
 * @brief The FileContainer class
 * stores a file path and optionally its content.
 */
class FileContainer
{
  friend class PathParameter;

protected:
  /**
     * @brief value_
     * original file name
     */
  boost::filesystem::path originalFilePath_;

  /**
   * @brief file_content_
   * Store content of file, if packed.
   * Contains plain file content, not encoded.
   */
  std::shared_ptr<std::string> file_content_;
  timespec fileContentTimestamp_;
//  MD5HashPtr fileContentHash_;

  mutable std::unique_ptr<std::istream> file_content_stream_;

protected:
  /**
   * @brief unpackFilePath
   * creates a path below baseDirectory (if required within a unique subdirectory)
   * @param baseDirectory
   * @return
   */
  boost::filesystem::path unpackFilePath(boost::filesystem::path baseDirectory = "") const;

  bool needsUnpack(const boost::filesystem::path& unpackPath) const;

  virtual void signalContentChange();

public:
  FileContainer();

  FileContainer(
      const FileContainer& other );

  FileContainer(
      const boost::filesystem::path& originalFileName,
      std::shared_ptr<std::string> binary_content = std::shared_ptr<std::string>() );

  FileContainer(
      const boost::filesystem::path& originalFileName,
      const TemporaryFile& tf );

  FileContainer(
      const char *binaryFileContent, size_t size,
      const boost::filesystem::path& originalFileName );

  virtual ~FileContainer();

  bool hasFileContent() const;

  bool isValid() const;

  /**
   * @brief filePath
   * Get the path of the file.
   * It will be created, if it does not exist on the filesystem yet
   * but its content is available in memory.
   * @param baseDirectory
   * The working directory. If the file is only in memory,
   * it will be created in a temporary directory under this path.
   * @return
   */
//  boost::filesystem::path filePath(boost::filesystem::path baseDirectory = "") const;

  /**
   * @brief originalFilePath
   * The path of the originally referenced file.
   * Since the file might have been packed into the parameter set,
   * this file must not necessarily be present under the specified path.
   * @return
   */
  const boost::filesystem::path& originalFilePath() const;

  /**
   * @brief fileName
   * @return returns the file name component only
   */
  boost::filesystem::path fileName() const;

  void setOriginalFilePath(const boost::filesystem::path& value);


  std::istream& stream() const;
  const char* binaryFileContent() const;


//  /**
//   * @brief isPacked
//   * check, if contains file contents
//   * @return
//   */
//  bool isPacked() const;

//  /**
//   * @brief pack
//   * pack the external file. Replace stored content, if present.
//   */
//  void pack();

//  /**
//   * @brief unpack
//   * restore file contents on disk, if file is not there
//   */
//  void unpack(const boost::filesystem::path& basePath);

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
      boost::filesystem::path inputfilepath,
      const std::string& fileNameAttribName = "value",
      const std::string& contentAttribName = "content" ) const;

  void readFromNode (
      const rapidxml::xml_node<>& node,
      boost::filesystem::path inputfilepath,
      const std::string& fileNameAttribName = "value",
      const std::string& contentAttribName = "content" );

  const timespec& contentModificationTime() const;

};





} // namespace insight

#endif // INSIGHT_FILECONTAINER_H
