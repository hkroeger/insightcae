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

/**
  * This tool fixes missing cell values after mapping by inserting appropriate values from neighbouring cells
  * Required e.g. for pressure fields in compressible cases.
  */

#include "fvCFD.H"

#include "uniof.h"
#include <memory>
#include <vector>

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Main program:

int main(int argc, char *argv[])
{

    argList::noParallel();
    argList::validArgs.clear();

    argList::validArgs.append("indicator field name (will be corrected as well)");
    argList::validArgs.append("list of fields to correct");
    argList::validOptions.insert("threshold", "indicator field value below which mapping is considered to have failed");

#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"

    word indicatorFieldName(IStringStream(UNIOF_ADDARG(args,0))());
    wordList fieldNames(IStringStream(UNIOF_ADDARG(args,1))());

    scalar threshold = SMALL;
    UNIOF_OPTIONREADIFPRESENT(args, "threshold", threshold);

    std::shared_ptr<volScalarField> indicator(
          new volScalarField
          (
                IOobject
                (
                    indicatorFieldName,
                    runTime.timeName(),
                    mesh,
                    IOobject::MUST_READ,
                    IOobject::AUTO_WRITE
                ),
                mesh
           )
    );

    std::vector<std::shared_ptr<volScalarField> > scalarFields;
    scalarFields.push_back(indicator);

    std::vector<std::shared_ptr<volVectorField> > vectorFields;
    std::vector<std::shared_ptr<volTensorField> > tensorFields;
    std::vector<std::shared_ptr<volSymmTensorField> > symmTensorFields;

    for(const auto& fn: fieldNames)
    {
      IOobject ioo
      (
          fn,
          runTime.timeName(),
          mesh,
          IOobject::MUST_READ,
          IOobject::AUTO_WRITE
      );

      if (UNIOF_HEADEROK(ioo, volScalarField))
        scalarFields.push_back(std::shared_ptr<volScalarField>(new volScalarField(ioo, mesh)));
      else if (UNIOF_HEADEROK(ioo, volVectorField))
        vectorFields.push_back(std::shared_ptr<volVectorField>(new volVectorField(ioo, mesh)));
      else if (UNIOF_HEADEROK(ioo, volTensorField))
        tensorFields.push_back(std::shared_ptr<volTensorField>(new volTensorField(ioo, mesh)));
      else if (UNIOF_HEADEROK(ioo, volSymmTensorField))
        symmTensorFields.push_back(std::shared_ptr<volSymmTensorField>(new volSymmTensorField(ioo, mesh)));
    }



    bool needMoreFix;



    // fix internal volume first
    label iter=0;
    do
    {
      needMoreFix=false;
      label n_fixed=0, n_skipped=0;

      forAll(*indicator, i)
      {

        if ((*indicator)[i] < threshold)
        {
          // go through neighbours; set average of valid neighbours; if no valid neighbours skip cell and retry in next loop
          auto nei_i = mesh.cellCells()[i];
          std::vector<scalar> mean_scalars(scalarFields.size(), 0.0);
          std::vector<vector> mean_vectors(vectorFields.size(), pTraits<vector>::zero);
          std::vector<tensor> mean_tensors(tensorFields.size(), pTraits<tensor>::zero);
          std::vector<symmTensor> mean_symmtensor(symmTensorFields.size(), pTraits<symmTensor>::zero);

          label n_valid=0;
          forAll(nei_i, j)
          {
            label i_n = nei_i[j];
            if ((*indicator)[i_n]>threshold)
            {
              n_valid++;
              for (size_t k=0; k<scalarFields.size(); k++) mean_scalars[k]+=(*scalarFields[k])[i_n];
              for (size_t k=0; k<vectorFields.size(); k++) mean_vectors[k]+=(*vectorFields[k])[i_n];
              for (size_t k=0; k<tensorFields.size(); k++) mean_tensors[k]+=(*tensorFields[k])[i_n];
              for (size_t k=0; k<symmTensorFields.size(); k++) mean_symmtensor[k]+=(*symmTensorFields[k])[i_n];
            }
          }
          if (n_valid>0)
          {
            for (size_t k=0; k<scalarFields.size(); k++) (*scalarFields[k])[i]=mean_scalars[k]/scalar(n_valid);
            for (size_t k=0; k<vectorFields.size(); k++) (*vectorFields[k])[i]=mean_vectors[k]/scalar(n_valid);
            for (size_t k=0; k<tensorFields.size(); k++) (*tensorFields[k])[i]=mean_tensors[k]/scalar(n_valid);
            for (size_t k=0; k<symmTensorFields.size(); k++) (*symmTensorFields[k])[i]=mean_symmtensor[k]/scalar(n_valid);
            n_fixed++;
          }
          else
          {
            needMoreFix=true;
            n_skipped++;
          }
        }

      }

      reduce(n_fixed, sumOp<label>());
      reduce(n_skipped, sumOp<label>());

      Info<<"Iter #"<<iter<<": fixed cells = "<<n_fixed<<", skipped cells = "<<n_skipped<<endl;

      iter++;
    }
    while (needMoreFix);



    // now fix boundary values
    forAll(mesh.boundary(), pi)
    {
      const fvPatch& p = mesh.boundary()[pi];
      forAll(p, j)
      {
        if (indicator->boundaryField()[pi][j] < threshold)
        {
          for (size_t k=0; k<scalarFields.size(); k++)
          {
            volScalarField& field = *scalarFields[k];
            auto& bf = UNIOF_BOUNDARY_NONCONST(field)[pi];
            bf[j]=bf.internalField()[j];
          }
          for (size_t k=0; k<vectorFields.size(); k++)
          {
            volVectorField& field = *vectorFields[k];
            auto& bf = UNIOF_BOUNDARY_NONCONST(field)[pi];
            bf[j]=bf.internalField()[j];
          }
          for (size_t k=0; k<tensorFields.size(); k++)
          {
            volTensorField& field = *tensorFields[k];
            auto& bf = UNIOF_BOUNDARY_NONCONST(field)[pi];
            bf[j]=bf.internalField()[j];
          }
          for (size_t k=0; k<symmTensorFields.size(); k++)
          {
            volSymmTensorField& field = *symmTensorFields[k];
            auto& bf = UNIOF_BOUNDARY_NONCONST(field)[pi];
            bf[j]=bf.internalField()[j];
          }
        }
      }
    }



    for (auto sf: scalarFields) sf->write();
    for (auto vf: vectorFields) vf->write();
    for (auto tf: tensorFields) tf->write();
    for (auto stf: symmTensorFields) stf->write();

    Info << "End\n" << endl;

    return 0;
}


// ************************************************************************* //
