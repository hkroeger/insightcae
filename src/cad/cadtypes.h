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

#define INSIGHT_CAD_DEBUG 1

#include <set>
#include <string>
#include <vector>

#include "base/exception.h"
#include "base/boost_include.h"

#ifndef Q_MOC_RUN
#include "boost/variant.hpp"
#include "boost/fusion/container.hpp"
#include "base/linearalgebra.h"
#endif

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
class Filter;
class Model;
class PostprocAction;

typedef boost::shared_ptr<Feature> FeaturePtr;
typedef boost::shared_ptr<Feature const> ConstFeaturePtr;
typedef boost::shared_ptr<Datum> DatumPtr;
typedef boost::shared_ptr<Datum const> ConstDatumPtr;
typedef boost::shared_ptr<FeatureSet> FeatureSetPtr;
typedef boost::shared_ptr<FeatureSet const > ConstFeatureSetPtr;
typedef boost::shared_ptr<Model> ModelPtr;
typedef boost::shared_ptr<Model const> ConstModelPtr;
typedef boost::shared_ptr<PostprocAction> PostprocActionPtr;
typedef boost::shared_ptr<PostprocAction const> ConstPostprocActionPtr;
typedef boost::shared_ptr<Scalar> ScalarPtr;
typedef boost::shared_ptr<Scalar const> ConstScalarPtr;
typedef boost::shared_ptr<Vector> VectorPtr;
typedef boost::shared_ptr<Vector const> ConstVectorPtr;
typedef boost::shared_ptr<FeatureSet> FeatureSetPtr;
typedef boost::shared_ptr<FeatureSet const> ConstFeatureSetPtr;
typedef boost::shared_ptr<Filter> FilterPtr;
typedef boost::shared_ptr<Filter const> ConstFilterPtr;

typedef int FeatureID;
typedef boost::variant<FeatureSetPtr,VectorPtr,ScalarPtr> FeatureSetParserArg;
typedef std::vector<FeatureSetParserArg> FeatureSetParserArgList;
typedef std::vector<FeatureSetPtr> FeatureSetList;
typedef std::set<FeatureID> FeatureSetData;
typedef std::map<std::string, FeaturePtr> SubfeatureMap;
typedef std::map<std::string, FeatureSetPtr> FeatureSetPtrMap;
typedef std::map<std::string, DatumPtr > DatumPtrMap;
typedef boost::variant<FeaturePtr, DatumPtr, VectorPtr, ScalarPtr>  ModelVariable;
typedef std::vector<boost::fusion::vector2<std::string, ModelVariable> > ModelVariableTable;


boost::filesystem::path sharedModelFilePath(const std::string& name);

class CADException
: public insight::Exception
{
public:
    CADException(const Feature& errorfeat, const std::string message);
};

}
}

#endif
