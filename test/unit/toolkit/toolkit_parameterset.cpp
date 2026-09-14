#include "base/exception.h"
#include "base/filecontainer.h"
#include "base/parameterset.h"

#include <iostream>

#include "boost/filesystem/operations.hpp"
#include "test_pdl.h"

using namespace std;
using namespace insight;

int main()
{
    try
    {
        auto ps = TestPDL::defaultParameters();

        std::cout
            <<"======== CONTENT ========\n"
            <<*ps
            <<"=========================\n\n"
            <<std::endl;

        auto &et=ps->get<DoubleParameter>("run/regime/endTime");
        std::cout<<"name="<<et.name()<<std::endl;
        std::cout<<"path="<<et.path()<<std::endl;

        auto &np=ps->get<DoubleParameter>("run/initialization/preRuns/resolutions/1/nax_parameter");
        insight::assertion(
            np.name()=="nax_parameter",
            "expected another name!") ;
        insight::assertion(
            np.path()=="run/initialization/preRuns/resolutions/1/nax_parameter",
            "expected another path!") ;

        std::cout<<"path="<<np.path(true)<<std::endl;
        insight::assertion(
            np.path(true)=="run/initialization/preRuns/resolutions/default/nax_parameter",
            "expected another path with default redirection!") ;


        std::cout<<ps->getPath("absFile")<<std::endl;

        auto &afile = ps->get<PathParameter>("absFile");
        std::cout<<afile.accessibleFilePath()<<std::endl;

        std::cout<<ps->getPath("relFile")<<std::endl;
        try
        {
            std::cout<<ps->get<PathParameter>("relFile").accessibleFilePath()<<std::endl;
            throw insight::Exception("expected error during attempt to get full path of relative path without base directory set");
        }
        catch (const insight::UnsetBaseDirectory& ex)
        {
            // expected
        }

        // resolve relative paths
        ps->resolveRelativePaths( boost::filesystem::current_path() );


        auto rp = ps->get<PathParameter>("relFile").accessibleFilePath();
        std::cout<<rp<<std::endl;

        insight::assertion(
            rp==boost::filesystem::current_path()/ps->getPath("relFile"),
            "unexpected path");


        // --- test ParameterSet::insert(name, const Parameter&) (clone-based overload) ---
        {
            DoubleParameter dp(2.71828, "a double parameter to be cloned");
            auto& insertedDouble = ps->insert("clonedDouble", dp);
            std::cout<<"cloned double value="<<insertedDouble.plainTextRepresentation(0)<<std::endl;

            auto& viaGet = ps->get<DoubleParameter>("clonedDouble");
            insight::assertion(
                viaGet()==2.71828,
                "cloned double parameter has unexpected value");

            auto nested = ParameterSet::create("a nested parameter set");
            nested->insert("innerDouble", DoubleParameter(1.41421, "inner double"));
            auto& insertedSubset = ps->insert("clonedSubset", *nested);
            std::cout
                <<"cloned subset content:\n"
                <<dynamic_cast<const ParameterSet&>(insertedSubset)
                <<std::endl;

            auto& viaGetSubset = ps->get<ParameterSet>("clonedSubset");
            insight::assertion(
                viaGetSubset.get<DoubleParameter>("innerDouble")()==1.41421,
                "cloned nested parameter set lost/corrupted its inner double parameter");

            // mutate the original after cloning to make sure the clone is truly independent
            nested->get<DoubleParameter>("innerDouble").set(9.99);
            insight::assertion(
                viaGetSubset.get<DoubleParameter>("innerDouble")()==1.41421,
                "cloned nested parameter set was NOT independent of the original (aliasing/shallow-clone bug)");

            std::cout<<"ParameterSet::insert(name, const Parameter&) overload test passed"<<std::endl;
        }

        return 0;
    }
    catch (std::exception& e)
    {
        cerr<<e.what()<<endl;
        return -1;
    }
}
