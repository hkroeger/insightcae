/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2013  hannes <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "linearalgebra.h"
#include "boost/lexical_cast.hpp"
#include "boost/tuple/tuple.hpp"
#include "gsl/gsl_multimin.h"

using namespace arma;
using namespace boost;

namespace insight
{

mat vec3(double x, double y, double z)
{
  mat v;
  v << x <<endr << y << endr << z <<endr;
  return v;
}

arma::mat tensor3(
  double xx, double xy, double xz,
  double yx, double yy, double yz,
  double zx, double zy, double zz
)
{
  mat v;
  v 
    << xx << xy <<  xz <<endr
    << yx << yy <<  yz <<endr
    << zx << zy <<  zz <<endr;
    
  return v;
}

mat vec2(double x, double y)
{
  mat v;
  v << x <<endr << y << endr;
  return v;
}

mat rotMatrix( double theta, mat u )
{
    double s=sin(theta);
    double c=cos(theta);
    double ux=u[0];
    double uy=u[1];
    double uz=u[2];
    mat m;
    m << ux*ux+(1-ux*ux)*c << ux*uy*(1-c)-uz*s << ux*uz*(1-c)+uy*s << endr
      << ux*uy*(1-c)+uz*s << uy*uy+(1-uy*uy)*c << uy*uz*(1-c)-ux*s << endr
      << ux*uz*(1-c)-uy*s << uy*uz*(1-c)+ux*s << uz*uz+(1-uz*uz)*c << endr;
    return m;
}

std::string toStr(const arma::mat& v3)
{
  std::string s="";
  for (int i=0; i<3; i++)
    s+=" "+lexical_cast<std::string>(v3(i));
  return s+" ";
}

arma::mat linearRegression(const arma::mat& y, const arma::mat& x)
{
  return solve(x.t()*x, x.t()*y);
}

arma::mat polynomialRegression(const arma::mat& y, const arma::mat& x, int maxorder, int minorder)
{
  arma::mat xx(x.n_rows, maxorder-minorder);
  for (int i=0; i<maxorder-minorder; i++)
    xx.col(i)=pow(x, minorder+i);
  return linearRegression(y, xx);
}

typedef boost::tuple<RegressionModel&, const arma::mat&, const arma::mat&> RegressionData;

double f_nonlinearRegression(const gsl_vector * p, void * params)
{
  RegressionData* md = static_cast<RegressionData*>(params);
  
  RegressionModel& m = boost::get<0>(*md);
  const arma::mat& y = boost::get<1>(*md);
  const arma::mat& x = boost::get<2>(*md);
  
  m.setParameters(p->data);
  
  return m.computeQuality(y, x);
}

RegressionModel::~RegressionModel()
{
}

arma::mat RegressionModel::weights(const arma::mat& x) const
{
  return ones(x.n_rows);
}

double RegressionModel::computeQuality(const arma::mat& y, const arma::mat& x) const
{
  double q=0.0;
  arma::mat w=weights(x);
  for (int r=0; r<y.n_rows; r++)
  {
    q +=  w(r) * pow(norm( y.row(r) - evaluateObjective(x.row(r)), 2 ), 2);
  }
  return q;
}


double nonlinearRegression(const arma::mat& y, const arma::mat& x,RegressionModel& model)
{
  const gsl_multimin_fminimizer_type *T = 
    gsl_multimin_fminimizer_nmsimplex2;
  gsl_multimin_fminimizer *s = NULL;
  gsl_vector *ss, *p;
  gsl_multimin_function minex_func;

  size_t iter = 0;
  int status;
  double size;

  /* Starting point */
  p = gsl_vector_alloc (model.numP());
  gsl_vector_set_all (p, 1.0);

  /* Set initial step sizes to 0.1 */
  ss = gsl_vector_alloc (model.numP());
  gsl_vector_set_all (ss, 0.1);

  /* Initialize method and iterate */
  RegressionData param(model, y, x);
  minex_func.n = model.numP();
  minex_func.f = f_nonlinearRegression;
  minex_func.params = (void*) (&param);

  s = gsl_multimin_fminimizer_alloc (T, model.numP());
  gsl_multimin_fminimizer_set (s, &minex_func, p, ss);

  do
    {
      iter++;
      status = gsl_multimin_fminimizer_iterate(s);
      
      if (status) 
        break;

      size = gsl_multimin_fminimizer_size (s);
      status = gsl_multimin_test_size (size, 1e-3);

      if (status == GSL_SUCCESS)
        {
          printf ("converged to minimum at\n");
        }

      printf ("%5d %10.3e %10.3e f() = %7.3f size = %.3f\n", 
              iter,
              gsl_vector_get (s->x, 0), 
              s->fval, size);
    }
  while (status == GSL_CONTINUE && iter < 100);
  
  model.setParameters(s->x->data);
  
  gsl_vector_free(p);
  gsl_vector_free(ss);
  gsl_multimin_fminimizer_free (s);

  return model.computeQuality(y, x);
}

#include <gsl/gsl_errno.h>
#include <gsl/gsl_roots.h>

Objective1D::~Objective1D()
{
}


double F_obj(double x, void *param)
{
  const Objective1D& model=*static_cast<Objective1D*>(param);
  return model(x);
}

double nonlinearSolve1D(const Objective1D& model, double x_min, double x_max)
{
  int i, times, status;
  gsl_function f;
  gsl_root_fsolver *workspace_f;
  double x, x_l, x_r;

 
  workspace_f = gsl_root_fsolver_alloc(gsl_root_fsolver_bisection);

  f.function = &F_obj;
  f.params = const_cast<void *>(static_cast<const void*>(&model));

  x_l = x_min;
  x_r = x_max;

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

arma::mat movingAverage(const arma::mat& timeProfs, double fraction)
{
  if ( (timeProfs.n_rows>0) && (timeProfs.n_cols>2) )
  {
    int n_raw=timeProfs.n_rows;
    int window=std::min(n_raw, std::max(1, int( double(n_raw)*fraction )) );
    int n_avg=n_raw-window;
    
    arma::mat result=zeros(n_avg, timeProfs.n_cols);
    
    for (int i=0; i<n_avg; i++)
    {
      int from=i, to=from+window;
      //cout<<i<<" "<<n_avg<<" "<<n_raw<<" "<<from<<" "<<to<<endl;
      result(i,0)=timeProfs(to, 0); // copy time
      for (int j=1; j<timeProfs.n_cols; j++)
	result(i, j)=mean(timeProfs.rows(from, to).col(j));
    }
    return result;
  } else 
    return timeProfs;
}

}