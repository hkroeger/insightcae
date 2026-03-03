#ifndef UNIOF_FUNCTIONOBJECT_H
#define UNIOF_FUNCTIONOBJECT_H

#include "uniof.h"
#include "uniof_tools.h"
#include "functionObject.H"

namespace Foam {


class UniFunctionObject : public Foam::functionObject
{
    autoPtr<dictionary> initialDict_;

public:
    UniFunctionObject(const word& name, const dictionary& dict);

public:
#if (defined(OF_FORK_extend)) || (!defined(OF_FORK_extend) && OF_VERSION<040000)
    bool start() override;
#endif

    bool read(const dictionary&) override;

#if (defined(OF_FORK_extend))
    virtual bool write() =0;
#endif

    virtual bool perform() =0;

#if (defined(OF_FORK_extend) && OF_VERSION>=010604) || (!defined(OF_FORK_extend) && OF_VERSION<040000)
    bool execute(bool forceWrite) override;
#else
    bool execute() override;
#endif

#if OF_VERSION>=020100 || (defined(OF_FORK_extend) && OF_VERSION>=010604) //(!defined(OF16ext)||defined(Fx41)) && !defined(OF21x)
    //- Update for changes of mesh
    void updateMesh(const mapPolyMesh& mpm) override;

    //- Update for changes of mesh
    void movePoints(
#if (defined(OF_FORK_extend) && OF_VERSION>=010604) //defined(Fx41)
        const pointField&
#else
        const polyMesh& mesh
#endif
    );
#endif
};


} // namespace Foam

#endif // UNIOF_FUNCTIONOBJECT_H
