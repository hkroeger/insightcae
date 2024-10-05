#include "alignwithboundingbox.h"
#include "base/tools.h"
#include "base/translations.h"
#include "cadfeatures/transform.h"
#include "cadtypes.h"

namespace insight {
namespace cad {

defineType(AlignWithBoundingBox);
addToStaticFunctionTable(Feature, AlignWithBoundingBox, insertrule);
addToStaticFunctionTable(Feature, AlignWithBoundingBox, ruleDocumentation);


AlignWithBoundingBox::AlignWithBoundingBox (
    FeaturePtr m1,
    const std::vector<boost::fusion::vector<
        FeaturePtr, VectorPtr, AlignLocation, AlignLocation> >& other_direction_atOther_atThis    )
: DerivedFeature(m1),
  m1_(m1)
{
    for (auto& odot: other_direction_atOther_atThis)
    {
        alignments_.push_back({
            boost::fusion::get<0>(odot),
            boost::fusion::get<1>(odot),
            boost::fusion::get<2>(odot),
            boost::fusion::get<3>(odot)
        });
    }
}

size_t AlignWithBoundingBox::calcHash() const
{
    ParameterListHash h;
    h+=this->type();
    h+=*m1_;
    h+=alignments_.size();
    for (auto& a: alignments_)
    {
        h+=*a.other_;
        h+=a.direction_->value();
        h+=int(a.atOther_);
        h+=int(a.atThis_);
    }
    return h.getHash();
}

void AlignWithBoundingBox::build()
{
    ExecTimer t("Transform::build() ["+featureSymbolName()+"]");

    if (!cache.contains(hash()))
    {
        FeaturePtr org = m1_;

        for (auto& a: alignments_)
        {
            gp_Trsf toWorkCS;
            Bnd_Box bbOrg, bbOther;

            gp_Ax3 from(gp::Origin(), to_Vec(a.direction_->value()));
            gp_Ax3 to(gp::Origin(), gp_Dir(0,0,1));


            toWorkCS.SetTransformation( from, to );
            toWorkCS.Invert(); // http://opencascade.wikidot.com/occrefdoc


            BRepMesh_IncrementalMesh Inc1(*org, 0.0001);
            BRepBndLib::Add(BRepBuilderAPI_Transform(*org, toWorkCS).Shape(), bbOrg);
            BRepMesh_IncrementalMesh Inc2(*a.other_, 0.0001);
            BRepBndLib::Add(BRepBuilderAPI_Transform(*a.other_, toWorkCS).Shape(), bbOther);

            gp_Trsf t;

            auto z = [](const Bnd_Box& bb, AlignLocation at) {
                switch (at)
                {
                case Max: return bb.CornerMax().Z();
                case Center: return 0.5*(bb.CornerMax().Z()+bb.CornerMin().Z());
                case Min: return bb.CornerMin().Z();
                };
                throw UnhandledSelection();
                return 0.;
            };

            t.SetTranslation(gp_Vec(0,0,
                    z(bbOther, a.atOther_)
                    -
                    z(bbOrg, a.atThis_) ));

            t.Multiply(toWorkCS);
            t.PreMultiply(toWorkCS.Inverted());

            org = cad::Transform::create(org, t);
    //         setShape(BRepBuilderAPI_Transform(*m1_, *trsf_).Shape());

    //         // Transform all ref points and ref vectors
    //         copyDatumsTransformed(
    //             *m1_, *trsf_, "",
    //             { "scaleFactor", "translation", "rotationOrigin", "rotation" }
    //             );
        }

        // providedSubshapes_["basefeat"]=m1_; // overwrite existing basefeat, if there

        setShape(org->shape());
        copyDatums(*org);
        trsf_=org->transformation();

        cache.insert(shared_from_this());
    }
    else
    {
        this->operator=(*cache.markAsUsed<AlignWithBoundingBox>(hash()));
    }
}


void AlignWithBoundingBox::insertrule ( parser::ISCADParser& ruleset )
{
    static boost::spirit::qi::symbols<char, AlignLocation> alignLocation{
        std::vector<std::string>{"max", "center", "min"},
        std::vector<AlignLocation>{AlignLocation::Max, AlignLocation::Center, AlignLocation::Min},
        "align location"
    };
    ruleset.modelstepFunctionRules.add
        (
            typeName,
            std::make_shared<parser::ISCADParser::ModelstepRule>(

                ( '('
                 > ruleset.r_solidmodel_expression > ','
                 > ( ruleset.r_solidmodel_expression > ','
                     > ruleset.r_vectorExpression > ','
                     > alignLocation > ','
                     > alignLocation ) % ','
                 > ')' )
                    [ qi::_val = phx::bind(
                         &AlignWithBoundingBox::create<
                           FeaturePtr, const std::vector<boost::fusion::vector<
                             FeaturePtr, VectorPtr, AlignLocation, AlignLocation> >& >,
                         qi::_1, qi::_2 ) ]

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
    return trsf_;
}



} // namespace cad
} // namespace insight
