#include "inhomscale.h"

#include "base/translations.h"

#include "gp_GTrsf.hxx"
#include "BRepBuilderAPI_GTransform.hxx"
#include "occtools.h"



namespace insight {
namespace cad {




defineType(InhomScale);
addToStaticFunctionTable(Feature, InhomScale, insertrule);
addToStaticFunctionTable(Feature, InhomScale, ruleDocumentation);




InhomScale::InhomScale(const InhomScale&o, TreeCloneMap& tcm)
  : DerivedFeature(o, tcm),
    CL(scale_)
{}




InhomScale::InhomScale(ConstFeaturePtr m1, VectorPtr scale)
  : DerivedFeature(m1),
    scale_(scale)
{}




size_t InhomScale::calcHash() const
{
    ParameterListHash h;
    h+=this->type();
    if (scale_) h+=*scale_;
    return h.getHash()+DerivedFeature::calcHash();
}




void InhomScale::build()
{
    ExecTimer t("Transform::build() ["+featureSymbolName()+"]");

    if (!cache.contains(hash()))
    {
        arma::mat sf=scale_->value();
        refvectors_["scaleFactors"]=sf;

        gp_GTrsf tr;
        gp_Mat m (sf(0), 0, 0,
                 0, sf(1), 0,
                 0, 0, sf(2));
        tr.SetVectorialPart(m);

        BRepBuilderAPI_GTransform aBRepGTrsf(*baseFeature(), tr);

        TopoDS_Shape s = aBRepGTrsf.Shape();

        transformTriangulation(*baseFeature(), s, tr);

        setShape(s);


        // Transform all ref points and ref vectors
        copyDatums(
            *baseFeature(), "",
            { "scaleFactors" }
            );

        providedSubshapes_["basefeat"]=
            std::const_pointer_cast<Feature>(baseFeature()); // overwrite existing basefeat, if there

        cache.insert(shared_from_this());
    }
    else
    {
        this->operator=(*cache.markAsUsed<InhomScale>(hash()));
    }
}




void InhomScale::insertrule ( parser::ISCADParser& ruleset )
{
    ruleset.modelstepFunctionRules.add
        (
            typeName,
            std::make_shared<parser::ISCADParser::ModelstepRule>(
                '(' > ruleset.r_solidmodel_expression [qi::_val = qi::_1 ] > ',' >
                    (
                        ruleset.r_vectorExpression
                        > ')'
                        ) [ qi::_val = phx::bind(
                                    &InhomScale::create<FeaturePtr, VectorPtr>,
                                    qi::_val, qi::_1) ]
                )
            );
}




FeatureCmdInfoList InhomScale::ruleDocumentation()
{
    return {
        FeatureCmdInfo
            (
                typeName,
                "( <feature:base>, <vector:scalefactor X Y Z> )",
                _("Scales the base feature by independent factors for each coordinate direction.")
                )
    };
}




} // namespace cad
} // namespace insight
