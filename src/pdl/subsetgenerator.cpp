#include "subsetgenerator.h"
#include <algorithm>
#include <iterator>

using namespace std;



defineType(SubsetGenerator);
addToStaticFunctionTable(ParameterGenerator, SubsetGenerator, insertrule);


SubsetGenerator::SubsetGenerator(
    const ParameterSetData& v, const std::string& d)
    : ParameterGenerator(d), value(v)
{}

SubsetGenerator::SubsetGenerator(
    const ParameterSetData &v,
    const boost::optional<std::string> &templateArg,
    BaseTypes inherits,
    const boost::optional<std::string> &description,
    const boost::optional<std::string> &addCode )
    : ParameterGenerator(description.value_or("")),
    value(v),
    templateArg_(templateArg),
    base_types_(inherits),
    addTo_makeDefault_(addCode)
{}

void SubsetGenerator::setPath(const std::string &containerPath)
{
    ParameterGenerator::setPath(containerPath);
    for (auto& pe: value)
    {
        pe->setPath(parameterPath);
    }
}


void SubsetGenerator::cppAddRequiredInclude(std::set< std::string >& headers) const
{
    headers.insert("\"base/parameters/subsetparameter.h\"");

    for (auto& pe: value)
    {
        pe->cppAddRequiredInclude(headers);
    }
}


std::string SubsetGenerator::cppInsightType() const
{
    return "insight::ParameterSet";
}


std::string SubsetGenerator::cppStaticType() const
{
    return "#error";
}


std::string SubsetGenerator::cppDefaultValueExpression() const
{
    std::vector<std::string> items;

    std::transform(
        value.begin(), value.end(),
        std::back_inserter(items),
        [this](const ParameterSetEntry& pe)
        {
            return
                pe->cppConstructorParameters(/*
                    extendtype(thisscope, name+"_type")*/);
        }
        );

    return cppTypeName()+"{"+boost::join(items, ",\n")+"}";
}



void SubsetGenerator::writeCppTypeDeclMemberDefinitions(
    std::ostream &os ) const
{
    // members
    for (const auto& pe: value)
    {
        pe->writeCppStaticVariableDefinition( os );
    }
}



void SubsetGenerator::writeCppTypeDeclConstructors(
    std::ostream &os,
    CodeMode mode, const std::string& qual ) const
{
    auto writeBaseInitializers = [&](const std::string& args)
    {
        if (base_types_ && base_types_->size())
        {
            std::vector<std::string> bt;
            std::transform(
                base_types_->begin(), base_types_->end(),
                std::back_inserter(bt),
                [&](const BaseType& t)
                { return boost::fusion::at_c<1>(t)+"("+args+")"; } );

            os<<": "<<boost::join(bt, ",\n")<<"\n";
        }
    };

    // default constructor
    if (mode==CodeMode::Declaration)
    {
        os << cppTypeName() << "();\n";
    }
    else
    {
        os << qual << cppTypeName() << "()\n";
        writeBaseInitializers("");
        os<<"{ get(*makeDefault()); }\n";
    }

    // produce virtual destructor
    switch (mode)
    {
    case CodeMode::Inline:
        os << "virtual ~" << cppTypeName() << "()\n"
           << "{}\n";
        break;
    case CodeMode::Declaration:
        os << "virtual ~" << cppTypeName() << "();\n";
        break;
    case CodeMode::Definition:
        os << qual << "~" << cppTypeName() << "()\n"
           << "{}\n";
        break;
    }

    if (mode!=CodeMode::Definition && value.size()>0)
    {
        // explicit constructor (trivial, always inline)
        os << cppTypeName() << "(";
        {
            bool first=true;
            for (const ParameterSetEntry& pe: value)
            {
                if (!first) os<<",\n"; else first=false;
                os << "const "<<pe->cppTypeName()<<"& "<<(pe->name+"_value");
            }
        }
        os << ") : \n";
        {
            bool first=true;
            for (const ParameterSetEntry& pe: value)
            {
                if (!first) os<<",\n"; else first=false;
                os << pe->name<<"("<<(pe->name+"_value")<<")";
            }
        }
        os <<"{}\n";
    }

    //get from other ParameterSet
    if (mode==CodeMode::Declaration)
    {
        os << cppTypeName() << "(const insight::ParameterSet& p);\n";
    }
    else
    {
        os << qual << cppTypeName() << "(const insight::ParameterSet& p)\n";
        writeBaseInitializers("p");
        os <<"{ get(p); }\n";
    }
}



void SubsetGenerator::writeCppTypeDeclGetSetFunctions(
    std::ostream &os,
    CodeMode mode, const std::string& qual ) const
{
    // ============== SET =====================
    //set static data into other ParameterSet
    if (mode==CodeMode::Declaration)
    {
        os << "virtual void set(insight::ParameterSet& p) const;\n";
    }
    else
    {
        os << (mode==CodeMode::Inline ? "virtual " : "")
           << "void " << qual << "set(insight::ParameterSet& p) const\n"
           << "{\n";

        // if is derived, call base class method
        if ( base_types_ && base_types_->size() )
        {
            for (auto& bt: *base_types_)
            {
                os << boost::fusion::at_c<1>(bt)<<"::set(p);\n";
            }
        }

        for ( auto& member: value )
        {
            std::string subname=member->name;
            os << "{\n"

               <<  "auto& "<<subname<<"_dyn = "
                     "p.get< "<<member->cppInsightType() <<" >(\""<<subname<<"\");\n"

               << "const auto& "<<subname<<"_static = this->"<<subname<<";\n";

            member->cppWriteSetStatement(
                    os, subname+"_dyn", subname+"_static" );
            os << "}\n";
        }
        os << "}\n";  // END set()
    }


    // ============== GET =====================
    // set static data from dynamic ParameterSet
    if (mode==CodeMode::Declaration)
    {
        os << "virtual void get(const insight::ParameterSet& p);\n";
    }
    else
    {
        os << (mode==CodeMode::Inline ? "virtual " : "")
           << "void " << qual << "get(const insight::ParameterSet& p)\n"
           << "{\n";

        if ( base_types_ && base_types_->size() )
        {
            for (auto& bt: *base_types_)
            {
                os << boost::fusion::at_c<1>(bt) <<"::get(p);\n";
            }
        }

        for ( auto& member: value )
        {
            std::string subname=member->name;
            os << "{\n"
               <<  "const auto& "<<subname<<"_dyn = "
                    "p.get< "<<member->cppInsightType()<<" >(\""<<subname<<"\");\n"

               <<  "auto& "<<subname<<"_static = this->"<<subname<<";\n";

            member->cppWriteGetStatement(
                    os, subname+"_dyn", subname+"_static" );

            os << "}\n";
        }
        os << "}\n";  // END get()
    }


    // set_variable initialization function (trivial, always inline)
    if (mode!=CodeMode::Definition)
    {
        for ( auto& pe: value )
        {
            std::string subname=pe->name;

            os<< cppTypeName()<<"& set_"<<subname<<"(const "<<pe->cppTypeName() <<"& value)\n"
              << "{\n"
               <<" this->"<<subname<<"=value;\n"
               <<" return *this;\n"
              << "}\n";
        }
    }
}

void SubsetGenerator::writeCppTypeDeclMakeDefaultFunction_populate(std::ostream &os) const
{

    os<< "auto "<<name<<"Ptr = "
       << cppInsightType()<<"::create_uninitialized( ParameterSetBuilder::key(), "
       <<   cppInsightTypeConstructorParameters()<<" ); "
       << "auto &"<<name<<" = *"<<name<<"Ptr;\n";

    if (base_types_)
    {
        for ( auto& bt: *base_types_ )
        {
            os << name<<".extend( *"<<boost::fusion::at_c<1>(bt)<<"::makeDefault() );\n";
        }
    }

    for (auto& pe: value )
    {
        pe->cppWriteInsertStatement( os, name );
    }

    if (addTo_makeDefault_)
    {
        os << "{\n"
           << "  auto& p=*"<<name<<"Ptr;\n"
           << addTo_makeDefault_.value()<<"\n"
           << "}\n";
    }
}




void SubsetGenerator::writeCppTypeDeclMakeDefaultFunction(
    std::ostream &os,
    CodeMode mode, const std::string& qual ) const
{
    // create a dynamic variable with default values set
    if (mode==CodeMode::Declaration)
    {
        os << "static std::unique_ptr< "<<cppInsightType()<<" > makeDefault();\n";
    }
    else
    {
        os << (mode==CodeMode::Inline ? "static " : "")
           << "std::unique_ptr< "<<cppInsightType()<<" > "<<qual<<"makeDefault()\n"
           <<"{\n";

        writeCppTypeDeclMakeDefaultFunction_populate(os);

        os << "return std::move("<<name<<"Ptr);\n"
           << "}\n";
    }
}




bool SubsetGenerator::isWrittenOutOfLine() const
{
    return outOfLineDefinitions && !templateArg_;
}




void SubsetGenerator::writeCppTypeDecl(
    std::ostream& os ) const
{
    std::string subscope = extendtype(thisscope, name+"_type");

    bool outOfLine = isWrittenOutOfLine();
    CodeMode mode = outOfLine ? CodeMode::Declaration : CodeMode::Inline;

    // nested types of a template struct have to be written inline as well
    bool outOfLineDefinitionsBefore = outOfLineDefinitions;
    outOfLineDefinitions = outOfLine;

    if (templateArg_)
        os<<"template<"<<templateArg_.value()<<">\n";

    os << "struct "<<cppTypeName()<<"\n";
    if (base_types_ && base_types_->size())
    {
        std::vector<std::string> bt;
        std::transform(
            base_types_->begin(), base_types_->end(),
            std::back_inserter(bt),
            [](const BaseType& t)
            { return "public "+boost::fusion::at_c<1>(t); } );

        os<<": "<<boost::join(bt, ",\n")<<"\n";
    }
    else
    {
        os << ": public ParameterSetBuilder\n";
    }
    os << "{\n";

    writeCppTypeDeclMemberDefinitions(os);
    writeCppTypeDeclConstructors(os, mode, "");
    writeCppTypeDeclGetSetFunctions(os, mode, "");
    writeCppTypeDeclMakeDefaultFunction(os, mode, "");

    os << "};\n";

    outOfLineDefinitions = outOfLineDefinitionsBefore;
}




void SubsetGenerator::writeCppOutOfLineDefinitions(
    std::ostream& os,
    const std::string& scope ) const
{
    if (!isWrittenOutOfLine())
        return; // everything inline, including nested types

    std::string qual = scope + cppTypeName() + "::";

    // nested types first
    for (const auto& pe: value)
    {
        pe->writeCppOutOfLineDefinitions(os, qual);
    }

    os << "\n// ===== " << qual << " =====\n";
    writeCppTypeDeclConstructors(os, CodeMode::Definition, qual);
    writeCppTypeDeclGetSetFunctions(os, CodeMode::Definition, qual);
    writeCppTypeDeclMakeDefaultFunction(os, CodeMode::Definition, qual);
}



void SubsetGenerator::cppWriteInsertStatement
(
    std::ostream& os,
    const std::string& psvarname
) const
{
    os<<"{ ";
      os<<"std::string k(\""<<name<<"\"); ";
      this->cppWriteCreateStatement(os, name/*, extendtype(thisscope, name+"_type")*/);
      os<<"if ("<<psvarname<<".contains(k)) {"<<endl;
        os<<psvarname<<".getSubset(k).extend(*"<<name<<");\n";
        os<<psvarname<<".getSubset(k).copyMatching(*"<<name<<");\n";
      os<<"} else {"<<endl;
        os<<psvarname<<".insertUninitialized(ParameterSetBuilder::key(), k, std::move("<<name<<")); ";
      os<<"}"<<endl;
    os<<"}"<<endl;
}

void SubsetGenerator::cppWriteCreateStatement
(
    std::ostream& os,
    const std::string& psvarname
) const
{
    os<<"auto "<<psvarname<<" = "
          <<cppInsightType()<<"::create( "
        <<cppInsightTypeConstructorParameters()<<" ); ";

    // insert members
    os<<"{"<<endl;
    for (auto& pe: value)
    {
        pe->cppWriteInsertStatement
        ( os, "(*"+psvarname+")()"/*,
            extendtype(thisscope, name+"_type")*/
        );
    }
    os<<"}"<<endl;
}



void SubsetGenerator::cppWriteSetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
    os << staticname<<".set("<<varname<<");\n";
}



void SubsetGenerator::cppWriteGetStatement
(
    std::ostream& os,
    const std::string& varname,
    const std::string& staticname
) const
{
    os << staticname<< ".get("<<varname<<");\n"
       << staticname<< ".setPath( "<<varname<<" .path());\n";
}
