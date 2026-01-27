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

#ifndef INSIGHT_CAD_CADTYPES_H
#define INSIGHT_CAD_CADTYPES_H

#include "base/cppextensions.h"
#include "boost/variant/static_visitor.hpp"
#include "boost/preprocessor.hpp"
#include <algorithm>
#include <memory>

#include <set>
#include <string>
#include <vector>

#include "base/exception.h"
#include "base/boost_include.h"
#include "base/vtkrendering.h"

#include "TopoDS_Shape.hxx"


#ifndef Q_MOC_RUN
#include "boost/variant.hpp"
#include "boost/fusion/container.hpp"
#include "base/linearalgebra.h"
#include <boost/spirit/include/qi.hpp>
#endif

#include "treeclonemap.h"
#include "dependencysource.h"
#include "dependencyreplacement.h"

namespace insight 
{
namespace cad 
{




class Scalar;
class Vector;
class ASTBase;
class Feature;
class Datum;
class FeatureSet;
class DeferredFeatureSet;
class Filter;
class Model;
class PostprocAction;
class DependencyReplacement;
class DependencySource;
class TreeCloneMap;
class DescriptionWithParameters;
class BOMDescriptionData;


typedef std::shared_ptr<Feature> FeaturePtr;
typedef std::shared_ptr<Feature const> ConstFeaturePtr;
typedef std::shared_ptr<Datum> DatumPtr;
typedef std::shared_ptr<Datum const> ConstDatumPtr;
typedef std::shared_ptr<FeatureSet> FeatureSetPtr;
typedef std::shared_ptr<FeatureSet const > ConstFeatureSetPtr;
typedef std::shared_ptr<Model> ModelPtr;
typedef std::shared_ptr<Model const> ConstModelPtr;
typedef std::shared_ptr<PostprocAction> PostprocActionPtr;
typedef std::shared_ptr<PostprocAction const> ConstPostprocActionPtr;
typedef std::shared_ptr<Scalar> ScalarPtr;
typedef std::shared_ptr<Scalar const> ConstScalarPtr;
typedef std::shared_ptr<Vector> VectorPtr;
typedef std::shared_ptr<Vector const> ConstVectorPtr;
typedef std::shared_ptr<FeatureSet> FeatureSetPtr;
typedef std::shared_ptr<FeatureSet const> ConstFeatureSetPtr;
typedef std::shared_ptr<Filter> FilterPtr;
typedef std::shared_ptr<Filter const> ConstFilterPtr;
typedef std::shared_ptr<DescriptionWithParameters>
        DescriptionWithParametersPtr;
typedef std::shared_ptr<BOMDescriptionData>
    BOMDescriptionDataPtr;


typedef int FeatureID;
typedef boost::variant<FeatureSetPtr,VectorPtr,ScalarPtr> FeatureSetParserArg;
typedef std::vector<FeatureSetParserArg> FeatureSetParserArgList;
typedef std::vector<FeatureSetPtr> FeatureSetList;
typedef std::set<FeatureID> FeatureSetData;
typedef std::map<std::string, FeaturePtr> SubfeatureMap;
typedef std::map<std::string, FeatureSetPtr> FeatureSetPtrMap;
typedef std::map<std::string, DatumPtr > DatumPtrMap;
typedef enum { Point, Direction } VectorVariableType;
typedef boost::fusion::vector2<VectorPtr,VectorVariableType> VectorPtrAndType;
typedef boost::variant<VectorPtrAndType,ScalarPtr,FeaturePtr,DatumPtr>  ModelVariable;
typedef boost::fusion::vector2<std::string, ModelVariable> ModelVariableAndName;
typedef std::vector<ModelVariableAndName> ModelVariableTable;


ModelVariableTable mergeMVTs(const ModelVariableTable& mvt1, const ModelVariableTable& mvt2);


enum EntityType { Vertex, Edge, Face, Solid };




struct VisualizationStyle
{
    /**
     * @brief style
     * enforce style or leave default (=boost::blank)
     */
    boost::variant<boost::blank,DatasetRepresentation> style;
    boost::variant<boost::blank,double> opacity;
    arma::mat color = arma::mat();

    VisualizationStyle();
    VisualizationStyle(
        boost::variant<boost::blank,DatasetRepresentation> style,
        const arma::mat& color = arma::mat(),
        boost::variant<boost::blank,double> opacity=boost::blank() );
};




struct FeatureVisualizationStyle : public VisualizationStyle
{
    std::vector<std::string> associatedParameterPaths = {};
    bool initiallyVisible = true;

    static FeatureVisualizationStyle componentStyle();
    static FeatureVisualizationStyle intermediateFeatureStyle();

    FeatureVisualizationStyle();
    FeatureVisualizationStyle(
        boost::variant<boost::blank,DatasetRepresentation> style,
        const arma::mat& color = arma::mat(),
        const std::vector<std::string> associatedParameterPaths = {},
        boost::variant<boost::blank,double> opacity=boost::blank(),
        bool initiallyVisible = true );
};




struct sharedModelLocations
    : public std::vector<boost::filesystem::path>
{
  sharedModelLocations();
};




boost::filesystem::path sharedModelFilePath(const std::string& name);


#define MAKE_ADDDEPENDENCY_CALLBACK(r, data, elem)  \
DependencySource::DepListInserter(dl, BOOST_PP_STRINGIZE(elem))(elem);

#define MAKE_ADDDEPENDENCY_CALLS(...) \
BOOST_PP_LIST_FOR_EACH( \
MAKE_ADDDEPENDENCY_CALLBACK, _, BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__) )



#define CL(MBRNAME) \
MBRNAME(tcm.clone(o.MBRNAME))


#define CLONEABLE(TYPE) \
inline std::shared_ptr<DependencySource> shallowClone(TreeCloneMap& tcm) const override \
{ return std::shared_ptr<DependencySource>(new TYPE(*this, tcm) ); }

#define DEPENDS_NOINVALIDATE(VARS) \
void replaceDependency(const DependencyReplacement& repl) override \
{ std::for_each(std::tie VARS,repl); } \
void addDependencies(DependencyList& dl) const override \
{ MAKE_ADDDEPENDENCY_CALLS VARS }

#define DEPENDS_DECL \
void replaceDependency(const DependencyReplacement& repl) override;\
void addDependencies(DependencyList& dl) const override

#define DEPENDS_IMPL(TYPE,VARS) \
void TYPE::replaceDependency(const DependencyReplacement& repl) \
{ std::for_each(std::tie VARS,repl); this->invalidate(); } \
void TYPE::addDependencies(DependencySource::DependencyList& dl) const \
{ MAKE_ADDDEPENDENCY_CALLS VARS }

#define DEPENDS_IMPL_NOINVALIDATE(TYPE,VARS) \
void TYPE::replaceDependency(const DependencyReplacement& repl) \
{ std::for_each(std::tie VARS,repl); } \
void TYPE::addDependencies(DependencySource::DependencyList& dl) const \
{ MAKE_ADDDEPENDENCY_CALLS VARS }

#define DEPENDS(VARS) \
void replaceDependency(const DependencyReplacement& repl) override \
{ std::for_each(std::tie VARS,repl); this->invalidate(); } \
void addDependencies(DependencyList& dl) const override \
{ MAKE_ADDDEPENDENCY_CALLS VARS }

#define DEPENDS_W_BASE(BASE, VARS) \
void replaceDependency(const DependencyReplacement& repl) override \
{ BASE::replaceDependency(repl); std::for_each(std::tie VARS, repl); this->invalidate(); } \
void addDependencies(DependencyList& dl) const override \
{ BASE::addDependencies(dl); MAKE_ADDDEPENDENCY_CALLS VARS }



// template<class CloneResult>
// class Cloner
//     : public boost::static_visitor<CloneResult>
// {
//     TreeCloneMap& tcm_;

// public:
//     Cloner(TreeCloneMap& tcm)
//         : tcm_(tcm)
//     {}

//     template<class T>
//     void operator()(std::shared_ptr<const T>& t, std::shared_ptr<const T>& out) const
//     {
//         out=tcm_.clone(*t);
//     }

//     template<class T>
//     void operator()(std::shared_ptr<T>& t, std::shared_ptr<const T>& out) const
//     {
//         out=tcm_.clone(*t);
//     }


//     // dispatch functions
//     template<typename... Args>
//     void operator()(boost::variant<Args...>& t, boost::variant<Args...>& out) const
//     {
//         t.apply_visitor(*this);
//     }

//     template<class X, class Y>
//     void operator()(std::pair<X,Y>& p) const
//     {
//         (*this)(p.first);
//         (*this)(p.second);
//     }

//     template<class X>
//     void operator()(std::vector<X>& v) const
//     {
//         std::for_each(v.begin(), v.end(), *this);
//     }


//     template<class T, class C=char>
//     void operator()(boost::spirit::qi::symbols<C, T>& v) const
//     {
//         v.for_each(
//             [this](const std::string& name, T& v)
//             {
//                 (*this)(v);
//             }
//             );
//     }
// };

}
}

#endif
