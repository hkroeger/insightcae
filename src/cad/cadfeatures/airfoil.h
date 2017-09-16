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

#ifndef INSIGHT_CAD_AIRFOIL_H
#define INSIGHT_CAD_AIRFOIL_H

#include "cadfeature.h"

// GSL
#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>
#include <gsl/gsl_roots.h>


namespace insight {
namespace cad {

    
class InterpolatedCurve
{

  int n;
  double *x_, *y_;
  gsl_interp_accel* accel_;
  gsl_interp* interp_;

  void init();

public:
  InterpolatedCurve(const InterpolatedCurve& i);
  InterpolatedCurve
  (
   const std::vector<double>& datx, 
   const std::vector<double>& daty
  );
  ~InterpolatedCurve();

  double operator()(double x, int der=0) const;

};

typedef boost::shared_ptr<InterpolatedCurve> InterpolatedCurvePtr;

typedef boost::tuple<std::vector<double>, InterpolatedCurvePtr,InterpolatedCurvePtr> FoilShape;

class Airfoil
    : public SingleFaceFeature
{
    std::string name_;
    VectorPtr p0_;
    VectorPtr ez_;
    VectorPtr ex_;
    
    ScalarPtr c_; // camber
    ScalarPtr t_; // thickness

    ScalarPtr r_EK_; // leading edge radius
    ScalarPtr r_AK_; // trailing edge radius

    Airfoil 
    ( 
        const std::string& name, VectorPtr p0, VectorPtr ex, VectorPtr ez, ScalarPtr c, ScalarPtr t, ScalarPtr r_EK, ScalarPtr r_AK
    );
    
    FoilShape lookupFoil(const std::string& name) const;
    
    void generateDiscreteThickness
    (
        const InterpolatedCurve& tsp,
        int n,
        const double* x,
        double* thickness,
        double tr, double len,
        double rEk,
        double rAk
    ) const;

    void generateDiscreteSection
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
    ) const;
    
public:
    declareType ( "Airfoil" );
    Airfoil ();

    static FeaturePtr create 
    ( 
        const std::string& name, VectorPtr p0, VectorPtr ex, VectorPtr ez, ScalarPtr c, ScalarPtr t, ScalarPtr r_EK, ScalarPtr r_AK
    );

    virtual void build();
    operator const TopoDS_Face& () const;

    virtual void insertrule ( parser::ISCADParser& ruleset ) const;
    virtual FeatureCmdInfoList ruleDocumentation() const;

};




}
}

#endif // INSIGHT_CAD_AIRFOIL_H

