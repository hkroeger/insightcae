
#include <QAbstractItemModelTester>

#include "iqcaditemmodel.h"
#include "cadfeatures/box.h"

using namespace insight;

int main(int argc, char*argv[])
{
    try
    {
        IQCADItemModel modelToBeTested;
        modelToBeTested.addModelstep(
                    "test",
                    cad::Box::create(
                        cad::matconst(vec3Zero()),
                        cad::matconst(vec3(1,0,0)),
                        cad::matconst(vec3(0,1,0)),
                        cad::matconst(vec3(0,0,1))
                    ), true
                  );
        modelToBeTested.addModelstep(  // replace
                    "test",
                    cad::Box::create(
                        cad::matconst(vec3Zero()),
                        cad::matconst(vec3(2,0,0)),
                        cad::matconst(vec3(0,1,0)),
                        cad::matconst(vec3(0,0,1))
                    ), true
                  );

        QAbstractItemModelTester tester(
            &modelToBeTested,
            QAbstractItemModelTester::FailureReportingMode::Fatal);

        auto box =
                modelToBeTested
                .modelstepIndex("test")
                .siblingAtColumn(IQCADItemModel::entityCol)
                .data()
                .value<cad::FeaturePtr>()
                ;

        double l1=box->getDatumScalar("L1");
        std::cout<<"L1="<<l1<<std::endl;
    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
        return -1;
    }

    return 0;
}
