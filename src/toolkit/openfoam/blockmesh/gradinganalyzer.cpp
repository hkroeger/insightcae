#include "gradinganalyzer.h"

#include "base/exception.h"
#include "base/boost_include.h"
#include "base/linearalgebra.h"


namespace insight {
namespace bmd {


GradingAnalyzer::GradingAnalyzer(double grad)
: grad_(grad)
{
}

GradingAnalyzer::GradingAnalyzer(double delta0, double delta1)
: grad_(delta1/delta0)
{
}

GradingAnalyzer::GradingAnalyzer(double delta0, double L, int n)
{
  struct Obj: public Objective1D
  {
    double Lbydelta0;
    int n;
    double F(double R) const
    { return (pow(R, n/(n-1.))-1.) / (pow(R, 1./(n-1.))-1.); }
    virtual double operator()(double x) const { return Lbydelta0-F(x); }
  } obj;
  obj.n=n;
  obj.Lbydelta0=L/delta0;
  try
  {
   grad_=nonlinearSolve1D(obj, 0.0001, 1000000);
  }
  catch (const std::exception& e)
  {
    std::ostringstream os;
    os<<"Could not determine grading to get minimum cell length "<<delta0<<" on edge of length "<<L<<" discretized with "<<n<<" cells."<<std::endl;
    os<<"(Only gradings between "<<0.0001<<" and "<<100000<<" were considered)"<<std::endl;
    throw insight::Exception(os.str());
  }
}

#include <gsl/gsl_errno.h>
#include <gsl/gsl_roots.h>

typedef boost::tuple<const GradingAnalyzer*,double,double> f_calc_n_param;

double f_calc_n(double n, void* ga)
{
  f_calc_n_param* p = static_cast<f_calc_n_param*>(ga);
  double L=p->get<1>();
  double delta0=p->get<2>();

  double grad=p->get<0>()->grad();
  if (grad<1.) grad=1./grad;

  double L_c=delta0*grad;
  if ( n>(1.+insight::SMALL) )
  {
      double G=pow(grad, 1./(n-1.));
      L_c=delta0*( pow(G,n)-1. )/(G-1.);
  }

  insight::assertion(
    !isnan(L_c),
      "failed computed target resolution");

  return L_c-L;
}

int GradingAnalyzer::calc_n(double delta0, double L) const
{
    insight::assertion(
        delta0>insight::SMALL,
        "invalid input: base mesh size must be positive and not zero! Got: %g", delta0);

 if (delta0>L) return 1;

 if ( (1.+grad_)*delta0 > L) return 2;

 if (fabs(grad_-1.)<insight::SMALL)
 {
     return std::max(1., ::ceil(L/delta0));
 }

  int i, times, status;
  gsl_function f;
  gsl_root_fsolver *workspace_f;
  double x, x_l, x_r;


    workspace_f = gsl_root_fsolver_alloc(gsl_root_fsolver_bisection);

    f.function = &f_calc_n;
    f_calc_n_param p(this, L, delta0);
    f.params = static_cast<void*>(&p);

    x_l = 1.;
    x_r = 10000.;

    gsl_root_fsolver_set(workspace_f, &f, x_l, x_r);

    for(times = 0; times < 100; times++)
    {
        status = gsl_root_fsolver_iterate(workspace_f);

        x_l = gsl_root_fsolver_x_lower(workspace_f);
        x_r = gsl_root_fsolver_x_upper(workspace_f);

        status = gsl_root_test_interval(x_l, x_r, 1.0e-13, 1.0e-20);
        if(status != GSL_CONTINUE)
        {
            break;
        }
    }

    gsl_root_fsolver_free(workspace_f);

  return x_l;
}

double GradingAnalyzer::calc_L(double delta0, int n) const
{
  double r=pow(grad_, 1./(n-1.));
  return delta0 * (pow(r, n)-1.) / (r-1.);
}

double GradingAnalyzer::calc_delta0(double L, int n) const
{
  double r=pow(grad_, 1./(n-1.));
  return L / ((pow(r, n)-1.) / (r-1.));
}

double GradingAnalyzer::calc_delta1(double delta0) const
{
  return delta0*grad_;
}



} // namespace bmd
} // namespace insight
