#include "splitsurface.h"
#include "cadfeature.h"
#include "datum.h"
#include "TopTools_MapOfShape.hxx"
#include "base/exception.h"
#include "base/translations.h"

#include "cadexception.h"
#include "cadfeatures/importsolidmodel.h"

#include "GEOMAlgo_Splitter.hxx"
#include "BRepClass3d.hxx"

namespace insight {
namespace cad {



defineType(SplitSurface);
addToStaticFunctionTable(Feature, SplitSurface, insertrule);




size_t SplitSurface::calcHash() const
{
    ParameterListHash h;
    h+=this->type();
    h+=tools_.size();
    for (auto& t: tools_)
    {
        h+=*boost::fusion::get<0>(t);
        h+=boost::fusion::get<1>(t);
    }
    return h.getHash();
}

void SplitSurface::build()
{
    double tol = 1e-3;
    // 1. extract boundary
    // 2. subtract tools
    // 3. restitch boundary to solid
    // 4. create feature sets with names

    TopTools_IndexedMapOfShape m;
    TopExp::MapShapes(baseFeature()->shape(), TopAbs_SOLID, m);
    TopExp_Explorer ex(baseFeature()->shape(), TopAbs_SOLID);
    if (m.Extent()>1)
    {
        throw insight::CADException({
                {"input", baseFeature()}
            },
            "only a single solid input is expected, got %d", m.Extent()
        );
    }
    auto initialSolid = TopoDS::Solid(m.FindKey(1));

    TopoDS_Shell initialShell = BRepClass3d::OuterShell(initialSolid);

    TopoDS_Shape splitShell = initialShell;
    // std::map<int, TopTools_ListOfShape> generated;
    for (auto t: boost::adaptors::index(tools_))
    {
        auto tool = boost::fusion::get<0>(t.value())->shape();
        auto name = boost::fusion::get<1>(t.value());

        GEOMAlgo_Splitter spl;
        spl.AddArgument(splitShell);
        spl.AddTool(tool);

        spl.Perform();
        splitShell=spl.Shape();
    }

    {
        BRepBuilderAPI_Sewing sew(tol);
        for (TopExp_Explorer ex(splitShell, TopAbs_FACE);
             ex.More(); ex.Next())
        {
            sew.Add(ex.Current());
        }
        sew.Perform();

        auto finalShell = TopoDS::Shell(sew.SewedShape());

        BRepBuilderAPI_MakeSolid solidmaker(finalShell);

        if (!solidmaker.IsDone())
            throw insight::Exception(_("Creation of solid failed!"));

        auto solid = solidmaker.Solid();

        ShapeFix_Solid fix(solid);
        fix.Perform();

        setShape(fix.Shape());
    }

    for ( auto t: boost::adaptors::index(tools_) )
    {
        auto tool = boost::fusion::get<0>(t.value());
        auto name = boost::fusion::get<1>(t.value());

        // delayed evaluation: will be evaluated upon first use
        auto f = DeferredFeatureSet::create(
            shared_from_this(), Face,
            "isPartOfSolid(%0)",
            FeatureSetParserArgList{
                makeSolidFeatureSet(tool)
            } );

        providedFeatureSets_[name] = f;
        providedSubshapes_[name] = cad::Import::create(f);
    }
}



SplitSurface::SplitSurface(const SplitSurface&o, TreeCloneMap& tcm)
    : DerivedFeature(o, tcm)
{
    for (auto& t: o.tools_)
    {
        tools_.push_back(Tool{
            tcm.clone(boost::fusion::get<0>(t)),
            boost::fusion::get<1>(t)
        });
    }
}



SplitSurface::SplitSurface(FeaturePtr featToSplit, Tools tools)
    : DerivedFeature(featToSplit), tools_(tools)
{}




void SplitSurface::replaceDependency(const DependencyReplacement& repl)
{
    for (auto& t: tools_)
    {
        repl(boost::fusion::get<0>(t));
    }
    invalidate();
}



void SplitSurface::insertrule(parser::ISCADParser& ruleset)
{
    ruleset.modelstepFunctionRules.add
        (
            "SplitSurface",
            std::make_shared<parser::ISCADParser::ModelstepRule>(

                ( '(' > ruleset.r_solidmodel_expression > ','
             > ( ( ruleset.r_solidmodel_expression > ':' > ruleset.r_identifier ) % ',' ) > ')' )
                    [ qi::_val = phx::bind(
                         &SplitSurface::create<FeaturePtr, Tools>,
                         qi::_1, qi::_2) ]

                )
            );
}


} // namespace cad
} // namespace insight
