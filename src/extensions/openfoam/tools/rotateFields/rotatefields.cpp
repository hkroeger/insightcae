
#include "fvCFD.H"
#include "ReadFields.H"

#include "transformField.H"
#include "transformGeometricField.H"

#include "uniof.h"

using namespace Foam;
// using namespace Foam::constant::mathematical;


template<class GeoField>
void readAndRotateFields
(
    PtrList<GeoField>& flds,
    const fvMesh& mesh,
    const dimensionedTensor& rotT,
    const IOobjectList& objects
)
{
    ReadFields(mesh, objects, flds);
    for (GeoField& fld : flds)
    {
        Info<< "Transforming " << fld.name() << endl;
        transform(fld, rotT, fld);
    }
}


void rotateFields
(
    const word& regionName,
    const Time& runTime,
    const tensor& rotT
)
{
    Foam::fvMesh mesh
    (
        Foam::IOobject
        (
            regionName,
            runTime.timeName(),
            runTime,
            Foam::IOobject::MUST_READ
        )
    );

    // Need dimensionedTensor for geometric fields
    const dimensionedTensor T(rotT);

    Foam::Info << Foam::endl;
    // Read objects in time directory
    IOobjectList objects(mesh, runTime.timeName());

    // Read vol fields.

    PtrList<volScalarField> vsFlds;
    readAndRotateFields(vsFlds, mesh, T, objects);

    PtrList<volVectorField> vvFlds;
    readAndRotateFields(vvFlds, mesh, T, objects);

    PtrList<volSphericalTensorField> vstFlds;
    readAndRotateFields(vstFlds, mesh, T, objects);

    PtrList<volSymmTensorField> vsymtFlds;
    readAndRotateFields(vsymtFlds, mesh, T, objects);

    PtrList<volTensorField> vtFlds;
    readAndRotateFields(vtFlds, mesh, T, objects);

    // Read surface fields.

    PtrList<surfaceScalarField> ssFlds;
    readAndRotateFields(ssFlds, mesh, T, objects);

    PtrList<surfaceVectorField> svFlds;
    readAndRotateFields(svFlds, mesh, T, objects);

    PtrList<surfaceSphericalTensorField> sstFlds;
    readAndRotateFields(sstFlds, mesh, T, objects);

    PtrList<surfaceSymmTensorField> ssymtFlds;
    readAndRotateFields(ssymtFlds, mesh, T, objects);

    PtrList<surfaceTensorField> stFlds;
    readAndRotateFields(stFlds, mesh, T, objects);

    mesh.write();
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::noParallel();
    UNIOF_ADDOPT
    (   argList,
        "rotate",
        "(vectorA vectorB)",
        "transform in terms of a rotation between <vectorA> and <vectorB> "
        "- eg, '( (1 0 0) (0 0 1) )'"
    );
    UNIOF_ADDOPT
    (   argList,
        "rollPitchYaw",
        "vector",
        "transform in terms of '( roll pitch yaw )' in degrees"
    );
    UNIOF_ADDOPT
    (   argList,
        "yawPitchRoll",
        "vector",
        "transform in terms of '( yaw pitch roll )' in degrees"
    );

    #include "addRegionOption.H"
    #include "setRootCase.H"

    if (args.options().empty())
    {
        FatalErrorIn(args.executable())
            << "No options supplied, please use one or more of "
               "-translate, -rotate or -scale options."
            << exit(FatalError);
    }

    #include "createTime.H"

    word regionName = polyMesh::defaultRegion;
    fileName meshDir = polyMesh::meshSubDir;

    vector v;

    if (UNIOF_OPTIONREADIFPRESENT(args, "region", regionName))
    {
        meshDir = regionName/polyMesh::meshSubDir;
    }

    if (UNIOF_OPTIONFOUND(args, "rotate"))
    {
        Pair<vector> n1n2
        (
            UNIOF_OPTIONLOOKUP(args, "rotate")()
        );
        n1n2[0] /= mag(n1n2[0]);
        n1n2[1] /= mag(n1n2[1]);

        tensor T = rotationTensor(n1n2[0], n1n2[1]);

        rotateFields(regionName, runTime, T);
    }
    else if (UNIOF_OPTIONREADIFPRESENT(args, "rollPitchYaw", v))
    {
        Info<< "Rotating points by" << nl
            << "    roll  " << v.x() << nl
            << "    pitch " << v.y() << nl
            << "    yaw   " << v.z() << nl;

        // Convert to radians
        v *= M_PI/180.0;

#if (OF_VERSION>=040000 && OF_VERSION<060500) //defined(OFplus)||defined(OFdev)||defined(OFesi1806)
        quaternion R(quaternion::rotationSequence::XYZ, v);
#elif (OF_VERSION>=060500)
        quaternion R(quaternion::eulerOrder::XYZ, v);
#else
        quaternion R(v.x(), v.y(), v.z());
#endif

        rotateFields(regionName, runTime, R.R());
    }
    else if (UNIOF_OPTIONREADIFPRESENT(args, "yawPitchRoll", v))
    {
        Info<< "Rotating points by" << nl
            << "    yaw   " << v.x() << nl
            << "    pitch " << v.y() << nl
            << "    roll  " << v.z() << nl;


        // Convert to radians
        v *= M_PI/180.0;

        scalar yaw = v.x();
        scalar pitch = v.y();
        scalar roll = v.z();

        quaternion R(vector(0, 0, 1), yaw);
        R *= quaternion(vector(0, 1, 0), pitch);
        R *= quaternion(vector(1, 0, 0), roll);

        rotateFields(regionName, runTime, R.R());
    }

    Info<< nl << "End" << nl << endl;

    return 0;
}
