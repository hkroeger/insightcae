#ifndef UNIOF_HELPERS_H
#define UNIOF_HELPERS_H

#include "uniof.h"
#include "uniof_time.h"
#include "Pstream.H"
#include "OFstream.H"

#include "fvMesh.H"
#include "cellSet.H"

namespace Foam
{



void findBndFaces(const fvMesh& mesh, const cellSet& cells, labelList& res, scalarField& normal_direction);



template<class T>
tmp<Field<T> > pick_gf(
    const GeometricField<T, fvsPatchField, surfaceMesh>& data,
    const labelList& idxs,
    const scalarField* mult=nullptr )
{
    tmp<Field<T> > tres(new Field<T>(idxs.size(), pTraits<T>::zero));
    Field<T>& res = UNIOF_TMP_NONCONST(tres);

    forAll(idxs, i)
    {
        label fi=idxs[i];

        if (fi<data.mesh().nInternalFaces())
        {
            res[i] = data[idxs[i]];
        } else
        {
            label pi=data.mesh().boundaryMesh().whichPatch(fi);
            res[i] = data.boundaryField()[pi][fi-data.mesh().boundaryMesh()[pi].start()];
        }

        if (mult!=nullptr) res[i] *= (*mult)[i];
    }
    return tres;
}




template<const char* fName>
class FileOnDemand
{
    word name_;
    const Time& time_;
    mutable autoPtr<OFstream> filePtr_;

public:
    FileOnDemand(const word& name, const Time& time)
        : name_(name), time_(time)
    {}

    virtual ~FileOnDemand()
    {
        filePtr_.reset();
    }

    virtual void writeFileHeader(OFstream& os)
    {}

    Ostream& operator()()
    {
        // Create the minmax file if not already created
        if (filePtr_.empty())
        {
            // File update
            if (Pstream::master())
            {
                fileName dir;
                word startTimeName =
                    time_.timeName(time_.startTime().value());

                if (Pstream::parRun())
                {
                    // Put in undecomposed case (Note: gives problems for
                    // distributed data running)
                    dir = time_.path()/".."/"postProcessing"/name_/startTimeName;
                }
                else
                {
                    dir = time_.path()/"postProcessing"/name_/startTimeName;
                }

                // Create directory if does not exist.
                mkDir(dir);

                // Open new file at start up
                filePtr_.reset(new OFstream(dir/fName));

                // Add headers to output data
                writeFileHeader(filePtr_());
            }
            else
            {
                filePtr_.reset(new OFstream("/dev/null"));
            }
        }

        return filePtr_();
    }
};


}

#endif // UNIOF_HELPERS_H
