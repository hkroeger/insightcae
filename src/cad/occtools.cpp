/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  hannes <email>
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

#include "occtools.h"
#include "base/linearalgebra.h"
#include "base/units.h"


namespace insight {
namespace cad {


gp_Trsf OFtransformToOCC(const insight::SpatialTransformation& trsf)
{
    return OFtransformToOCC(trsf.translate(), trsf.rollPitchYaw(), trsf.scale());
}

gp_Trsf OFtransformToOCC(const arma::mat &translate, const arma::mat &rollPitchYaw, double scale)
{
  gp_Trsf tr; tr.SetTranslation(to_Vec(translate));
  gp_Trsf rx; rx.SetRotation(gp::OX(), rollPitchYaw(0)*SI::deg);
  gp_Trsf ry; ry.SetRotation(gp::OY(), rollPitchYaw(1)*SI::deg);
  gp_Trsf rz; rz.SetRotation(gp::OZ(), rollPitchYaw(2)*SI::deg);
  gp_Trsf sc; sc.SetScaleFactor(scale);
  return sc*rz*ry*rx*tr;
}

OCCtransformToOF::OCCtransformToOF(const gp_Trsf &t)
{
  scale_ = t.ScaleFactor();

  arma::mat R = arma::zeros(3,3);
  for (int i=0;i<3;i++)
    for (int j=0;j<3;j++)
      R(i,j)=t.Value(i+1,j+1);
  R*=1./scale_;
//  std::cout<<R<<std::endl;

//  rollPitchYaw_ = rotationMatrixToRollPitchYaw(R);
  R_ = R;
//  std::cout<<Vector(t.TranslationPart())<<std::endl;
  translate_ = (1./scale_)*inv(R)*Vector(t.TranslationPart());
}



//OCCtransformToVTK::OCCtransformToVTK(const gp_Trsf& t)
//  : OCCtransformToOF(t)
//{}

//vtkSmartPointer<vtkTransform> OCCtransformToVTK::operator()() const
//{
//  auto t = vtkSmartPointer<vtkTransform>::New();
//  t->Translate( translate()(0), translate()(1), translate()(2) );
//  t->RotateX( rollPitchYaw()(0) );
//  t->RotateY( rollPitchYaw()(1) );
//  t->RotateZ( rollPitchYaw()(2) );
//  t->Scale( scale(), scale(), scale() );
//  return t;
//}


}
}
