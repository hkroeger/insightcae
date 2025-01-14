#include "selectablesubsetgenerator.h"
#include "boost/algorithm/string/erase.hpp"
#include "boost/algorithm/string/join.hpp"
#include "parametergenerator.h"
#include "subsetgenerator.h"
#include "includedsubsetparameterparser.h"
#include <functional>
#include <iterator>
#include <memory>

using namespace std;

defineType(SelectableSubsetGenerator);
addToStaticFunctionTable(ParameterGenerator, SelectableSubsetGenerator, insertrule);


SelectableSubsetGenerator
::SelectableSubsetGenerator (
    const SubsetListData& values,
    const std::string& defaultSel,
    const std::string& d )
    : ParameterGenerator ( d ), default_sel(nullptr)
{
    for (auto&v: values)
    {

        auto key=boost::fusion::get<0>(v);
        auto sd = boost::fusion::get<1>(v);

        sd->setName(key);
        value.push_back(sd);

        if (key==defaultSel)
        {
            default_sel=sd.get();
        }
    }

    if (!default_sel)
    {
        throw PDLException("no valid default selection: "+defaultSel);
    }

}




void SelectableSubsetGenerator::setName(const std::string &name)
{
    for (auto&v: value)
    {
        v->setName(name + "_" + v->name);
    }
    ParameterGenerator::setName(name);
}

void SelectableSubsetGenerator::setPath(const std::string &containerPath)
{
    ParameterGenerator::setPath(containerPath);

    for (auto& pe: value)
    {
        pe->setPath(parameterPath);
    }
}




void SelectableSubsetGenerator::cppAddRequiredInclude (
    std::set<std::string>& headers ) const
{
    headers.insert ( "<boost/variant.hpp>" );
    headers.insert("\"base/parameters/selectablesubsetparameter.h\"");
    for ( const auto& sd: value )
    {
        sd->cppAddRequiredInclude ( headers );
    }
}




std::string SelectableSubsetGenerator::cppInsightType () const
{
    return "insight::SelectableSubsetParameter";
}




std::string SelectableSubsetGenerator::cppStaticType () const
{
    std::vector<std::string> variants;

    std::transform(
        value.begin(), value.end(),
        std::back_inserter(variants),
        [](const SubsetData&v) { return v->cppTypeName(); }
        );

    return "boost::variant< "+boost::join(variants, ",")+" >";
}




std::string SelectableSubsetGenerator::cppDefaultValueExpression() const
{
    return extendtype(thisscope, default_sel->name + "_type()");
}




void SelectableSubsetGenerator::writeCppTypeDecl(
    std::ostream& os ) const
{
    // write one additional typedef for each selectable subset
    for ( const auto& v: value )
    {
        v->writeCppTypeDecl( os );
    }

    ParameterGenerator::writeCppTypeDecl( os );
}








ParameterGenerator* getSubset(
    const SelectableSubsetGenerator::SubsetData& e, bool* empty=nullptr)
{
    ParameterGenerator* pdb = nullptr;

    if (auto pd =
            std::dynamic_pointer_cast<SubsetGenerator>(e)) // should be a set
    {
        pdb=pd.get();
        if (empty) *empty = pd->value.size()==0;
    }
    else if (auto pd =
             std::dynamic_pointer_cast<IncludedSubsetGenerator>(e))
    {
        pdb=pd.get();
        if (empty) *empty = false;
    }

    if (!pdb)
        throw PDLException("selectablesubset should contain only sets!");

    return pdb;
}




void SelectableSubsetGenerator::cppWriteCreateStatement
(
    std::ostream& os,
    const std::string& psvarname
) const
{

    os<<"std::unique_ptr< "<<cppInsightType () <<" > "<<psvarname<<";"<<endl;
    os<<"{"<<endl;
    os<<"insight::SelectableSubsetParameter::EntryReferences "<<psvarname<<"_selection;"<<endl;
    for ( auto& v: value )
    {
        const std::string& sel_name
            = boost::erase_first_copy( v->name, name+"_");

          v->cppWriteCreateStatement
          (
              os, v->name /*, extendtype(thisscope, v->name+"_type")*/
          );
        os<<psvarname<<"_selection.insert({\""<<sel_name<<"\", "<<v->name<<".get()});"<<endl;

    }

    const std::string& defl_sel_name
        = boost::erase_first_copy( default_sel->name, name+"_");

    os<<psvarname<<".reset(new "<<cppInsightType () <<"(\""
       <<defl_sel_name<<"\", "<<psvarname<<"_selection, "
       << cppInsightTypeConstructorParameters()
       <<")); "<<endl;

    os<<"}\n";
}




void SelectableSubsetGenerator::cppWriteSetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{

    os<<"{"<<endl;

    for ( auto&v: value ) {

        const std::string& sel_name=
            boost::erase_first_copy( v->name, name+"_");

        bool emptyset;
        auto pd = getSubset( v, &emptyset );

        std::string seliname=v->name;
        os<<"if ( ";
        if (!emptyset)
        {
            os <<"const auto* "<<seliname<<"_static = ";
        }
         os << "boost::get< "<<extendtype ( thisscope, pd->cppTypeName() ) <<" >(&"<< staticname <<")"
         ") {\n";

         os <<varname<<".setSelection(\""<<sel_name<<"\");\n";
         if (!emptyset)
         {
            os << "ParameterSet& "<<seliname<<"_param = "<<varname<<"();\n";
            pd->cppWriteSetStatement ( os, seliname+"_param", "(*"+seliname+"_static)" );
         }
        os<<"}"<<endl;
    }
    os<<"}"<<endl;
}

void SelectableSubsetGenerator::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
    os<<"{\n";
    for ( auto& sd: value )
    {
        const std::string& sel_name=
            boost::erase_first_copy( sd->name, name+"_");

        bool emptyset;
        auto pd=getSubset( sd, &emptyset );

        std::string seliname=sd->name;

        os << "if ("<<varname<<".selection() == \""<<sel_name<<"\")\n"
           << "{\n";

        os << extendtype(thisscope, pd->cppTypeName()) <<" "<< seliname<<"_static;\n";        
        os<<"const ParameterSet& "<<seliname<<"_param = "<<varname<<"();\n";
        if (!emptyset)
        {
            os << seliname<< "_static.get("<<seliname<<"_param);\n";
        }
        os <<staticname<< "="<<seliname<<"_static;\n"
           <<staticname<< ".setPath( "<<seliname<<"_param.path() );\n";

        os<<"}\n";
    }
    os<<"}\n";
}
