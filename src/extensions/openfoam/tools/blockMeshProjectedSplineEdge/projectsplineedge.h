#ifndef FOAM_PROJECTSPLINEEDGE_H
#define FOAM_PROJECTSPLINEEDGE_H

#include "blockEdge.H"
#include "CatmullRomSpline.H"

namespace Foam {

class pointConstraint;

namespace blockEdges
{

/*---------------------------------------------------------------------------*\
                         Class projectEdge Declaration
\*---------------------------------------------------------------------------*/

class projectSplineEdge
:
    public blockEdge,
    public CatmullRomSpline
{
    // Private Data

        const searchableSurfaces& geometry_;

        //- The indices of surfaces onto which the points are projected
        labelList surfaces_;


    // Private Member Functions

        //- Single point find nearest
        void findNearest(const point&, point& near, pointConstraint&) const;

        //- No copy construct
        projectSplineEdge(const projectSplineEdge&) = delete;

        //- No copy assignment
        void operator=(const projectSplineEdge&) = delete;


public:

    //- Runtime type information
    TypeName("projectSpline");


    // Constructors

        //- Construct from Istream and point field.
        projectSplineEdge
        (
            const dictionary& dict,
            const label index,
            const searchableSurfaces& geometry,
            const pointField& points,   //!< Referenced point field
            Istream& is
        );


    //- Destructor
    virtual ~projectSplineEdge() = default;


    // Member Functions

        //- The point position corresponding to the curve parameter
        //  0 <= lambda <= 1
        virtual point position(const scalar) const;

        //- The point positions corresponding to the curve parameters
        //  0 <= lambda <= 1
        virtual tmp<pointField> position(const scalarList&) const;

        //- The length of the edge
        //  \note NotImplemented
        virtual scalar length() const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace blockEdges

} // namespace Foam

#endif // FOAM_PROJECTSPLINEEDGE_H
