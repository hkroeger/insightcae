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

#include "fv.H"
#include "localLimitedSnGrad.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "localMax.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace fv
{

// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

template<class Type>
localLimitedSnGrad<Type>::~localLimitedSnGrad()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
tmp<GeometricField<Type, fvsPatchField, surfaceMesh> >
localLimitedSnGrad<Type>::correction
(
    const GeometricField<Type, fvPatchField, volMesh>& vf
) const
{
    const GeometricField<Type, fvsPatchField, surfaceMesh> corr
    (
        correctedScheme_().correction(vf)
    );
    
    const surfaceScalarField& limitCoeffField =
      this->mesh().objectRegistry::template 
      lookupObject<const surfaceScalarField>(limitCoeffFieldName_);

    surfaceScalarField limitCoeff = (1.0 - min(1.0, max(0.0, limitCoeffField*limitCoeffMult_)));
    
    const surfaceScalarField limiter
    (
        min
        (
            limitCoeff
           *mag(snGradScheme<Type>::snGrad(vf, deltaCoeffs(vf), "SndGrad"))
           /(
                (1.0 - limitCoeff)*mag(corr)
              + dimensionedScalar("small", corr.dimensions(), SMALL)
            ),
            dimensionedScalar("one", dimless, 1.0)
        )
    );

    if (fv::debug)
    {
        Info<< "localLimitedSnGrad :: limiter min: " << min(limiter.internalField())
            << " max: "<< max(limiter.internalField())
            << " avg: " << average(limiter.internalField()) << endl;
    }

    return limiter*corr;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
