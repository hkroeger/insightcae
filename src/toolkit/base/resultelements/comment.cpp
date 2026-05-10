#include "comment.h"
#include "base/hierarchicalelement.h"
#include "base/rapidxml.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace rapidxml;


namespace insight {

defineType ( Comment );
addToFactoryTable ( ResultElement, Comment );


Comment::Comment ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : ResultElement ( shortdesc, longdesc, unit )
{}


Comment::Comment ( const std::string& value, const std::string& shortDesc )
    : ResultElement ( shortDesc, "", "" ),
      value_ ( value )
{
}


std::string Comment::latexRepresentation(
    const std::string& name,
    int documentHierarchyLevel,
    const FileStorageInfo& fsi ) const
{
    std::ostringstream f;
    f << value_;
    return f.str();
}

string Comment::plainTextRepresentation(int indent) const
{
    return value_;
}


void Comment::exportDataToFile (
    const string& name,
    const boost::filesystem::path& outputdirectory ) const
{
    boost::filesystem::path fname ( outputdirectory/ ( name+".txt" ) );
    std::ofstream f ( fname.c_str() );
    f<<value_;
}

const rapidxml::xml_node<>*
Comment::readFromNode(
    const string &name,
    const rapidxml::xml_node<> &node )
{
  auto *child=ResultElement::readFromNode(name, node);
  value_=getMandatoryAttribute(*child, "value");
  return child;
}

int Comment::nChildren() const
{
    return 0;
}

bool Comment::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const Comment*>(&op))
    {
        if (value_!=oa->value_)
            return false;
        return true;
    }
    else
        return false;
}


xml_node< char >* Comment::appendToNode (
    const string& name,
    xml_document< char >& doc,
    xml_node< char >& node,
    const OutputProperties& outProps ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node, outProps );

    appendAttribute(doc, *child, "value", value_);

    return child;
}


std::unique_ptr<hierarchicalData::Element> Comment::cloneUninitialized() const
{
    auto res = std::make_unique<Comment> ( value_, shortDescription_.simpleLatex() );
    res->setOrder ( order() );
    return res;
}


} // namespace insight
