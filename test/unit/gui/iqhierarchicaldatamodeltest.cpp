
#include <QAbstractItemModelTester>
#include <exception>
#include <ostream>

#include "iqhierarchicaldatamodel.h"
// #include "base/parameterset.h"
// #include "base/parameters/simpleparameter.h"
#include "iqhierarchicaldataelement.h"
#include "test_pdl.h"

using namespace insight;

int main(int argc, char*argv[])
{
    try
    {
        // auto p = ParameterSet::create(
        //     {
        //      { "one", std::make_unique<DoubleParameter>("just a number") },
        //      { "two", ParameterSet::create({
        //                 { "twoPointOne", std::make_unique<DoubleParameter>("just another number") }
        //              }, "some subset") }
        //     }, "test parameter set" );

        auto p = TestPDL::defaultParameters();

        IQHierarchicalDataModel modelToBeTested(std::move(p));

        QAbstractItemModelTester tester(
            &modelToBeTested,
            QAbstractItemModelTester::FailureReportingMode::Fatal );
    }
    catch (std::exception& ex)
    {
        std::cerr<<"Failed: "<<ex.what()<<std::endl;
        return -1;
    }
    std::cout<<"Passed"<<std::endl;
    return 0;
}
