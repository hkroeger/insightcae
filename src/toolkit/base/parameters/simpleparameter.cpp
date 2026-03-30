#include "simpleparameter.h"

namespace insight
{

char VectorBaseName[] = "vectorBase";
char VectorName[] = "vector";


VectorParameter::VectorParameter (
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
    : SimpleParameter ( description, isHidden, isExpert, isNecessary, order ),
    vectorType_(Point)
{}

VectorParameter::VectorParameter (
    const arma::mat& value, const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
    : SimpleParameter ( value, description, isHidden, isExpert, isNecessary, order ),
    vectorType_(Point)
{}

VectorParameter::VectorParameter (
    VectorType vt, const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order )
    : SimpleParameter ( description, isHidden, isExpert, isNecessary, order ),
    vectorType_(vt)
{}

VectorParameter::VectorParameter (
    VectorType vt, const arma::mat& value,
    const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order )
    : SimpleParameter ( value, description, isHidden, isExpert, isNecessary, order ),
    vectorType_(vt)
{}

rapidxml::xml_node<> *VectorParameter::appendToNode(
    const std::string &name,
    rapidxml::xml_document<> &doc,
    rapidxml::xml_node<> &node,
    const OutputProperties& outProps) const
{
    auto c=SimpleParameter::appendToNode(name, doc, node, outProps);
    appendAttribute(doc, *c, "vectorType", vectorType());
    return c;
}

VectorParameter::VectorParameter(const rapidxml::xml_node<> &node, bool skipValueRead)
    : SimpleParameter(node, skipValueRead)
{
    vectorType_=VectorType(getMandatoryAttribute<int>(node, "vectorType"));
}

std::unique_ptr<hierarchicalData::Element> VectorParameter::cloneUninitialized() const
{
    auto p= std::make_unique<VectorParameter>(
        vectorType_, value_,
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order() );
    return p;
}


char DoubleName[] = "double";
char IntName[] = "int";
char BoolName[] = "bool";
char StringName[] = "string";
char DateName[] = "date";
char DateTimeName[] = "datetime";






template<> defineType(DoubleParameter);
template<> defineType(IntParameter);
template<> defineType(BoolParameter);
typedef  SimpleParameter<arma::mat, VectorBaseName> VectorParameterBase;
template<> defineType(VectorParameterBase);
template<> defineType(StringParameter);
template<> defineType(DateParameter);
template<> defineType(DateTimeParameter);
//typedef SimpleParameter<boost::filesystem::path, PathName> PathParameterBase;
//template<> defineType(PathParameterBase);

defineType(VectorParameter);


template<>
bool BoolParameter::isBooleanData() const
{
    return true;
}

template<>
bool BoolParameter::canSetFromBoolean() const
{
    return true;
}

template<>
bool BoolParameter::getAsBoolean() const
{
    return operator()();
}

template<>
void BoolParameter::setBoolean(bool b)
{
    set(b);
}


addParameterFactories(DoubleParameter);
addParameterFactories(IntParameter);
addParameterFactories(BoolParameter);
addParameterFactories(VectorParameter);
addParameterFactories(StringParameter);
addParameterFactories(DateParameter);
addParameterFactories(DateTimeParameter);


}
