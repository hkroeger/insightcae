
#include "fvCFD.H"
#include "wordReList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
   argList::validArgs.append("wordReList");
   
#   include "setRootCase.H"
#   include "createTime.H"
#   include "createMesh.H"
   
    
    wordReList patchRegex(IStringStream(
#if defined(OFdev)||defined(OFplus)
      args.arg(1)
#else
      args.additionalArgs()[0]
#endif
    )());

    labelHashSet ids=mesh.boundaryMesh().patchSet(patchRegex);
    
    forAllConstIter(labelHashSet, ids, it)
    {
        Info << "matched patch \"" << mesh.boundary()[it.key()].name() << "\" (id="<<it.key()<<")"<<endl;
    }
    
    Info<< endl;

    return(0);
}


// ************************************************************************* //
