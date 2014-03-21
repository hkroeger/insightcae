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
#include <boost/concept_check.hpp>

#include "occinclude.h"

namespace insight 
{
namespace cad 
{

typedef int FeatureID;
typedef std::set<FeatureID> FeatureSet;

class SolidModel;

class Filter
{
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

class edgeTopology
: public Filter
{
public:
  enum Topology { Line, Circle };
  
protected:
  Topology top_;
  
public:
  edgeTopology(Topology t);
  virtual bool checkMatch(FeatureID feature) const;
  
  virtual Filter* clone() const;
};

class SolidModel
{
protected :
  // the shape
  TopoDS_Shape shape_;
  // all the (sub) TopoDS_Shapes in 'shape'
  TopTools_IndexedMapOfShape fmap_, emap_, vmap_, somap_, shmap_, wmap_;
 
public:
    SolidModel(const TopoDS_Shape& shape);
    virtual ~SolidModel();

    void nameFeatures();
    
    inline const TopoDS_Face& face(FeatureID i) const { return TopoDS::Face(fmap_.FindKey(i)); }
    inline const TopoDS_Edge& edge(FeatureID i) const { return TopoDS::Edge(emap_.FindKey(i)); }
    inline const TopoDS_Vertex& vertex(FeatureID i) const { return TopoDS::Vertex(vmap_.FindKey(i)); }
    
    FeatureSet query_edges(const Filter& filter) const;
};

}
}

#endif // INSIGHT_CAD_SOLIDMODEL_H
