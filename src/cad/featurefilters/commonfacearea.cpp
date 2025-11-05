#include "commonfacearea.h"
#include "BRepAlgoAPI_Common.hxx"
#include "ShapeFix_ShapeTolerance.hxx"


namespace insight {
namespace cad {


hasCommonFaceArea::hasCommonFaceArea(FeaturePtr m)
    : f_(m->allFaces())
{}

hasCommonFaceArea::hasCommonFaceArea(FeatureSetPtr f)
    : f_(f)
{}

bool hasCommonFaceArea::checkMatch(FeatureID i) const
{
    ShapeFix_ShapeTolerance st;

    double A=0.;
    // double A=DBL_MAX;

    auto fi=model().face(i);
    st.SetTolerance (fi, 0.01, TopAbs_SHAPE);

    for (auto j: f_->data())
    {
        auto fj = f_->model()->face(j);

        TopTools_ListOfShape args;
        args.Append(fi);
        TopTools_ListOfShape tools;
        tools.Append(fj);
        BRepAlgoAPI_Common op;
        op.SetFuzzyValue(0.1);
        op.SetArguments(args);
        op.SetTools(tools);
        op.Build();

        auto cs=op.Shape();
        GProp_GProps props;
        BRepGProp::SurfaceProperties(cs, props);
        double a=props.Mass();
        if (fabs(a)>insight::SMALL)
            std::cout<<"A("<<j<<"/"<<i<<")="<<a<<std::endl;
        A+=a;

        // BRepExtrema_DistShapeShape dss
        //     (
        //         fi, fj,
        //         Precision::Confusion()
        //         );

        // if (!dss.Perform())
        //     throw insight::Exception("determination of minimum distance to point failed!");
        // auto a = dss.Value();

        // if (fabs(a)<insight::SMALL)
        //     std::cout<<"dist("<<j<<"/"<<i<<")="<<a<<std::endl;
        // A=std::min(a, A);

    }
    return A>insight::SMALL;
    // return A<insight::SMALL;
}

FilterPtr hasCommonFaceArea::clone() const
{
    return std::make_shared<hasCommonFaceArea>(f_);
}

}
}
