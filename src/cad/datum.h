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

#ifndef INSIGHT_CAD_DATUM_H
#define INSIGHT_CAD_DATUM_H

#include <map>

#include "base/linearalgebra.h"

#include "occinclude.h"
#include "cadtypes.h"
#include "astbase.h"
#include "cadparameter.h"

namespace insight {
namespace cad {

#ifndef SWIG
class Datum
: public ASTBase,
  public DependencySource
{
public:
  typedef std::map<std::string, DatumPtr> Map;
  
protected:
  bool providesPointReference_, providesAxisReference_, providesPlanarReference_;
  
  virtual size_t calcHash() const =0;

  Datum(const Datum& o);

public:
  declareType("Datum");
  Datum(bool point, bool axis, bool planar);
  Datum(std::istream&);
  
  virtual ~Datum();
  
  virtual inline bool providesPointReference() const { return providesPointReference_; }
  virtual gp_Pnt point() const;
  operator const gp_Pnt () const;
  
  virtual inline bool providesAxisReference() const { return providesAxisReference_; }
  virtual gp_Ax1 axis() const;
  operator const gp_Ax1 () const;

  virtual inline bool providesPlanarReference() const { return providesPlanarReference_; }
  virtual gp_Ax3 plane() const;
  operator const gp_Ax3 () const;

//  virtual Handle_AIS_InteractiveObject createAISRepr(AIS_InteractiveContext& context, const std::string& label, const gp_Trsf& tr = gp_Trsf()) const;

  virtual void write(std::ostream& file) const;
  
  /**
   * @brief generateScriptCommand
   * This API needs is conceptually incomplete.
   * currently only used to save constrained sketch scripts without external dependencies
   * everything else is unsupported.
   * @return
   */
  virtual std::string generateScriptCommand() const;

  virtual void checkForBuildDuringAccess() const;
};




class TransformedDatum
: public Datum
{
protected:
    DatumPtr base_;
    gp_Trsf tr_;
    VectorPtr translation_;

    size_t calcHash() const override;
    void build() override;
    
    TransformedDatum(const TransformedDatum&o, TreeCloneMap& tcm);
public:
    declareType("TransformedDatum");
    CLONEABLE(TransformedDatum);
    DEPENDS((base_, translation_));

    TransformedDatum(DatumPtr datum, gp_Trsf tr);
    TransformedDatum(DatumPtr datum, VectorPtr translation);
    
    
    gp_Pnt point() const override;
    gp_Ax1 axis() const override;
    gp_Ax3 plane() const override;
    gp_Trsf trsf() const;
    DatumPtr baseDatum() const;
    
//    virtual Handle_AIS_InteractiveObject createAISRepr(AIS_InteractiveContext& context, const std::string& label, const gp_Trsf& tr = gp_Trsf()) const;
};




class DatumPoint
: public Datum
{
protected:
  gp_Pnt p_;
  
  DatumPoint(const DatumPoint& o);
public:
  declareType("DatumPoint");
  DatumPoint();
  
  gp_Pnt point() const override;

//  virtual Handle_AIS_InteractiveObject createAISRepr(AIS_InteractiveContext& context, const std::string& label, const gp_Trsf& tr = gp_Trsf()) const;
};




class DatumAxis
: public Datum
{
protected:
  gp_Ax1 ax_;
  
  DatumAxis(const DatumAxis& o);
public:
  declareType("DatumAxis");
  DatumAxis();
  
  gp_Pnt point() const override;
  gp_Ax1 axis() const override;

//  virtual Handle_AIS_InteractiveObject createAISRepr(AIS_InteractiveContext& context, const std::string& label, const gp_Trsf& tr = gp_Trsf()) const;
};



class DatumPlaneData
: public Datum
{
protected:
  gp_Ax3 cs_;

  DatumPlaneData(const DatumPlaneData& o);
public:
  declareType("DatumPlaneData");
  DatumPlaneData();
  
  gp_Pnt point() const override;
  gp_Ax3 plane() const override;

  arma::mat origin() const;
  arma::mat normal() const;
  arma::mat ex() const;
  arma::mat ey() const;

  CoordinateSystem coordinateSystem() const;
};

#endif


class ProvidedDatum
: public Datum
{
protected:
  FeaturePtr feat_;
  std::string name_;
  DatumPtr dat_;

  size_t calcHash() const override;
  void build() override;
  
  ProvidedDatum(const ProvidedDatum&o, TreeCloneMap& tcm);
public:
  declareType("ProvidedDatum");
  CLONEABLE(ProvidedDatum);
#ifndef SWIG
  DEPENDS((feat_, dat_));
#endif

  ProvidedDatum(FeaturePtr feat, std::string name);
  
  inline bool providesPointReference() const  override{ checkForBuildDuringAccess(); return providesPointReference_; }
  inline bool providesAxisReference() const  override{ checkForBuildDuringAccess(); return providesAxisReference_; }
  inline bool providesPlanarReference() const  override{ checkForBuildDuringAccess(); return providesPlanarReference_; }

  gp_Pnt point() const override;
  gp_Ax1 axis() const override;
  gp_Ax3 plane() const override;
  DatumPtr baseDatum() const;
};




class ExplicitDatumPoint
: public DatumPoint
{
  VectorPtr coord_;

  size_t calcHash() const override;
  void build() override;

  ExplicitDatumPoint(const ExplicitDatumPoint&o, TreeCloneMap& tcm);
public:
  declareType("ExplicitDatumPoint");
  CLONEABLE(ExplicitDatumPoint);
#ifndef SWIG
  DEPENDS((coord_));
#endif

  ExplicitDatumPoint(VectorPtr c);
  
};


class ExplicitDatumAxis
: public DatumAxis
{
  VectorPtr p0_, ex_;

  size_t calcHash() const override;
  void build() override;

  ExplicitDatumAxis(const ExplicitDatumAxis&o, TreeCloneMap& tcm);
public:
  declareType("ExplicitDatumAxis");
  CLONEABLE(ExplicitDatumAxis);
#ifndef SWIG
  DEPENDS((p0_, ex_));
#endif

  ExplicitDatumAxis(VectorPtr p0, VectorPtr ex);
  
};


class DatumPlane
: public DatumPlaneData
{
  VectorPtr p0_, n_, up_;
  VectorPtr p1_, p2_;
  
  size_t calcHash() const override;
  void build() override;

  DatumPlane(const DatumPlane& o, TreeCloneMap& tcm);

public:
  declareType("DatumPlane");
  CLONEABLE(DatumPlane);
#ifndef SWIG
  DEPENDS((p0_, n_, up_, p1_, p2_));
#endif


  DatumPlane
  (
    VectorPtr p0, 
    VectorPtr n
  );
  
  DatumPlane
  (
    VectorPtr p0, 
    VectorPtr n,
    VectorPtr up
  );
  
  DatumPlane
  (
    VectorPtr p0, 
    VectorPtr p1,
    VectorPtr p2,
    bool dummy
  );
  
//   DatumPlane
//   (
//     FeaturePtr m, 
//     FeatureID f
//   );
  
  // DatumPlane(std::istream&);
  
  

  void write(std::ostream& file) const override;

};




/**
 * Plane/Plane intersection
 */

class XsecPlanePlane
: public DatumAxis
{
  DatumPtr pl1_, pl2_;
  
  size_t calcHash() const override;
  void build() override;

  XsecPlanePlane(const XsecPlanePlane&o, TreeCloneMap& tcm);
public:
  declareType("XsecPlanePlane");
  CLONEABLE(XsecPlanePlane);
#ifndef SWIG
  DEPENDS((pl1_, pl2_));
#endif

  XsecPlanePlane(DatumPtr pl1, DatumPtr pl2);
};




class XsecAxisPlane
: public DatumPoint
{
  DatumPtr ax_, pl_;
  
  size_t calcHash() const override;
  void build() override;

  XsecAxisPlane(const XsecAxisPlane&o, TreeCloneMap& tcm);
public:
  declareType("XsecAxisPlane");
  CLONEABLE(XsecAxisPlane);
#ifndef SWIG
  DEPENDS((ax_, pl_));
#endif

  XsecAxisPlane(DatumPtr ax, DatumPtr pl);

};



}
}

#endif // INSIGHT_CAD_DATUM_H
