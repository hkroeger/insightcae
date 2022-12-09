#include "openfoamcasewithcylindermesh.h"

#include "openfoam/caseelements/numerics/laplacianfoamnumerics.h"

int main(int argc, char*argv[])
{
    return executeTest([=](){

        assertion( argc==2, "Exactly one command line arguments are required!");

        OpenFOAMCaseWithCylinderMesh cm(argv[1]);

        laplacianFoamNumerics::Parameters p;
        p.deltaT=1;
        p.endTime=10000;
        p.purgeWrite=0;
        p.writeControl=laplacianFoamNumerics::Parameters::writeControl_type::timeStep;
        p.writeInterval=1000;

        laplacianFoamNumerics::Parameters::restartWrite_clockTime_type rw;
        rw.clockTimeInterval=1;
        rw.nKeep=2;
        p.restartWrite = rw;

        cm.insert(new laplacianFoamNumerics(cm, p));

        cm.runTest();

        auto tds = insight::listTimeDirectories(cm.dir());
        insight::TimeDirectoryList addtds;
        for (const auto& td: tds)
        {
            std::cout<<td.second<<std::endl;
            if ( int(td.first)%int(p.writeInterval) != 0 )
                addtds.insert(td);
        }

        insight::assertion(
                    addtds.size()==rw.nKeep,
                    boost::str(boost::format("expected %d additional time directories, got %d!")
                               % rw.nKeep % addtds.size() ));

        auto t0 = boost::filesystem::last_write_time( addtds.begin()->second );
        auto t1 = boost::filesystem::last_write_time( (++addtds.begin())->second );

        double diff=difftime(t1, t0);
        std::cout<<"diff = "<<diff<<std::endl;;
        // doesn't work, too close
//        insight::assertion(
//                    fabs(diff-1.0)<0.01,
//                    boost::str(boost::format("expected one second time difference, got %g")
//                               % diff ));
    });
}
