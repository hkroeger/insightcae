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

#ifndef INSIGHT_CAD_QUANTITYFUNCTIONS_H
#define INSIGHT_CAD_QUANTITYFUNCTIONS_H

#include "feature.h"

namespace insight
{
namespace cad
{

  

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
  virtual void initialize(FeaturePtr m)\
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
  virtual void initialize(FeaturePtr m)\
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
  virtual void initialize(FeaturePtr m)\
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

}
}

#endif
