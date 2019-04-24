#include "faceisocurve.h"


#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

#include "Adaptor3d_IsoCurve.hxx"
#include "GeomAPI_Interpolate.hxx"


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight
{
namespace cad
{




defineType(FaceIsoCurve);
addToFactoryTable(Feature, FaceIsoCurve);



size_t FaceIsoCurve::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=*faces_;
  h+=iso_value_->value();
  h+=int(coord_);
  return h.getHash();
}


FaceIsoCurve::FaceIsoCurve()
: Feature()
{
}




FaceIsoCurve::FaceIsoCurve(FeatureSetPtr faces, UV coord, ScalarPtr iso_value)
: faces_(faces), coord_(coord), iso_value_(iso_value)
{
}



FeaturePtr FaceIsoCurve::create ( FeatureSetPtr faces, UV coord, ScalarPtr iso_value )
{
    return FeaturePtr(new FaceIsoCurve(faces, coord, iso_value));
}



void FaceIsoCurve::build()
{
  BRep_Builder bb;
  TopoDS_Compound result;
  bb.MakeCompound ( result );

  for (const auto i_f: faces_->data())
  {
    cout<<"Generating iso curve on facce #"<<i_f<<endl;

    TopoDS_Face aFace = faces_->model()->face(i_f);

    BRepAdaptor_Surface bas(aFace, Standard_True);
    Handle_Adaptor3d_HSurface HS = new GeomAdaptor_HSurface( bas.Surface() );
    Adaptor3d_IsoCurve C( HS,  coord_==UV::U ? GeomAbs_IsoU : GeomAbs_IsoV, iso_value_->value() );

    int ni=std::min(10, C.NbIntervals(GeomAbs_C1));
    TColStd_Array1OfReal params(1, ni);
    //C.Intervals(params, GeomAbs_C1);
    for (int i=0; i<ni; i++)
    {
      params.SetValue(i+1, C.FirstParameter() + (C.LastParameter()-C.FirstParameter())*double(i)/double(ni));
    }

    Handle_TColgp_HArray1OfPnt pts_col = new TColgp_HArray1OfPnt( 1, params.Length() );
    for ( int j=1; j<params.Length(); j++ )
    {
        gp_Pnt pi=C.Value(j);
        pts_col->SetValue (j, pi);
        refpoints_[str(format("p%d")%j)]=insight::Vector(pi);
    }
//     GeomAPI_PointsToBSpline splbuilder ( pts_col );
    GeomAPI_Interpolate splbuilder ( pts_col, false, 1e-6 );
    splbuilder.Perform();
    Handle_Geom_BSplineCurve crv=splbuilder.Curve();
    bb.Add(result, BRepBuilderAPI_MakeEdge ( crv, crv->FirstParameter(), crv->LastParameter() ) );
  }

 setShape (result);
}




void FaceIsoCurve::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "FaceIsoCurve",
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

    ( '(' >> ruleset.r_faceFeaturesExpression >> ','
          >> ( ( qi::lit("u")>>qi::attr(UV::U) ) | ( qi::lit("v")>>qi::attr(UV::V) ) )  >> ','
          >> ruleset.r_scalarExpression >> ')' )
        [ qi::_val = phx::bind(&FaceIsoCurve::create, qi::_1, qi::_2, qi::_3) ]

    ))
  );
}




FeatureCmdInfoList FaceIsoCurve::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "FaceIsoCurve",

            "( <faceSelection>, u|v, <scalar:isovalue> )",

            "Creates a curve in all selected faces along a constant parameter value."
        )
    );
}





}
}
