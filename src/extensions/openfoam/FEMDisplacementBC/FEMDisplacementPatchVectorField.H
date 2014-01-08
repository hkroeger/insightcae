/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright held by original author
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Class
    FEMDisplacementPatchVectorField

Description

SourceFiles
    FEMDisplacementPatchVectorField.C

\*---------------------------------------------------------------------------*/

#ifndef FEMDisplacementPatchVectorField_H
#define FEMDisplacementPatchVectorField_H

#include "fvCFD.H"
#include "pointPatchFields.H"
#include "fixedValuePointPatchFields.H"
#include "surfaceMesh.H"
#include "Switch.H"
#include "septernion.H"
#include "OFstream.H"
#include "Tuple2.H"
#include "interpolateXY.H"
#include "fsitransform.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class relaxProfile
{
  scalarField x_, y_;
public:
  relaxProfile(scalar r)
  : x_(1, 0.0),
    y_(1, r)
  {
  }

  relaxProfile(Istream& is)
  {
    List<Tuple2<scalar, scalar> > r(is);
    x_.setSize(r.size());
    y_.setSize(r.size());
    forAll(r, i)
      {
	x_[i]=r[i].first();
	y_[i]=r[i].second();
      }
  }

  Ostream& write(Ostream& os) const
  {
    os << token::BEGIN_LIST << token::SPACE;
    forAll(x_, i)
      {
	os << token::BEGIN_LIST
	   << x_[i] << token::SPACE << y_[i]
	   << token::END_LIST 
	   << token::SPACE;
      }
    os << token::END_LIST;
    return os;
  }

  inline scalar operator()(scalar x) const
  {
    return interpolateXY(x, x_, y_);
  }
};

/*---------------------------------------------------------------------------*\
          Class FEMDisplacementPatchVectorField Declaration
\*---------------------------------------------------------------------------*/

template
<
  template<class> class PatchField,
    class Mesh,
    class PointPatch,
  template<class>   class MatrixType
>
class FEMDisplacementPatchVectorField
:
    public FixedValuePointPatchField
    <
        PatchField,
        Mesh,
        PointPatch,
        MatrixType,
        vector
    >
{

private:

    // Private data

  fileName FEMCaseDir_;
  //scalar lengthScale_;
  //septernion transform_;
  autoPtr<FSITransform> transform_;
  scalar pressureScale_;
  scalar minPressure_;
  label nSmoothIter_;
  Switch wallCollisionCheck_;

  //scalar relax_;
  relaxProfile relax_;
  scalar lateralRelax_;
  Switch prescribeMotionVelocity_;

  pointField initialPosition_;
  scalarField oldPressure_;
  label curTimeIndex_;
  autoPtr<OFstream> filePtr_;
  
  scalar omega_;
  autoPtr<scalarField> rm_;

  inline scalar relax() const
  {
    return relax_(this->db().time().value());
  }
  
  inline label pressurePatchSize() const
  {
    const polyMesh& mesh = this->patch().boundaryMesh().mesh()();
    return mesh.boundaryMesh()[this->patch().index()].size();
  }

  inline const fvPatchScalarField& pressure() const
  {
    const polyMesh& mesh = this->patch().boundaryMesh().mesh()();
    return mesh.lookupObject<volScalarField>("p").boundaryField()[this->patch().index()];
  }

    // Private Member Functions

        //- Disallow default bitwise assignment
        void operator=(const FEMDisplacementPatchVectorField&);

public:

    //- Runtime type information
    TypeName("FEMDisplacement");


    // Constructors

        //- Construct from patch and internal field
        FEMDisplacementPatchVectorField
        (
            const PointPatch&,
            const DimensionedField<vector, Mesh>&
        );

        //- Construct from patch, internal field and dictionary
        FEMDisplacementPatchVectorField
        (
            const PointPatch&,
            const DimensionedField<vector, Mesh>&,
            const dictionary&
        );

        //- Construct by mapping given patchField<vector> onto a new patch
        FEMDisplacementPatchVectorField
        (
            const FEMDisplacementPatchVectorField&,
            const PointPatch&,
            const DimensionedField<vector, Mesh>&,
            const PointPatchFieldMapper&
        );

        //- Construct as copy
        FEMDisplacementPatchVectorField
        (
            const FEMDisplacementPatchVectorField&
        );

        //- Construct and return a clone
        virtual 
	//autoPtr<PointPatchField<PatchField, Mesh, PointPatch, MatrixType, vector> > 
	autoPtr<PatchField<vector> > 
	clone() const
        {
            return autoPtr<PatchField<vector> >
            (
                new FEMDisplacementPatchVectorField
                (
                    *this
                )
            );
        }

        //- Construct as copy setting internal field reference
        FEMDisplacementPatchVectorField
        (
            const FEMDisplacementPatchVectorField&,
            const DimensionedField<vector, Mesh>&
        );

        //- Construct and return a clone setting internal field reference
        //virtual autoPtr<PointPatchField<PatchField, Mesh, Patch, MatrixType, vector> > clone
        virtual autoPtr<PatchField<vector> > clone
        (
            const DimensionedField<vector, Mesh>& iF
        ) const
        {
            return autoPtr<PatchField<vector> >
            (
                new FEMDisplacementPatchVectorField
                (
                    *this,
                    iF
                )
            );
        }

    // Member Functions

        //- Update the coefficients associated with the patch field
        //virtual void updateCoeffs();

        void initEvaluate(const Pstream::commsTypes commsType);

        //- Write
        virtual void write(Ostream&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "FEMDisplacementPatchVectorFieldI.H"

#endif

// ************************************************************************* //
