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

#include "boost/shared_ptr.hpp"
#include "boost/concept_check.hpp"
#include "boost/utility.hpp"
#include "boost/graph/graph_concepts.hpp"
#include "boost/filesystem.hpp"

#include "base/linearalgebra.h"
#include "base/exception.h"
#include "feature.h"
#include "occinclude.h"

namespace insight
{
namespace cad
{

std::ostream& operator<<(std::ostream& os, const FeatureSet& fs);

class SolidModel;
class Sketch;
class AND;
class NOT;
class OR;

class Filter
{

protected:
    const SolidModel* model_;
public:
    Filter();
    virtual ~Filter();

    virtual void initialize(const SolidModel& m);
    virtual void firstPass(FeatureID feature);
    virtual bool checkMatch(FeatureID feature) const =0;

    virtual boost::shared_ptr<Filter> clone() const =0;

    boost::shared_ptr<Filter> operator&&(const Filter& f2);
    boost::shared_ptr<Filter> operator!();

};

typedef boost::shared_ptr<Filter> FilterPtr;

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

typedef coincident<Edge> coincidentEdge;
typedef coincident<Face> coincidentFace;

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

// #ifdef SWIG
// %template(doubleQuantityComputer) QuantityComputer<double>;
// %template(matQuantityComputer) QuantityComputer<arma::mat>;
// #endif

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
  virtual typename QuantityComputer<T>::Ptr clone() const { return QuantityComputer<T>::Ptr(new OPERATED_QTC_NAME(*qtc_)); };\
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
UNARY_FUNCTION_QTC_RET(as_scalar, (arma::as_scalar(value)), double);

BINARY_FUNCTION_QTC(multiplied, (value1*value2), MultiplyResult );
// BINARY_FUNCTION_QTC_OP(multiplied, operator* );
BINARY_FUNCTION_QTC(divided, (value1/value2), DivisionResult );
// BINARY_FUNCTION_QTC_OP(divided, operator/ );
BINARY_FUNCTION_QTC(added, (value1+value2), AdditionResult );
// BINARY_FUNCTION_QTC_OP(added, operator+ );
BINARY_FUNCTION_QTC(subtracted, (value1-value2), SubtractionResult );
// BINARY_FUNCTION_QTC_OP(subtracted, operator- );
BINARY_FUNCTION_QTC(dotted, (dot(value1,value2)), DotResult );
BINARY_FUNCTION_QTC(angle, (acos(dot(value1,value2)/norm(value1)/norm(value2))), DotResult );
BINARY_FUNCTION_QTC(angleMag, (abs(acos(dot(value1,value2)/norm(value1)/norm(value2)))), DotResult );

class edgeCoG
    : public QuantityComputer<arma::mat>
{
public:
    edgeCoG();
    virtual ~edgeCoG();

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

FilterPtr parseEdgeFilterExpr(std::istream& stream, const std::vector<FeatureSet>& refs=std::vector<FeatureSet>() );
FilterPtr parseFaceFilterExpr(std::istream& stream, const std::vector<FeatureSet>& refs=std::vector<FeatureSet>());

}
}

#endif
