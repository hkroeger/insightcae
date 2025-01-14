#include "labeledarraygenerator.h"

using namespace std;


defineType(LabeledArrayGenerator);
addToStaticFunctionTable(ParameterGenerator, LabeledArrayGenerator, insertrule);



LabeledArrayGenerator::LabeledArrayGenerator(
    const std::string& pok,
    ArgType at,
    ParameterGeneratorPtr v,
    size_t n,
    const std::string& d)
: ParameterGenerator(d),
    patternOrKeysPath(pok),
    argType(at),
    value(v),
    num(n)
{}



void LabeledArrayGenerator::setName(const std::string &name)
{
    ParameterGenerator::setName(name);
    value->setName(name+"_default");
}




void LabeledArrayGenerator::cppAddRequiredInclude(
    std::set<std::string>& headers ) const
{
    headers.insert("<map>");
    headers.insert("\"base/parameters/labeledarrayparameter.h\"");
    value->cppAddRequiredInclude(headers);
}




std::string LabeledArrayGenerator::cppInsightType() const
{
    return "insight::LabeledArrayParameter";
}




std::string LabeledArrayGenerator::cppStaticType( ) const
{
    return "std::map<std::string, StaticValueWrap<"+value->cppTypeName()+"> >";
}




std::string LabeledArrayGenerator::cppDefaultValueExpression() const
{
    std::ostringstream rep;
    rep << "{\n";

    if (argType==Pattern)
    {
        auto pattern = patternOrKeysPath;
        for (size_t i=0; i<num; i++)
        {
            if (i>0) rep<<",";
            rep << "{\n";
            rep << "\""<< str(boost::format(pattern) % i) << "\", ";
            rep << value->cppConstructorParameters() ;
            rep << "}\n";
        }
    }
    rep << "}\n";
    return rep.str();
}




void LabeledArrayGenerator::writeCppTypeDecl(
    std::ostream& os ) const
{
    value->writeCppTypeDecl ( os );

    ParameterGenerator::writeCppTypeDecl ( os );
}



void LabeledArrayGenerator::cppWriteCreateStatement
    (
        std::ostream& os,
    const std::string& psvarname
        ) const
{

    os<<"auto "<<psvarname<<" = std::make_unique< "<<cppInsightType()<<" >( "
       <<cppInsightTypeConstructorParameters()<<");\n"
       <<"{\n";

    if (argType==Pattern)
    {
        auto pattern = patternOrKeysPath;
        os<<psvarname<<"->setLabelPattern(\""<<pattern<<"\");";
    }
    else if (argType==KeysPath)
    {
        auto keyPath = patternOrKeysPath;
        os<<psvarname<<"->setKeySourceParameterPath(\""<<keyPath<<"\");";
    }

    value->cppWriteCreateStatement(os, name+"_default_value");

    os<<psvarname<<"->setDefaultValue(*"<<name<<"_default_value);\n";
    if (num>0)
    {
        os<<"for (size_t i=0; i<"<<num<<"; i++) "
           <<psvarname<<"->appendEmpty();\n";
    }
    os<<"}\n";
}




void LabeledArrayGenerator::cppWriteSetStatement(
    std::ostream& os,
    const std::string& dynvarname,
    const std::string& staticvarname
    ) const
{
    // insert/update new entries
    os << "for (auto iii: "<<staticvarname<<")\n"
       << "{\n"
       << "auto& "<<dynvarname <<"_cur = "
           "dynamic_cast< "<< value->cppInsightType() <<"& >"
              "("<<dynvarname<<".getOrInsertDefaultValue(iii.first));\n"

           "const auto& "<<dynvarname<<"_cur_static = iii.second;\n";

           value->cppWriteSetStatement(os, dynvarname+"_cur", dynvarname+"_cur_static");

    os << "}\n";

    // remove vanished
    os<<
        "for (int iii="<<dynvarname<<".nChildren()-1; iii>=0; --iii)\n"
        "{\n"
          "auto key="<<dynvarname<<".childParameterName(iii);\n"
          "if ("<<staticvarname<<".count(key)==0)"
            <<dynvarname<<".eraseValue(key);\n"
        "}\n";
}




void LabeledArrayGenerator::cppWriteGetStatement(
    std::ostream& os,
    const std::string& dynvarname,
    const std::string& staticname
    ) const
{
    os << staticname<<".clear();\n"
       << staticname<< ".setPath( "<<dynvarname<<" .path());\n"
       << "for(int k=0; k<"<<dynvarname<<".size(); k++)\n"
       << "{\n"
       <<  "const auto& "<<dynvarname<<"_cur = "
            "dynamic_cast<const "<< value->cppInsightType() <<"& >"
             "("<<dynvarname<<".childParameter(k));\n"
       <<  "const auto& label="<<dynvarname<<".childParameterName(k);\n"
       <<  "auto& "<<dynvarname<<"_cur_static = "<<staticname<<"[label];\n";
       // <<  dynvarname<< "_cur_static.setPath( "<<dynvarname<<"_cur.path() );\n"
       // <<  dynvarname<< "_cur_static.get("<<dynvarname<<"_cur());\n"
       value->cppWriteGetStatement(os, dynvarname+"_cur", dynvarname+"_cur_static");
    os << "}\n";
}

