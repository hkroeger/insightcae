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

#ifndef INSIGHT_LINEARALGEBRA_H
#define INSIGHT_LINEARALGEBRA_H

#include <armadillo>
#include <map>
#include <set>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>
#include "boost/blank.hpp"
#include "gsl/gsl_multimin.h"
#include "gsl/gsl_integration.h"

#include "boost/optional.hpp"
#include "boost/ptr_container/ptr_vector.hpp"
#include "boost/variant.hpp"

#include "base/exception.h"

// #define SIGN(x) ((x)<0.0?-1.0:1.0)

namespace std
{

extern double armaMatIdentityTolerance;
/**
 * @brief operator <
 * required for using amra::mat as key in STL containers
 * @param v1
 * @param v2
 * @return
 */
bool operator<(const arma::mat& v1, const arma::mat& v2);

}

class vtkMatrix4x4;

namespace insight 
{

extern const double VSMALL;
extern const double SMALL;
extern const double LSMALL;

inline double pos(double x)
{
    if (x>=0)
        return 1.;
    else
        return 0.;
}

inline double neg(double x)
{
    if (x<0)
        return 1.;
    else
        return 0.;
}

inline double sgn(double x)
{
    if (x<0)
        return -1.;
    else
        return 1.;
}

class GSLExceptionHandling
{
  gsl_error_handler_t *oldHandler_;
public:
  GSLExceptionHandling();
  ~GSLExceptionHandling();
};

class GSLException : public Exception
{
  std::string gsl_reason_;
  int gsl_errno_;

public:
    GSLException(
            const char * reason, const char * file,
            int line, int gsl_errno );

    const std::string& gsl_reason() const { return gsl_reason_; }
    int gsl_errno() const;
};

// ====================================================================================
// ======== generation of vectors/matrices/tensors

arma::mat vec1(double x);
arma::mat vec2(double x, double y);
arma::mat vec3(double x, double y, double z);
arma::mat vec3Zero();
arma::mat vec3One();
arma::mat vec3X(double x = 1);
arma::mat vec3Y(double y = 1);
arma::mat vec3Z(double z = 1);
arma::mat vec3FromComponents(const double* c);
arma::mat vec3FromComponents(const float* c);
arma::mat readVec3(std::istream& is);
arma::mat normalized(const arma::mat& vec);

arma::mat tensor3(
  double xx, double xy, double xz,
  double yx, double yy, double yz,
  double zx, double zy, double zz
);

template<class T>
arma::mat Tensor(const T& t)
{
  arma::mat rt;
  rt << t.XX() << t.XY() << t.XZ() << arma::endr
     << t.YX() << t.YY() << t.YZ() << arma::endr
     << t.ZX() << t.ZY() << t.ZZ() << arma::endr;
  return rt;
}

template<class T>
arma::mat tensor(const T& t)
{
  arma::mat rt;
  rt << t.xx() << t.xy() << t.xz() << arma::endr
     << t.yx() << t.yy() << t.yz() << arma::endr
     << t.zx() << t.zx() << t.zz() << arma::endr;
  return rt;
}

template<class T>
arma::mat vector(const T& t)
{
  arma::mat rt;
  rt
          << t.x() << arma::endr
          << t.y() << arma::endr
          << t.z() << arma::endr;
  return rt;
}


template<class T>
arma::mat Vector(const T& t)
{
  arma::mat rt;
  rt
          << t.X() << arma::endr
          << t.Y() << arma::endr
          << t.Z() << arma::endr;
  return rt;
}

template<class T>
arma::mat vec3(const T& t)
{
  arma::mat rt;
  rt
          << t.X() << arma::endr
          << t.Y() << arma::endr
          << t.Z() << arma::endr;
  return rt;
}

bool operator!=(const arma::mat& m1, const arma::mat& m2);


// ====================================================================================
// ======== conversion of vectors into different formats

template<class T>
T toVec(const arma::mat& v)
{
  return T(v(0), v(1), v(2));
}

template<class T>
T toMat(const arma::mat& m)
{
    return T(
        m(0,0), m(0,1), m(0,2),
        m(1,0), m(1,1), m(1,2),
        m(2,0), m(2,1), m(2,2)
    );
}

double* toArray(const arma::mat& v); // !non-const! return value to match VTK functions

std::string toStr(const arma::mat& v3);

/**
 * @brief rotMatrix
 * @param theta
 * angle in radians
 * @param u
 * rotation axis (defaults to Z axis)
 * @return
 */
arma::mat rotMatrix( double theta, arma::mat u=vec3(0,0,1) );
arma::mat rotated( const arma::mat&p, double theta, const arma::mat& axis=vec3(0,0,1), const arma::mat& p0 = vec3(0,0,0) );

/**
 * @brief rotationMatrixToRollPitchYaw
 * @param R
 * @return
 * euler angles in degrees
 */
arma::mat rotationMatrixToRollPitchYaw(const arma::mat& R);

/**
 * @brief rollPitchYawToRotationMatrix
 * @param rollPitchYaw
 * angles in degrees!
 * @return
 */
arma::mat rollPitchYawToRotationMatrix(const arma::mat& rollPitchYaw);

/**
 * Fits c_j in
 *  c_j*x_ij approx y_i
 * add a column with all values "1" to x_ij to include constant offset!
 * 
 * y: column vector with values to fit
 * x: matrix with x-values
 */
arma::mat linearRegression(const arma::mat& y, const arma::mat& x);

arma::mat polynomialRegression(const arma::mat& y, const arma::mat& x, int maxorder, int minorder=0);

/**
 * evaluate polynomial
 * coeffs: coefficients, highest order coefficient first
 */
double evalPolynomial(double x, const arma::mat& coeffs);

class RegressionModel
{
public:
  virtual ~RegressionModel();
  
  virtual int numP() const =0;
  virtual void setParameters(const double* params) =0;
  virtual void getParameters(double* params) const;
  virtual arma::mat evaluateObjective(const arma::mat& x) const =0;
  virtual void setInitialValues(double* x) const =0;
  virtual void setStepHints(double* x) const;
  virtual arma::mat weights(const arma::mat& x) const;
  double computeQuality(const arma::mat& y, const arma::mat& x) const;
};

/**
 * fits parameters of a nonlinear model F
 * return fit quality
 */
double nonlinearRegression(const arma::mat& y, const arma::mat& x, RegressionModel& model, double tol=1e-3);

class Objective1D
{
public:
  size_t maxiter=100;

  virtual ~Objective1D();
  
  virtual double operator()(double x) const =0;
};

class ObjectiveND
{
public:
  size_t maxiter=10000;

  virtual ~ObjectiveND();
  
  virtual double operator()(const arma::mat& x) const =0;
  virtual int numP() const =0;
};

arma::mat vec(
    const gsl_vector * x,
    boost::variant<boost::blank,double,arma::mat> replaceNaN
        = boost::blank() );

arma::mat mat( const gsl_matrix* m );

void gsl_vector_set(const arma::mat& x, gsl_vector * xo);

double nonlinearSolve1D(const Objective1D& model, double x_min, double x_max);
double nonlinearSolve1D(const std::function<double(double)>& model, double x_min, double x_max);
double nonlinearMinimize1D(const Objective1D& model, double x_min, double x_max, double tol=1e-3);
double nonlinearMinimize1D(const std::function<double(double)>& model, double x_min, double x_max, double tol=1e-3);

arma::mat nonlinearMinimizeND(
    const ObjectiveND& model, const arma::mat& x0,
    double tol=1e-3, const arma::mat& steps = arma::mat(), double relax=1.0 );

arma::mat nonlinearMinimizeND(
    const std::function<double(const arma::mat&)>& model, const arma::mat& x0,
    double tol=1e-3, const arma::mat& steps = arma::mat(), int nMaxIter=10000, double relax=1.0 );


class JacobiDeterminatException
    : public Exception
{
    arma::mat J_;
    std::set<size_t> zeroCols_;
public:
    JacobiDeterminatException(const arma::mat& J);
    const std::set<size_t>& zeroCols() const;
};


class NonConvergenceException
: public Exception
{
public:
    NonConvergenceException(int performedIterations);
};

arma::mat nonlinearSolveND(
    std::function<arma::mat(const arma::mat& x)> obj,
    const arma::mat& x0,
    double tol=1e-3, int nMaxIter=10000, double relax=1.0,
    std::function<void(const arma::mat&)> perIterationCallback
        = std::function<void(const arma::mat&)>()
    );

arma::mat movingAverage(const arma::mat& timeProfs, double fraction=0.5, bool first_col_is_time=true, bool centerwindow=false);

arma::mat sortedByCol(const arma::mat&m, int c);

/**
 * interpolates in a 2D-matrix using GSL spline routines.
 * The first column is assumed to contain the x-values.
 * All remaining columns are dependents and to be interpolated.
 */
class Interpolator
{
public:
  typedef enum {
    IP_INBOUND,
    IP_OUTBOUND_LARGE,
    IP_OUTBOUND_SMALL
  } OutOfBounds;

private:
  arma::mat xy_, first_, last_;
  std::shared_ptr<gsl_interp_accel> acc;
  boost::ptr_vector<gsl_spline> spline ;
  
//   Interpolator(const Interpolator&);
  void initialize(const arma::mat& xy, bool force_linear=false);
  
public:
  Interpolator(const arma::mat& xy, bool force_linear=false);
  Interpolator(const arma::mat& x, const arma::mat& y, bool force_linear=false);
  ~Interpolator();
  
  /**
   * integrate column col fromx=a to x=b
   */
  double integrate(double a, double b, int col=0) const;

  /**
   * solve for x at given y-value
   */
  double solve(
          double y, int col=0,
          boost::optional<double> a=boost::optional<double>(),
          boost::optional<double> b=boost::optional<double>() ) const;

  /**
   * returns a single y-value from column col
   */
  double y(double x, int col=0, OutOfBounds* outOfBounds=NULL) const;

  double maxY(int col=0) const;

  /**
   * returns a single dy/dx-value from column col
   */
  double dydx(double x, int col=0, OutOfBounds* outOfBounds=NULL) const;
  /**
   * interpolates all y values (row vector) at x
   */
  arma::mat operator()(double x, OutOfBounds* outOfBounds=NULL) const;
  /**
   * interpolates all y values (row vector) at x
   */
  arma::mat dydxs(double x, OutOfBounds* outOfBounds=NULL) const;
  /**
   * interpolates all y values (row vector) 
   * at multiple locations given in x
   * returns only the y values, no x-values in the first column
   */
  arma::mat operator()(const arma::mat& x, OutOfBounds* outOfBounds=NULL) const;
  /**
   * computes all derivative values (row vector) 
   * at multiple locations given in x
   */
  arma::mat dydxs(const arma::mat& x, OutOfBounds* outOfBounds=NULL) const;

  /**
   * interpolates all y values (row vector) 
   * at multiple locations given in x
   * and return matrix with x as first column
   */
  arma::mat xy(const arma::mat& x, OutOfBounds* outOfBounds=NULL) const;
  
  inline const arma::mat& rawdata() const { return xy_; }
  inline const arma::mat& first() const { return first_; }
  inline const arma::mat& last() const { return last_; }
  inline double firstX() const { return first_(0); }
  inline double lastX() const { return last_(0); }

  inline int ncol() const { return spline.size(); }
};

arma::mat integrate(const arma::mat& xy);

double integrate(const Interpolator& ipol, double a, double b, int comp);
arma::mat integrate(const Interpolator& ipol, double a, double b);



template<class F>
double functor_int (double x, void * params) {
  F* p = static_cast<F *>(params);
  return (*p)(x);
}

/**
 * computes the definite integral over f from a to b numerically
 */
template<class F>
double integrate(F f, double a, double b)
{
  gsl_integration_workspace * w
    = gsl_integration_workspace_alloc (1000);

  double result, error;

  F* p=&f;
  gsl_function FUNC;
  FUNC.function = &functor_int<F>;
  FUNC.params = p;

  gsl_integration_qags
  (
    &FUNC,
    a, b,
    0, 1e-5, 1000,
    w, &result, &error
  );
//  std::cout<<"integration residual = "<<error<<" (result="<<result<<")"<<std::endl;

  gsl_integration_workspace_free (w);

  return result;
}


/**
 * computes the definite integral over f from a to b numerically using trapez rule
 */
template<class T, class F>
T integrate_trpz(F f, double a, double b, int n=20)
{
  T res=0.;

  double dx=(b-a)/double(n);
  for (int i=0; i<n; i++)
  {
      double x = a + dx*(double(i)+0.5);
      res+=f(x)*dx;
  }
  return res;
}


/**
 * computes the semi-indefinite integral over f from a to infinity numerically
 */
template<class F>
double integrate_indef(F f, double a=0)
{
  gsl_integration_workspace * w
    = gsl_integration_workspace_alloc (1000);

  double result, error;

  F* p=&f;
  gsl_function FUNC;
  FUNC.function = &functor_int<F>;
  FUNC.params = p;

  gsl_integration_qagiu
  (
    &FUNC,
    a,
    0, 1e-5, 1000,
    w, &result, &error
  );
//  std::cout<<"integration residual = "<<error<<" (result="<<result<<")"<<std::endl;

  gsl_integration_workspace_free (w);

  return result;
}




double stabilize(double value, double nonZeroThreshold);


}


namespace std
{

template<> struct hash<arma::mat>
{
  std::size_t operator()(const arma::mat& v) const;
};

}

#endif // INSIGHT_LINEARALGEBRA_H
