#include "spatialtransformationparameter.h"
#include "base/rapidxml.h"

namespace insight {




defineType(SpatialTransformationParameter);
addToFactoryTable(Parameter, SpatialTransformationParameter);




SpatialTransformationParameter::SpatialTransformationParameter(
        const std::string& description,
        bool isHidden,
        bool isExpert,
        bool isNecessary,
        int order )
    : Parameter(description, isHidden, isExpert, isNecessary, order)
{}




SpatialTransformationParameter::SpatialTransformationParameter(
        const SpatialTransformation& trsf,
        const std::string& description,
        bool isHidden,
        bool isExpert,
        bool isNecessary,
        int order )
    : Parameter(description, isHidden, isExpert, isNecessary, order),
      SpatialTransformation(trsf)
{}




bool SpatialTransformationParameter::isDifferent(const Parameter& p) const
{
    if (const auto* mp =
            dynamic_cast<const SpatialTransformationParameter*>(
                &p ) )
    {
      return (*mp) != (*this);
    }
    else
      return true;
}

std::string SpatialTransformationParameter::latexRepresentation() const
{
    std::ostringstream os;

    os << "\\begin{enumeration}\n";

    os << "\\item translate by\n";
    os << "$\\left("
        <<translate()[0]
        <<"~~"
        <<translate()[1]
        <<"~~"
        <<translate()[2]
        <<" \\right)^T$\n";

    auto rpy=rollPitchYaw();
    os << "\\item rotate\n";
    os << "\\begin{enumeration}\n";
    os << "\\item around x (roll) by $"<<rpy[0]<<"^\\circ$\n";
    os << "\\item around y (pitch) by $"<<rpy[1]<<"^\\circ$\n";
    os << "\\item around z (yaw) by $"<<rpy[2]<<"^\\circ$\n";
    os << "\\end{enumeration}\n";

    os << "\\item scale by "<<scale()<<"\n";

    os << "\\end{enumeration}\n";

    return os.str();
}




std::string SpatialTransformationParameter::plainTextRepresentation(int indent) const
{
    std::ostringstream os;

    os << "1. translate by ( "
        <<translate()[0]
        <<" "
        <<translate()[1]
        <<" "
        <<translate()[2]
        <<" )^T\n";

    auto rpy=rollPitchYaw();
    os << "2. rotate by\n";
    os << " a) around x (roll) by "<<rpy[0]<<"deg\n";
    os << " b) around y (pitch) by "<<rpy[1]<<"deg\n";
    os << " c) around z (yaw) by "<<rpy[2]<<"deg\n";

    os << "3. scale by "<<scale()<<"\n";

    return os.str();
}




rapidxml::xml_node<>* SpatialTransformationParameter::appendToNode (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        boost::filesystem::path inputfilepath ) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);

    xml_node<>* translateNode = doc.allocate_node(
                node_element,
                doc.allocate_string("translate"));
    child->append_node(translateNode);
    writeMatToXMLNode(translate(), doc, *translateNode);

    xml_node<>* RNode = doc.allocate_node(
                node_element,
                doc.allocate_string("rotationMatrix"));
    child->append_node(RNode);
    writeMatToXMLNode(R(), doc, *RNode);

    child->append_attribute
    (
        doc.allocate_attribute
        (
            "scale",
            doc.allocate_string ( valueToString ( scale() ).c_str() )
        )
    );

    return child;
}




void SpatialTransformationParameter::readFromNode (
        const std::string& name,
        const rapidxml::xml_node<>& node,
        boost::filesystem::path inputfilepath )
{
    using namespace rapidxml;
    auto* child = findNode(node, name, type());
    if (child)
    {
        {
            auto *translateNode = child->first_node("translate");
            insight::assertion(translateNode!=nullptr, "no node labelled \"translate\" found in "+name+"!");
            std::string value_str=translateNode->value();
            std::istringstream iss(value_str);
            translate_.load(iss, arma::raw_ascii);
        }
        {
            auto *RNode = child->first_node("rotationMatrix");
            insight::assertion(RNode!=nullptr, "no node labelled \"rotationMatrix\" found in "+name+"!");
            std::string value_str=RNode->value();
            std::istringstream iss(value_str);
            R_.load(iss, arma::raw_ascii);
        }

        {
            auto scaleAttr=child->first_attribute ( "scale" );
            insight::assertion(scaleAttr, "No attribute \"scale\" present in "+name+"!");
            stringToValue ( scaleAttr->value(), scale_ );
        }
        triggerValueChanged();
    }
    else
    {
      insight::Warning(
            boost::str(
              boost::format(
               "No xml node found with type '%s' and name '%s', default value '%s' is used."
               ) % type() % name % plainTextRepresentation()
             )
          );
    }
}




std::unique_ptr<Parameter>
SpatialTransformationParameter::clone(bool init) const
{
    auto p= std::make_unique<SpatialTransformationParameter>(
        operator()(), description().simpleLatex(),
        isHidden(), isNecessary(), isExpert(), order()
    );
    if (init) p->initialize();
    return p;
}




void SpatialTransformationParameter::copyFrom(const Parameter& p)
{
    operator=(dynamic_cast<const SpatialTransformationParameter&>(p));

}




void SpatialTransformationParameter::operator=(const SpatialTransformationParameter& op)
{
    SpatialTransformation::operator=(op);

    Parameter::copyFrom(op);
}




int SpatialTransformationParameter::nChildren() const
{
    return 0;
}




} // namespace insight
