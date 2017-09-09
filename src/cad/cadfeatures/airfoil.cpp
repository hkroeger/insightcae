/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "airfoil.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {

    
    

defineType(Airfoil);
addToFactoryTable(Feature, Airfoil);




Airfoil::Airfoil()
{}




Airfoil::Airfoil
(
    const std::string& name, VectorPtr p0, VectorPtr ex, VectorPtr ez, ScalarPtr c, ScalarPtr t, ScalarPtr r_EK, ScalarPtr r_AK
)
: name_(name), p0_(p0), ez_(ez), ex_(ex), c_(c), t_(t), r_EK_(r_EK), r_AK_(r_AK)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=name_;
  h+=p0_->value();
  h+=ez_->value();
  h+=ex_->value();
  h+=c_->value();
  h+=t_->value();
  h+=r_EK->value();
  h+=r_AK->value();
}




FeaturePtr Airfoil::create
(
    const std::string& name, VectorPtr p0, VectorPtr ex, VectorPtr ez, ScalarPtr c, ScalarPtr t, ScalarPtr r_EK, ScalarPtr r_AK
)
{
    return FeaturePtr(new Airfoil(name, p0, ex, ez, c, t, r_EK, r_AK));
}

const double SMALL=1e-10;
const double LSMALL=1e-6;




void InterpolatedCurve::init()
{
  accel_=gsl_interp_accel_alloc();
  interp_=gsl_interp_alloc (gsl_interp_akima /*gsl_interp_linear*/, n);
  gsl_interp_init(interp_, x_, y_, n);
}


InterpolatedCurve::InterpolatedCurve
(
    const InterpolatedCurve& i
)
: n(i.n),
  x_(new double[n]),
  y_(new double[n])
{
  memcpy(i.x_, x_, sizeof(double)*n);
  memcpy(i.y_, y_, sizeof(double)*n);
  init();
}

InterpolatedCurve::InterpolatedCurve
(
    const std::vector<double>& datx,
    const std::vector<double>& daty
)
: x_(new double[datx.size()]),
  y_(new double[datx.size()])
{
  n=datx.size();
  for (int i=0; i<n; i++)
  {
    x_[i]=datx[i];
    y_[i]=daty[i];
  }
  init();
}

InterpolatedCurve::~InterpolatedCurve()
{
  gsl_interp_free(interp_);
  gsl_interp_accel_free(accel_);
  delete x_;
  delete y_;
}

double InterpolatedCurve::operator()(double x, int der) const
{
  switch (der)
  {
  case 0:
    return gsl_interp_eval(interp_, x_, y_, x, accel_);
  case 1:
    return gsl_interp_eval_deriv(interp_, x_, y_, x, accel_);
  }
  return 0.0;
}


FoilShape Airfoil::lookupFoil(const std::string& name) const
{
    // hardcoded foils
    if (name=="naca16")
    {
        std::vector<double> x=boost::assign::list_of
            (0.0)
            (0.0000182976)
            (0.000290262)
            (0.00144858)
            (0.0044873)
            (0.0106757)
            (0.0214466)
            (0.0382669)
            (0.0625)
            (0.0952699)
            (0.137337)
            (0.188996)
            (0.25)
            (0.319522)
            (0.396156)
            (0.477953)
            (0.5625)
            (0.647028)
            (0.728553)
            (0.804029)
            (0.870513)
            (0.925328)
            (0.966216)
            (0.991463)
            (1.0);
            
        std::vector<double> c=boost::assign::list_of
            (0.0)
            (0.000228043)
            (0.0033802)
            (0.0147125)
            (0.0386641)
            (0.0796435)
            (0.140361)
            (0.221377)
            (0.320047)
            (0.433501)
            (0.554658)
            (0.676396)
            (0.790484)
            (0.887262)
            (0.958834)
            (0.996397)
            (0.993823)
            (0.945836)
            (0.848174)
            (0.691811)
            (0.464924)
            (0.259792)
            (0.111414)
            (0.0268224)
            (0.000290785);
            
        std::vector<double> t=boost::assign::list_of
            (0.0)
            (0.013292)
            (0.0429429)
            (0.0812459)
            (0.136199)
            (0.205944)
            (0.281348)
            (0.364899)
            (0.458295)
            (0.559135)
            (0.662756)
            (0.761906)
            (0.850152)
            (0.923509)
            (0.975216)
            (1.0)
            (0.989225)
            (0.937383)
            (0.832259)
            (0.678651)
            (0.500726)
            (0.318269)
            (0.157658)
            (0.0424425)
            (0.0000000263408);

        return FoilShape
        (
            x,
            InterpolatedCurvePtr(new InterpolatedCurve(x, c)),
            InterpolatedCurvePtr(new InterpolatedCurve(x, t))
        );
    }
    else if (name=="naca2412")
    {
        std::vector<double> x=boost::assign::list_of
(0.)
(0.0000182976)
(0.000290262)
(0.00144858)
(0.0044873)
(0.0106757)
(0.0214466)
(0.0382669)
(0.0625)
(0.0952699)
(0.137337)
(0.188996)
(0.25)
(0.319522)
(0.396156)
(0.477953)
(0.5625)
(0.647028)
(0.728553)
(0.804029)
(0.870513)
(0.925328)
(0.966216)
(0.991463)
(1.)
            ;
            
        std::vector<double> c=boost::assign::list_of
(0.)
(0.000236056)
(0.00274534)
(0.0120594)
(0.0351464)
(0.0784082)
(0.148252)
(0.245463)
(0.362424)
(0.49397)
(0.636297)
(0.770426)
(0.881327)
(0.960535)
(1.)
(0.994468)
(0.942711)
(0.849471)
(0.722737)
(0.571847)
(0.408578)
(0.250761)
(0.118682)
(0.0308032)
(0.)
            ;
            
        std::vector<double> t=boost::assign::list_of
(0.)
(0.0141199)
(0.055254)
(0.122292)
(0.210959)
(0.315492)
(0.431485)
(0.55637)
(0.678584)
(0.790159)
(0.885483)
(0.955271)
(0.994842)
(1.)
(0.967327)
(0.897681)
(0.797067)
(0.675783)
(0.5455)
(0.415362)
(0.290824)
(0.177513)
(0.0842453)
(0.0219849)
(0.)
            ;

        return FoilShape
        (
            x,
            InterpolatedCurvePtr(new InterpolatedCurve(x, c)),
            InterpolatedCurvePtr(new InterpolatedCurve(x, t))
        );
    }
}


//-----------------------------------------------------------------------------------------------------------------------------------------------

struct Params_findTangent
{
  const InterpolatedCurve* tsp;
  double rho;
  double tr, len;
};

#define sign(x) (x<0.0?-1.0:1.0)

#define xL_LE(rho, dydx) \
    (fabs(dydx)<1e-10?rho:(rho - sign(dydx)*pow(dydx*rho, 2)/::sqrt(pow(dydx,2)*(1.0+pow(dydx,2))*rho*rho)))

#define yL_LE(rho, xL) \
    (::sqrt( pow(rho,2) - pow(rho-xL,2) ))

#define xT_TE(len, rho, dydx) \
    (fabs(dydx)<1e-10?len-rho:(len + rho*( -sign(dydx)*pow(dydx, 2)*rho/::sqrt(pow(dydx,2)*(1.0+pow(dydx,2))*rho*rho)-1.0) ))

#define yT_TE(len, rho, xT) \
    (::sqrt( pow(rho,2) - pow(rho-(len-xT),2) ))


double thicknessLE_findTangent(double xnorm, void *params)
{
  Params_findTangent* p=(Params_findTangent*) params;
  if (xnorm<0.0) xnorm=0.0;
  if (xnorm>1.0) xnorm=1.0;

  double x=xnorm*p->len;

  double y=p->tr * p->tsp->operator()(xnorm, 0);
  double dydx=p->tr * p->tsp->operator()(xnorm, 1)/p->len;

  double xL=xL_LE(p->rho, dydx);
  double yL=yL_LE(p->rho, xL);

  //cout<<"xL="<<xL<<", yL="<<yL<<", x="<<x<<", y="<<y<<", dydx="<<dydx<<", xnorm="<<xnorm<<endl;

  //double lin_dydx=(y-yL)/(x-xL);
  double y2=yL+(x-xL)*dydx;

  //cout<<"xL="<<xL<<", yL="<<yL<<", xnorm="<<xnorm<<", y="<<y<<", dydx="<<dydx<<" ("<<(y-y2)<<")"<<endl;
  return y-y2;
}


double thicknessTE_findTangent(double xnorm, void *params)
{
  Params_findTangent* p=(Params_findTangent*) params;
  if (xnorm<0.0) xnorm=0.0;
  if (xnorm>1.0) xnorm=1.0;

  double x=xnorm*p->len;

  double y=p->tr * p->tsp->operator()(xnorm, 0);
  double dydx=p->tr * p->tsp->operator()(xnorm, 1)/p->len;


  double xT=xT_TE(p->len, p->rho, dydx);
  double yT=yT_TE(p->len, p->rho, xT);

  //cout<<"xT="<<xT<<", yT="<<yT<<", x="<<x<<", y="<<y<<", dydx="<<dydx<<", xnorm="<<xnorm<<endl;

  //double lin_dydx=(y-yT)/(x-xT);
  double y2=yT+(x-xT)*dydx;

  //cout<<dydx<<" "<<lin_dydx<<" ("<<(y-y2)<<")"<<endl;
  return y-y2;
}


double thicknessLE_findTangent_df(double xnorm, void *params)
{
  //Params_findTangent* p=(Params_findTangent*) params;
  if (xnorm<0.0) xnorm=0.0;
  if (xnorm>1.0) xnorm=1.0;

  double xc=xnorm;

  const double eps=1e-6;
  double xm=xc-eps;
  if (xm<=0.0) xm = 1e-3;
  double xp=xc+eps;
  if (xp>=1.0) xp = 1.0 - 1e-3;
  double ym=thicknessLE_findTangent(xm, params);
  double yp=thicknessLE_findTangent(xp, params);
  double der=(yp-ym)/(xp-xm);
  //cout<<"xc="<<xc<<" => xm="<<xm<<"/xp="<<xp<<" : der="<<der<<endl;
  return der;
}

void thicknessLE_findTangent_fdf(double xnorm, void *params, 
    double * f, double * df)
{
  *f=thicknessLE_findTangent(xnorm, params);
  *df=thicknessLE_findTangent_df(xnorm, params);
}

double thicknessTE_findTangent_df(double xnorm, void *params)
{
  //Params_findTangent* p=(Params_findTangent*) params;
  if (xnorm<0.0) xnorm=0.0;
  if (xnorm>1.0) xnorm=1.0;

  double xc=xnorm;
  const double eps=1e-6;
  double xm=xc-eps;
  if (xm<0.0) xm=0.0;
  double xp=xc+eps;
  if (xp>1.0) xp=1.0;
  double ym=thicknessTE_findTangent(xm, params);
  double yp=thicknessTE_findTangent(xp, params);
  return (yp-ym)/(xp-xm);
}

void thicknessTE_findTangent_fdf(double xnorm, void *params, 
    double * f, double * df)
{
  *f=thicknessTE_findTangent(xnorm, params);
  *df=thicknessTE_findTangent_df(xnorm, params);
}

void airfoil_gsl_error_handler
(
                const char * reason,
                const char *,
                int,
                int
)
{
        throw insight::Exception("Error in GSL library: "+std::string(reason));
}

void Airfoil::generateDiscreteThickness
(
    const InterpolatedCurve& tsp,
    int n,
    const double* x,
    double* thickness,
    double tr, double len,
    double rEk,
    double rAk
) const
{
  gsl_error_handler_t *oldhandler=gsl_set_error_handler(&airfoil_gsl_error_handler);

  if (tr>SMALL)
  {
    double x_LE=0.0, x_TE=1.0;

    if (rEk>LSMALL)
    {
      //cout<<"rEK:"<<endl;
      int status;
      int iter = 0, max_iter = 100;
      //int retry=0;

      const gsl_root_fsolver_type *TT;
      gsl_root_fsolver *s;
      double x_lo = 0.001;
      double x_hi = 0.5;

      gsl_function F;
      struct Params_findTangent params = {&tsp, rEk, tr, len};

      F.function = &thicknessLE_findTangent;
      F.params = &params;

      TT = gsl_root_fsolver_brent;
      s = gsl_root_fsolver_alloc (TT);
      try
      {
        gsl_root_fsolver_set (s, &F, x_lo, x_hi);
        do
        {
          iter++;
          status = gsl_root_fsolver_iterate (s);
          x_LE = gsl_root_fsolver_root (s);
          x_lo = gsl_root_fsolver_x_lower (s);
          x_hi = gsl_root_fsolver_x_upper (s);
          status = gsl_root_test_interval (x_lo, x_hi, 1e-12, 0);
        }
        while (status == GSL_CONTINUE && iter < max_iter);
      }
      catch (insight::Exception e)
      {
        //cout<<"Bisection (LE) was not successful: "<<e.message()<<endl;
      }
      gsl_root_fsolver_free (s);

    }

    if (rAk>LSMALL)
    {
      int status;
      int iter = 0, max_iter = 100;

      const gsl_root_fsolver_type *TT;
      gsl_root_fsolver *s;
      double x_lo = 0.5;
      double x_hi = 0.999;

      gsl_function F;
      struct Params_findTangent params = {&tsp, rAk, tr, len};

      F.function = &thicknessTE_findTangent;
      F.params = &params;

      TT = gsl_root_fsolver_brent;
      s = gsl_root_fsolver_alloc (TT);
      try
      {
        gsl_root_fsolver_set (s, &F, x_lo, x_hi);
        do
        {
          iter++;
          status = gsl_root_fsolver_iterate (s);
          x_TE = gsl_root_fsolver_root (s);
          x_lo = gsl_root_fsolver_x_lower (s);
          x_hi = gsl_root_fsolver_x_upper (s);
          status = gsl_root_test_interval (x_lo, x_hi, 1e-12, 0);
        }
        while (status == GSL_CONTINUE && iter < max_iter);
      }
      catch (insight::Exception e)
      {
        //cout<<"Bisection (TE) was not successful: "<<e.message()<<endl;
      }
      gsl_root_fsolver_free (s);
    }

    for (int i=0; i<n; i++)
    {
      double xnorm=x[i];
      double xx=len*x[i];
      if (xnorm<x_LE)
      {
        double y=tr*tsp(x_LE, 0);
        double dydx=tr*tsp(x_LE, 1)/len;

        double xL=xL_LE(rEk, dydx);

        if (xx<=xL)
          thickness[i]=yL_LE(rEk, xx);
        else
          thickness[i]=y-dydx*(len*x_LE-xx);
      }
      else if (xnorm>x_TE)
      {
        double y=tr*tsp(x_TE, 0);
        double dydx=tr*tsp(x_TE, 1)/len;
        double xT=xT_TE(len, rAk, dydx);
        if (xx>=xT)
          thickness[i]=yT_TE(len, rAk, xx);
        else
          thickness[i]=y+dydx*(xx-len*x_TE);
      }
      else
        thickness[i]=tr*tsp(x[i]);
    }
  }
  else
  {
    for (int i=0; i<n; i++)
    {
      thickness[i]=0.0;
    }
  }

//   gsl_set_error_handler(oldhandler);
}

void Airfoil::generateDiscreteSection
(
    const FoilShape& f,
    double len,
    double thickness,
    double camber,
    double r_Ek,
    double r_Ak,
    int n,
    const double* x,
    double* xup, double* yup,
    double* xlo, double* ylo,
    bool doEdgeThickening
) const
{

  InterpolatedCurve& csp=*boost::get<1>(f);
  InterpolatedCurve& tsp=*boost::get<2>(f);

  double tk[n];

  if (!doEdgeThickening)
  {
    r_Ek=0.0;
    r_Ak=0.0;
  }
  generateDiscreteThickness(tsp, n, x, tk, thickness, len, r_Ek, r_Ak);

  for (int i=0; i<n; i++)
  {
    double c=camber*csp(x[i]);
    double t=tk[i]; //thickness*tsp(x[i]);
//     if (ymean!=NULL) ymean[i]=c;
    double c_der=0.0;
    if (len>1e-12)
      c_der=camber*csp(x[i], 1)/len;
    double psi=::atan(c_der);
    double spsi=::sin(psi); //0.0
    double cpsi=::cos(psi); //1.0

    xup[i] = len*x[i] - 0.5*t*spsi;
    yup[i] = c    + 0.5*t*cpsi;
    xlo[i] = len*x[i] + 0.5*t*spsi;
    ylo[i] = c    - 0.5*t*cpsi;
  }
}




void Airfoil::build()
{
 
  FoilShape f = lookupFoil(name_);
  
  std::vector<double>& x = boost::get<0>(f);
  
  arma::mat ex=ex_->value();
  arma::mat ez=ez_->value();
  
  double L=arma::norm(ex,2);
  ex/=L;
  ez/=arma::norm(ez,2);
  arma::mat ey=arma::cross(ez, ex);
  
  double 
    t=L*t_->value(), // thickness
    c=L*c_->value(), // max camber
    r_EK=r_EK_->value(),
    r_AK=r_AK_->value();
    
  // create support points
  // ===================================================
  int np=x.size();
  double xs[2][np], ys[2][np]; //, ym2[n];
  
  generateDiscreteSection
  (
      f, 
      L, t, c,
      r_EK, r_AK,
      np, x.data(), 
      xs[0], ys[0], xs[1], ys[1],
      true/*, ym2*/
  );


  TColgp_Array1OfPnt pts_up(1, np), pts_lo(1, np);

  for (int j=0; j<np; j++) 
  {
    pts_up.SetValue
    (
        j+1, 
        to_Pnt( p0_->value() +xs[0][j]*ex +ys[0][j]*ey )
    );
    pts_lo.SetValue
    (
        j+1, 
        to_Pnt( p0_->value() +xs[1][j]*ex +ys[1][j]*ey )
    );
  }
  
  // build splines from points
  // ===================================================
  GeomAPI_PointsToBSpline splbuilderup(pts_up);
  Handle_Geom_BSplineCurve crvup=splbuilderup.Curve();
  TopoDS_Edge eup=BRepBuilderAPI_MakeEdge(crvup, crvup->FirstParameter(), crvup->LastParameter());

  GeomAPI_PointsToBSpline splbuilderlo(pts_lo);
  Handle_Geom_BSplineCurve crvlo=splbuilderlo.Curve();
  TopoDS_Edge elo=BRepBuilderAPI_MakeEdge(crvlo, crvlo->FirstParameter(), crvlo->LastParameter());
  
  
  BRepBuilderAPI_MakeWire w;
  w.Add(eup);
  w.Add(elo);
    
  BRepBuilderAPI_MakeFace fb(w.Wire(), true);
  if (!fb.IsDone())
    throw insight::Exception("Failed to generate planar face!");
  
//   providedSubshapes_["OuterWire"].reset(new SolidModel(w.Wire()));
  providedSubshapes_["OuterWire"]=FeaturePtr(new Feature(w.Wire()));
  
  refvalues_["L"]=L;
  
  refpoints_["p_le"]=p0_->value();
  refpoints_["p_te"]=p0_->value() +L*ex;
  
  setShape(fb.Face());
//   setShape(w.Wire());
}




Airfoil::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape());
}




void Airfoil::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Airfoil",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '('  >> ruleset.r_string >> ',' 
           >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression
           >> ',' >> ruleset.r_scalarExpression //c 
           >> ',' >> ruleset.r_scalarExpression //t 
           >> ( (',' >> qi::lit("r_EK") >> ruleset.r_scalarExpression) | qi::attr(scalarconst(0.0)) ) 
           >> ( (',' >> qi::lit("r_AK") >> ruleset.r_scalarExpression) | qi::attr(scalarconst(0.0)) ) 
           >> ')' ) 
	[ qi::_val = phx::bind(&Airfoil::create, qi::_1, qi::_2, qi::_3, qi::_4, qi::_5, qi::_6, qi::_7, qi::_8) ]
      
    ))
  );
}




FeatureCmdInfoList Airfoil::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Airfoil",
            "( <string:name>, <vector:p0>, <vector:L>, <vector:ez>, <scalar:c>, <scalar:t> [, r_EK <scalar:rEK>] [, <scalar:rAK> ])",
            "Creates an airfoil section with specified camber c and thickness t. The camber and thickness distribution are selected by the name argument."
            "Optionally, minimum leading and trailing edge radii can be enforced."
            " The leading edge is positioned at point p0. Length and direction of the chord line are specified by vector L."
            " The normal direction of the foil section, i.e. spanwise direction of the wing, is given by vector ez."
        )
    );
}



}
}

