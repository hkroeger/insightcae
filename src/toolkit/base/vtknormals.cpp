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


#include "vtknormals.h"

#include "base/exception.h"

#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"

namespace insight
{

bool checkNormalsOrientation(vtkPolyData* vpm, const arma::mat& pFar, bool modifyNormalsFields)
{
  auto* pn = vpm->GetPointData()->GetArray("Normals");
  insight::assertion(pn!=nullptr, "the input data does not contain a field \"Normals\"!");

  bool toBeInverted=false;

  if (pn->GetNumberOfTuples()>0) // skip empty mesh
  {
    // calc mean normals and CoG
    arma::mat meanN, CoG;
    meanN=CoG=arma::zeros(3);

    insight::assertion(pn->GetNumberOfTuples()==vpm->GetNumberOfPoints(),
                       "internal error: number of point normals not equal to number of points!");

    for (vtkIdType i=0; i<pn->GetNumberOfTuples(); ++i)
    {
      arma::mat n(pn->GetTuple3(i), 3, 1);
      meanN += n;
      arma::mat p(vpm->GetPoint(i), 3, 1);
      CoG += p;
    }
    meanN /= double(pn->GetNumberOfTuples());
    CoG /= double(vpm->GetNumberOfPoints());

    // check side of pFar
    arma::mat dir = pFar-CoG;
    double ldir=arma::norm(dir,2);
    insight::assertion(ldir>1e-10, "pFar must not be identical with the CoG of the model points!");
    dir/=ldir;

    double Lmean=arma::norm(meanN, 2);
    insight::assertion(Lmean>1e-10, "mean normal vector is zero!");
    meanN /= Lmean;

    toBeInverted = arma::dot(dir, meanN)<0;

    if (!modifyNormalsFields)
      return toBeInverted;

    if (toBeInverted)
    {
      // invert point normals (and, if present also cell normals)

      std::vector<vtkDataArray*> normalArraysToBeInverted = {pn};
      if (auto* cn = vpm->GetCellData()->GetArray("Normals"))
      {
        normalArraysToBeInverted.push_back(cn);
      }

      for (vtkDataArray* narray: normalArraysToBeInverted)
      {
        for (vtkIdType i=0; i<narray->GetNumberOfTuples(); ++i)
        {
          double n[3];
          narray->GetTuple(i, n);
          for (int i=0; i<3; i++) n[i]*=-1.;
          narray->SetTuple(i, n);
        }
      }
    }
  }

  return toBeInverted;
}

}
