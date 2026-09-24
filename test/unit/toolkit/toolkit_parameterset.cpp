#include "base/exception.h"
#include "base/filecontainer.h"
#include "base/parameterset.h"
#include "base/parameters/arrayparameter.h"
#include "base/parameters/labeledarrayparameter.h"
#include "cadgeometryparameter.h"

#include <iostream>

#include "boost/filesystem.hpp"
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

        // --- test ArrayParameter/LabeledArrayParameter propagate baseDirectory_ to new elements ---
        {
            using boost::filesystem::path;

            auto makeGeometryEntry = []()
            {
                auto entry = ParameterSet::create("geometry entry");
                entry->insert("file", CADGeometryParameter(boost::filesystem::path(), "geometry file"));
                return entry;
            };

            path tmpDir = boost::filesystem::temp_directory_path()
                / boost::filesystem::unique_path("insight_test_array_basedir_%%%%%%");
            boost::filesystem::create_directories(tmpDir);

            // --- LabeledArrayParameter ---
            {
                auto defaultEntry = makeGeometryEntry();
                LabeledArrayParameter geomArray(*defaultEntry, 1, "geometry entries");

                auto k0 = *geomArray.keys().begin();
                auto &entry0 = dynamic_cast<ParameterSet&>(geomArray[k0]);
                entry0.get<CADGeometryParameter>("file").setGeometryFile(tmpDir/"part1.stl");

                // mirrors AnalysisForm::onSaveParametersAs, called once the
                // parameter set is (about to be) saved
                geomArray.resolveRelativePaths(tmpDir);

                insight::assertion(
                    bool(entry0.get<CADGeometryParameter>("file").filePath())
                        && *entry0.get<CADGeometryParameter>("file").filePath()==path("part1.stl"),
                    "expected first geometry entry's path to be relative after resolveRelativePaths");

                // mirrors the GUI's "+ Add new" button
                geomArray.appendEmpty();

                std::string k1;
                for (auto& k: geomArray.keys())
                    if (k!=k0) k1=k;
                insight::assertion(!k1.empty(), "new array entry not found");

                auto &entry1 = dynamic_cast<ParameterSet&>(geomArray[k1]);
                auto &file1 = entry1.get<CADGeometryParameter>("file");

                insight::assertion(
                    bool(file1.baseDirectory()) && *file1.baseDirectory()==tmpDir,
                    "newly appended array element did not inherit baseDirectory_");

                // must NOT throw insight::UnsetBaseDirectory
                file1.setGeometryFile(path("part2.stl"));

                insight::assertion(
                    bool(file1.filePath()) && *file1.filePath()==path("part2.stl"),
                    "geometry file selected on a newly appended array element "
                    "was not stored as a relative path");

                auto afp = file1.accessibleFilePath();
                insight::assertion(
                    bool(afp) && *afp==tmpDir/"part2.stl",
                    "newly appended array element's geometry file did not "
                    "resolve to the expected absolute path");

                std::cout<<"LabeledArrayParameter new-element baseDirectory propagation test passed"<<std::endl;

                // --- clone-then-append regression (doCloneUninitialized path) ---
                auto cloned = geomArray.cloneAs<LabeledArrayParameter>();
                cloned->appendEmpty();

                std::string k2;
                for (auto& k: cloned->keys())
                    if (k!=k0 && k!=k1) k2=k;
                insight::assertion(!k2.empty(), "new array entry not found in clone");

                auto &clonedEntry2 = dynamic_cast<ParameterSet&>((*cloned)[k2]);
                // must NOT throw insight::UnsetBaseDirectory
                clonedEntry2.get<CADGeometryParameter>("file").setGeometryFile(path("part3.stl"));

                std::cout<<"LabeledArrayParameter clone-then-append baseDirectory propagation test passed"<<std::endl;
            }

            // --- ArrayParameter (plain) ---
            {
                auto defaultEntry = makeGeometryEntry();
                ArrayParameter geomArray(*defaultEntry, 1, "geometry entries");

                auto &entry0 = dynamic_cast<ParameterSet&>(geomArray.elementRef(0));
                entry0.get<CADGeometryParameter>("file").setGeometryFile(tmpDir/"arrpart1.stl");

                geomArray.resolveRelativePaths(tmpDir);

                geomArray.appendEmpty();
                auto &entry1 = dynamic_cast<ParameterSet&>(geomArray.elementRef(1));
                auto &file1 = entry1.get<CADGeometryParameter>("file");

                // must NOT throw insight::UnsetBaseDirectory
                file1.setGeometryFile(path("arrpart2.stl"));

                insight::assertion(
                    bool(file1.filePath()) && *file1.filePath()==path("arrpart2.stl"),
                    "ArrayParameter: geometry file on newly appended element "
                    "was not stored as a relative path");

                std::cout<<"ArrayParameter new-element baseDirectory propagation test passed"<<std::endl;
            }

            // --- CADGeometryParameter::setGeometryFile must resolve a relative
            //     path against baseDirectory_, not against the process's CWD ---
            {
                CADGeometryParameter geom(boost::filesystem::path(), "geometry file");
                geom.resolveRelativePaths(tmpDir);

                auto savedCwd = boost::filesystem::current_path();
                boost::filesystem::current_path(boost::filesystem::temp_directory_path());

                geom.setGeometryFile( path("models")/"part.stl" );

                boost::filesystem::current_path(savedCwd);

                insight::assertion(
                    bool(geom.filePath()) && *geom.filePath()==path("models")/"part.stl",
                    "CADGeometryParameter::setGeometryFile resolved relative path "
                    "against the wrong base directory: got "
                        + (geom.filePath() ? geom.filePath()->string() : std::string("(none)")) );

                std::cout<<"CADGeometryParameter::setGeometryFile CWD-independence test passed"<<std::endl;
            }

            boost::filesystem::remove_all(tmpDir);
        }

        return 0;
    }
    catch (std::exception& e)
    {
        cerr<<e.what()<<endl;
        return -1;
    }
}
