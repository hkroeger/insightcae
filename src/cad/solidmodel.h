/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef INSIGHT_CAD_SOLIDMODEL_H
#define INSIGHT_CAD_SOLIDMODEL_H

#include <set>
#include <map>
#include <vector>

#include "boost/shared_ptr.hpp"
#include "boost/concept_check.hpp"
#include "boost/utility.hpp"
#include "boost/graph/graph_concepts.hpp"
#include "boost/filesystem.hpp"

#include "base/linearalgebra.h"
#include "base/exception.h"

#include "occinclude.h"

namespace boost
{
  
template <>
struct is_arithmetic<arma::mat> 
: public true_type
{
};

}

namespace insight 
{
namespace cad 
{

typedef int FeatureID;
typedef std::set<FeatureID> FeatureSet;

std::ostream& operator<<(std::ostream& os, const FeatureSet& fs);

class SolidModel;
class Sketch;

class Filter
{
public:
  typedef boost::shared_ptr<Filter> Ptr;
  
protected:
  const SolidModel* model_;
public:
  Filter();
  virtual ~Filter();
  
  virtual void initialize(const SolidModel& m);
  virtual bool checkMatch(FeatureID feature) const =0;
  
  virtual Filter* clone() const =0;
};

inline Filter* new_clone(const Filter& f)
{
  return f.clone();
}

class ANDFilter
: public Filter
{
protected:
  boost::shared_ptr<Filter> f1_;
  boost::shared_ptr<Filter> f2_;
public:
  ANDFilter(const Filter& f1, const Filter& f2);
  virtual void initialize(const SolidModel& m);
  virtual bool checkMatch(FeatureID feature) const;
  
  virtual Filter* clone() const;
};

class NOTFilter
: public Filter
{
protected:
  boost::shared_ptr<Filter> f1_;
public:
  NOTFilter(const Filter& f1);
  virtual void initialize(const SolidModel& m);
  virtual bool checkMatch(FeatureID feature) const;
  
  virtual Filter* clone() const;
};

ANDFilter operator&&(const Filter& f1, const Filter& f2);
NOTFilter operator!(const Filter& f1);

class edgeTopology
: public Filter
{
protected:
  GeomAbs_CurveType ct_;
  
public:
  edgeTopology(GeomAbs_CurveType ct);
  virtual bool checkMatch(FeatureID feature) const;
  
  virtual Filter* clone() const;
};

class everything
: public Filter
{
  
public:
  everything();
  virtual bool checkMatch(FeatureID feature) const;
  
  virtual Filter* clone() const;
};

enum EntityType { Edge, Face};

template<EntityType T>
class coincident
: public Filter
{
protected:
  const SolidModel& m_;
  FeatureSet f_;
  
public:
  coincident(const SolidModel& m)
  : m_(m)
  {
    throw insight::Exception("coincident filter: not implemented!");
  }

  coincident(const SolidModel& m, FeatureSet f)
  : m_(m),
    f_(f)
  {}

  bool checkMatch(FeatureID feature) const
  {
    throw insight::Exception("coincident filter: not implemented!");
  }

  Filter* clone() const
  {
    return new coincident(m_, f_);
  }
    
};

template<> coincident<Edge>::coincident(const SolidModel& m);
template<> bool coincident<Edge>::checkMatch(FeatureID feature) const;
template<> coincident<Face>::coincident(const SolidModel& m);
template<> bool coincident<Face>::checkMatch(FeatureID feature) const;


template<class T>
class QuantityComputer
{
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
  virtual T evaluate(FeatureID) =0;  
  virtual QuantityComputer* clone() const =0;
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
  virtual T evaluate(FeatureID) { return refValue_; };
  virtual QuantityComputer<T>* clone() const { return new constantQuantity<T>(refValue_); };
};

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
  virtual T evaluate(FeatureID f)\
  {\
    T value = qtc_->evaluate(f);\
    value=OPERATED_QTC_OP;\
    return value;\
  }\
  \
  virtual QuantityComputer<T>* clone() const { return new OPERATED_QTC_NAME(*qtc_); };\
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
  virtual RETURN_T evaluate(FeatureID f)\
  {\
    T value = qtc_->evaluate(f);\
    RETURN_T rvalue=OPERATED_QTC_OP;\
    return rvalue;\
  }\
  \
  virtual QuantityComputer<RETURN_T>* clone() const { return new OPERATED_QTC_NAME(*qtc_); };\
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
  virtual typename RESULT_T<T1,T2>::type evaluate(FeatureID f)\
  {\
    T1 value1 = qtc1_->evaluate(f);\
    T2 value2 = qtc2_->evaluate(f);\
    typename RESULT_T<T1,T2>::type value=OPERATED_QTC_OP;\
    return value;\
  }\
  \
  virtual QuantityComputer< typename RESULT_T<T1,T2>::type >* clone() const { return new OPERATED_QTC_NAME(*qtc1_, *qtc2_); };\
};

#define BINARY_FUNCTION_QTC_OP(OPERATED_QTC_NAME, OPERATED_QTC_OP) \
template<class T1, class T2> \
OPERATED_QTC_NAME<T1,T2> OPERATED_QTC_OP(const QuantityComputer<T1>& qtc1, const QuantityComputer<T2>& qtc2) \
{ \
  return OPERATED_QTC_NAME<T1,T2>::type >(qtc1, qtc2); \
} \
template<class T1, class T2> \
OPERATED_QTC_NAME<typename boost::enable_if<boost::is_arithmetic<T1>, T1 >::type, T2> OPERATED_QTC_OP(const T1& qt1, const QuantityComputer<T2>& qtc2) \
{ \
  return OPERATED_QTC_NAME<T1, T2>(constantQuantity<T1>(qt1), qtc2); \
} \
template<class T1, class T2> \
OPERATED_QTC_NAME<T1, typename boost::enable_if<boost::is_arithmetic<T2>, T2 >::type> OPERATED_QTC_OP(const QuantityComputer<T1>& qtc1, const T2& qt2) \
{ \
  return OPERATED_QTC_NAME<T1, T2>(qtc1, constantQuantity<T2>(qt2)); \
}

template<class T1, class T2> struct MultiplyResult {};
template<> struct MultiplyResult<arma::mat, arma::mat> { typedef arma::mat type; };
template<> struct MultiplyResult<double, arma::mat> { typedef arma::mat type; };
template<> struct MultiplyResult<arma::mat, double> { typedef arma::mat type; };
template<> struct MultiplyResult<double, double> { typedef double type; };

template<class T1, class T2> struct DivisionResult {};
template<> struct DivisionResult<arma::mat, arma::mat> { typedef arma::mat type; };
template<> struct DivisionResult<arma::mat, double> { typedef arma::mat type; };
template<> struct DivisionResult<double, double> { typedef double type; };

template<class T1, class T2> struct AdditionResult {};
template<> struct AdditionResult<arma::mat, arma::mat> { typedef arma::mat type; };
template<> struct AdditionResult<double, double> { typedef double type; };

template<class T1, class T2> struct SubtractionResult {};
template<> struct SubtractionResult<arma::mat, arma::mat> { typedef arma::mat type; };
template<> struct SubtractionResult<double, double> { typedef double type; };

UNARY_FUNCTION_QTC(transposed, (trans(value)) );
UNARY_FUNCTION_QTC_RET(as_scalar, (arma::as_scalar(value)), double);
UNARY_FUNCTION_QTC(sin, (sin(value)) );
UNARY_FUNCTION_QTC(cos, (cos(value)) );

BINARY_FUNCTION_QTC(multiplied, (value1*value2), MultiplyResult );
BINARY_FUNCTION_QTC_OP(multiplied, operator* );
BINARY_FUNCTION_QTC(divided, (value1/value2), DivisionResult );
BINARY_FUNCTION_QTC_OP(divided, operator/ );
BINARY_FUNCTION_QTC(added, (value1+value2), AdditionResult );
BINARY_FUNCTION_QTC_OP(added, operator+ );
BINARY_FUNCTION_QTC(subtracted, (value1-value2), SubtractionResult );
BINARY_FUNCTION_QTC_OP(subtracted, operator- );

class edgeCoG
: public QuantityComputer<arma::mat>
{
public:
  edgeCoG();
  ~edgeCoG();
  
  virtual arma::mat evaluate(FeatureID ei);
  
  virtual QuantityComputer<arma::mat>* clone() const;
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
  virtual Filter* clone() const\
  {\
    return new RELATION_QTY_FILTER_NAME(*qtc1_, *qtc2_);\
  }\
};

RELATION_QTY_FILTER(greater, (value1>value2));
RELATION_QTY_FILTER(greaterequal, (value1>=value2));
RELATION_QTY_FILTER(less, (value1<value2));
RELATION_QTY_FILTER(lessequal, (value1<=value2));
RELATION_QTY_FILTER(equal, (value1==value2));

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

std::ostream& operator<<(std::ostream& os, const SolidModel& m);

class SolidModel
{
public:
  typedef boost::shared_ptr<SolidModel> Ptr;
  
  struct View
  {
    TopoDS_Shape visibleEdges, hiddenEdges, crossSection;
  };
  typedef std::map<std::string, View> Views;
  
protected :
  // the shape
  TopoDS_Shape shape_;
  // all the (sub) TopoDS_Shapes in 'shape'
  TopTools_IndexedMapOfShape fmap_, emap_, vmap_, somap_, shmap_, wmap_;
  
  TopoDS_Shape loadShapeFromFile(const boost::filesystem::path& filepath);
 
public:
  
  SolidModel();
  SolidModel(const SolidModel& o);
  SolidModel(const TopoDS_Shape& shape);
  SolidModel(const boost::filesystem::path& filepath);
  virtual ~SolidModel();
  
  SolidModel& operator=(const SolidModel& o);

  void nameFeatures();
  
  inline const TopoDS_Face& face(FeatureID i) const { return TopoDS::Face(fmap_.FindKey(i)); }
  inline const TopoDS_Edge& edge(FeatureID i) const { return TopoDS::Edge(emap_.FindKey(i)); }
  inline const TopoDS_Vertex& vertex(FeatureID i) const { return TopoDS::Vertex(vmap_.FindKey(i)); }
  
  GeomAbs_CurveType edgeType(FeatureID i) const;
  GeomAbs_SurfaceType faceType(FeatureID i) const;
  
  arma::mat edgeCoG(FeatureID i) const;
  arma::mat faceCoG(FeatureID i) const;
  
  arma::mat faceNormal(FeatureID i) const;

  FeatureSet allEdges() const;
  FeatureSet allFaces() const;
  
  FeatureSet query_edges(const Filter& filter) const;
  
  void saveAs(const boost::filesystem::path& filename) const;
  
  operator const TopoDS_Shape& () const;
  
  View createView
  (
    const arma::mat p0,
    const arma::mat n,
    bool section
  ) const;
  
  friend std::ostream& operator<<(std::ostream& os, const SolidModel& m);
};

// =================== Primitives ======================
class Cylinder
: public SolidModel
{
public:
  Cylinder(const arma::mat& p1, const arma::mat& p2, double D);
};

class Box
: public SolidModel
{
protected:
  TopoDS_Shape makeBox
  (
    const arma::mat& p0,
    const arma::mat& L1,
    const arma::mat& L2,
    const arma::mat& L3
  );
  
public:
  Box
  (
    const arma::mat& p0, 
    const arma::mat& L1, 
    const arma::mat& L2, 
    const arma::mat& L3
  );
};

class Sphere
: public SolidModel
{
public:
  Sphere(const arma::mat& p, double D);
};

class Extrusion
: public SolidModel
{
public:
  Extrusion(const Sketch& sk, const arma::mat& L);
};

// =================== Boolean operations ======================
class BooleanUnion
: public SolidModel
{
public:
  BooleanUnion(const SolidModel& m1, const SolidModel& m2);
};

SolidModel operator|(const SolidModel& m1, const SolidModel& m2);

class BooleanSubtract
: public SolidModel
{
public:
  BooleanSubtract(const SolidModel& m1, const SolidModel& m2);
};

SolidModel operator-(const SolidModel& m1, const SolidModel& m2);

// =================== Cosmetic features ======================

class Fillet
: public SolidModel
{
  TopoDS_Shape makeFillets(const SolidModel& m1, const FeatureSet& edges, double r);
  
public:
  Fillet(const SolidModel& m1, const FeatureSet& edges, double r);
};

class Chamfer
: public SolidModel
{
  TopoDS_Shape makeChamfers(const SolidModel& m1, const FeatureSet& edges, double l);
  
public:
  Chamfer(const SolidModel& m1, const FeatureSet& edges, double l);
};

}
}

#endif // INSIGHT_CAD_SOLIDMODEL_H
