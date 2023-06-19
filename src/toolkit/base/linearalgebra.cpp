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
 *
 */

#include <stdio.h>
#include <math.h>

#include "linearalgebra.h"
#include "boost/lexical_cast.hpp"
#include "boost/tuple/tuple.hpp"
#include "boost/format.hpp"
#include "base/exception.h"
#include "base/units.h"
#include "base/cppextensions.h"

// #include "minpack.h"
// #include <dlib/optimization.h>

using namespace arma;
using namespace boost;

namespace std
{

double armaMatIdentityTolerance = insight::SMALL;

bool operator<(const arma::mat& v1, const arma::mat& v2)
{
    insight::assertion(v1.n_elem==3,
                       "Internal error: comparison only defined for 3-vectors!");
    insight::assertion(v2.n_elem==3,
                       "Internal error: comparison only defined for 3-vectors!");

    if ( fabs(v1(0) - v2(0))<armaMatIdentityTolerance )
    {
        if ( fabs(v1(1) - v2(1))<armaMatIdentityTolerance )
        {
            if (fabs(v1(2)-v2(2))<armaMatIdentityTolerance )
            {
                return false;
            }
            else return v1(2)<v2(2);
        }
        else return v1(1)<v2(1);
    }
    else return v1(0)<v2(0);
}

}

namespace insight
{

const double SMALL=1e-10;
const double LSMALL=1e-6;
  
void insight_gsl_error_handler
(
 const char* reason,
 const char* file,
 int line,
 int gsl_errno
)
{
  throw insight::GSLException(
                reason, file,
                line, gsl_errno
                );
}

GSLExceptionHandling::GSLExceptionHandling()
{
  oldHandler_ = gsl_set_error_handler(&insight_gsl_error_handler);
}

GSLExceptionHandling::~GSLExceptionHandling()
{
  gsl_set_error_handler(oldHandler_);
}



GSLException::GSLException(const char *reason, const char *file, int line, int gsl_errno)
    : insight::Exception(
          "Error in GSL subroutine: %s (errno %d)",
          reason, gsl_errno
          ),
      gsl_errno_(gsl_errno)
{}

int GSLException::gsl_errno() const
{
    return gsl_errno_;
}

mat vec1(double x)
{
  mat v;
  v << x << endr;
  return v;
}

mat vec2(double x, double y)
{
  mat v;
  v << x <<endr << y << endr;
  return v;
}

mat vec3(double x, double y, double z)
{
  mat v;
  v << x <<endr << y << endr << z <<endr;
  return v;
}

mat vec3FromComponents(const double* c)
{
    return vec3(c[0], c[1], c[2]);
}

arma::mat vec3FromComponents(const float *c)
{
    return vec3(c[0], c[1], c[2]);
}

mat readVec3(std::istream& is)
{
    double c[3];
    is >> c[0] >> c[1] >> c[2];
    return vec3FromComponents(c);
}

arma::mat normalized(const arma::mat &vec)
{
    double l = arma::norm(vec, 2);
    if (l>SMALL)
    {
        return vec/l;
    }
    else
    {
        throw insight::Exception("attempt to normalize a null vector!");
    }
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

double* toArray(const arma::mat& v)
{
  return const_cast<double*>(v.memptr());
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


bool isRotationMatrix(const arma::mat &R)
{
  return arma::norm(R.t()*R - arma::eye(3,3), 2) < 1e-6;
}

/**
* @brief rotationMatrixToRollPitchYaw
* computes euler angles from a rotation matrix
* @param R
* @return
* vector of euler angles in degrees
*/
arma::mat rotationMatrixToRollPitchYaw(const arma::mat& R)
{
  std::ostringstream os; os<<R;
  CurrentExceptionContext ex("compution euler angles from rotation matrix ("+os.str()+")", false);

  insight::assertion(
              isRotationMatrix(R),
              str(format("the argument is not a rotation matrix, residual=%e")
                  %(arma::norm(R.t()*R - arma::eye(3,3), 2)))
              );

  double sy=sqrt(R(0,0) * R(0,0) +  R(1,0) * R(1,0) );
  bool singular = sy < 1e-10;

  double x, y, z;

  if (!singular)
  {
    x = atan2(R(2,1), R(2,2));
    y = atan2(-R(2,0), sy);
    z = atan2(R(1,0), R(0,0));
  }
  else
  {
    x = atan2(-R(1,2), R(1,1));
    y = atan2(-R(2,0), sy);
    z = 0;
  }
  return vec3(x/SI::deg, y/SI::deg, z/SI::deg);
}


arma::mat rollPitchYawToRotationMatrix(const arma::mat& rollPitchYaw)
{
    return
             rotMatrix(rollPitchYaw(2)*SI::deg, vec3(0,0,1))
            *rotMatrix(rollPitchYaw(1)*SI::deg, vec3(0,1,0))
            *rotMatrix(rollPitchYaw(0)*SI::deg, vec3(1,0,0))
            ;
}

arma::mat rotated( const arma::mat&p, double theta, const arma::mat& axis, const arma::mat& p0 )
{
    return p0 + rotMatrix(theta, axis)*(p-p0);
}

std::string toStr(const arma::mat& v3)
{
  std::string s="";
  for (arma::uword i=0; i<3; i++)
    s+=" "+lexical_cast<std::string>(v3(i));
  return s+" ";
}

arma::mat linearRegression(const arma::mat& y, const arma::mat& x)
{
  return solve(x.t()*x, x.t()*y);
}

arma::mat polynomialRegression(const arma::mat& y, const arma::mat& x, int maxorder, int minorder)
{
  arma::mat xx(x.n_rows, arma::uword(maxorder-minorder) );
  for (arma::uword i=0; i<arma::uword(maxorder-minorder); i++)
    xx.col(i)=pow(x, arma::uword(minorder)+i);
  return linearRegression(y, xx);
}

double evalPolynomial(double x, const arma::mat& coeffs)
{
  double y=0;
  for (arma::uword k=0; k<coeffs.n_elem; k++)
  {
    arma::uword p=coeffs.n_elem-k-1;
    y+=coeffs(k)*pow(x,p);
  }
  return y;
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

void RegressionModel::getParameters(double*) const
{
  throw insight::Exception("not implemented!");
}

void RegressionModel::setStepHints(double* x) const
{
  setInitialValues(x);
  for (int i=0; i<numP(); i++)
    x[i] = std::copysign( std::max(1e-4, 0.1*std::fabs(x[i])), x[i] );
}

arma::mat RegressionModel::weights(const arma::mat& x) const
{
  return ones(x.n_rows);
}

double RegressionModel::computeQuality(const arma::mat& y, const arma::mat& x) const
{
  return arma::as_scalar( weights(x).t() * pow( y - evaluateObjective(x), 2) );
}


double nonlinearRegression(const arma::mat& y, const arma::mat& x,RegressionModel& model, double tol)
{
    try
    {
        const gsl_multimin_fminimizer_type *T =
            gsl_multimin_fminimizer_nmsimplex;
        gsl_multimin_fminimizer *s = NULL;
        gsl_vector *ss, *p;
        gsl_multimin_function minex_func;

        size_t iter = 0;
        int status;
        double size;

        /* Starting point */
        p = gsl_vector_alloc (model.numP());
        //gsl_vector_set_all (p, 1.0);
        model.setInitialValues(p->data);

        /* Set initial step sizes to 0.1 */
        ss = gsl_vector_alloc (model.numP());
//        gsl_vector_set_all (ss, 0.1);
        model.setStepHints(ss->data);

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
            status = gsl_multimin_test_size (size, tol);

        }
        while (status == GSL_CONTINUE && iter < 1000);

        model.setParameters(s->x->data);

        gsl_vector_free(p);
        gsl_vector_free(ss);
        gsl_multimin_fminimizer_free (s);

        return model.computeQuality(y, x);
    }
    catch (...)
    {
        std::ostringstream os;
        os<<"x=["<<x.t()<<"]\ty=["<<y.t()<<"]";
        throw insight::Exception("nonlinearRegression(): Failed to do regression.\nSupplied data: "+os.str());
    }

    return DBL_MAX;
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
  insight::CurrentExceptionContext ex(
              str(format("solving for root between x_min=%g and x_max=%g")
                  % x_min % x_max )
              );

  int i, times, status;
  gsl_function f;
  gsl_root_fsolver *workspace_f;
  double x, x_l, x_r;

 
  workspace_f = gsl_root_fsolver_alloc(gsl_root_fsolver_bisection);

  try
  {

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
  }
  catch (GSLException& ex)
  {
      gsl_root_fsolver_free(workspace_f);
      ex.messageRef() +=
              str(format("\ny(x_min)=%g, y(x_max)=%g, y(0.5*(x_min+x_max))=%g")
                  % model(x_min) % model(x_max) % model(0.5*(x_min+x_max)) );
      throw ex;
  }

  return x_l;
}




double nonlinearSolve1D(const std::function<double(double)>& model, double x_min, double x_max)
{
  struct Obj : public Objective1D
  {
    const std::function<double(double)>& model_;
    Obj(const std::function<double(double)>& m) : model_(m) {}
    double operator()(double x) const override
    {
      return model_(x);
    }
  } obj(model);
  return nonlinearSolve1D(obj, x_min, x_max);
}

double F_min_obj(const gsl_vector* x, void *param)
{
  const Objective1D& model=*static_cast<Objective1D*>(param);
//  cout<<"ITER: X="<<x->data[0]<<" F="<<model(x->data[0])<<endl;
  return model(x->data[0]);
}


double nonlinearMinimize1D(const Objective1D& model, double x_min, double x_max, double tol)
{
  try
  {
    const gsl_multimin_fminimizer_type *T = 
      gsl_multimin_fminimizer_nmsimplex;
    gsl_multimin_fminimizer *s = nullptr;
    gsl_vector *ss, *p;
    gsl_multimin_function minex_func;

    size_t iter = 0;
    int status;
    double size;

    /* Starting point */
    p = gsl_vector_alloc (1);
    gsl_vector_set_all (p, 0.5*(x_min+x_max));

    /* Set initial step sizes to 0.1 */
    ss = gsl_vector_alloc (1);
    gsl_vector_set_all (ss, 0.1);

    /* Initialize method and iterate */
    minex_func.n = 1;
    minex_func.f = &F_min_obj;
    minex_func.params = const_cast<void *>(static_cast<const void*>(&model));

    s = gsl_multimin_fminimizer_alloc (T, 1);
    gsl_multimin_fminimizer_set (s, &minex_func, p, ss);

    do
      {
	iter++;
	status = gsl_multimin_fminimizer_iterate(s);
	
	if (status) 
	  break;

	size = gsl_multimin_fminimizer_size (s);
    status = gsl_multimin_test_size (size, tol);

// 	if (status == GSL_SUCCESS)
// 	  {
// 	    printf ("converged to minimum at\n");
// 	  }

// 	printf ("%5d %10.3e %10.3e f() = %7.3f size = %.3f\n", 
// 		iter,
// 		gsl_vector_get (s->x, 0), 
// 		s->fval, size);
      }
    while (status == GSL_CONTINUE && iter < model.maxiter);
    
    double solution=s->x->data[0];
//     model.setParameters(s->x->data);
    
    gsl_vector_free(p);
    gsl_vector_free(ss);
    gsl_multimin_fminimizer_free (s);

    return solution; //model.computeQuality(y, x);
  }
  catch (...)
  {
    std::ostringstream os;
//     os<<"x=["<<x.t()<<"]\ty=["<<y.t()<<"]";
    throw insight::Exception("nonlinearMinimize1D(): Failed to do regression.\nSupplied data: "+os.str());
  }
  
  return DBL_MAX;
}



double nonlinearMinimize1D(const std::function<double(double)>& model, double x_min, double x_max, double tol)
{
  struct Obj : public Objective1D
  {
    const std::function<double(double)>& model_;
    Obj(const std::function<double(double)>& m) : model_(m) {}
    double operator()(double x) const override
    {
      return model_(x);
    }
  } obj(model);
  return nonlinearMinimize1D(obj, x_min, x_max, tol);
}



ObjectiveND::~ObjectiveND()
{}

typedef boost::tuple<const ObjectiveND&> nonlinearMinimizeNDData;

double f_nonlinearMinimizeND(const gsl_vector * p, void * params)
{
  nonlinearMinimizeNDData* md = static_cast<nonlinearMinimizeNDData*>(params);
  
  const ObjectiveND& m = boost::get<0>(*md);
  
  arma::mat x=arma::zeros(m.numP());
  for (int i=0; i<x.n_elem; i++)
  {
      x(i) = gsl_vector_get (p, i);
  }
  
  return m(x);
}



    
arma::mat nonlinearMinimizeND(
    const ObjectiveND& model, const arma::mat& x0,
    double tol, const arma::mat& steps, double relax )
{
    try
    {
        const gsl_multimin_fminimizer_type *T =
            gsl_multimin_fminimizer_nmsimplex;
            
        gsl_multimin_fminimizer *s = nullptr;
        gsl_vector *ss, *p, *olditer_p;
        gsl_multimin_function minex_func;

        size_t iter = 0;
        int status;
        double size;

        size_t numP = size_t(model.numP());

        /* Starting point */
        p = gsl_vector_alloc (numP);
        olditer_p = gsl_vector_alloc (model.numP());

        //gsl_vector_set_all (p, 1.0);
        for (size_t i=0; i<numP; i++)
        {
            gsl_vector_set (p, i, x0(i));
        }

        /* Set initial step sizes to 0.1 */
        ss = gsl_vector_alloc (numP);
        gsl_vector_set_all (ss, 0.1);

        if (steps.n_elem!=0)
          {
            for (size_t i=0; i<numP; i++)
              gsl_vector_set(ss, i, steps(i));
          }

        /* Initialize method and iterate */
        nonlinearMinimizeNDData param(model);
        minex_func.n = numP;
        minex_func.f = f_nonlinearMinimizeND;
        minex_func.params = (void*) (&param);

        s = gsl_multimin_fminimizer_alloc (T, numP);
        gsl_multimin_fminimizer_set (s, &minex_func, p, ss);


        do
        {
            gsl_vector_memcpy(olditer_p, s->x);
	    
            iter++;
            status = gsl_multimin_fminimizer_iterate(s);

            if (status) break;

            size = gsl_multimin_fminimizer_size (s);
            status = gsl_multimin_test_size (size, tol);
//            std::cerr<<"i="<<iter<<": F="<<s->fval<<std::endl;

            // relax
            for (int i=0; i<model.numP(); i++)
            {
              gsl_vector_set(s->x, i,
                  relax * gsl_vector_get(s->x, i)
                  +
                  (1.-relax) * gsl_vector_get(olditer_p, i)
              );
            }

        }
        while ( status == GSL_CONTINUE && (iter < model.maxiter) );
        
        arma::mat res=arma::zeros(numP);
        for (size_t i=0; i<numP; i++)
        {
            res(i)=gsl_vector_get (s->x, i);
        };
        
        gsl_vector_free(p);
        gsl_vector_free(ss);
//         gsl_vector_free(olditer_p);
        gsl_multimin_fminimizer_free (s);

        return res; //model.computeQuality(y, x);
    }
    catch (const std::exception& e)
    {
        std::ostringstream os;
        os<<"x0=["<<x0.t()<<"]";
        throw insight::Exception("nonlinearMinimizeND(): Exception occurred during regression.\nSupplied data: "+os.str()+"\n"+e.what());
    }

    return arma::zeros(x0.n_elem)+DBL_MAX;
}




arma::mat nonlinearMinimizeND(
    const std::function<double(const arma::mat&)>& model, const arma::mat& x0,
    double tol, const arma::mat& steps, int nMaxIter, double relax)
{
    struct Obj : public ObjectiveND
    {
      int np_;
      const std::function<double(const arma::mat&)>& model_;
      Obj(int np, const std::function<double(const arma::mat&)>& m) : np_(np), model_(m) {}
      double operator()(const arma::mat& x) const override
      {
        return model_(x);
      }
      int numP() const override { return np_; }
    } obj(x0.n_elem, model);

    obj.maxiter=nMaxIter;

    return nonlinearMinimizeND(obj, x0, tol, steps, relax);
}




arma::mat movingAverage(const arma::mat& timeProfs, double fraction, bool first_col_is_time, bool centerwindow)
{
  std::ostringstream msg;
  msg<<"Computing moving average for "
       "t="<<valueList_to_string(timeProfs.col(0))<<" and "
       "y="<<valueList_to_string(timeProfs.cols(1,timeProfs.n_cols-1))<<
       " with fraction="<<fraction<<", first_col_is_time="<<first_col_is_time<<" and centerwindow="<<centerwindow;
  CurrentExceptionContext ce(msg.str(), false);

  if (!first_col_is_time)
    throw insight::Exception("Internal error: moving average without time column is currently unsupported!");

  if (timeProfs.n_cols<2)
    throw insight::Exception("movingAverage: only dataset with "
      +lexical_cast<std::string>(timeProfs.n_cols)+" columns given. There is no data to average.");

  const arma::uword n_raw_max=10000;
  const arma::uword n_avg_max=1000;

  arma::mat data;
  if (timeProfs.n_rows>1)
  {
    arma::uword n_raw=timeProfs.n_rows;

    if (n_raw>n_raw_max)
    {
      // remove some rows to limit execution time
      arma::uword n_il=std::max<arma::uword>(1, n_raw/n_raw_max);
      arma::uword n_red=n_raw/n_il + (n_raw%n_il>0? 1 : 0);

      data=arma::zeros( n_red, timeProfs.n_cols);

      arma::uword j=0;
      for (arma::uword i=0; i<n_raw; i++)
      {
        if (i%n_il==0)
        {
//          if (j>data.n_rows-1) cout<<"j="<<j<<", i="<<i<<", n_raw="<<n_raw<<", n_il="<<n_il<<", n_red="<<n_red<<endl;
          data.row(j++)=timeProfs.row(i);
        }
      }

      insight::assertion( j == data.n_rows, str(format("wrong number of elements after reduction (%d <=> %d)")%j%data.n_rows) );
//      assert( j == data.n_rows );

      n_raw=data.n_rows;
    }
    else
    {
      data=timeProfs;
    }

    double x0=arma::min(data.col(0));
    double dx_raw=arma::max(data.col(0))-x0;

//    std::cout<<"mvg avg: range ["<<x0<<":"<<data.col(0).max()<<"]"<<std::endl;

    double window=fraction*dx_raw;
    double avgdx=dx_raw/double( std::min<size_t>(n_raw, n_avg_max) );

    // number of averages to compute
    arma::uword n_avg=std::min(n_raw, std::max(arma::uword(2), arma::uword((dx_raw-window)/avgdx) ));

    double window_ofs=window;
    if (centerwindow)
    {
        window_ofs=window/2.0;
    }

    arma::mat result=zeros(n_avg, data.n_cols);

//    cout<<"computing "<<n_avg<<" averages on "<<n_raw<<" raw samples."<<endl;
    
    for (arma::uword i=0; i<n_avg; i++)
    {
        double x = x0 + window_ofs + double(i)*avgdx;

        double from = x - window_ofs, to = from + window;

        arma::uword j0=0;
        if (first_col_is_time)
        {
            j0=1;
            result(i,0)=x;
        }
        arma::uvec indices = arma::find( (data.col(0)>=from) && (data.col(0)<=to) );
        arma::mat selrows=data.rows( indices );
        if (selrows.n_rows==0) // nothing selected: take the closest row
        {
            indices = arma::sort_index(
                        arma::mat(pow(data.col(0) - 0.5*(from+to), 2))
                        );
            selrows=data.row( indices(0) );
        }

        if (selrows.n_rows==1)
        {
            for (arma::uword j=j0; j<data.n_cols; j++)
            {
               result(i, j) = arma::as_scalar(selrows.col(j));
            }
        }
        else
        {

            arma::mat xcol=selrows.col(0);
            for (arma::uword j=j0; j<data.n_cols; j++)
            {
              arma::mat ccol=selrows.col(j);
              double I=0;
              for (arma::uword k=1; k<xcol.n_rows; k++)
              {
                I+=0.5*(ccol(k)+ccol(k-1))*(xcol(k)-xcol(k-1));
              }
              result(i, j) = I/(xcol.max()-xcol.min());
            }
        }
    }
    
    return result;
    
  }
  else
  {
    return timeProfs;
  }
}

arma::mat sortedByCol(const arma::mat&m, int c)
{

  arma::uvec indices = arma::sort_index(m.col(c));
  arma::mat xy = arma::zeros(m.n_rows, m.n_cols);
  for (arma::uword r=0; r<m.n_rows; r++)
    xy.row(r)=m.row(indices(r));
  return xy;
}

arma::mat filterDuplicates(const arma::mat&m)
{

  arma::mat xy = arma::zeros(m.n_rows, m.n_cols);

  xy.row(0)=m.row(0);

  arma::uword j=1;
  for (arma::uword r=1; r<m.n_rows; r++)
  {
    if (arma::norm(m.row(r)-m.row(j-1),2) > 1e-8)
    {
      xy.row(j++)=m.row(r);
    }
  }
  xy.resize(j, m.n_cols);

  //std::cout<<xy.row(0)<<std::endl<<xy.row(xy.n_rows-1)<<std::endl;

  return xy;
}


void Interpolator::initialize(const arma::mat& xy_us, bool force_linear)
{
    try
    {
        arma::mat xy = filterDuplicates(sortedByCol(xy_us, 0));
        xy_=xy;

        if (xy.n_cols<2)
            throw insight::Exception("Interpolate: interpolator requires at least 2 columns!");
        if (xy.n_rows<2)
            throw insight::Exception("Interpolate: interpolator requires at least 2 rows!");
        spline.clear();

        int nf=xy.n_cols-1;
        int nrows=xy.n_rows;

        acc.reset( gsl_interp_accel_alloc () );
        for (int i=0; i<nf; i++)
        {
//             cout<<"building interpolator for col "<<i<<endl;
            if ( (xy.n_rows==2) || force_linear )
                spline.push_back( gsl_spline_alloc (gsl_interp_linear, nrows) );
            else
                spline.push_back( gsl_spline_alloc (gsl_interp_cspline, nrows) );
            //cout<<"x="<<xy.col(0)<<endl<<"y="<<xy.col(i+1)<<endl;
            gsl_spline_init (&spline[i], xy.colptr(0), xy.colptr(i+1), nrows);
        }

        first_=xy.row(0);
        last_=xy.row(xy.n_rows-1);
    }
    catch (...)
    {
        std::ostringstream os;
        os<<xy_us;
        throw insight::Exception("Interpolator::Interpolator(): Failed to initialize interpolator.\nSupplied data: "+os.str());
    }
}

Interpolator::Interpolator(const arma::mat& xy_us, bool force_linear)
{
    initialize(xy_us, force_linear);
}


Interpolator::Interpolator(const arma::mat& x, const arma::mat& y, bool force_linear)
{
    arma::mat xy = arma::zeros(x.n_rows, 2);
    if (x.n_rows!=y.n_rows)
        throw insight::Exception(boost::str(boost::format("number of data points in x (%d) and y (%d) array differs!")%x.n_rows%y.n_rows));
    xy.col(0)=x;
    xy.col(1)=y;
    initialize(xy, force_linear);
}

Interpolator::~Interpolator()
{
//   for (int i=0; i<spline.size(); i++)
//     gsl_spline_free (spline[i]);
//   gsl_interp_accel_free (acc);
}

double Interpolator::integrate(double a, double b, int col) const
{
  if (col>=spline.size())
    throw insight::Exception(str(format("requested value interpolation in data column %d while there are only %d columns!")
			    % col % spline.size()));
    
//   double small=1e-20;
//   if (first(0)-a > small);
//     throw insight::Exception(str(format("Begin of integration interval (%g) before beginning of definition interval (%g)!")
// 			    % a % first(0)));
//   if (a-last(0) > small);
//     throw insight::Exception(str(format("Begin of integration interval (%g) after end of definition interval (%g)!")
// 			    % a % last(0)));
//   if (first(0)-b>small);
//     throw insight::Exception(str(format("End of integration interval (%g) before beginning of definition interval (%g)!")
// 			    % b % first(0)));
//   if (b-last(0)>small);
//     throw insight::Exception(str(format("End of integration interval (%g) after end of definition interval (%g)!")
// 			    % b % last(0)));
  
  return gsl_spline_eval_integ( &(spline[col]), a, b, &(*acc) );
}



double Interpolator::solve(
        double yTarget, int col,
        boost::optional<double> av,
        boost::optional<double> bv ) const
{
    double a = firstX();
    double b = lastX();
    if (av) a=*av;
    if (bv) b=*bv;

    return nonlinearSolve1D(
                [&](double x) -> double { return y(x, col) - yTarget; },
                a, b
            );
}



double Interpolator::y(double x, int col, OutOfBounds* outOfBounds) const
{
  if (col>=spline.size())
    throw insight::Exception(str(format("requested value interpolation in data column %d while there are only %d columns!")
			    % col % spline.size()));
    
  if (x<first_(0)) { if (outOfBounds) *outOfBounds=IP_OUTBOUND_SMALL; return first_(col+1); }
  if (x>last_(0)) { if (outOfBounds) *outOfBounds=IP_OUTBOUND_LARGE; return last_(col+1); }
  if (outOfBounds) *outOfBounds=IP_INBOUND;

  double v=gsl_spline_eval (&(spline[col]), x, &(*acc));
  return v;
}

double Interpolator::dydx(double x, int col, OutOfBounds* outOfBounds) const
{
  if (col>=spline.size())
    throw insight::Exception(str(format("requested derivative interpolation in data column %d while there are only %d columns!")
			    % col % spline.size()));
    
  if (x<first_(0)) { if (outOfBounds) *outOfBounds=IP_OUTBOUND_SMALL; return dydx(first_(0), col); }
  if (x>last_(0)) { if (outOfBounds) *outOfBounds=IP_OUTBOUND_LARGE; return dydx(last_(0), col); }
  if (outOfBounds) *outOfBounds=IP_INBOUND;

  double v=gsl_spline_eval_deriv (&(spline[col]), x, &(*acc));
  return v;
}

arma::mat Interpolator::operator()(double x, OutOfBounds* outOfBounds) const
{
  arma::mat result=zeros(1, spline.size());
  for (int i=0; i<spline.size(); i++)
    result(0,i)=y(x, i, outOfBounds);
  return result;
}

arma::mat Interpolator::dydxs(double x, OutOfBounds* outOfBounds) const
{
  arma::mat result=zeros(1, spline.size());
  for (int i=0; i<spline.size(); i++)
    result(0,i)=dydx(x, i, outOfBounds);
  return result;
}

arma::mat Interpolator::operator()(const arma::mat& x, OutOfBounds* outOfBounds) const
{
  arma::mat result=zeros(x.n_rows, spline.size());
  for (int j=0; j<x.n_rows; j++)
  {
    result.row(j) = this->operator()(x(j), outOfBounds);
  }
  return result;
}

arma::mat Interpolator::dydxs(const arma::mat& x, OutOfBounds* outOfBounds) const
{
  arma::mat result=zeros(x.n_rows, spline.size());
  for (int j=0; j<x.n_rows; j++)
  {
    result.row(j) = this->dydxs(x(j), outOfBounds);
  }
  return result;
}

arma::mat Interpolator::xy(const arma::mat& x, OutOfBounds* outOfBounds) const
{
  return arma::mat(join_rows(x, operator()(x, outOfBounds)));
}

arma::mat integrate(const arma::mat& xy)
{
  arma::mat integ(zeros(xy.n_cols-1));
  
  arma::mat x = xy.col(0);
  arma::mat y = xy.cols(1, xy.n_cols-1);

  for (int i=0; i<xy.n_rows-1; i++)
  {
    integ += 0.5*( y(i) + y(i+1) ) * ( x(i+1) - x(i) );
  }
  
  return integ;
}

struct p_int
{
  const Interpolator* ipol_;
  int col_;
};

double f_int (double x, void * params) {
  p_int* p = static_cast<p_int *>(params);
  return p->ipol_->y(x, p->col_);
}

double integrate(const Interpolator& ipol, double a, double b, int col)
{
  gsl_integration_workspace * w 
    = gsl_integration_workspace_alloc (1000);

  double result, error;
  
  p_int p;
  p.ipol_=&ipol;
  p.col_=col;
  gsl_function F;
  F.function = &f_int;
  F.params = &p;

  gsl_integration_qags 
  (
    &F, 
    a, b, 
    0, 1e-5, 1000,
    w, &result, &error
  ); 
  cout<<"integration residual = "<<error<<" (result="<<result<<")"<<endl;

  gsl_integration_workspace_free (w);

  return result;
}

arma::mat integrate(const Interpolator& ipol, double a, double b)
{
  arma::mat res=zeros(ipol.ncol());
  for (int i=0; i<res.n_elem; i++)
  {
    res(i)=integrate(ipol, a, b, i);
  }
  return res;
}


bool operator!=(const arma::mat &m1, const arma::mat &m2)
{
  if ( m1.n_rows!=m2.n_rows || m1.n_cols!=m2.n_cols )
    return true;

  bool isdiff=false;
  for (arma::uword i=0; i<m2.n_rows; ++i)
    for (arma::uword j=0; j<m2.n_cols; ++j)
    {
      if ( m1(i,j)!=m2(i,j) ) return true;
    }

  return false;
}

arma::mat vec3Zero()
{
    return vec3(0,0,0);
}

arma::mat vec3One()
{
    return vec3(1,1,1);
}


arma::mat vec3X(double x)
{
    return vec3(x, 0, 0);
}

arma::mat vec3Y(double y)
{
    return vec3(0, y, 0);
}

arma::mat vec3Z(double z)
{
    return vec3(0, 0, z);
}


CoordinateSystem::CoordinateSystem()
    : origin(vec3Zero()),
      ex(vec3X(1)), ey(vec3Y(1)), ez(vec3Z(1))
{}

CoordinateSystem::CoordinateSystem(const arma::mat &p0, const arma::mat &x)
    : origin(p0),
      ex(x/arma::norm(x,2))
{
    arma::mat tz=vec3(0,0,1);
    if ( fabs(arma::dot(tz,ex) - 1.) < SMALL )
    {
        tz=vec3(0,1,0);
    }

    ey=-arma::cross(ex,tz);
    ey/=arma::norm(ey,2);

    ez=arma::cross(ex,ey);
    ez/=arma::norm(ez,2);
}




CoordinateSystem::CoordinateSystem(const arma::mat &p0, const arma::mat &x, const arma::mat &z)
    : origin(p0),
      ex(x/arma::norm(x,2))
{
    if ( fabs(arma::dot(z,ex) - 1.) < SMALL )
    {
        throw insight::Exception("X and Z axis are colinear!");
    }

    ey=-arma::cross(ex,z);
    ey/=arma::norm(ey,2);

    ez=arma::cross(ex,ey);
    ez/=arma::norm(ez,2);
}

double stabilize(double value, double nonZeroThreshold)
{
    // 1 or -1, not 0
    double sign = 1.0;
    if (value<0.0) sign=-1.0;

    double m = std::fabs(value);
    if (m<nonZeroThreshold)
    {
        m=nonZeroThreshold;
    }
    return sign*m;
}



}

namespace std
{

std::size_t hash<arma::mat>::operator()
    (const arma::mat& v) const
{
    std::hash<double> dh;
    size_t h=0;
    for (arma::uword i=0; i<v.n_elem; i++)
    {
        std::hash_combine(h, dh(v(i)));
    }
    return h;
}

}
