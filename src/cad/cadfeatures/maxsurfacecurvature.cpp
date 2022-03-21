#include "maxsurfacecurvature.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

#include "Geom2d_BSplineCurve.hxx"
#include "Geom2dAPI_Interpolate.hxx"
#include "TColgp_HArray1OfPnt2d.hxx"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {




defineType(MaxSurfaceCurvature);
addToFactoryTable(Feature, MaxSurfaceCurvature);


size_t MaxSurfaceCurvature::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=*faces_;
  return h.getHash();
}


MaxSurfaceCurvature::MaxSurfaceCurvature()
{}




void MaxSurfaceCurvature::build()
{

  class Obj : public ObjectiveND
  {
  public:
    Handle_Geom_Surface surf;
    mutable GeomLProp_SLProps props;
    double u1, u2, v1, v2;

    Obj(const TopoDS_Face& f)
    : surf(BRep_Tool::Surface(f)),
      props(surf, 2, 1e-2)
    {
      BRepTools::UVBounds ( f, u1, u2, v1, v2 );
      //surf->Bounds(u1, u2, v1, v2);
      cout<<"u1,...="<<u1<<", "<<u2<<", "<<v1<<", "<<v2<<endl;
    }

    gp_XY uv(const arma::mat& x) const
    {
      double u=std::min(u2, std::max(u1, u1+(u2-u1)*x(0)));
      double v=std::min(v2, std::max(v1, v1+(v2-v1)*x(1)));
      return gp_XY(u, v);
    }

    double maxCurvMag() const
    {
      return std::max(fabs(props.MaxCurvature()), fabs(props.MinCurvature()));
    }

    gp_Dir maxCurvDir() const
    {
      gp_Dir maxc, minc;
      props.CurvatureDirections(maxc, minc);
      if ( fabs(props.MaxCurvature()) > fabs(props.MinCurvature()) )
      {
        return maxc;
      }
      else
      {
        return minc;
      }
    }

    virtual double operator()(const arma::mat& x) const
    {
      auto p = uv(x);
      props.SetParameters(p.X(), p.Y());
      double C=maxCurvMag();
      cout<<"uv="<<p.X()<<" "<<p.Y()<<", C="<<C<<endl;
      return -C;
    }

    virtual int numP() const { return 2; }

  };

  // minimize lateral
  class Obj2 : public Obj
  {
  public:
    gp_XY a,b;

    Obj2(const TopoDS_Face& f, gp_XY uv_start, gp_XY uv_dir)
      : Obj(f), a(uv_dir), b(uv_start)
    {
      maxiter=100;
    }

    gp_XY curuv(const arma::mat& t) const
    {
      gp_XY uv=a*t(0)+b;
      double u=std::min(u2, std::max(u1, uv.X()));
      double v=std::min(v2, std::max(v1, uv.Y()));
      return gp_XY(u, v);
    }

    virtual double operator()(const arma::mat& x) const
    {
      gp_XY uv = curuv(x);
      props.SetParameters(uv.X(), uv.Y());
      double C=maxCurvMag();
      cout<<"t="<<x(0)<<", uv="<<uv.X()<<" "<<uv.Y()<<", C="<<C<<endl;
      return -C;
    }

    virtual int numP() const { return 1; }
  };





  BRep_Builder bb;
  TopoDS_Compound res;
  bb.MakeCompound ( res );

  for (const auto f_i: faces_->data())
  {
    TopoDS_Face f = faces_->model()->face(f_i);

    // search for global max. curvature first => uv0
    Obj obj(f);
    double delta_uv_max=std::max(obj.u2-obj.u1, obj.v2-obj.v1);
    arma::mat x0, steps;
    x0 << 0.5 << 0.5;
    steps << 0.25 << 0.25;
    arma::mat x = nonlinearMinimizeND(obj, x0, 1e-3, steps);
    gp_XY uv0=obj.uv(x);
    double global_max_curv=obj(x);

    std::vector<gp_XY> pts;

    const double itermax=10000;
    const double pdist=1e-2*delta_uv_max;

    for (double dir: {1., -1.})
    {
      gp_XY uv=uv0;
      cout<<"uv="<<uv.X()<<", "<<uv.Y()<<endl;
      int iter=0;
      while ( (uv.X()-obj.u1>pdist)&&(obj.u2-uv.X()>pdist) && (uv.Y()-obj.v1>pdist)&&(obj.v2-uv.Y()>pdist) && (iter<itermax) )
      {
        iter++;

        gp_XY uv_last=uv;

        if (dir>0)
          pts.push_back(uv);
        else if (iter>1)
          pts.insert(pts.begin(), uv);

        obj.props.SetParameters(uv.X(), uv.Y());
//        gp_Dir maxc, minc, minc_uv;
        gp_Vec d1u=obj.props.D1U();
        gp_Vec d1v=obj.props.D1V();
        d1u.Normalize();
        d1v.Normalize();

        gp_Dir maxc, minc;
        obj.props.CurvatureDirections(maxc, minc);
//        cout<<"maxc=["<<maxc.X()<<","<<maxc.Y()<<","<<maxc.Z()<<"]"<<endl;
//        cout<<"minc=["<<minc.X()<<","<<minc.Y()<<","<<minc.Z()<<"]"<<endl;

        gp_Dir cd=maxc; //obj.maxCurvDir();
        bool retry=false, stop=false;

        int pass=0;
        double cur_curv;
        do {
          cout<<"try "<<pass<<endl;
          gp_XY duv(
                cd.XYZ().Dot(d1u.XYZ()),
                cd.XYZ().Dot(d1v.XYZ())
                );
          duv.Normalize();

          uv += duv*dir * 1e-2*delta_uv_max;

          Obj2 obj2( f, uv, duv );

          cout<<"uv="<<uv.X()<<", "<<uv.Y()<<" => "<<flush;

          arma::mat t0, steps;
          t0 << 0.;
          steps<<1e-3*delta_uv_max;
          arma::mat t = nonlinearMinimizeND(obj2, t0, 1e-6, steps);
          uv=obj2.curuv(t);

          cout<<"uv="<<uv.X()<<", "<<uv.Y()<<endl;

          cur_curv = obj2(t);


          // no progress
          if (uv.Subtracted(uv_last).Modulus()<1e-8)
          {
            if (pass==0)
            {
              cd=minc;
              retry=true;
            }
            else
            {
              retry=false;
              stop=true;
            }
          }

          pass++;

        } while (retry && (pass<2));

        if (stop) break;

        // hit region with no curvature
        if ( fabs(cur_curv)/fabs(global_max_curv) < 0.05 )
          break;

      }
    }

  //  TopoDS_Compound res;
  //  BRep_Builder bb;
  //  bb.MakeCompound ( res );
  //  std::ofstream dbg("uv.txt");
  //  for (size_t i=0; i<pts.size(); i++)
  //  {
  //    dbg<<pts[i].X()<<" "<<pts[i].Y()<<endl;
  //    bb.Add(res, BRepBuilderAPI_MakeVertex(obj.surf->Value(pts[i].X(), pts[i].Y())));
  //  }
  //  setShape(res);

//    int erased=0;
//    for (size_t i=0; i<pts.size()-1; i++)
//    {
//      if ( pts[i].Subtracted(pts[i+1]).Modulus() < 1e-8 )
//      {
//        pts.erase( pts.begin()+ int(i+1) );
//        erased++;
//      }
//    }
//    cout << "erased "<<erased<<" duplicates"<<endl;

    if (pts.size()>1)
    {
      cout<<"Performing interpolation over "<<pts.size()<<" points"<<endl;

      Handle_TColgp_HArray1OfPnt2d pts2(new TColgp_HArray1OfPnt2d(1, pts.size()));
      for (size_t i=0; i<pts.size(); i++) pts2->SetValue(i+1, pts[i]);
      Geom2dAPI_Interpolate ip(pts2, false, 1e-3);
      ip.Perform();
      if (!ip.IsDone())
      {
        throw insight::Exception("Building 2D spline failed!");
      }

      cout<<"edge"<<endl;
      TopoDS_Edge ec = BRepBuilderAPI_MakeEdge(ip.Curve(), obj.surf).Edge();
      BRepLib::BuildCurve3d(ec);
    //  Handle_Geom_Curve crv;
    //  setShape(BRepBuilderAPI_MakeEdge(crv));

      cout<<"done"<<endl;

      bb.Add(res, ec);
    }
    else
    {
      cout<<"not a sufficient number of points for interpolation (only "<<pts.size()<<")"<<endl;
    }
  }

  setShape(res);

}




MaxSurfaceCurvature::MaxSurfaceCurvature(FeatureSetPtr faces)
: faces_(faces)
{
}




FeaturePtr MaxSurfaceCurvature::create(FeatureSetPtr faces)
{
    return FeaturePtr(new MaxSurfaceCurvature(faces));
}



MaxSurfaceCurvature::operator const TopoDS_Edge& () const
{
  return TopoDS::Edge(shape());
}




void MaxSurfaceCurvature::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "MaxSurfaceCurvature",
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(

    ( '(' >> ruleset.r_faceFeaturesExpression >> ')' )
        [ qi::_val = phx::bind(&MaxSurfaceCurvature::create, qi::_1) ]

    ))
  );
}




FeatureCmdInfoList MaxSurfaceCurvature::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "MaxSurfaceCurvature",

            "( <faceSelection> )",

            "Computes the maximum curvature line on a surface originating from the point of maximum curvature in the selected faces. Returns a compound."
        )
    );
}



}
}

