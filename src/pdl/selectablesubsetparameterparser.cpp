#include "selectablesubsetparameterparser.h"
#include "subsetparameterparser.h"
#include "includedsubsetparameterparser.h"

using namespace std;

SelectableSubsetParameterParser::Data::Data ( const SubsetListData& v, const std::string& ds, const std::string& d )
    : ParserDataBase ( d ), default_sel ( ds ), value ( v )
{}

void SelectableSubsetParameterParser::Data::cppAddHeader ( std::set<std::string>& headers ) const
{
    headers.insert ( "<boost/variant.hpp>" );
    headers.insert("\"base/parameters/selectablesubsetparameter.h\"");
    for ( const SubsetData& pe: value ) {
        boost::fusion::get<1> ( pe )->cppAddHeader ( headers );
    }
}

std::string SelectableSubsetParameterParser::Data::cppType ( const std::string& name ) const
{
    std::ostringstream os;
    os<<"boost::variant<";
    std::string comma="";
    for ( const SubsetData& pe: value ) {
        os<<comma<< ( name+"_"+boost::fusion::get<0> ( pe )+"_type" );
        comma=",";
    }
    os<<">";
    return os.str();
}

std::string SelectableSubsetParameterParser::Data::cppValueRep ( const std::string&  name, const std::string& thisscope) const
{
    return extendtype(thisscope, name + "_" + default_sel+"_type()");
}

std::string SelectableSubsetParameterParser::Data::cppTypeDecl ( const std::string& name,
                                                                 const std::string& thisscope ) const
{
    std::ostringstream os;
    for ( const SubsetData& pe: value )
    {
        std::string tname= ( name+"_"+boost::fusion::get<0> ( pe ) );
        ParserDataBase::Ptr pd=boost::fusion::get<1> ( pe );
        os<<pd->cppTypeDecl ( tname, thisscope ) <<endl;
    }
    return
        os.str()+"\n"
        +ParserDataBase::cppTypeDecl ( name, thisscope );
}

std::string SelectableSubsetParameterParser::Data::cppParamType ( const std::string&  ) const
{
    return "insight::SelectableSubsetParameter";
}

ParserDataBase* getSubset(ParserDataBase::Ptr e, bool* empty=nullptr)
{
    ParserDataBase* pdb = nullptr;

    if (SubsetParameterParser::Data *pd =
            dynamic_cast<SubsetParameterParser::Data*>(e.get())) // should be a set
    {
        pdb=pd;
        if (empty) *empty = pd->value.size()==0;
    }
    else if (IncludedSubsetParameterParser::Data* pd =
             dynamic_cast<IncludedSubsetParameterParser::Data*>(e.get()))
    {
        pdb=pd;
        if (empty) *empty = false;
    }

    if (!pdb)
        throw PDLException("selectablesubset should contain only sets!");

    return pdb;
}

void SelectableSubsetParameterParser::Data::cppWriteCreateStatement
(
    std::ostream& os,
    const std::string& name,
    const std::string& thisscope
) const
{

    os<<"std::unique_ptr< "<<cppParamType ( name ) <<" > "<<name<<";"<<endl;
    os<<"{"<<endl;
    os<<"insight::SelectableSubsetParameter::SubsetList "<<name<<"_selection;"<<endl;
    for ( const SubsetData& sd: value ) {
        const std::string& sel_name=boost::fusion::get<0> ( sd );

        auto pd = getSubset( boost::fusion::get<1>(sd) );

        os<<"{"<<endl;
          pd->cppWriteCreateStatement
          (
              os, sel_name, extendtype(thisscope, name+"_"+sel_name+"_type")
          );
        os<<name<<"_selection.push_back(insight::SelectableSubsetParameter::SingleSubset(\""<<sel_name<<"\", "<<sel_name<<".release()));"<<endl;
        os<<"}"<<endl;
    }
    os<<name<<".reset(new "<<cppParamType ( name ) <<"(\""<<default_sel<<"\", "<<name<<"_selection, \""<<description<<"\", "
        << (isHidden?"true":"false")<<","
        << (isExpert?"true":"false")<<","
        << (isNecessary?"true":"false")<<","
        <<order
       <<")); "<<endl;
    os<<"}"<<endl;
}

void SelectableSubsetParameterParser::Data::cppWriteSetStatement
(
    std::ostream& os,
    const std::string& name,
    const std::string& varname,
    const std::string& staticname,
    const std::string& thisscope
) const
{

    os<<"{"<<endl;

    for ( const SubsetData& sd: value ) {

        const std::string& sel_name=boost::fusion::get<0> ( sd );

//        SubsetParameterParser::Data* pd
//            = dynamic_cast<SubsetParameterParser::Data*>(boost::fusion::get<1>(sd).get()); // should be a set

//        if (!pd) throw PDLException("selectablesubset should contain only sets!");
        bool emptyset;
        auto pd = getSubset( boost::fusion::get<1>(sd), &emptyset );

        std::string seliname=name+"_"+sel_name;
        os<<"if ( ";
         if (!emptyset) os <<"const "<<extendtype ( thisscope, pd->cppTypeName ( name+"_"+sel_name ) )<<"* "<<seliname<<"_static = ";
         os << "boost::get< "<<extendtype ( thisscope, pd->cppTypeName ( name+"_"+sel_name ) ) <<" >(&"<< staticname <<")"
         ") {\n";

         os
            <<varname<<".selection() = \""<<sel_name<<"\";\n";
         if (!emptyset) {
              os <<
              "ParameterSet& "<<seliname<<"_param = "<<varname<<"();\n";
              pd->cppWriteSetStatement ( os, seliname, seliname+"_param", "(*"+seliname+"_static)", thisscope );
           }
        os<<"}"<<endl;
    }
    os<<"}"<<endl;
}

void SelectableSubsetParameterParser::Data::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& name,
    const std::string& varname,
    const std::string& staticname,
    const std::string& thisscope
) const
{

    os<<"{"<<endl;

    for ( const SubsetData& sd: value ) {
        const std::string& sel_name=boost::fusion::get<0> ( sd );

//        SubsetParameterParser::Data* pd
//            = dynamic_cast<SubsetParameterParser::Data*>(boost::fusion::get<1>(sd).get()); // should be a set

//        if (!pd) throw PDLException("selectablesubset should contain only sets!");
        bool emptyset/* = pd->value.size()==0*/;
        auto pd=getSubset( boost::fusion::get<1>(sd), &emptyset );
        std::string seliname=name+"_"+sel_name;

        os<<"if ( "<<varname<<".selection() == \""<<sel_name<<"\" ) {\n";


         os<<extendtype ( thisscope, pd->cppTypeName ( name+"_"+sel_name ) ) <<" "<<seliname<<"_static;\n";
         if (!emptyset)
           {
             os<<"const ParameterSet& "<<seliname<<"_param = "<<varname<<"();\n";
             pd->cppWriteGetStatement ( os, seliname, seliname+"_param", seliname+"_static", thisscope );
           }
         os<<staticname<<" = "<<seliname<<"_static;\n";

        os<<"}\n";
    }
    os<<"}\n";
}
