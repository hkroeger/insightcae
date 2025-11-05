#include "resultelement.h"
#include "base/hierarchicalelement.h"
#include "base/rapidxml.h"


using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;


namespace insight {







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
    : Element(0),
      shortDescription_ ( shortdesc ),
      longDescription_ ( longdesc ),
      unit_ ( unit )
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




void ResultElement::exportDataToFile ( const std::string& , const boost::filesystem::path&  ) const
{
}


rapidxml::xml_node< char >*
ResultElement::appendToNode
(
    const string& name,
    rapidxml::xml_document< char >& doc,
    rapidxml::xml_node< char >& node
) const
{
    auto child = Element::appendToNode(name, doc, node);

    appendAttribute(doc, *child, "shortDescription", shortDescription_.simpleLatex());
    appendAttribute(doc, *child, "longDescription", longDescription_.simpleLatex());
    appendAttribute(doc, *child, "unit", unit_.simpleLatex());

    return child;
}



const rapidxml::xml_node<>* ResultElement::readFromNode
    (
        const std::string& name,
        const rapidxml::xml_node<>& parentNode
        )
{
    auto *node=hierarchicalData::Element::readFromNode(name, parentNode);
    shortDescription_=SimpleLatex(getMandatoryAttribute(*node, "shortDescription"));
    longDescription_=SimpleLatex(getMandatoryAttribute(*node, "longDescription"));
    unit_=SimpleLatex(getMandatoryAttribute(*node, "unit"));
    return node;
}


std::unique_ptr<Parameter> ResultElement::convertIntoParameter() const
{
    return nullptr;
}

bool ResultElement::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const ResultElement*>(&op))
    {
        if (shortDescription_!=oa->shortDescription_)
            return false;
        if (longDescription_!=oa->longDescription_)
            return false;
        if (unit_!=oa->unit_)
            return false;
        return true;
    }
    else
        return false;
}



} // namespace insight
