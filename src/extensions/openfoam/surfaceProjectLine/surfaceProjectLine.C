
#include "argList.H"
#include "OFstream.H"
#include "IFstream.H"
#include "boundBox.H"
#include "transformField.H"
#include "Pair.H"
#include "quaternion.H"
#include "mathematicalConstants.H"
#include "triSurfaceMesh.H"
#include "treeDataTriSurface.H"
#include "indexedOctree.H"

#include "MeshedSurfaces.H"
#include "PackedBoolList.H"
#include "uniof.h"

using namespace Foam;


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::noParallel();
    
    argList::validArgs.append("surfaceFile");
    argList::validArgs.append("start");
    argList::validArgs.append("end");
    argList::validArgs.append("npts");
    argList::validArgs.append("projdir");

    argList args(argc, argv);

    fileName surfFileName = UNIOF_ADDARG(args, 0); //args.additionalArgs()[0];
    point start(IStringStream(UNIOF_ADDARG(args, 1))()); //args.additionalArgs()[1])());
    point end(IStringStream(UNIOF_ADDARG(args, 2))()); //args.additionalArgs()[2])());
    label npts(readLabel(IStringStream(UNIOF_ADDARG(args, 3))())); //args.additionalArgs()[3])()));
    vector projdir(IStringStream(UNIOF_ADDARG(args, 4))()); //args.additionalArgs()[4])());
    projdir/=mag(projdir);
    vector axis=end-start;
    axis/=mag(axis);

    Info<< "Reading surf from " << surfFileName << endl;

    triSurface surf1(surfFileName);
    
            // Calculate bb without constructing local point numbering.
        treeBoundBox bb;
        label nPoints;
	
	PackedBoolList pointIsUsed(surf1.points().size());

    nPoints = 0;
    bb = treeBoundBox::invertedBox;

    forAll(surf1, triI)
    {
        const labelledTri& f = surf1[triI];

        forAll(f, fp)
        {
            label pointI = f[fp];
            if (pointIsUsed.set(pointI, 1u))
            {
                bb.min() = ::Foam::min(bb.min(), surf1.points()[pointI]);
                bb.max() = ::Foam::max(bb.max(), surf1.points()[pointI]);
                nPoints++;
            }
        }
    }


        if (nPoints != surf1.points().size())
        {   
            WarningIn("triSurfaceMesh::tree() const")
                << "Surface does not have compact point numbering."
                << " Of " << surf1.points().size() << " only " << nPoints
                << " are used. This might give problems in some routines."
                << endl;
        }
        
        // Random number generator. Bit dodgy since not exactly random ;-)
        Random rndGen(65431);

        // Slightly extended bb. Slightly off-centred just so on symmetric
        // geometry there are less face/edge aligned items.
        bb = bb.extend(rndGen, 1E-4);
        bb.min() -= point(ROOTVSMALL, ROOTVSMALL, ROOTVSMALL);
        bb.max() += point(ROOTVSMALL, ROOTVSMALL, ROOTVSMALL);

//         scalar oldTol = indexedOctree<treeDataTriSurface>::perturbTol();
//         indexedOctree<treeDataTriSurface>::perturbTol() = tolerance_;

        indexedOctree<treeDataTriSurface> tree
	(
	    treeDataTriSurface(
#if !(defined(OF22eng)||defined(Fx40))
            true, 
#endif
            surf1
#if !defined(Fx40)
            , 
            1e-6
#endif
        ),
	    bb,
	    10, //maxTreeDepth_,  // maxLevel
	    10,             // leafsize
	    3.0             // duplicity
        );

    scalar maxR=0.0;
    {
      vectorField r=surf1.points()-start;
      r-=axis*(r&axis);
      maxR=max(mag(r));
    }
    
//     pointField starts(npts), ends(npts);
//     List< pointIndexHit > hits(npts);
    
    Info<<"curve = (";
    for (int i=0; i<npts; i++)
    {
      point p = start + (end-start)*double(i)/double(npts-1);
      pointIndexHit ph = tree.findLine(p, p+projdir*maxR);
      if (ph.hit())
      {
	point hp=ph.hitPoint();
	double x=(hp)&axis;
	vector r=hp-start; r-=axis*(r&axis);
	if (i>0) Info<<",";
	Info<<x<<" "<<mag(r);
      }
    }
    Info<<")"<<endl;

    Info<< "End\n" << endl;

    return 0;
}
