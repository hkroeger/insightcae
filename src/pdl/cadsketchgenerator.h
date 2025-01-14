#ifndef CADSKETCHGENERATOR_H
#define CADSKETCHGENERATOR_H


#include "parametergenerator.h"

struct CADSketchGenerator
{
    struct Data
        : public ParameterGenerator
    {
        std::string
            script_,
            EntityPropertiesDelegateSharedPtrExpression_,
            SketchPresentationLookupKeyExpression_;

        std::vector<boost::fusion::vector2<int, std::string> > references_;

        Data(
            const std::string& script,
            const std::string& EntityPropertiesDelegateSharedPtrExpression,
            const std::string& SketchPresentationLookupKeyExpression,
            const std::vector<boost::fusion::vector2<int, std::string> >& references,
            const std::string& d);

        std::string refParameter() const;

        void cppAddRequiredInclude(std::set< std::string >& headers) const override;

        std::string cppInsightType() const override;
        std::string cppStaticType() const override;
        std::string cppDefaultValueExpression() const override;

        std::string cppConstructorParameters() const override;

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

    };

    declareType("cadsketch");

    inline static void insertrule(PDLParserRuleset& ruleset)
    {
        ruleset.parameterDataRules.add
            (
                typeName,
                std::make_shared<PDLParserRuleset::ParameterDataRule>(
                    (
                       ruleset.r_string
                    >> ruleset.r_string
                    >> ruleset.r_string
                    >> ( ( qi::lit("references") >> *( qi::int_ > '=' > ruleset.r_string ) )
                        | qi::attr(std::vector<boost::fusion::vector2<int, std::string> >()) )
                    >> ruleset.r_description_string
                    )
                    [ qi::_val = phx::construct<ParameterGeneratorPtr>(
                         phx::new_<Data>(
                             qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
                    )
                );
    }
};

#endif // CADSKETCHGENERATOR_H
