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

#ifndef INSIGHT_CAD_SKETCH_H
#define INSIGHT_CAD_SKETCH_H

#include "base/linearalgebra.h"
#include "occinclude.h"

#include "cadtypes.h"
#include "cadparameters.h"
#include "cadfeature.h"

#include "dxfreader.h"

#include "base/boost_include.h"

#include "cadfeatures/line.h"

namespace insight {
namespace cad {



typedef std::vector<boost::fusion::vector2<std::string, ScalarPtr> > SketchVarList;

class Sketch
: public Feature
{
  DatumPtr pl_;
  boost::filesystem::path fn_;
  std::string ln_;
  SketchVarList vars_;
  double tol_;

  Sketch
  (
    DatumPtr pl, 
    const boost::filesystem::path& filename, 
    const std::string& layername="0", 
    const SketchVarList& vars = SketchVarList(), 
    double tol=Precision::Confusion() 
  );

  size_t calcHash() const override;
  void build() override;

public:
  declareType("Sketch");
  CREATE_FUNCTION(Sketch);

  
  void operator=(const Sketch& o);
  
  
  void executeEditor();
  
//   virtual bool isSingleCloseWire() const;
//   virtual TopoDS_Wire asSingleClosedWire() const;
  bool isSingleFace() const override;
  operator const TopoDS_Face& () const;

  static void insertrule(parser::ISCADParser& ruleset);
  
  const boost::filesystem::path& fn() const { return fn_; }
};




class SketchPoint
: public insight::cad::Vector,
  public ConstrainedSketchEntity
{

  DatumPtr plane_;
  double x_, y_;

public:
  declareType("SketchPoint");

  SketchPoint(DatumPtr plane, double x, double y);
  void setCoords2D(double x, double y);
  arma::mat coords2D() const;
  arma::mat value() const override;

  int nDoF() const override;
  double getDoFValue(unsigned int iDoF) const override;
  void setDoFValue(unsigned int iDoF, double value) override;
  void scaleSketch(double scaleFactor) override;

  void generateScriptCommand(
      ConstrainedSketchScriptBuffer& script,
      const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const override;

  static void addParserRule(ConstrainedSketchGrammar& ruleset, MakeDefaultGeometryParametersFunction mdpf);
};


typedef std::shared_ptr<SketchPoint> SketchPointPtr;



class ConstrainedSketchScriptBuffer
{
  std::set<int> entitiesPresent_;
  std::vector<std::string> script_;

public:
  void insertCommandFor(int entityLabel, const std::string& cmd);
  void write(std::ostream& os);
};





class ConstrainedSketch
: public Feature
{
  DatumPtr pl_;
  std::set<ConstrainedSketchEntityPtr> geometry_;
  double solverTolerance_;

  ConstrainedSketch( DatumPtr pl );

  size_t calcHash() const override;
  void build() override;

public:
  declareType("ConstrainedSketch");

  CREATE_FUNCTION(ConstrainedSketch);
  static std::shared_ptr<ConstrainedSketch> createFromStream(
      DatumPtr pl,
      std::istream& is,
      const ParameterSet& geomPS = insight::ParameterSet() );

  const DatumPtr& plane() const;

  std::set<ConstrainedSketchEntityPtr>& geometry();
  const std::set<ConstrainedSketchEntityPtr>& geometry() const;

  std::set<ConstrainedSketchEntityPtr> filterGeometryByParameters(
      std::function<bool(const ParameterSet& geomPS)> filterFunction
      );

  void operator=(const ConstrainedSketch& o);

  double solverTolerance() const;
  void setSolverTolerance(double tol);

  void resolveConstraints();

  static void insertrule(parser::ISCADParser& ruleset);

  void generateScript(std::ostream& os) const;

  std::string generateScriptCommand() const override;

};

typedef std::shared_ptr<ConstrainedSketch> ConstrainedSketchPtr;


}
}

#endif // INSIGHT_CAD_SKETCH_H
