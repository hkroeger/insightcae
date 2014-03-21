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

#include <memory>
#include "solidmodel.h"
#include <base/exception.h>

namespace insight 
{
namespace cad 
{
  
Filter::Filter()
: model_(NULL)
{
}

Filter::~Filter()
{
}

void Filter::initialize(const SolidModel& m)
{
  model_=&m;
}


edgeTopology::edgeTopology(Topology t)
: top_(t)
{
}

bool edgeTopology::checkMatch(FeatureID feature) const
{
  const TopoDS_Edge& edge = model_->edge(feature);
  double t0, t1;
  Handle_Geom_Curve crv=BRep_Tool::Curve(edge, t0, t1);
  GeomAdaptor_Curve adapt(crv);
  if (adapt.GetType()==GeomAbs_Circle)
    return true;
  else
    return false;
}

Filter* edgeTopology::clone() const
{
  return new edgeTopology(top_);
}

SolidModel::SolidModel(const TopoDS_Shape& shape)
: shape_(shape)
{
  nameFeatures();
}

SolidModel::~SolidModel()
{
}

FeatureSet SolidModel::query_edges(const Filter& filter) const
{
  std::auto_ptr<Filter> f(filter.clone());
  
  f->initialize(*this);
  FeatureSet res;
  for (int i=1; i<=emap_.Extent(); i++)
  {
    if (f->checkMatch(i)) res.insert(i);
  }
  return res;
}


void SolidModel::nameFeatures()
{

    // Solids
    TopExp_Explorer exp0, exp1, exp2, exp3, exp4, exp5;
    for(exp0.Init(shape_, TopAbs_SOLID); exp0.More(); exp0.Next()) {
        TopoDS_Solid solid = TopoDS::Solid(exp0.Current());
        if(somap_.FindIndex(solid) < 1) {
            somap_.Add(solid);

            for(exp1.Init(solid, TopAbs_SHELL); exp1.More(); exp1.Next()) {
                TopoDS_Shell shell = TopoDS::Shell(exp1.Current());
                if(shmap_.FindIndex(shell) < 1) {
                    shmap_.Add(shell);

                    for(exp2.Init(shell, TopAbs_FACE); exp2.More(); exp2.Next()) {
                        TopoDS_Face face = TopoDS::Face(exp2.Current());
                        if(fmap_.FindIndex(face) < 1) {
                            fmap_.Add(face);

                            for(exp3.Init(exp2.Current(), TopAbs_WIRE); exp3.More(); exp3.Next()) {
                                TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
                                if(wmap_.FindIndex(wire) < 1) {
                                    wmap_.Add(wire);

                                    for(exp4.Init(exp3.Current(), TopAbs_EDGE); exp4.More(); exp4.Next()) {
                                        TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
                                        if(emap_.FindIndex(edge) < 1) {
                                            emap_.Add(edge);

                                            for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
                                                TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
                                                if(vmap_.FindIndex(vertex) < 1)
                                                    vmap_.Add(vertex);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Free Shells
    for(exp1.Init(exp0.Current(), TopAbs_SHELL, TopAbs_SOLID); exp1.More(); exp1.Next()) {
        TopoDS_Shape shell = exp1.Current();
        if(shmap_.FindIndex(shell) < 1) {
            shmap_.Add(shell);

            for(exp2.Init(shell, TopAbs_FACE); exp2.More(); exp2.Next()) {
                TopoDS_Face face = TopoDS::Face(exp2.Current());
                if(fmap_.FindIndex(face) < 1) {
                    fmap_.Add(face);

                    for(exp3.Init(exp2.Current(), TopAbs_WIRE); exp3.More(); exp3.Next()) {
                        TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
                        if(wmap_.FindIndex(wire) < 1) {
                            wmap_.Add(wire);

                            for(exp4.Init(exp3.Current(), TopAbs_EDGE); exp4.More(); exp4.Next()) {
                                TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
                                if(emap_.FindIndex(edge) < 1) {
                                    emap_.Add(edge);

                                    for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
                                        TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
                                        if(vmap_.FindIndex(vertex) < 1)
                                            vmap_.Add(vertex);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Free Faces
    for(exp2.Init(shape_, TopAbs_FACE, TopAbs_SHELL); exp2.More(); exp2.Next()) {
        TopoDS_Face face = TopoDS::Face(exp2.Current());
        if(fmap_.FindIndex(face) < 1) {
            fmap_.Add(face);

            for(exp3.Init(exp2.Current(), TopAbs_WIRE); exp3.More(); exp3.Next()) {
                TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
                if(wmap_.FindIndex(wire) < 1) {
                    wmap_.Add(wire);

                    for(exp4.Init(exp3.Current(), TopAbs_EDGE); exp4.More(); exp4.Next()) {
                        TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
                        if(emap_.FindIndex(edge) < 1) {
                            emap_.Add(edge);

                            for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
                                TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
                                if(vmap_.FindIndex(vertex) < 1)
                                    vmap_.Add(vertex);
                            }
                        }
                    }
                }
            }
        }
    }

    // Free Wires
    for(exp3.Init(shape_, TopAbs_WIRE, TopAbs_FACE); exp3.More(); exp3.Next()) {
        TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
        if(wmap_.FindIndex(wire) < 1) {
            wmap_.Add(wire);

            for(exp4.Init(exp3.Current(), TopAbs_EDGE); exp4.More(); exp4.Next()) {
                TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
                if(emap_.FindIndex(edge) < 1) {
                    emap_.Add(edge);

                    for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
                        TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
                        if(vmap_.FindIndex(vertex) < 1)
                            vmap_.Add(vertex);
                    }
                }
            }
        }
    }

    // Free Edges
    for(exp4.Init(shape_, TopAbs_EDGE, TopAbs_WIRE); exp4.More(); exp4.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
        if(emap_.FindIndex(edge) < 1) {
            emap_.Add(edge);

            for(exp5.Init(exp4.Current(), TopAbs_VERTEX); exp5.More(); exp5.Next()) {
                TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
                if(vmap_.FindIndex(vertex) < 1)
                    vmap_.Add(vertex);
            }
        }
    }

    // Free Vertices
    for(exp5.Init(shape_, TopAbs_VERTEX, TopAbs_EDGE); exp5.More(); exp5.Next()) {
        TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
        if(vmap_.FindIndex(vertex) < 1)
            vmap_.Add(vertex);
    }

}


}
}