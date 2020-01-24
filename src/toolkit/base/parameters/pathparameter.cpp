#include "pathparameter.h"

#include <streambuf>
#include <ostream>

#include "boost/archive/iterators/base64_from_binary.hpp"
#include "boost/archive/iterators/binary_from_base64.hpp"
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>


#include "base/tools.h"



namespace boost
{
namespace filesystem
{




template < >
// path& path::append< typename path::iterator >( typename path::iterator begin, typename path::iterator end, const codecvt_type& cvt)
path& path::append< typename path::iterator >( typename path::iterator begin, typename path::iterator end)
{
  for( ; begin != end ; ++begin )
          *this /= *begin;
  return *this;
}



// Return path when appended to a_From will resolve to same as a_To
boost::filesystem::path make_relative( boost::filesystem::path a_From, boost::filesystem::path a_To )
{
  a_From = boost::filesystem::absolute( a_From ); a_To = boost::filesystem::absolute( a_To );
  boost::filesystem::path ret;
  boost::filesystem::path::const_iterator itrFrom( a_From.begin() ), itrTo( a_To.begin() );
  // Find common base
  for( boost::filesystem::path::const_iterator toEnd( a_To.end() ), fromEnd( a_From.end() ) ; itrFrom != fromEnd && itrTo != toEnd && *itrFrom == *itrTo; ++itrFrom, ++itrTo );
  // Navigate backwards in directory to reach previously found base
  for( boost::filesystem::path::const_iterator fromEnd( a_From.end() ); itrFrom != fromEnd; ++itrFrom )
  {
          if( (*itrFrom) != "." )
                  ret /= "..";
  }
  // Now navigate down the directory branch
  ret.append( itrTo, a_To.end() );
  return ret;
}


}
}





namespace insight
{


const std::string base64_padding[] = {"", "==","="};



std::string base64_encode(const std::string& s)
{
  namespace bai = boost::archive::iterators;

  std::stringstream os;

  // convert binary values to base64 characters
  typedef bai::base64_from_binary
  // retrieve 6 bit integers from a sequence of 8 bit bytes
  <bai::transform_width<const char *, 6, 8> > base64_enc; // compose all the above operations in to a new iterator

  std::copy(base64_enc(s.c_str()), base64_enc(s.c_str() + s.size()),
            std::ostream_iterator<char>(os));

  os << base64_padding[s.size() % 3];
  return os.str();
}

std::string base64_encode(const boost::filesystem::path& f)
{
  std::ifstream in(f.c_str());
  std::string contents_raw;
  in.seekg(0, std::ios::end);
  contents_raw.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents_raw[0], contents_raw.size());

  return base64_encode(contents_raw);
}


std::string base64_decode(const std::string& s)
{
  namespace bai = boost::archive::iterators;

  std::stringstream os;

  typedef bai::transform_width<bai::binary_from_base64<const char *>, 8, 6> base64_dec;

  unsigned int size = s.size();

  // Remove the padding characters, cf. https://svn.boost.org/trac/boost/ticket/5629
  if (size && s[size - 1] == '=') {
    --size;
    if (size && s[size - 1] == '=') --size;
  }
  if (size == 0) return std::string();

  std::copy(base64_dec(s.data()), base64_dec(s.data() + size),
            std::ostream_iterator<char>(os));

  return os.str();
}


defineType(PathParameter);
addToFactoryTable(Parameter, PathParameter);

PathParameter::PathParameter(
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order
    )
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
}

PathParameter::PathParameter(
    const boost::filesystem::path& value, const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order,
    std::shared_ptr<std::string> base64_content
    )
: Parameter(description, isHidden, isExpert, isNecessary, order),
  value_(value),
  file_content_(base64_content)
{
}


std::string PathParameter::latexRepresentation() const
{
    return SimpleLatex( valueToString ( value_ ) ).toLaTeX();
}

std::string PathParameter::plainTextRepresentation(int /*indent*/) const
{
  return SimpleLatex( valueToString ( value_ ) ).toPlainText();
}

bool PathParameter::isValid() const
{
  return !value_.empty() && ( boost::filesystem::exists(value_) || file_content_ );
}


boost::filesystem::path PathParameter::unpackFilePath(boost::filesystem::path baseDir) const
{
  if (baseDir.empty())
  {
    baseDir=GlobalTemporaryDirectory::path();
  }
  if (!boost::filesystem::exists(baseDir))
  {
    throw insight::Exception("Error while trying to unpack embedded file: target path "+baseDir.string()+"does not exist!");
  }

  auto dirHash = std::hash<std::string>()(value_.parent_path().string());
  auto rp = baseDir / "embeddedFiles" / boost::lexical_cast<std::string>(dirHash) / fileName();
  std::cerr<<rp<<std::endl;
  return rp;
}


boost::filesystem::path PathParameter::filePath(boost::filesystem::path baseDir) const
{
  if (isPacked())
  {
    const_cast<PathParameter*>(this)->unpack(baseDir); // does nothing, if already unpacked
    return unpackFilePath(baseDir);
  }

  return value_;
}

const boost::filesystem::path &PathParameter::originalFilePath() const
{
  return value_;
}

boost::filesystem::path PathParameter::fileName() const
{
  return value_.filename();
}


void PathParameter::setOriginalFilePath(const boost::filesystem::path &value)
{
  value_=value;
  clearPackedData();
}

std::istream& PathParameter::stream() const
{
  if (isPacked())
  {
    file_content_stream_.reset(new std::istringstream(*file_content_));
  }
  else
  {
    file_content_stream_.reset(new std::ifstream(value_.c_str()));
    if (! (*file_content_stream_) )
    {
      throw insight::Exception("Could not open file "+value_.string()+" for reading!");
    }
  }

  return *file_content_stream_;
}


bool PathParameter::isPacked() const
{
  return bool(file_content_);
}

void PathParameter::pack()
{
  // read and store file
  replaceContent(value_);
}

void PathParameter::unpack(const boost::filesystem::path& basePath)
{
  if (!value_.empty())
  {
    bool needUnpack = false;

    boost::filesystem::path unpackPath = unpackFilePath(basePath);

    if (file_content_) // unpack, if we have content
    {
      if (!exists(unpackPath)) // unpack only, if not already done
        needUnpack = true;
    }

    if (needUnpack)
    {

      // extract file, create parent path.
      if (!exists(unpackPath.parent_path()) )
      {
        boost::filesystem::create_directories( unpackPath.parent_path() );
      }

      std::ofstream file( unpackPath.c_str(), std::ios::out | std::ios::binary);
      if (file.good())
      {
          file.write(file_content_->c_str(), long(file_content_->size()) );
          file.close();
      }

    }
  }
}

void PathParameter::copyTo(const boost::filesystem::path &filePath) const
{
  if (file_content_)
  {
    std::ofstream file( filePath.c_str(), std::ios::out | std::ios::binary);
    if (file.good())
    {
        file.write(file_content_->c_str(), long(file_content_->size()) );
        file.close();
    }
  }
  else
  {
    boost::filesystem::copy_file(value_, filePath, boost::filesystem::copy_option::overwrite_if_exists);
  }
}


void PathParameter::replaceContent(const boost::filesystem::path& filePath)
{
  if (exists(filePath) && is_regular_file(filePath) )
  {
    // read raw file into buffer
    std::cout<<"reading content of file "<<filePath<<std::endl;
    std::ifstream in(filePath.c_str());
    std::istreambuf_iterator<char> inputBegin(in), inputEnd;
    file_content_.reset(new std::string);
    std::back_insert_iterator<std::string> stringInsert(*file_content_);
    copy(inputBegin, inputEnd, stringInsert);

    // compute hash
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, file_content_->data(), file_content_->size());
    MD5_Final(file_content_hash_, &ctx);
  }
}


void PathParameter::clearPackedData()
{
  file_content_.reset();
}

//template <typename char_type>
//struct ostreambuf
//    : public std::basic_streambuf<char_type, std::char_traits<char_type> >
//{
//    ostreambuf(char_type* buffer, std::streamsize bufferLength)
//    {
//        // set the "put" pointer the start of the buffer and record it's length.
//        this->setp(buffer, buffer + bufferLength);
//    }
//};

rapidxml::xml_node<>* PathParameter::appendToNode
(
  const std::string& name,
  rapidxml::xml_document<>& doc,
  rapidxml::xml_node<>& node,
  boost::filesystem::path inputfilepath
) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
    std::string relpath="";
    if (!value_.empty())
    {
      relpath=make_relative(inputfilepath, value_).string();
    }
    child->append_attribute(doc.allocate_attribute
    (
      "value",
      doc.allocate_string(relpath.c_str())
    ));

    if (isPacked())
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


//      // read raw file into buffer
//      std::ifstream in(value_.c_str());
//      std::string raw_data ( static_cast<std::stringstream const&>(std::stringstream() << in.rdbuf()).str() );


      // base64-encode
//      unsigned int writePaddChars = (3-raw_data.length()%3)%3;

      char tail[3] = {0,0,0};
      size_t len=file_content_->size();
      uint one_third_len = len/3;
      uint len_rounded_down = one_third_len*3;
      uint j = len_rounded_down + one_third_len;
      unsigned int base64length = ((4 * file_content_->size() / 3) + 3) & ~3;

      auto *xml_content = doc.allocate_string(0, base64length+1);
      std::copy(
            base64_enc(file_content_->c_str()),
            base64_enc(file_content_->c_str()+len_rounded_down),
            xml_content
            );
//      std::ostringstream os;
//      std::copy(
//            base64_enc(file_content_->c_str()),
//            base64_enc(file_content_->c_str()+file_content_->size()),
//            std::ostream_iterator<char>(os)
//            );
//      os << base64_padding[file_content_->size() % 3];

      if (len_rounded_down != len)
      {
          uint i=0;
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

//      ostreambuf<char> ostreamBuffer(xml_content, base64length);
//      std::ostream os(&ostreamBuffer);
//      std::copy(
//            base64_text(file_content_.begin()),
//            base64_text(file_content_.end()),
//            std::ostream_iterator<char>(os)
//         );
//      os<<std::string(writePaddChars, '=');

      // ===========================================================================================
      // 2.) append to node
      child->append_attribute(doc.allocate_attribute
      (
        "content",
        //os.str().c_str()
        xml_content
      ));

    }
    return child;

}

void PathParameter::readFromNode
(
  const std::string& name,
  rapidxml::xml_document<>&,
  rapidxml::xml_node<>& node,
  boost::filesystem::path inputfilepath
)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());
  if (child)
  {
    boost::filesystem::path abspath(child->first_attribute("value")->value());

    if (!abspath.empty())
    {
      if (abspath.is_relative())
      {
        abspath = boost::filesystem::absolute(inputfilepath / abspath);
      }
  #if BOOST_VERSION < 104800
  #warning Conversion into canonical paths disabled!
  #else
      if (boost::filesystem::exists(abspath)) // avoid throwing errors during read of parameterset
          abspath=boost::filesystem::canonical(abspath);
  #endif
    }

    value_=abspath;

    if (auto* a = child->first_attribute("content"))
    {

      char *src = a->value();
      size_t size = a->value_size();

      if ((size>0) && src[size - 1] == '=')
      {
        --size;
        if ((size>0) && src[size - 1] == '=')
        {
           --size;
        }
      }

      if (size == 0)
      {
        file_content_->clear();
      }
      else
      {
        using namespace boost::archive::iterators;

        typedef
          transform_width<
           binary_from_base64<
            remove_whitespace<
             const char*
            >
           >,
           8, 6
          >
          base64_dec;

          file_content_.reset(new std::string( base64_dec(src), base64_dec(src + size) ));
      }
    }
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
             ) % type() % name % value_.string()
           )
        );
  }
}

PathParameter *PathParameter::clonePathParameter() const
{
  return new PathParameter(value_, description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_, file_content_);
}

Parameter* PathParameter::clone() const
{
  return clonePathParameter();
}

void PathParameter::reset(const Parameter& p)
{
  if (const auto* op = dynamic_cast<const PathParameter*>(&p))
  {
    Parameter::reset(p);
    file_content_=op->file_content_;
  }
  else
    throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
}

void PathParameter::operator=(const PathParameter &op)
{
  description_ = op.description_;
  isHidden_ = op.isHidden_;
  isExpert_ = op.isExpert_;
  isNecessary_ = op.isNecessary_;
  order_ = op.order_;

  value_ = op.value_;
  file_content_ = op.file_content_;
  memcpy(file_content_hash_, op.file_content_hash_, sizeof(op.file_content_hash_));
}





std::shared_ptr<PathParameter> make_filepath(const boost::filesystem::path& path)
{
  return std::shared_ptr<PathParameter>(new PathParameter(path, "temporary file path"));
}



defineType(DirectoryParameter);
addToFactoryTable(Parameter, DirectoryParameter);

DirectoryParameter::DirectoryParameter(const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: PathParameter(".", description, isHidden, isExpert, isNecessary, order)
{}

DirectoryParameter::DirectoryParameter(const boost::filesystem::path& value, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: PathParameter(value, description, isHidden, isExpert, isNecessary, order)
{}

std::string DirectoryParameter::latexRepresentation() const
{
    return std::string()
      + "{\\ttfamily "
      + SimpleLatex( boost::lexical_cast<std::string>(boost::filesystem::absolute(value_)) ).toLaTeX()
      + "}";
}

//std::string DirectoryParameter::plainTextRepresentation(int indent) const
//{
//    return std::string(indent, ' ')
//      + "\""
//      + SimpleLatex( boost::lexical_cast<std::string>(boost::filesystem::absolute(value_)) ).toPlainText()
//      + "\"\n";
//}


rapidxml::xml_node<>* DirectoryParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
    boost::filesystem::path inputfilepath) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
    child->append_attribute(doc.allocate_attribute
    (
      "value",
      doc.allocate_string(value_.c_str())
    ));
    return child;
}

void DirectoryParameter::readFromNode
(
    const std::string& name,
    rapidxml::xml_document<>&,
    rapidxml::xml_node<>& node,
    boost::filesystem::path
)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());
  if (child)
  {
    value_=boost::filesystem::path(child->first_attribute("value")->value());
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
             ) % type() % name % value_.string()
           )
        );
  }}



Parameter* DirectoryParameter::clone() const
{
  return new DirectoryParameter(value_, description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_);
}


void DirectoryParameter::reset(const Parameter& p)
{
  if (const auto* op = dynamic_cast<const DirectoryParameter*>(&p))
  {
    PathParameter::reset(p);
  }
  else
    throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
}


}
