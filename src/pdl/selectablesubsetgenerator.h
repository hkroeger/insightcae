#ifndef SELECTABLESUBSETGENERATOR_H
#define SELECTABLESUBSETGENERATOR_H

#include "parametergenerator.h"
#include "subsetgenerator.h"

struct SelectableSubsetGenerator
: public ParameterGenerator
{
        typedef std::vector<boost::fusion::vector2<std::string, ParameterGeneratorPtr> > SubsetListData;

        typedef ParameterGeneratorPtr  SubsetData;
        std::vector<SubsetData> value;
        ParameterGenerator* default_sel;

        SelectableSubsetGenerator(
            const SubsetListData& v, const std::string& ds, const std::string& d );

        void setName(const std::string& name) override;
        void setPath(const std::string &containerPath) override;

        void cppAddRequiredInclude ( std::set<std::string>& headers ) const override;

        std::string cppInsightType ( ) const override;
        std::string cppStaticType () const override;
        std::string cppDefaultValueExpression () const override;


        void writeCppTypeDecl ( std::ostream& os ) const override;


        void cppWriteCreateStatement
        (
            std::ostream& os,
            const std::string& psvarname
        ) const override;

        void cppWriteSetStatement
        (
            std::ostream& os,
            const std::string& varname,
            const std::string& staticname
        ) const override;

        void cppWriteGetStatement
        (
            std::ostream& os,
            const std::string& varname,
            const std::string& staticname
        ) const override;

    declareType("selectablesubset");

    inline static void insertrule ( PDLParserRuleset& ruleset )
    {
        ruleset.parameterDataRules.add
        (
            typeName,
            std::make_shared<PDLParserRuleset::ParameterDataRule>(

                ( qi::lit ( "{{" ) >>
                  * ( ruleset.r_identifier >> ruleset.r_parameterdata )
                  >> qi::lit ( "}}" )
                  >> ruleset.r_identifier
                  >> ruleset.r_description_string )
                [ qi::_val = phx::construct<ParameterGeneratorPtr> (
                        phx::new_<SelectableSubsetGenerator> (
                             qi::_1, qi::_2, qi::_3 )
                              ) ]

            )
        );
    }
};

#endif // SELECTABLESUBSETGENERATOR_H
