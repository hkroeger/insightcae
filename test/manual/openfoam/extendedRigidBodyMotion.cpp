
#include "fvCFD.H"
#include "rigidBodyMotion.H"

#include "boost/lexical_cast.hpp"

int main(int argc, char* argv[])
{
    Foam::argList args(argc, argv);
    Foam::Time runTime(Foam::Time::controlDictName, args);

    scalar m=52030000.;
    point CoG(110.3, 0., -3.52);

    string dynamicMeshDictContent=
    "  solver"
    "  {"
    "   gamma 1;"
    "   type Newmark;"
    "  }"
    "  bodies"
    "  {"
    "   hull"
    "   {"
    "    centreOfMass ( "
            +boost::lexical_cast<string>(CoG.x())+" "
            +boost::lexical_cast<string>(CoG.y())+" "
            +boost::lexical_cast<string>(CoG.z())+" );"
    "    inertia (8.6e9 0 0 1.9e11 0 1.9e11);"
    "    joint"
    "    {"
    "     joints ("
    "         {"
    "             type Pxyz;"
    "         }"
    "         {"
    "             type Rxyz;"
    "         }"
    "     );"
    "     type composite;"
    "    }"
    "    mass "+boost::lexical_cast<string>(m)+";"
    "    parent root;"
    "    transform (1 0 0 0 1 0 0 0 1) (0 0 0);"
    "    type rigidBody;"
    "   }"
    "  }";
    IStringStream dmdis(dynamicMeshDictContent);
    dictionary dynamicMeshDict(dmdis);

    string rigidBodyMotionStateContent=
    "q               6 ( 8064.5221 366.96109 -0.34998102 0. 0. 1.3733301 );"
    "qDot            6 ( 2.3819893 5.4010008 0 0 0 0.0054218023 );"
    "qDdot           6 ( 0 0 0 0 0 0 );"
    "t               0;"
    "deltaT          0.06;";
    IStringStream rbmsis(rigidBodyMotionStateContent);
    dictionary rigidBodyMotionState(rbmsis);

    RBD::rigidBodyMotion model_
    (
#if OF_VERSION>=060000 //defined(OFesi1806)
        runTime,
#endif
        dynamicMeshDict,
        rigidBodyMotionState
    );

    const label bodyID = model_.bodyID("hull");

    scalar deltaT=0.06;
    scalar time=0.;
    scalar g=9.81;
    scalar T=850e3;

    std::ofstream f("out.log");
    f << "# t";
    forAll(model_.state().q(), i)
    {
        f << " q" << i;
    }
    forAll(model_.state().qDot(), i)
    {
        f << " qDot" << i;
    }
    f<<std::endl;

    for (int i=0; i<100000; ++i)
    {
        std::cout<<"iter = "<<i<<std::endl;
        time+=deltaT;

        model_.newTime();
        model_.g() = vector(0, 0, -g);

        Field<spatialVector> fx(model_.nBodies(), Zero);


        spatialTransform X(model_.X0(bodyID).inv() & model_.X00(bodyID));
        point lCoG(X.transformPoint(CoG));
        point OR(X.transformPoint(point(0, 0, 0)));
        vector ez((X&spatialVector(vector::zero, vector(0,0,1))).l());
        vector ex((X&spatialVector(vector::zero, vector(1,0,0))).l());
        vector ey = ex^ez;

        scalar Fhx = (870e3-507e3);
        scalar Fhy = 1.25e6;
        scalar Mhz = 83.4e6;
        scalar dhx = Mhz/Fhy;

        scalar Fry = 373e3;
        scalar Mrz = -41.27e6;
        scalar drx = Mrz/Fry;

        Info<<lCoG<<OR<<ex<<ey<<ez<<dhx<<drx<<endl;


        fx[bodyID] = spatialVector
        (
//            (lCoG ^ F_CoG) + (OT ^ F_T), // angular (moment)
//            F_CoG + F_T  // linear (force)
              (lCoG^(Fhx*ex)) + ((lCoG+dhx*ex)^(Fhy*ey)) + ((lCoG+drx*ex)^(Fry*ey)) + (lCoG^vector(0,0,m*g)),
              (Fhx*ex) + (Fhy*ey) + (Fry*ey) + vector(0,0,m*g)
        );

        model_.solve
        (
#if OF_VERSION>=060000 //defined(OFesi1806)
            time,
#endif
            deltaT,
            scalarField(model_.nDoF(), Zero),
            fx
        );

        model_.report();


        forAll(model_.state().q(), i)
        {
            f<<token::SPACE << model_.state().q()[i];
        }
        forAll(model_.state().qDot(), i)
        {
            f<<token::SPACE << model_.state().qDot()[i];
        }
        f<<std::endl;
    }

}
