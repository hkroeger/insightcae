
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
   
    
    wordReList patchRegex(IStringStream( UNIOF_ADDARG(args, 0) )());

    labelHashSet ids=mesh.boundaryMesh().patchSet(patchRegex);
    
    forAllConstIter(labelHashSet, ids, it)
    {
        Info << "matched patch \"" << mesh.boundary()[it.key()].name() << "\" (id="<<it.key()<<")"<<endl;
    }
    
    Info<< endl;

    return(0);
}


// ************************************************************************* //
