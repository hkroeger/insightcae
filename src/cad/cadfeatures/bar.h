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

#ifndef INSIGHT_CAD_BAR_H
#define INSIGHT_CAD_BAR_H

#include "cadparameters.h"
#include "cadfeature.h"

namespace insight {
namespace cad {


class Bar
: public Feature
{
public:
  typedef boost::variant<
    FeaturePtr,
    std::pair<VectorPtr,VectorPtr>
  > EndPoints;

  typedef boost::fusion::vector3<ScalarPtr,ScalarPtr,ScalarPtr> boost_EndPointMod;

  struct EndPointMod
  {
      ScalarPtr ext = cad::scalarconst(0.0);
      ScalarPtr miterAngleVert = cad::scalarconst(0.0);
      ScalarPtr miterAngleHorz = cad::scalarconst(0.0);
  };

private:
  EndPoints endPts_;
  FeaturePtr xsec_;
  ScalarPtr thickness_;
  VectorPtr vert_;
  ScalarPtr ext0_;
  ScalarPtr ext1_;
  ScalarPtr miterangle0_vert_;
  ScalarPtr miterangle1_vert_;
  ScalarPtr miterangle0_hor_; 
  ScalarPtr miterangle1_hor_;
  VectorPtr attractPt_;

  size_t calcHash() const override;
  void build() override;

 /**
   * crate bar between p0 and p1. Cross section's xsec (single face) y-axis will be aligned with vertical direction vert.
   * bar is elongated at p0 by ext0 and at p1 by ext1, respectively.
   * 
   * @param miterangle0_vert Miter angle of bar end at (elongated) p0 around vertical direction
   * @param miterangle0_hor Miter angle of bar end at (elongated) p0 around horizontal direction
   * @param miterangle1_vert Miter angle of bar end at (elongated) p1 around vertical direction
   * @param miterangle1_hor Miter angle of bar end at (elongated) p1 around horizontal direction
   */
  Bar
  (
    EndPoints endPts,
    FeaturePtr xsec, VectorPtr vert,
    ScalarPtr ext0, ScalarPtr ext1,
    ScalarPtr miterangle0_vert, ScalarPtr miterangle1_vert,
    ScalarPtr miterangle0_hor, ScalarPtr miterangle1_hor,
    VectorPtr attractPt
  );

  Bar
  (
    EndPoints endPts,
    FeaturePtr xsec, VectorPtr vert,
    const boost_EndPointMod& ext_miterv_miterh0,
    const boost_EndPointMod& ext_miterv_miterh1,
    VectorPtr attractPt
  );

  Bar
  (
      EndPoints endPts,
      FeaturePtr xsec, ScalarPtr thickness,
      VectorPtr vert,
      const boost_EndPointMod& ext_miterv_miterh0,
      const boost_EndPointMod& ext_miterv_miterh1,
      VectorPtr attractPt
  );


public:
  declareType("Bar");

  CREATE_FUNCTION(Bar);

  static std::shared_ptr<Bar> create_derived
  (
    FeaturePtr skel,
    FeaturePtr xsec, VectorPtr vert,
    const EndPointMod& ext_miterv_miterh0,
    const EndPointMod& ext_miterv_miterh1,
    VectorPtr attractPt
  );

  void operator=(const Bar& o);

  static void insertrule(parser::ISCADParser& ruleset);
  static FeatureCmdInfoList ruleDocumentation();

};


}
}

#endif // INSIGHT_CAD_BAR_H
