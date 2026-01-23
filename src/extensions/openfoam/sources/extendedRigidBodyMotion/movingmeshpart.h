#ifndef FOAM_MOVINGMESHPART_H
#define FOAM_MOVINGMESHPART_H

#include "fvMesh.H"
#include "labelList.H"
#include "septernion.H"

namespace Foam {

struct movingMeshPart
{
    const polyMesh& mesh_;
    labelList pointIDs_;

    movingMeshPart(const dictionary& dict, const polyMesh& mesh);
    virtual ~movingMeshPart();



    template<class T>
    class INew
    {

        const polyMesh& mesh_;

    public:

        //- Construct null
        INew(const polyMesh& mesh)
            : mesh_(mesh)
        {}

        //- Construct from Istream
        autoPtr<T> operator()(Istream& is) const
        {
            return autoPtr<T>(
                        new T(dictionary(is), mesh_)
                        );
        }
    };
};

struct IndependentMovingMeshPart
: public movingMeshPart
{
    using movingMeshPart::movingMeshPart;

    virtual septernion transformation() const =0;
};


} // namespace Foam

#endif // FOAM_MOVINGMESHPART_H
