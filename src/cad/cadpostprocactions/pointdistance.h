 
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
 */

#ifndef INSIGHT_CAD_DISTANCEPP_H
#define INSIGHT_CAD_DISTANCEPP_H

#include "base/cppextensions.h"
#include "cadtypes.h"
#include "cadparameters.h"
#include "cadpostprocaction.h"
#include "constrainedsketchgeometry.h"

namespace insight {
namespace cad {

class Distance
: public PostprocAction
{


  size_t calcHash() const override;

public:
  VectorPtr p1_, p2_;
  double distance_;

public:
  declareType("Distance");

  Distance(VectorPtr p1, VectorPtr p2);

  void build() override;

  void write(std::ostream&) const override;
//  virtual Handle_AIS_InteractiveObject createAISRepr() const;

  void operator=(const Distance& other);

  /**
   * @brief dimLineOffset
   * @return
   * offset of dimension line from connection between points.
   */
  virtual arma::mat dimLineOffset() const;
  virtual double relativeArrowSize() const;

};



class DistanceConstraint
: public Distance,
  public ConstrainedSketchEntity
{
    VectorPtr planeNormal_;

    size_t calcHash() const override;


    DistanceConstraint(VectorPtr p1, VectorPtr p2, VectorPtr planeNormal,
                       double targetValue, const std::string& layerName = std::string());
    DistanceConstraint(VectorPtr p1, VectorPtr p2, VectorPtr planeNormal);

public:
    declareType("DistanceConstraint");

    CREATE_FUNCTION(DistanceConstraint);

    double targetValue() const;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        ConstrainedSketchScriptBuffer& script,
        const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const override;

    static void addParserRule(ConstrainedSketchGrammar& ruleset, MakeDefaultGeometryParametersFunction mdpf);

    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > dependencies() const override;
    void replaceDependency(
        const std::weak_ptr<ConstrainedSketchEntity>& entity,
        const std::shared_ptr<ConstrainedSketchEntity>& newEntity) override;

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const DistanceConstraint& other);

    arma::mat dimLineOffset() const override;
    void setDimLineOffset(const arma::mat& p);
    double relativeArrowSize() const override;
};

}
}

#endif // INSIGHT_CAD_DISTANCEPP_H
