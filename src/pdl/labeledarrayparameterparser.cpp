#include "labeledarrayparameterparser.h"

using namespace std;


defineType(LabeledArrayParameterParser);
addToStaticFunctionTable(ParserDataBase, LabeledArrayParameterParser, insertrule);



LabeledArrayParameterParser::Data::Data(
    const std::string& pok, ArgType at, ParserDataBase::Ptr v, size_t n, const std::string& d)
    : ParserDataBase(d), patternOrKeysPath(pok), argType(at), value(v), num(n)
{}

void LabeledArrayParameterParser::Data::cppAddHeader(
    std::set<std::string>& headers ) const
{
    headers.insert("<map>");
    headers.insert("\"base/parameters/labeledarrayparameter.h\"");
    value->cppAddHeader(headers);
}

std::string LabeledArrayParameterParser::Data::cppValueRep(
    const std::string& name,
    const std::string& thisscope ) const
{
    std::ostringstream rep;
    rep << "{";

    if (argType==Pattern)
    {
        auto pattern = patternOrKeysPath;
        for (size_t i=0; i<num; i++)
        {
            if (i>0) rep<<",";
            rep << "{";
            rep << "\""<< str(boost::format(pattern) % i) << "\", ";
            rep << value->cppConstructorParameters(name+"_default", thisscope) ;
            rep << "}";
        }
    }
    rep << "}";
    return rep.str();
}

std::string LabeledArrayParameterParser::Data::cppType(
    const std::string& name ) const
{
    return "std::map<std::string, "+value->cppTypeName(name+"_default")+">";
}

std::string LabeledArrayParameterParser::Data::cppTypeDecl (
    const std::string& name,
    const std::string& thisscope ) const
{
    return
        value->cppTypeDecl ( name+"_default", thisscope )
        +"\n"
        +ParserDataBase::cppTypeDecl ( name, thisscope );
}

std::string LabeledArrayParameterParser::Data::cppParamType(
    const std::string& ) const
{
    return "insight::LabeledArrayParameter";
}

void LabeledArrayParameterParser::Data::cppWriteCreateStatement
    (
        std::ostream& os,
        const std::string& name,
        const std::string& thisscope
        ) const
{

    os<<"std::unique_ptr< "<<cppParamType(name)<<" > "
       <<name<<"(new "<<cppParamType(name)<<"(\""<<description<<"\")); "<<endl;

    os<<"{"<<endl;

    if (argType==Pattern)
    {
        auto pattern = patternOrKeysPath;
        os<<name<<"->setLabelPattern(\""<<pattern<<"\");";
    }
    else if (argType==KeysPath)
    {
        auto keyPath = patternOrKeysPath;
        os<<name<<"->setKeySourceParameterPath(\""<<keyPath<<"\");";
    }

    value->cppWriteCreateStatement
        (
            os, name+"_default_value", thisscope
            );
    os<<name<<"->setDefaultValue(*"<<name<<"_default_value);"<<endl;
    if (num>0)
    {
        os<<"for (size_t i=0; i<"<<num<<"; i++) "
           <<name<<"->appendEmpty();"<<endl;
    }
    os<<"}"<<endl;
}

void LabeledArrayParameterParser::Data::cppWriteSetStatement(
    std::ostream& os,
    const std::string& name,
    const std::string& dynvarname,
    const std::string& staticvarname,
    const std::string& thisscope
    ) const
{
    // insert/update new entries
    os<<
        "for (auto iii: "<<staticvarname<<")"
        "{"

          "auto& "<<dynvarname <<"_cur = "
           "dynamic_cast< "<< value->cppParamType(name+"_default") <<"& >"
           "("<<dynvarname<<".getOrInsertDefaultValue(iii.first));"

           "const auto& "<<dynvarname<<"_cur_static = iii.second;";

           value->cppWriteSetStatement(os, name+"_default", name+"_cur", name+"_cur_static", thisscope);

    os<<"}\n";

    // remove vanished
    os<<
        "for (int iii="<<dynvarname<<".nChildren()-1; iii>=0; --iii)"
        "{"
          "auto key="<<dynvarname<<".childParameterName(iii);"
          "if ("<<staticvarname<<".count(key)==0)"
            <<dynvarname<<".eraseValue(key);"
        "}\n";
}

void LabeledArrayParameterParser::Data::cppWriteGetStatement(
    std::ostream& os,
    const std::string& name,
    const std::string& dynvarname,
    const std::string& staticname,
    const std::string& thisscope
    ) const
{
    os<<staticname<<".clear();"<<endl;
    os<<"for(int k=0; k<"<<dynvarname<<".size(); k++)"<<endl;
    os<<"{"<<endl;
    os<<"const auto& "<<dynvarname<<"_cur = "
         "dynamic_cast<const "<< value->cppParamType(name+"_default") <<"& >"
         "("<<dynvarname<<".childParameter(k));"<<endl;
    os<<"const auto& label="<<dynvarname<<".childParameterName(k);"<<endl;
    os<<"auto& "<<dynvarname<<"_cur_static = "<<staticname<<"[label];"<<endl;

    value->cppWriteGetStatement(os, name+"_default", name+"_cur", name+"_cur_static", thisscope);

    os<<"}"<<endl;
}

