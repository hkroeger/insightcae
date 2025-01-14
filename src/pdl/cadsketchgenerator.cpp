#include "cadsketchgenerator.h"

#include "boost/range/adaptors.hpp"

defineType(CADSketchGenerator);
addToStaticFunctionTable(ParameterGenerator, CADSketchGenerator, insertrule);


CADSketchGenerator::Data::Data(
    const std::string& script,
    const std::string& EntityPropertiesDelegateSharedPtrExpression,
    const std::string& SketchPresentationLookupKeyExpression,
    const std::vector<boost::fusion::vector2<int, std::string> >& references,
    const std::string& d)
    : ParameterGenerator(d),
    EntityPropertiesDelegateSharedPtrExpression_(
          EntityPropertiesDelegateSharedPtrExpression),
    SketchPresentationLookupKeyExpression_(
        SketchPresentationLookupKeyExpression),
    script_(script),
    references_(references)
{}


std::string CADSketchGenerator::Data::refParameter() const
{
    std::string r="insight::CADSketchParameter::References{";
    for (auto ref: boost::adaptors::index(references_))
    {
        if (ref.index()>0) r+=", ";
        r+=str(boost::format("{%d, \"%s\"}")
                 % boost::fusion::get<0>(ref.value())
                 % boost::fusion::get<1>(ref.value()) );
    }
    r+="}";
    return r;
}

void CADSketchGenerator::Data::cppAddRequiredInclude(std::set< std::string >& headers) const
{
    headers.insert("<memory>");
    headers.insert("\"cadsketchparameter.h\"");
}

std::string CADSketchGenerator::Data::cppInsightType() const
{
    return "insight::CADSketchParameter";
}

std::string CADSketchGenerator::Data::cppStaticType() const
{
    return "std::observer_ptr<const insight::CADSketchParameter>";
}


std::string CADSketchGenerator::Data::cppDefaultValueExpression() const
{
    std::string epd(EntityPropertiesDelegateSharedPtrExpression_);
    if (epd.empty()) epd="nullptr";

    std::string sple("\""+SketchPresentationLookupKeyExpression_+"\"");
    if (SketchPresentationLookupKeyExpression_.size()>1)
    {
        if (
            (SketchPresentationLookupKeyExpression_.front()=='{')
            &&
            (SketchPresentationLookupKeyExpression_.back()=='}') )
        {
            sple=SketchPresentationLookupKeyExpression_.substr(
                1,
                SketchPresentationLookupKeyExpression_.size()-2 );
        }
    }

    return
        "\""+ script_ + "\",\n"
           + epd + ",\n"
           + sple + ",\n"
           + refParameter();
}

std::string CADSketchGenerator::Data::cppConstructorParameters() const
{
    return cppStaticType()+"(new "
           + cppInsightType()+"("
           + cppDefaultValueExpression() + ",\n"
           + cppInsightTypeConstructorParameters()
           +"))";
}




/**
 * write the code to
 * transfer the values form the static c++ struct into the dynamic parameter set
 */
void CADSketchGenerator::Data::cppWriteSetStatement
    (
        std::ostream& os,
        const std::string& varname,
        const std::string& staticname
        ) const
{
    os<<varname<<" = *"<<staticname<<";\n";
}

/**
 * write the code to
 * transfer values from the dynamic parameter set into the static c++ data structure
 */
void CADSketchGenerator::Data::cppWriteGetStatement
    (
        std::ostream& os,
        const std::string& varname,
        const std::string& staticname
        ) const
{
    os <<staticname<< "= &"<<varname<<";\n"
       <<staticname<< ".setPath( "<<varname<<" .path());\n";
}

