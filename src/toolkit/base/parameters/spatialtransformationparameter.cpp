#include "spatialtransformationparameter.h"
#include "base/rapidxml.h"
#include "base/spatialtransformation.h"

namespace insight {




defineType(SpatialTransformationParameter);
addParameterFactories(SpatialTransformationParameter);






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

std::string SpatialTransformationParameter::latexRepresentation(
    const std::string&,
    int,
    const FileStorageInfo& ) const
{
    std::ostringstream os;

    os << "\\begin{enumerate}\n";

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
    os << "\\begin{enumerate}\n";
    os << "\\item around x (roll) by $"<<rpy[0]<<"^\\circ$\n";
    os << "\\item around y (pitch) by $"<<rpy[1]<<"^\\circ$\n";
    os << "\\item around z (yaw) by $"<<rpy[2]<<"^\\circ$\n";
    os << "\\end{enumerate}\n";

    os << "\\item scale by "<<scale()<<"\n";

    os << "\\end{enumerate}\n";

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
    const OutputProperties& outProps ) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, outProps);

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

    appendAttribute(doc, *child, "scale", toString ( scale() ));

    return child;
}




const rapidxml::xml_node<>*
SpatialTransformationParameter::readFromNode (
        const std::string& name,
        const rapidxml::xml_node<>& node )
{
    using namespace rapidxml;
    auto* child = Parameter::readFromNode(name, node);
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

        scale_=getMandatoryAttribute<double>(*child, "scale" );

        triggerValueChanged();
    }
    else
    {
      insight::Warning(
            boost::str(
              boost::format(
               "No xml node found with type '%s' and name '%s', default value '%s' is used."
               ) % type() % name % plainTextRepresentation(0)
             )
          );
    }
    return child;
}



SpatialTransformationParameter::SpatialTransformationParameter(
    const rapidxml::xml_node<> &node)
    : Parameter(node)
{
    {
        auto *translateNode = node.first_node("translate");
        insight::assertion(translateNode!=nullptr, "no node labelled \"translate\" found!");
        std::string value_str=translateNode->value();
        std::istringstream iss(value_str);
        translate_.load(iss, arma::raw_ascii);
    }
    {
        auto *RNode = node.first_node("rotationMatrix");
        insight::assertion(RNode!=nullptr, "no node labelled \"rotationMatrix\" found!");
        std::string value_str=RNode->value();
        std::istringstream iss(value_str);
        R_.load(iss, arma::raw_ascii);
    }

    scale_=getMandatoryAttribute<double>(node, "scale" );

    triggerValueChanged();
}


std::unique_ptr<hierarchicalData::Element>
SpatialTransformationParameter::clone() const
{
    auto p= std::make_unique<SpatialTransformationParameter>(
        operator()(), description().simpleLatex(),
        isHidden(), isNecessary(), isExpert(), order()
    );
    return p;
}





void SpatialTransformationParameter::assignFrom(const Element& e)
{
    auto &op=dynamic_cast<const SpatialTransformationParameter&>(e);

    SpatialTransformation::operator=(op);

    Parameter::assignFrom(op);
}




bool SpatialTransformationParameter::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const SpatialTransformationParameter*>(&op))
    {
        return SpatialTransformation::operator==(*oa);
    }
    else
        return false;
}




int SpatialTransformationParameter::nChildren() const
{
    return 0;
}




} // namespace insight
