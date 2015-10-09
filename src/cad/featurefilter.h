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

#ifndef INSIGHT_CAD_FEATUREFILTER_H
#define INSIGHT_CAD_FEATUREFILTER_H

#include <set>
#include <map>
#include <vector>
// #include <iostream>

#include "base/boost_include.h"

#include "base/linearalgebra.h"
#include "base/exception.h"
#include "feature.h"
#include "occinclude.h"

namespace insight
{
namespace cad
{


class SolidModel;
class Sketch;
class AND;
class NOT;
class OR;


inline FilterPtr new_clone(const Filter& f)
{
    return f.clone();
}

class AND
    : public Filter
{
protected:
    FilterPtr f1_;
    FilterPtr f2_;
public:
    AND(const Filter& f1, const Filter& f2);
    virtual void initialize(const SolidModel& m);
    virtual void firstPass(FeatureID feature);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};

class OR
    : public Filter
{
protected:
    FilterPtr f1_;
    FilterPtr f2_;
public:
    OR(const Filter& f1, const Filter& f2);
    virtual void initialize(const SolidModel& m);
    virtual void firstPass(FeatureID feature);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};

class NOT
    : public Filter
{
protected:
    FilterPtr f1_;
public:
    NOT(const Filter& f1);
    virtual void initialize(const SolidModel& m);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};

// ANDFilter operator&&(const Filter& f1, const Filter& f2);
// NOTFilter operator!(const Filter& f1);

class boundaryEdge
    : public Filter
{
protected:
  mutable boost::shared_ptr<ShapeAnalysis_FreeBounds> safb_;

public:
    boundaryEdge();
    virtual void initialize(const SolidModel& m);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};

class edgeTopology
    : public Filter
{
protected:
    GeomAbs_CurveType ct_;

public:
    edgeTopology(GeomAbs_CurveType ct);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};

class faceTopology
    : public Filter
{
protected:
    GeomAbs_SurfaceType ct_;

public:
    faceTopology(GeomAbs_SurfaceType ct);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};

class in
    : public Filter
{
protected:
    FeatureSet set_;

public:
    in(FeatureSet set);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};

class faceAdjacentToEdges
    : public Filter
{
protected:
    FeatureSet edges_;

public:
    faceAdjacentToEdges(FeatureSet edges);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};

class faceAdjacentToFaces
    : public Filter
{
protected:
    FeatureSet faces_;

public:
    faceAdjacentToFaces(FeatureSet faces);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};

class cylFaceOrientation
    : public Filter
{
protected:
    bool io_;

public:
    /**
     * @param io inside:true, outside: false
     */
    cylFaceOrientation(bool io);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};


class everything
    : public Filter
{

public:
    everything();
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};

template<EntityType T>
class coincident
    : public Filter
{
protected:
    FeatureSet f_;

public:
    coincident(const SolidModel& m)
        : f_(m, T)
    {
        throw insight::Exception("coincident filter: not implemented!");
    }

    coincident(FeatureSet f)
        : f_(f)
    {}

    bool checkMatch(FeatureID feature) const
    {
        throw insight::Exception("coincident filter: not implemented!");
    }

    FilterPtr clone() const
    {
        return FilterPtr(new coincident(f_));
    }

};


template<> coincident<Edge>::coincident(const SolidModel& m);
template<> bool coincident<Edge>::checkMatch(FeatureID feature) const;
template<> coincident<Face>::coincident(const SolidModel& m);
template<> bool coincident<Face>::checkMatch(FeatureID feature) const;
template<> coincident<Solid>::coincident(const SolidModel& m);
template<> bool coincident<Solid>::checkMatch(FeatureID feature) const;

typedef coincident<Edge> coincidentEdge;
typedef coincident<Face> coincidentFace;
typedef coincident<Solid> coincidentSolid;



template<EntityType T>
class isPartOfSolid
    : public Filter
{
protected:
    TopoDS_Solid s_;

public:
    isPartOfSolid(const TopoDS_Solid& s)
    : s_(s)
    {
    }

    isPartOfSolid(const SolidModel& m)
    : s_(m)
    {
        throw insight::Exception("isPartOfSolid filter: not implemented!");
    }

    isPartOfSolid(FeatureSet f)
        : s_(TopoDS::Solid(static_cast<TopoDS_Shape>(f.model())))
    {}

    bool checkMatch(FeatureID feature) const
    {
        throw insight::Exception("isPartOfSolid filter: not implemented!");
    }

    FilterPtr clone() const
    {
        return FilterPtr(new isPartOfSolid(s_));
    }

};


template<> isPartOfSolid<Edge>::isPartOfSolid(const SolidModel& m);
template<> bool isPartOfSolid<Edge>::checkMatch(FeatureID feature) const;
template<> isPartOfSolid<Face>::isPartOfSolid(const SolidModel& m);
template<> bool isPartOfSolid<Face>::checkMatch(FeatureID feature) const;

typedef isPartOfSolid<Edge> isPartOfSolidEdge;
typedef isPartOfSolid<Face> isPartOfSolidFace;


template<EntityType T>
class secant
    : public Filter
{
protected:
    arma::mat dir_;

public:
    secant(const arma::mat& dir)
        : dir_(dir)
    {
    }

    bool checkMatch(FeatureID feature) const
    {
        throw insight::Exception("secant filter: not implemented!");
    }

    FilterPtr clone() const
    {
        return FilterPtr(new secant(dir_));
    }

};

template<> bool secant<Edge>::checkMatch(FeatureID feature) const;


template<class T>
class QuantityComputer
{
public:
    typedef boost::shared_ptr<QuantityComputer<T> > Ptr;

protected:
    const SolidModel* model_;

public:
    QuantityComputer()
        : model_(NULL)
    {}

    virtual ~QuantityComputer()
    {}

    virtual void initialize(const SolidModel& m)
    {
        model_=&m;
    }

    virtual bool isValidForFeature(FeatureID) const
    {
      return true;
    }
    
    virtual T evaluate(FeatureID) =0;
    virtual QuantityComputer::Ptr clone() const =0;

    typename QuantityComputer<T>::Ptr operator+(const typename QuantityComputer<T>::Ptr& other) const;
    typename QuantityComputer<T>::Ptr operator+(const T& constant) const;

    FilterPtr operator==(const typename QuantityComputer<T>::Ptr& other) const;
    FilterPtr operator==(const T& constant) const
    {
        cout<<"comopare"<<endl;
        return FilterPtr();
    }
};

typedef QuantityComputer<double> scalarQuantityComputer;
typedef QuantityComputer<arma::mat> matQuantityComputer;
typedef boost::shared_ptr<scalarQuantityComputer> scalarQuantityComputerPtr;
typedef boost::shared_ptr<matQuantityComputer> matQuantityComputerPtr;

// #ifdef SWIG
// %template(doubleQuantityComputer) QuantityComputer<double>;
// %template(matQuantityComputer) QuantityComputer<arma::mat>;
// #endif


class coincidentProjectedEdge
    : public Filter
{
protected:
    FeatureSet f_;
    matQuantityComputerPtr p0_, n_, up_;
    scalarQuantityComputerPtr tol_;
    
    arma::mat samplePts_;
    
//     boost::shared_ptr<std::ofstream> dbgfile_;

public:
    coincidentProjectedEdge
    (
      const SolidModel& m,
      const matQuantityComputerPtr& p0, 
      const matQuantityComputerPtr& n, 
      const matQuantityComputerPtr& up,
      const scalarQuantityComputerPtr& tol
    );

    coincidentProjectedEdge
    (
      FeatureSet f,
      const matQuantityComputerPtr& p0, 
      const matQuantityComputerPtr& n, 
      const matQuantityComputerPtr& up,
      const scalarQuantityComputerPtr& tol
    );

    virtual void firstPass(FeatureID feature);
    bool checkMatch(FeatureID feature) const;

    FilterPtr clone() const;

};




template<class T>
class constantQuantity
    : public QuantityComputer<T>
{
protected:
    T refValue_;
public:
    constantQuantity(const T& refValue)
        : refValue_(refValue)
    {}
    virtual T evaluate(FeatureID) {
        return refValue_;
    };
    virtual typename QuantityComputer<T>::Ptr clone() const {
        return typename QuantityComputer<T>::Ptr(new constantQuantity<T>(refValue_));
    };

};


class distance
: public QuantityComputer<double>
{
protected:
    matQuantityComputerPtr p0_;
    matQuantityComputerPtr p1_;
public:
    distance(const matQuantityComputerPtr& p0, const matQuantityComputerPtr& p1)
        : p0_(p0), p1_(p1)
    {}
    
    virtual void initialize(const SolidModel& m)
    {
      p0_->initialize(m);
      p1_->initialize(m);
    }
    
    virtual double evaluate(FeatureID i) {
        return arma::norm( p0_->evaluate(i) - p1_->evaluate(i) , 2);
    };
    virtual typename QuantityComputer<double>::Ptr clone() const {
        return typename QuantityComputer<double>::Ptr(new distance(p0_, p1_));
    };

};
#ifdef SWIG
%template(doubleConstantQuantity) constantQuantity<double>;
%template(matConstantQuantity) constantQuantity<arma::mat>;
#endif


#define UNARY_FUNCTION_QTC(OPERATED_QTC_NAME, OPERATED_QTC_OP) \
template<class T> \
class OPERATED_QTC_NAME \
: public QuantityComputer<T> \
{ \
protected: \
  boost::shared_ptr<QuantityComputer<T> > qtc_; \
public:\
  OPERATED_QTC_NAME(const QuantityComputer<T>& qtc) \
  : qtc_(qtc.clone())\
  {}\
  \
  ~OPERATED_QTC_NAME() {}\
  \
  virtual void initialize(const SolidModel& m)\
  {\
    QuantityComputer<T>::initialize(m);\
    qtc_->initialize(m);\
  }\
  virtual bool isValidForFeature(FeatureID f) const \
  { return qtc_->isValidForFeature(f); } \
  \
  virtual T evaluate(FeatureID f)\
  {\
    T value = qtc_->evaluate(f);\
    value=OPERATED_QTC_OP;\
    return value;\
  }\
  \
  virtual typename QuantityComputer<T>::Ptr clone() const { return typename QuantityComputer<T>::Ptr(new OPERATED_QTC_NAME(*qtc_)); };\
};

#define UNARY_FUNCTION_QTC_RET(OPERATED_QTC_NAME, OPERATED_QTC_OP, RETURN_T) \
template<class T> \
class OPERATED_QTC_NAME \
: public QuantityComputer<RETURN_T> \
{ \
protected: \
  boost::shared_ptr<QuantityComputer<T> > qtc_; \
public:\
  OPERATED_QTC_NAME(const QuantityComputer<T>& qtc) \
  : qtc_(qtc.clone())\
  {}\
  \
  ~OPERATED_QTC_NAME() {}\
  \
  virtual void initialize(const SolidModel& m)\
  {\
    QuantityComputer<RETURN_T>::initialize(m);\
    qtc_->initialize(m);\
  }\
  virtual bool isValidForFeature(FeatureID f) const \
  { return qtc_->isValidForFeature(f); } \
  \
  virtual RETURN_T evaluate(FeatureID f)\
  {\
    T value = qtc_->evaluate(f);\
    RETURN_T rvalue=OPERATED_QTC_OP;\
    return rvalue;\
  }\
  \
  virtual QuantityComputer<RETURN_T>::Ptr clone() const { return QuantityComputer<RETURN_T>::Ptr(new OPERATED_QTC_NAME(*qtc_)); };\
};

#define BINARY_FUNCTION_QTC(OPERATED_QTC_NAME, OPERATED_QTC_OP, RESULT_T) \
template<class T1, class T2> \
class OPERATED_QTC_NAME \
: public QuantityComputer< typename RESULT_T<T1,T2>::type > \
{ \
protected: \
  boost::shared_ptr<QuantityComputer<T1> > qtc1_; \
  boost::shared_ptr<QuantityComputer<T2> > qtc2_; \
public:\
  OPERATED_QTC_NAME(const QuantityComputer<T1>& qtc1, const QuantityComputer<T2>& qtc2) \
  : qtc1_(qtc1.clone()), qtc2_(qtc2.clone())\
  {}\
  \
  ~OPERATED_QTC_NAME() {}\
  \
  virtual void initialize(const SolidModel& m)\
  {\
    QuantityComputer< typename RESULT_T<T1,T2>::type >::initialize(m);\
    qtc1_->initialize(m);\
    qtc2_->initialize(m);\
  }\
  virtual bool isValidForFeature(FeatureID f) const \
  { return qtc1_->isValidForFeature(f) && qtc2_->isValidForFeature(f); } \
  \
  virtual typename RESULT_T<T1,T2>::type evaluate(FeatureID f)\
  {\
    T1 value1 = qtc1_->evaluate(f);\
    T2 value2 = qtc2_->evaluate(f);\
    typename RESULT_T<T1,T2>::type value=OPERATED_QTC_OP;\
    return value;\
  }\
  \
  virtual typename QuantityComputer< typename RESULT_T<T1,T2>::type >::Ptr clone() const \
   { return typename QuantityComputer< typename RESULT_T<T1,T2>::type >::Ptr(new OPERATED_QTC_NAME(*qtc1_, *qtc2_)); };\
};

// #define BINARY_FUNCTION_QTC_OP(OPERATED_QTC_NAME, OPERATED_QTC_OP) \
// template<class T1, class T2> \
// OPERATED_QTC_NAME<T1,T2> OPERATED_QTC_OP(const QuantityComputer<T1>& qtc1, const QuantityComputer<T2>& qtc2) \
// { \
//   return OPERATED_QTC_NAME<T1,T2>::type >(qtc1, qtc2); \
// } \
// template<class T1, class T2> \
// OPERATED_QTC_NAME<typename boost::enable_if<boost::is_arithmetic<T1>, T1 >::type, T2> OPERATED_QTC_OP(const T1& qt1, const QuantityComputer<T2>& qtc2) \
// { \
//   return OPERATED_QTC_NAME<T1, T2>(constantQuantity<T1>(qt1), qtc2); \
// } \
// template<class T1, class T2> \
// OPERATED_QTC_NAME<T1, typename boost::enable_if<boost::is_arithmetic<T2>, T2 >::type> OPERATED_QTC_OP(const QuantityComputer<T1>& qtc1, const T2& qt2) \
// { \
//   return OPERATED_QTC_NAME<T1, T2>(qtc1, constantQuantity<T2>(qt2)); \
// }

template<class T1, class T2> struct MultiplyResult {};
template<> struct MultiplyResult<arma::mat, arma::mat> {
    typedef arma::mat type;
};
template<> struct MultiplyResult<double, arma::mat> {
    typedef arma::mat type;
};
template<> struct MultiplyResult<arma::mat, double> {
    typedef arma::mat type;
};
template<> struct MultiplyResult<double, double> {
    typedef double type;
};

template<class T1, class T2> struct DivisionResult {};
template<> struct DivisionResult<arma::mat, arma::mat> {
    typedef arma::mat type;
};
template<> struct DivisionResult<arma::mat, double> {
    typedef arma::mat type;
};
template<> struct DivisionResult<double, double> {
    typedef double type;
};

template<class T1, class T2> struct AdditionResult {};
template<> struct AdditionResult<arma::mat, arma::mat> {
    typedef arma::mat type;
};
template<> struct AdditionResult<double, double> {
    typedef double type;
};

template<class T1, class T2> struct SubtractionResult {};
template<> struct SubtractionResult<arma::mat, arma::mat> {
    typedef arma::mat type;
};
template<> struct SubtractionResult<double, double> {
    typedef double type;
};

template<class T1, class T2> struct DotResult {};
template<> struct DotResult<arma::mat, arma::mat> {
    typedef double type;
};

UNARY_FUNCTION_QTC(transposed, (trans(value)) );
UNARY_FUNCTION_QTC(sin, (sin(value)) );
UNARY_FUNCTION_QTC(cos, (cos(value)) );
UNARY_FUNCTION_QTC(sqr, (pow(value,2)) );
UNARY_FUNCTION_QTC_RET(mag, (fabs(value)), double);
UNARY_FUNCTION_QTC_RET(as_scalar, (arma::as_scalar(value)), double);
UNARY_FUNCTION_QTC_RET(compX, (value(0)), double);
UNARY_FUNCTION_QTC_RET(compY, (value(1)), double);
UNARY_FUNCTION_QTC_RET(compZ, (value(2)), double);

BINARY_FUNCTION_QTC(multiplied, (value1*value2), MultiplyResult );
// BINARY_FUNCTION_QTC_OP(multiplied, operator* );
BINARY_FUNCTION_QTC(divided, (value1/value2), DivisionResult );
// BINARY_FUNCTION_QTC_OP(divided, operator/ );
BINARY_FUNCTION_QTC(added, (value1+value2), AdditionResult );
// BINARY_FUNCTION_QTC_OP(added, operator+ );
BINARY_FUNCTION_QTC(subtracted, (value1-value2), SubtractionResult );
// BINARY_FUNCTION_QTC_OP(subtracted, operator- );
BINARY_FUNCTION_QTC(dotted, (dot(value1,value2)), DotResult );
BINARY_FUNCTION_QTC(angle, (acos(dot(value1,value2)/norm(value1,2)/norm(value2,2))), DotResult );
BINARY_FUNCTION_QTC(angleMag, (acos(abs(dot(value1,value2)/norm(value1,2)/norm(value2,2)))), DotResult );

class vertexLocation
    : public QuantityComputer<arma::mat>
{
public:
    vertexLocation();
    virtual ~vertexLocation();

    virtual arma::mat evaluate(FeatureID ei);

    virtual QuantityComputer<arma::mat>::Ptr clone() const;
};

class edgeCoG
    : public QuantityComputer<arma::mat>
{
public:
    edgeCoG();
    virtual ~edgeCoG();

    virtual arma::mat evaluate(FeatureID ei);

    virtual QuantityComputer<arma::mat>::Ptr clone() const;
};

class edgeMidTangent
    : public QuantityComputer<arma::mat>
{
public:
    edgeMidTangent();
    virtual ~edgeMidTangent();

    virtual arma::mat evaluate(FeatureID ei);

    virtual QuantityComputer<arma::mat>::Ptr clone() const;
};

class edgeStart
    : public QuantityComputer<arma::mat>
{
public:
    edgeStart();
    virtual ~edgeStart();

    virtual arma::mat evaluate(FeatureID ei);

    virtual QuantityComputer<arma::mat>::Ptr clone() const;
};

class edgeEnd
    : public QuantityComputer<arma::mat>
{
public:
    edgeEnd();
    virtual ~edgeEnd();

    virtual arma::mat evaluate(FeatureID ei);

    virtual QuantityComputer<arma::mat>::Ptr clone() const;
};

class edgeLen
    : public QuantityComputer<double>
{
public:
    edgeLen();
    virtual ~edgeLen();

    virtual double evaluate(FeatureID ei);

    virtual QuantityComputer<double>::Ptr clone() const;
};

class faceArea
    : public QuantityComputer<double>
{
public:
    faceArea();
    virtual ~faceArea();

    virtual double evaluate(FeatureID ei);

    virtual QuantityComputer<double>::Ptr clone() const;
};


class edgeRadialLen
    : public QuantityComputer<double>
{
protected:
    boost::shared_ptr<matQuantityComputer > ax_;
    boost::shared_ptr<matQuantityComputer > p0_;
    
public:
    edgeRadialLen(const boost::shared_ptr<matQuantityComputer >& ax, const boost::shared_ptr<matQuantityComputer >& p0);
    virtual ~edgeRadialLen();

    virtual double evaluate(FeatureID ei);

    virtual QuantityComputer<double>::Ptr clone() const;
};


class faceCoG
    : public QuantityComputer<arma::mat>
{
public:
    faceCoG();
    virtual ~faceCoG();

    virtual arma::mat evaluate(FeatureID ei);

    virtual QuantityComputer<arma::mat>::Ptr clone() const;
};

class solidCoG
    : public QuantityComputer<arma::mat>
{
public:
    solidCoG();
    virtual ~solidCoG();

    virtual arma::mat evaluate(FeatureID ei);

    virtual QuantityComputer<arma::mat>::Ptr clone() const;
};

class faceNormalVector
: public QuantityComputer<arma::mat>
{
public:
    faceNormalVector();
    virtual ~faceNormalVector();

    virtual arma::mat evaluate(FeatureID fi);

    virtual QuantityComputer<arma::mat>::Ptr clone() const;
};

class cylRadius
    : public QuantityComputer<double>
{
public:
    cylRadius();
    virtual ~cylRadius();

    virtual bool isValidForFeature(FeatureID) const;
    virtual double evaluate(FeatureID fi);

    virtual QuantityComputer<double>::Ptr clone() const;
};

#define RELATION_QTY_FILTER(RELATION_QTY_FILTER_NAME, RELATION_QTY_FILTER_OP) \
template <class T1, class T2>\
class RELATION_QTY_FILTER_NAME\
: public Filter\
{\
protected:\
  boost::shared_ptr<QuantityComputer<T1> > qtc1_;\
  boost::shared_ptr<QuantityComputer<T2> > qtc2_;\
  \
public:\
  RELATION_QTY_FILTER_NAME(const QuantityComputer<T1>& qtc1, const QuantityComputer<T2>& qtc2)\
  : qtc1_(qtc1.clone()),\
    qtc2_(qtc2.clone())\
  {}\
  \
  virtual void initialize(const SolidModel& m)\
  {\
    Filter::initialize(m);\
    qtc1_->initialize(m);\
    qtc2_->initialize(m);\
  }\
  virtual bool checkMatch(FeatureID f) const\
  {\
    T1 value1 = qtc1_->evaluate(f);\
    T2 value2 = qtc2_->evaluate(f);\
    bool result = (RELATION_QTY_FILTER_OP);\
    return result;\
  }\
  \
  virtual FilterPtr clone() const\
  {\
    return FilterPtr(new RELATION_QTY_FILTER_NAME(*qtc1_, *qtc2_));\
  }\
};

RELATION_QTY_FILTER(greater, (value1>value2));
RELATION_QTY_FILTER(greaterequal, (value1>=value2));
RELATION_QTY_FILTER(less, (value1<value2));
RELATION_QTY_FILTER(lessequal, (value1<=value2));
RELATION_QTY_FILTER(equal, (value1==value2));

template <class T>
class approximatelyEqual
    : public Filter
{
protected:
    boost::shared_ptr<QuantityComputer<T> > qtc1_;
    boost::shared_ptr<QuantityComputer<T> > qtc2_;
    double tol_;

public:
    approximatelyEqual(const QuantityComputer<T>& qtc1, const QuantityComputer<T>& qtc2, double tol)
        : qtc1_(qtc1.clone()),
          qtc2_(qtc2.clone()),
          tol_(tol)
    {}

    virtual void initialize(const SolidModel& m)
    {
        Filter::initialize(m);
        qtc1_->initialize(m);
        qtc2_->initialize(m);
    }

    virtual bool checkMatch(FeatureID f) const
    {
        T value1 = qtc1_->evaluate(f);
        T value2 = qtc2_->evaluate(f);

        T atol=tol_*value2;
        bool result = ( fabs(value1-value2) < atol );
        return result;
    }

    virtual FilterPtr clone() const
    {
        return FilterPtr(new approximatelyEqual(*qtc1_, *qtc2_, tol_));
    }
};

class maximal
    : public Filter
{
protected:
    int rank_;
    boost::shared_ptr<scalarQuantityComputer> qtc_;
    std::map<double, std::set<FeatureID> > ranking_;

public:
    maximal(const scalarQuantityComputer& qtc, int rank=0);
    virtual void firstPass(FeatureID feature);
    virtual void initialize(const SolidModel& m);
    virtual bool checkMatch(FeatureID feature) const;

    virtual FilterPtr clone() const;
};

class minimal
    : public maximal
{
public:
    minimal(const scalarQuantityComputer& qtc, int rank=0);
    virtual void firstPass(FeatureID feature);
    virtual FilterPtr clone() const;
};
/*

#define RELATION_QTY_FILTER_OPERATOR(RELATION_QTY_FILTER_NAME, RELATION_QTY_FILTER_OP) \
template <class T1, class T2> \
RELATION_QTY_FILTER_NAME<T1, T2> RELATION_QTY_FILTER_OP(const QuantityComputer<T1>& qtc1, const QuantityComputer<T2>& qtc2) \
{\
  return RELATION_QTY_FILTER_NAME<T1, T2>(qtc1, qtc2);\
}\
template <class T1, class T2> \
RELATION_QTY_FILTER_NAME<typename boost::enable_if<boost::is_arithmetic<T1>, T1 >::type, T2> RELATION_QTY_FILTER_OP(const T1& qt1, const QuantityComputer<T2>& qtc2) \
{\
  return RELATION_QTY_FILTER_NAME<T1, T2>(constantQuantity<T1>(qt1), qtc2);\
}\
template <class T1, class T2> \
RELATION_QTY_FILTER_NAME<T1, typename boost::enable_if<boost::is_arithmetic<T2>, T2 >::type> RELATION_QTY_FILTER_OP(const QuantityComputer<T1>& qtc1, const T2& qt2) \
{\
  return RELATION_QTY_FILTER_NAME<T1, T2>(qtc1, constantQuantity<T2>(qt2));\
}


RELATION_QTY_FILTER_OPERATOR(greater, operator> );
RELATION_QTY_FILTER_OPERATOR(greaterequal, operator>= );
RELATION_QTY_FILTER_OPERATOR(less, operator< );
RELATION_QTY_FILTER_OPERATOR(lessequal, operator<= );
RELATION_QTY_FILTER_OPERATOR(equal, operator== );

#ifdef SWIG
%template(greaterDouble) greater<double, double>;
%template(equalDouble) equal<double, double>;
#endif*/

FilterPtr parseVertexFilterExpr(std::istream& stream, const FeatureSetParserArgList& refs=FeatureSetParserArgList() );
FilterPtr parseEdgeFilterExpr(std::istream& stream, const FeatureSetParserArgList& refs=FeatureSetParserArgList() );
FilterPtr parseFaceFilterExpr(std::istream& stream, const FeatureSetParserArgList& refs=FeatureSetParserArgList() );
FilterPtr parseSolidFilterExpr(std::istream& stream, const FeatureSetParserArgList& refs=FeatureSetParserArgList() );

}
}

#endif
