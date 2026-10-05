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

#include "hydrostatics.h"
#include "cadfeatures.h"
#include "datum.h"

#include "AIS_Point.hxx"
// #include "AIS_Drawer.hxx"
#include "Prs3d_TextAspect.hxx"
#include "occtools.h"
#include "base/warningdispatcher.h"

using namespace boost;
using namespace std;

namespace insight
{
namespace cad
{

defineType(Hydrostatics);

addToStaticFunctionTable2(
    PostprocAction, InsertRule, insertrule,
    Hydrostatics, &Hydrostatics::insertrule );

size_t Hydrostatics::calcHash() const
{
  ParameterListHash h;
  h+=*hullvolume_;
  h+=*shipmodel_;
  h+=*psurf_;
  h+=*nsurf_;
  h+=*elong_;
  h+=*evert_;
  return h.getHash();
}
  
Hydrostatics::Hydrostatics
(
  FeaturePtr hullvolume, 
  FeaturePtr shipmodel, 
  VectorPtr psurf, 
  VectorPtr nsurf, 
  VectorPtr elong, 
  VectorPtr evert
)
: hullvolume_(hullvolume), shipmodel_(shipmodel),
  psurf_(psurf), nsurf_(nsurf),
  elong_(elong), evert_(evert)
{}


void Hydrostatics::build()
{
  elat_=arma::cross(nsurf_->value(), elong_->value());
  
  std::shared_ptr<Cutaway> submerged_volume = std::dynamic_pointer_cast<Cutaway,Feature>( Cutaway::create(hullvolume_, psurf_, nsurf_) );
  submerged_volume->checkForBuildDuringAccess();
  V_=submerged_volume->modelVolume();
  m_=shipmodel_->mass();

  FeaturePtr csf = submerged_volume->providedSubshapes().find("CutSurface")->second;
  if (!csf)
    throw insight::Exception("No cut surface present!");

  TopoDS_Shape issh=static_cast<const TopoDS_Shape&>(*csf);
  
  TopExp_Explorer ex(issh, TopAbs_FACE);
  TopoDS_Face f=TopoDS::Face(ex.Current());
  if (ex.More()) { ex.Next();
  if (ex.More()) insight::Warning("cut surface consists of more than a single face! Using only the first one."); }
  
  GProp_GProps props;
  BRepGProp::SurfaceProperties(f, props);
  GProp_PrincipalProps pcp = props.PrincipalProperties();
  I_=arma::zeros(3);
  pcp.Moments(I_(0), I_(1), I_(2));
  double BM = I_.min()/V_;

  G_ = shipmodel_->modelCoG();
  B_ = submerged_volume->modelCoG();
  M_ = B_ + BM*(evert_->value());
}




void Hydrostatics::write(std::ostream& console) const
{
  console<<"displacement V="<<V_<<endl;
  console<<"ship mass m="<<m_<<endl;
  console<<"I="<<I_<<endl;
  console<<"BM="<<arma::norm(M_ - B_, 2)<<endl;
  console<<"G="<<G_<<endl;
  console<<"B="<<B_<<endl;
  console<<"M="<<M_<<endl;
  console<<"GM="<<arma::norm(M_ - G_, 2)<<endl;
}




void Hydrostatics::insertrule(parser::ISCADParser& ruleset)
{
    ruleset.postProcFunctionRules.add
        (
            "Hydrostatics",
            std::make_shared<parser::ISCADParser::PostProcFunctionRule>(
                ( '(' > ruleset.r_identifier > ','
                 > ruleset.r_vectorExpression > ','
                 > ruleset.r_vectorExpression > ','
                 > ruleset.r_vectorExpression > ','
                 > ruleset.r_vectorExpression > ')'
                 > qi::lit("<<")
                 > '(' > ruleset.r_solidmodel_expression > ','
                 > ruleset.r_solidmodel_expression > ')' > ';' ) // (1) hull and (2) ship
                    [ qi::_val = phx::bind(
                     &Hydrostatics::create<FeaturePtr,FeaturePtr,VectorPtr,VectorPtr,VectorPtr,VectorPtr>,
                     qi::_6, qi::_7, qi::_2, qi::_3, qi::_4, qi::_5) ]
                )
            );
}


}
}
