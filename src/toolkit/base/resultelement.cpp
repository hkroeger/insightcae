#include "resultelement.h"


using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;


namespace insight {




Ordering::Ordering ( double ordering_base, double ordering_step_fraction )
    : ordering_ ( ordering_base ),
      step_ ( ordering_base*ordering_step_fraction )
{}

double Ordering::next()
{
    ordering_+=step_;
    return ordering_;
}



string latex_subsection ( int level )
{
    string cmd="\\";
    if ( level==2 ) {
        cmd+="paragraph";
    } else if ( level==3 ) {
        cmd+="subparagraph";
    } else if ( level>3 ) {
        cmd="";
    } else {
        for ( int i=0; i<min ( 2,level ); i++ ) {
            cmd+="sub";
        }
        cmd+="section";
    }
    return cmd;
}



defineType(ResultElement);
defineFactoryTable
(
  ResultElement,
  LIST(const std::string& shortdesc, const std::string& longdesc, const std::string& unit),
  LIST(shortdesc, longdesc, unit)
);


ResultElement::ResultElement ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
    : shortDescription_ ( shortdesc ),
      longDescription_ ( longdesc ),
      unit_ ( unit ),
      order_ ( 0 )
{}


ResultElement::~ResultElement()
{}


const SimpleLatex& ResultElement::shortDescription() const
{
  return shortDescription_;
}


const SimpleLatex& ResultElement::longDescription() const
{
  return longDescription_;
}


const SimpleLatex& ResultElement::unit() const
{
  return unit_;
}


void ResultElement::insertLatexHeaderCode ( std::set<std::string>& ) const
{}

void ResultElement::writeLatexCode ( ostream& , const std::string& , int , const boost::filesystem::path&  ) const
{
}

void ResultElement::exportDataToFile ( const std::string& , const boost::filesystem::path&  ) const
{
}


rapidxml::xml_node< char >* ResultElement::appendToNode
(
    const string& name,
    rapidxml::xml_document< char >& doc,
    rapidxml::xml_node< char >& node
) const
{
    using namespace rapidxml;
    xml_node<>* child = doc.allocate_node ( node_element, doc.allocate_string ( this->type().c_str() ) );
    node.append_node ( child );
    child->append_attribute ( doc.allocate_attribute
                              (
                                  "name",
                                  doc.allocate_string ( name.c_str() ) )
                            );
//   child->append_attribute(doc.allocate_attribute
//   (
//     "type",
//     doc.allocate_string( type().c_str() ))
//   );

    child->append_attribute ( doc.allocate_attribute
                              (
                                  "shortDescription",
                                  doc.allocate_string ( shortDescription_.simpleLatex().c_str() ) )
                            );
    child->append_attribute ( doc.allocate_attribute
                              (
                                  "longDescription",
                                  doc.allocate_string ( longDescription_.simpleLatex().c_str() ) )
                            );
    child->append_attribute ( doc.allocate_attribute
                              (
                                  "unit",
                                  doc.allocate_string ( unit_.simpleLatex().c_str() ) )
                            );
    child->append_attribute ( doc.allocate_attribute
                              (
                                  "order",
                                  doc.allocate_string ( str ( format ( "%g" ) % order_ ).c_str() ) )
                            );

    return child;
}

void ResultElement::readBaseAttributesFromNode(
    const string &name, const rapidxml::xml_node<> &node)
{
  shortDescription_=SimpleLatex(node.first_attribute("shortDescription")->value());
  longDescription_=SimpleLatex(node.first_attribute("longDescription")->value());
  unit_=SimpleLatex(node.first_attribute("unit")->value());
  order_=boost::lexical_cast<double>(node.first_attribute("order")->value());
}

void ResultElement::readFromNode ( const string& name, const rapidxml::xml_node< >& )
{
  insight::Warning("Not implemented: restoring result from XML file is not implemented for result element of type "+type()+" (appeared in node "+name+")");
}


std::unique_ptr<Parameter> ResultElement::convertIntoParameter() const
{
    return nullptr;
}



} // namespace insight
