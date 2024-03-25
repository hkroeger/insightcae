#include "polygon.h"

#include <boost/spirit/include/qi.hpp>
#include "base/translations.h"


namespace qi = boost::spirit::qi;
namespace phx   = boost::phoenix;

namespace insight {
namespace cad {


defineType(Polygon);
//addToFactoryTable(Feature, Wire);
addToStaticFunctionTable(Feature, Polygon, insertrule);
addToStaticFunctionTable(Feature, Polygon, ruleDocumentation);




size_t Polygon::calcHash() const
{
    ParameterListHash h;
    h+=this->type();
    h+=close_;
    for (const auto& c: corners_)
    {
        h+=c->value();
    }
    return h.getHash();
}







Polygon::Polygon(
    const std::vector<VectorPtr>& corners,
    bool close
    )
    : corners_(corners),
    close_(close)
{}






void Polygon::build()
{
    TopTools_ListOfShape ee;

    for (int i=1; i<corners_.size(); ++i)
    {
        ee.Append ( BRepBuilderAPI_MakeEdge(
            to_Pnt(corners_[i-1]->value()),
            to_Pnt(corners_[i]->value())).Edge() );
    }
    if (close_)
    {
        ee.Append ( BRepBuilderAPI_MakeEdge(
                  to_Pnt(corners_.back()->value()),
                  to_Pnt(corners_.front()->value())).Edge() );
    }

    BRepBuilderAPI_MakeWire wb;
    wb.Add ( ee );

    ShapeFix_Wire wf;
    wf.Load(wb.Wire());
    wf.Perform();

    setShape ( wf.Wire() );
}




void Polygon::insertrule(parser::ISCADParser& ruleset)
{
    ruleset.modelstepFunctionRules.add
        (
            "Polygon",
            std::make_shared<parser::ISCADParser::ModelstepRule>(

                '(' >
                    ( ruleset.r_vectorExpression % ',' )
                        [ qi::_val = phx::bind(
                             &Polygon::create<const std::vector<VectorPtr>&, bool>,
                             qi::_1, true) ]
                     > ')'
                )
            );

    ruleset.modelstepFunctionRules.add
        (
            "Polyline",
            std::make_shared<parser::ISCADParser::ModelstepRule>(

                '(' >
                ( ruleset.r_vectorExpression % ',' )
                    [ qi::_val = phx::bind(
                         &Polygon::create<const std::vector<VectorPtr>&, bool>,
                         qi::_1, false) ]
                > ')'
                )
            );
}




FeatureCmdInfoList Polygon::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "Polygon",
            "( <vertex list> )",
            _("Creates a polygon wire from a number of corners, connected by straight edges.")
            ),
        FeatureCmdInfo
        (
            "Polyline",
            "( <vertex list> )",
            _("Creates a polyline wire from a number of corners, connected by straight edges.")
            )
    };
}



bool Polygon::isSingleClosedWire() const
{
    return close_;
}




bool Polygon::isSingleOpenWire() const
{
    return !isSingleClosedWire();
}


} // namespace cad
} // namespace insight
