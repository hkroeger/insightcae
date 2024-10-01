#include "alignwithboundingbox.h"
#include "base/tools.h"
#include "base/translations.h"

namespace insight {
namespace cad {

defineType(AlignWithBoundingBox);
addToStaticFunctionTable(Feature, AlignWithBoundingBox, insertrule);
addToStaticFunctionTable(Feature, AlignWithBoundingBox, ruleDocumentation);


AlignWithBoundingBox::AlignWithBoundingBox (
    FeaturePtr m1, FeaturePtr other, VectorPtr direction )
: DerivedFeature(m1),
  m1_(m1), other_(other), direction_(direction)
{}

size_t AlignWithBoundingBox::calcHash() const
{
    ParameterListHash h;
    h+=this->type();
    h+=*m1_;
    h+=*other_;
    h+=direction_->value();
    return h.getHash();
}

void AlignWithBoundingBox::build()
{
    ExecTimer t("Transform::build() ["+featureSymbolName()+"]");

    if (!cache.contains(hash()))
    {

        gp_Trsf toWorkCS;
        Bnd_Box bbOrg, bbOther;

        gp_Ax3 from(gp::Origin(), to_Vec(direction_->value()));
        gp_Ax3 to(gp::Origin(), gp_Dir(0,0,1));

        // std::cout<<"from\n";
        // from.DumpJson(std::cout);

        // std::cout<<"to\n";
        // to.DumpJson(std::cout);

        toWorkCS.SetTransformation( from, to );
        toWorkCS.Invert(); // http://opencascade.wikidot.com/occrefdoc

        // std::cout<<"tWorkCS\n";
        // toWorkCS.DumpJson(std::cout);

        BRepMesh_IncrementalMesh Inc1(*m1_, 0.0001);
        BRepBndLib::Add(BRepBuilderAPI_Transform(*m1_, toWorkCS).Shape(), bbOrg);
        BRepMesh_IncrementalMesh Inc2(*other_, 0.0001);
        BRepBndLib::Add(BRepBuilderAPI_Transform(*other_, toWorkCS).Shape(), bbOther);

        // std::cout<<"bbOrg\n";
        // bbOrg.Dump();
        // std::cout<<"bbOther\n";
        // bbOther.Dump();

        gp_Trsf t;
        t.SetTranslation(gp_Vec(0,0, bbOther.CornerMax().Z() - bbOrg.CornerMax().Z()));

        // std::cout<<"t\n";
        // t.DumpJson(std::cout);

        t.Multiply(toWorkCS);
        t.PreMultiply(toWorkCS.Inverted());

        trsf_=std::make_shared<gp_Trsf>(t);

        // std::cout<<"trsf\n";
        // trsf_->DumpJson(std::cout);

        setShape(BRepBuilderAPI_Transform(*m1_, *trsf_).Shape());

        // Transform all ref points and ref vectors
        copyDatumsTransformed(
            *m1_, *trsf_, "",
            { "scaleFactor", "translation", "rotationOrigin", "rotation" }
            );

        providedSubshapes_["basefeat"]=m1_; // overwrite existing basefeat, if there

        cache.insert(shared_from_this());
    }
    else
    {
        this->operator=(*cache.markAsUsed<AlignWithBoundingBox>(hash()));
    }
}


void AlignWithBoundingBox::insertrule ( parser::ISCADParser& ruleset )
{
    ruleset.modelstepFunctionRules.add
        (
            typeName,
            std::make_shared<parser::ISCADParser::ModelstepRule>(

                ( '('
                 > ruleset.r_solidmodel_expression > ','
                 > ruleset.r_solidmodel_expression > ','
                 > ruleset.r_vectorExpression
                 > ')' )
                    [ qi::_val = phx::bind(
                         &AlignWithBoundingBox::create<FeaturePtr, FeaturePtr, VectorPtr>,
                         qi::_1, qi::_2, qi::_3) ]

                )
            );
}

FeatureCmdInfoList AlignWithBoundingBox::ruleDocumentation()
{
    return {
        FeatureCmdInfo
            (
                typeName,
                "( <feature:base>, <feature:other>, <vector:direction> )",
                _("align the extremities of the first object with those of other, measured along direction.")
            )
    };
}

bool AlignWithBoundingBox::isTransformationFeature() const
{
    return true;
}

gp_Trsf AlignWithBoundingBox::transformation() const
{
    checkForBuildDuringAccess();
    return *trsf_;
}

} // namespace cad
} // namespace insight
