/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | foam-extend: Open Source CFD
   \\    /   O peration     |
    \\  /    A nd           | For copyright notice see file Copyright
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of foam-extend.

    foam-extend is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    foam-extend is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with foam-extend.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "leastSquares2Vectors.H"
#include "surfaceFields.H"
#include "volFields.H"
#include "mapPolyMesh.H"
#include "uniof.h"

#if defined(OFplus)
#define UNALLOCLABELLIST labelList
#else
#define UNALLOCLABELLIST unallocLabelList
#endif

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(leastSquares2Vectors, 0);
}


// * * * * * * * * * * * * * * * * Constructors * * * * * * * * * * * * * * //

Foam::leastSquares2Vectors::leastSquares2Vectors(const fvMesh& mesh)
:
    MeshObject<fvMesh, 
#if !defined(OF16ext) && !defined(OF21x)
    MoveableMeshObject, 
#endif
    leastSquares2Vectors>(mesh),
    pVectorsPtr_(NULL),
    nVectorsPtr_(NULL)
{}


// * * * * * * * * * * * * * * * * Destructor * * * * * * * * * * * * * * * //

Foam::leastSquares2Vectors::~leastSquares2Vectors()
{
    deleteDemandDrivenData(pVectorsPtr_);
    deleteDemandDrivenData(nVectorsPtr_);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

#if !defined(OF16ext)

namespace Foam 
{
  
// Return eigenvalues in ascending order of absolute values
vector eigenValues2(const symmTensor& t)
{
    scalar i = 0;
    scalar ii = 0;
    scalar iii = 0;

    if
    (
        (
            mag(t.xy()) + mag(t.xz()) + mag(t.xy())
          + mag(t.yz()) + mag(t.xz()) + mag(t.yz())
        )
      < SMALL
    )
    {
        // diagonal matrix
        i = t.xx();
        ii = t.yy();
        iii = t.zz();
    }
    else
    {
        scalar a = -t.xx() - t.yy() - t.zz();

        scalar b = t.xx()*t.yy() + t.xx()*t.zz() + t.yy()*t.zz()
            - t.xy()*t.xy() - t.xz()*t.xz() - t.yz()*t.yz();

        scalar c = - t.xx()*t.yy()*t.zz() - t.xy()*t.yz()*t.xz()
            - t.xz()*t.xy()*t.yz() + t.xz()*t.yy()*t.xz()
            + t.xy()*t.xy()*t.zz() + t.xx()*t.yz()*t.yz();

        // If there is a zero root
        if (mag(c) < 1.0e-100)
        {
            scalar disc = sqr(a) - 4*b;

            if (disc >= -SMALL)
            {
                scalar q = -0.5*sqrt(max(0.0, disc));

                i = 0;
                ii = -0.5*a + q;
                iii = -0.5*a - q;
            }
            else
            {
                FatalErrorIn("eigenValues2(const tensor&)")
                    << "zero and complex eigenvalues in tensor: " << t
                    << abort(FatalError);
            }
        }
        else
        {
            scalar Q = (a*a - 3*b)/9;
            scalar R = (2*a*a*a - 9*a*b + 27*c)/54;

            scalar R2 = sqr(R);
            scalar Q3 = pow3(Q);

            // Three different real roots
            if (R2 < Q3)
            {
                scalar sqrtQ = sqrt(Q);
                scalar theta = acos(R/(Q*sqrtQ));

                scalar m2SqrtQ = -2*sqrtQ;
                scalar aBy3 = a/3;

                i = m2SqrtQ*cos(theta/3) - aBy3;
                ii =
                    m2SqrtQ
                   *cos((theta + constant::mathematical::twoPi)/3)
                  - aBy3;
                iii =
                    m2SqrtQ
                   *cos((theta - constant::mathematical::twoPi)/3)
                 - aBy3;
            }
            else
            {
                scalar A = cbrt(R + sqrt(R2 - Q3));

                // Three equal real roots
                if (A < SMALL)
                {
                    scalar root = -a/3;
                    return vector(root, root, root);
                }
                else
                {
                    // Complex roots
                    WarningIn("eigenValues2(const symmTensor&)")
                        << "complex eigenvalues detected for symmTensor: " << t
                        << endl;

                    return vector::zero;
                }
            }
        }
    }


    // Sort the eigenvalues into ascending order
    if (i > ii)
    {
        Swap(i, ii);
    }

    if (ii > iii)
    {
        Swap(ii, iii);
    }

    if (i > ii)
    {
        Swap(i, ii);
    }

    return vector(i, ii, iii);
}

vector eigenVector2(const symmTensor& t, const scalar lambda)
{
    //if (mag(lambda) < SMALL)
    //{
    //    return vector::zero;
    //}

    // Construct the matrix for the eigenvector problem
    symmTensor A(t - lambda*I);

    // Calculate the sub-determinants of the 3 components
    scalar sd0 = A.yy()*A.zz() - A.yz()*A.yz();
    scalar sd1 = A.xx()*A.zz() - A.xz()*A.xz();
    scalar sd2 = A.xx()*A.yy() - A.xy()*A.xy();

    scalar magSd0 = mag(sd0);
    scalar magSd1 = mag(sd1);
    scalar magSd2 = mag(sd2);

    // Evaluate the eigenvector using the largest sub-determinant
    if (magSd0 > magSd1 && magSd0 > magSd2 && magSd0 > SMALL)
    {
        vector ev
        (
            1,
            (A.yz()*A.xz() - A.zz()*A.xy())/sd0,
            (A.yz()*A.xy() - A.yy()*A.xz())/sd0
        );
        ev /= mag(ev);

        return ev;
    }
    else if (magSd1 > magSd2 && magSd1 > SMALL)
    {
        vector ev
        (
            (A.xz()*A.yz() - A.zz()*A.xy())/sd1,
            1,
            (A.xz()*A.xy() - A.xx()*A.yz())/sd1
        );
        ev /= mag(ev);

        return ev;
    }
    else if (magSd2 > SMALL)
    {
        vector ev
        (
            (A.xy()*A.yz() - A.yy()*A.xz())/sd2,
            (A.xy()*A.xz() - A.xx()*A.yz())/sd2,
            1
        );
        ev /= mag(ev);

        return ev;
    }
    else
    {
        return vector::zero;
    }
}

tensor eigenVectors2(const symmTensor& t)
{
    vector evals(eigenValues2(t));

    tensor evs;

    // Test for null eigen values to return a not null eigen vector.
    // Jovani Favero, 18/Nov/2009
    if (mag(evals.x()) < SMALL)
    {
        evs.xx() = 0;
        evs.xy() = 0;
        evs.xz() = 1;
    }
    else
    {
        vector evec(eigenVector2(t, evals.x()));
        evs.xx() = evec.x();
        evs.xy() = evec.y();
        evs.xz() = evec.z();
    }

    if (mag(evals.y()) < SMALL)
    {
        evs.yx() = 0;
        evs.yy() = 1;
        evs.yz() = 0;
    }
    else
    {
        vector evec(eigenVector2(t, evals.y()));
        evs.yx() = evec.x();
        evs.yy() = evec.y();
        evs.yz() = evec.z();
    }

    if (mag(evals.z()) < SMALL)
    {
        evs.zx() = 1;
        evs.zy() = 0;
        evs.zz() = 0;
    }
    else
    {
        vector evec(eigenVector2(t, evals.z()));
        evs.zx() = evec.x();
        evs.zy() = evec.y();
        evs.zz() = evec.z();
    }

    return evs;
}
  
symmTensor hinv(const symmTensor& t)
{
    static const scalar large = 1e10;
    static const scalar small = 1e-10;

    scalar dett(det(t));
    if (dett > small)
    {
        return inv(t, dett);
    }
    else
    {
        //check to filter out small tensors
        //by scaling with tensor magnitude
        scalar tmagPow3 = pow(magSqr(t), 1.5);
        
        if (dett > tmagPow3*small)
        {
            return inv(t, dett);
        }
        else //degenerate
        {
            vector eig = eigenValues2(t);
            tensor eigVecs = eigenVectors2(t);

            symmTensor zeroInv(symmTensor::zero);

            // Test if all eigen values are zero,
            // If this happens then eig.z() = SMALL
            // and hinv(t) return a zero tensor.
            // Jovani Favero, 18/Nov/2009
            if (mag(eig.z()) == large*mag(eig.z()))
            {
                zeroInv 
                    += sqr(vector(eigVecs.zx(), eigVecs.zy(), eigVecs.zz()));
                eig.z() += SMALL;
            }

            if (mag(eig.z()) > large*mag(eig.x()))
            {
                zeroInv 
                    += sqr(vector(eigVecs.xx(), eigVecs.xy(), eigVecs.xz()));
            }

            if (mag(eig.z()) > large*mag(eig.y()))
            {
                // singular direction 1
                zeroInv 
                    += sqr(vector(eigVecs.yx(), eigVecs.yy(), eigVecs.yz()));
            }

            return inv(t + zeroInv) - zeroInv;
        }
    }
}

tmp<symmTensorField> hinv(const symmTensorField& dd)
{
  tmp<symmTensorField> res(new symmTensorField(dd.size()));
  forAll(dd, i) 
  {
    UNIOF_TMP_NONCONST(res)[i]=hinv(dd[i]);
  }
  return res;
}

}
#endif

void Foam::leastSquares2Vectors::makeLeastSquaresVectors() const
{
    if (debug)
    {
        Info<< "leastSquares2Vectors::makeLeastSquaresVectors() :"
            << "Constructing least square gradient vectors"
            << endl;
    }

    pVectorsPtr_ = new surfaceVectorField
    (
        IOobject
        (
            "LeastSquaresP",
            mesh().pointsInstance(),
            mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            false
        ),
        mesh(),
        dimensionedVector("zero", dimless/dimLength, vector::zero)
    );
    surfaceVectorField& lsP = *pVectorsPtr_;

    nVectorsPtr_ = new surfaceVectorField
    (
        IOobject
        (
            "LeastSquaresN",
            mesh().pointsInstance(),
            mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            false
        ),
        mesh(),
        dimensionedVector("zero", dimless/dimLength, vector::zero)
    );
    surfaceVectorField& lsN = *nVectorsPtr_;

    // Set local references to mesh data
    const UNALLOCLABELLIST& owner = mesh().owner();
    const UNALLOCLABELLIST& neighbour = mesh().neighbour();

    const volVectorField& C = mesh().C();
    const surfaceScalarField& w = mesh().weights();
//     const surfaceScalarField& magSf = mesh().magSf();


    // Set up temporary storage for the dd tensor (before inversion)
    symmTensorField dd(mesh().nCells(), symmTensor::zero);

    forAll(owner, facei)
    {
        label own = owner[facei];
        label nei = neighbour[facei];

        vector d = C[nei] - C[own];
        symmTensor wdd = (1.0/magSqr(d))*sqr(d);

        dd[own] += wdd;
        dd[nei] += wdd;
    }


    forAll(lsP.boundaryField(), patchi)
    {
        const fvsPatchScalarField& pw = w.boundaryField()[patchi];
        // Note: least squares in 1.4.1 and other OpenCFD versions
        // are wrong because of incorrect weighting.  HJ, 23/Oct/2008
//         const fvsPatchScalarField& pMagSf = magSf.boundaryField()[patchi];

        const fvPatch& p = pw.patch();
        const UNALLOCLABELLIST& faceCells = p.patch().faceCells();

        // Build the d-vectors

        // Original version: closest distance to boundary
        vectorField pd =
            mesh().Sf().boundaryField()[patchi]
           /(
               mesh().magSf().boundaryField()[patchi]
              *mesh().deltaCoeffs().boundaryField()[patchi]
           );
#ifdef OF16ext
        if (!mesh().orthogonal())
#endif
        {
            pd -= 
#if defined(OF16ext)
	      mesh().correctionVectors().boundaryField()[patchi]
#else
	      mesh().nonOrthCorrectionVectors().boundaryField()[patchi]
#endif
                /mesh().deltaCoeffs().boundaryField()[patchi];
        }

        // Better version of d-vectors: Zeljko Tukovic, 25/Apr/2010
        // Experimental: review fixed gradient condition.  HJ, 30/Sep/2010
//         vectorField pd = p.delta();

        if (p.coupled())
        {
            forAll(pd, patchFacei)
            {
                const vector& d = pd[patchFacei];

                dd[faceCells[patchFacei]] += (1.0/magSqr(d))*sqr(d);
            }
        }
        else
        {
            forAll(pd, patchFacei)
            {
                const vector& d = pd[patchFacei];

                dd[faceCells[patchFacei]] += (1.0/magSqr(d))*sqr(d);
            }
        }
    }


    // Invert the dd tensor
//     symmTensorField invDd = inv(dd);
    // Fix: householder inverse.  HJ, 3/Nov/2009
    symmTensorField invDd = hinv(dd);


    // Revisit all faces and calculate the lsP and lsN vectors
    forAll(owner, facei)
    {
        label own = owner[facei];
        label nei = neighbour[facei];

        vector d = C[nei] - C[own];
        scalar magSfByMagSqrd = 1.0/magSqr(d);

        lsP[facei] = magSfByMagSqrd*(invDd[own] & d);
        lsN[facei] = -magSfByMagSqrd*(invDd[nei] & d);
    }

    forAll(lsP.boundaryField(), patchi)
    {
        fvsPatchVectorField& patchLsP = UNIOF_BOUNDARY_NONCONST(lsP)[patchi];

        const fvsPatchScalarField& pw = w.boundaryField()[patchi];
        // Note: least squares in 1.4.1 and other OpenCFD versions
        // are wrong because of incorrect weighting.  HJ, 23/Oct/2008
//         const fvsPatchScalarField& pMagSf = magSf.boundaryField()[patchi];

        const fvPatch& p = pw.patch();
        const UNALLOCLABELLIST& faceCells = p.faceCells();

        // Build the d-vectors
        // Better version of d-vectors: Zeljko Tukovic, 25/Apr/2010
        vectorField pd = p.delta();

        if (p.coupled())
        {
            forAll(pd, patchFacei)
            {
                const vector& d = pd[patchFacei];

                patchLsP[patchFacei] =
                    (1.0/magSqr(d))
                   *(invDd[faceCells[patchFacei]] & d);
            }
        }
        else
        {
            forAll(pd, patchFacei)
            {
                const vector& d = pd[patchFacei];

                patchLsP[patchFacei] =
                    (1.0/magSqr(d))
                   *(invDd[faceCells[patchFacei]] & d);
            }
        }
    }

    if (debug)
    {
        Info<< "leastSquares2Vectors::makeLeastSquaresVectors() :"
            << "Finished constructing least square gradient vectors"
            << endl;
    }
}


const Foam::surfaceVectorField& Foam::leastSquares2Vectors::pVectors() const
{
    if (!pVectorsPtr_)
    {
        makeLeastSquaresVectors();
    }

    return *pVectorsPtr_;
}


const Foam::surfaceVectorField& Foam::leastSquares2Vectors::nVectors() const
{
    if (!nVectorsPtr_)
    {
        makeLeastSquaresVectors();
    }

    return *nVectorsPtr_;
}


bool Foam::leastSquares2Vectors::movePoints()
#if defined(OF16ext) || defined(OF21x)
	const
#endif
{
    if (debug)
    {
        InfoIn("bool leastSquares2Vectors::movePoints() const")
            << "Clearing least square data" << endl;
    }

    deleteDemandDrivenData(pVectorsPtr_);
    deleteDemandDrivenData(nVectorsPtr_);

    return true;
}

bool Foam::leastSquares2Vectors::updateMesh(const mapPolyMesh& mpm) const
{
    if (debug)
    {
        InfoIn("bool leastSquares2Vectors::updateMesh(const mapPolyMesh&) const")
            << "Clearing least square data" << endl;
    }

    deleteDemandDrivenData(pVectorsPtr_);
    deleteDemandDrivenData(nVectorsPtr_);

    return true;
}

// ************************************************************************* //
