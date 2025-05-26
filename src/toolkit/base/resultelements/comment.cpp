#include "comment.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
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


void Comment::writeLatexCode ( std::ostream& f, const std::string& , int , const boost::filesystem::path&  ) const
{
    f << value_ <<endl;
}


void Comment::exportDataToFile ( const string& name, const path& outputdirectory ) const
{
    boost::filesystem::path fname ( outputdirectory/ ( name+".txt" ) );
    std::ofstream f ( fname.c_str() );
    f<<value_;
}

void Comment::readFromNode(const string &name, const rapidxml::xml_node<> &node)
{
  readBaseAttributesFromNode(name, node);
  value_=node.first_attribute("value")->value();
}


xml_node< char >* Comment::appendToNode ( const string& name, xml_document< char >& doc, xml_node< char >& node ) const
{
    using namespace rapidxml;
    xml_node<>* child = ResultElement::appendToNode ( name, doc, node );

    child->append_attribute ( doc.allocate_attribute
                              (
                                  "value",
                                  doc.allocate_string ( value_.c_str() )
                              ) );

    return child;
}


ResultElementPtr Comment::clone() const
{
    ResultElementPtr res ( new Comment ( value_, shortDescription_.simpleLatex() ) );
    res->setOrder ( order() );
    return res;
}


} // namespace insight
