#include "uniof_functionobject.h"
#include "dictionary.H"




namespace Foam {




UniFunctionObject::UniFunctionObject(
    const word& name, const dictionary& dict)
    : functionObject(name)
{
    initialDict_.reset(new dictionary(dict));
}


bool UniFunctionObject::read(const dictionary& d)
{
#if !defined(OF_FORK_extend)
    return functionObject::read(d);
#else
    return true;
#endif
}

#if (defined(OF_FORK_extend)) || (!defined(OF_FORK_extend) && OF_VERSION<040000)
bool UniFunctionObject::start()
{
    return true;
}
#endif




#if (defined(OF_FORK_extend) && OF_VERSION>=010604) || (!defined(OF_FORK_extend) && OF_VERSION<040000)
bool UniFunctionObject::execute(bool forceWrite)
{
    bool ok=true;
    if (initialDict_.valid())
    {
        ok = ok && read(initialDict_());
        initialDict_.reset();
    }
    if (ok) ok = ok && perform();
    if (ok && forceWrite) ok = ok && write();
    return ok;
}
#else
bool UniFunctionObject::execute()
{
    bool ok=true;
    if (initialDict_.valid())
    {
        ok = ok && read(initialDict_());
        initialDict_.reset();
    }
    ok = perform();
    return ok;
}
#endif



#if OF_VERSION>=020100 || (defined(OF_FORK_extend) && OF_VERSION>=010604) //(!defined(OF16ext)||defined(Fx41)) && !defined(OF21x)
//- Update for changes of mesh
void UniFunctionObject::updateMesh(const mapPolyMesh& mpm)
{}

//- Update for changes of mesh
void UniFunctionObject::movePoints(
#if (defined(OF_FORK_extend) && OF_VERSION>=010604) //defined(Fx41)
    const pointField&
#else
    const polyMesh& mesh
#endif
    )
{}
#endif



} // namespace Foam
