#ifndef SUBSETGENERATOR_H
#define SUBSETGENERATOR_H

#include "parametergenerator.h"
#include <string>

struct SubsetGenerator
: public ParameterGenerator
{
        ParameterSetData value;

        boost::optional<std::string> templateArg_, base_type_name_, addTo_makeDefault_;

        SubsetGenerator(const ParameterSetData& v, const std::string& d);
        SubsetGenerator(const ParameterSetData& v,
                        const boost::optional<std::string>& templateArg,
                        const boost::optional<std::string>& inherits,
                        const boost::optional<std::string>& description,
                        const boost::optional<std::string>& addCode
                        );

        void setPath(const std::string& containerPath) override;

        void cppAddRequiredInclude(std::set< std::string >& headers) const override;

        std::string cppInsightType() const override;
        std::string cppStaticType() const override;
        std::string cppDefaultValueExpression() const override;

        virtual void writeCppTypeDeclMemberDefinitions(
            std::ostream& os ) const;

        virtual void writeCppTypeDeclConstructors(
            std::ostream& os ) const;

        virtual void writeCppTypeDeclGetSetFunctions(
            std::ostream& os ) const;

        virtual void writeCppTypeDeclMakeDefaultFunction_populate(
            std::ostream& os ) const;

        virtual void writeCppTypeDeclMakeDefaultFunction(
            std::ostream& os ) const;

        // write the struct
        void writeCppTypeDecl(
            std::ostream& os
        ) const override;


        void cppWriteInsertStatement
        (
            std::ostream& os,
            const std::string& psvarname
        ) const override;

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

    declareType("set");

    inline static void insertrule(PDLParserRuleset& ruleset)
    {
        ruleset.parameterDataRules.add
        (
            typeName,
            std::make_shared<PDLParserRuleset::ParameterDataRule>(

                ( "{" > ruleset.r_parameterset > "}" >> ruleset.r_description_string )
                [ qi::_val = qi::_1,
                  phx::bind(&ParameterGenerator::changeDescription,
                               *qi::_val, qi::_2) ]

            )
        );
    }
};


typedef std::shared_ptr<SubsetGenerator> SubsetGeneratorPtr;


#endif // SUBSETGENERATOR_H
