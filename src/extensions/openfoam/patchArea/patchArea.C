
#include "fvCFD.H"
#include "wordReList.H"

#include "uniof.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
   argList::validArgs.append("wordReList");

#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"


    wordRe patchRegex(IStringStream( UNIOF_ADDARG(args,0) )());

    wordReList patchSelector(1);
    patchSelector[0]=patchRegex;
    labelHashSet ids=mesh.boundaryMesh().patchSet(patchSelector);

    scalar A_tot=0;
    vector avg_normal_tot=vector::zero;
    point ctr_tot=point::zero;
    forAllConstIter(labelHashSet, ids, it)
    {
      const fvPatch& p = mesh.boundary()[it.key()];
      scalar A=gSum(p.magSf());
      vector avg_normal=gSum(p.nf()*p.magSf())/A;
      point ctr=gSum(p.Cf()*p.magSf())/A;
      Info << "matched \"" << p.name() << "\" id="<<it.key()<<" A="<<A<<" avg_normal="<<avg_normal<<" ctr="<<ctr<<endl;
      A_tot+=A;
      avg_normal_tot+=avg_normal*A;
      ctr_tot+=ctr*A;
    }
    avg_normal_tot/=A_tot;
    ctr_tot/=A_tot;

    Info<<"TOTAL A="<<A_tot<<" normal="<<avg_normal_tot<<" ctr="<<ctr_tot<<endl;

    return(0);
}


// ************************************************************************* //
