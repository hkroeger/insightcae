#include "base/exception.h"
#include "base/filecontainer.h"
#include "base/parameterset.h"
#include "base/parameters/arrayparameter.h"
#include "base/parameters/labeledarrayparameter.h"
#include "base/parameters/labeledarraykeyselectionparameter.h"
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


        // --- test hasPath with absolute and ".." paths, consistent with getByPath ---
        {
            auto root = ParameterSet::create("root");
            {
                auto b = ParameterSet::create("b");
                b->insert("x", DoubleParameter(1., "x"));
                auto a = ParameterSet::create("a");
                a->insert("b", *b);
                root->insert("a", *a);

                auto c = ParameterSet::create("c");
                c->insert("y", DoubleParameter(2., "y"));
                root->insert("c", *c);
            }

            auto& b = root->get<ParameterSet>("a/b");

            auto check = [](hierarchicalData::Element& e, const std::string& p, bool expected)
            {
                bool hp = e.hasPath(p);
                bool gp = true;
                try { e.getByPath(p); }
                catch (const insight::ElementNotFoundException&) { gp = false; }

                insight::assertion(
                    hp==expected,
                    "hasPath(\""+p+"\") returned unexpected result" );
                insight::assertion(
                    hp==gp,
                    "hasPath(\""+p+"\") is inconsistent with getByPath" );
            };

            check(b, "x", true);
            check(b, "/c/y", true);
            check(b, "/a/b/x", true);
            check(b, "/c/nope", false);
            check(b, "../../c/y", true);
            check(b, "../../c/nope", false);
            check(*root, "/c/y", true);

            insight::assertion(
                !root->hasPath(".."),
                "hasPath(\"..\") on root should be false" );
            insight::assertion(
                !root->hasPath("../c/y"),
                "hasPath(\"../c/y\") on root should be false" );

            std::cout<<"hasPath absolute/relative path test passed"<<std::endl;
        }


        // --- test re-sync of labeled array keys with key source after reading ---
        {
            auto makeSet = [](const std::set<std::string>& depKeys, bool bind)
            {
                auto root = ParameterSet::create("root");

                LabeledArrayParameter species(DoubleParameter(0., "fraction"), 0, "species");
                species.insertValue("A", std::make_unique<DoubleParameter>(0.5, "fraction"));
                species.insertValue("B", std::make_unique<DoubleParameter>(0.5, "fraction"));
                auto fluid = ParameterSet::create("fluid");
                fluid->insert("species", species);
                root->insert("fluid", *fluid);

                LabeledArrayParameter dep(DoubleParameter(1., "Lewis number"), 0, "dependent");
                for (const auto& k: depKeys)
                    dep.insertValue(k, std::make_unique<DoubleParameter>(2.5, "Lewis number"));
                root->insert("a_dep", dep);  // read before key source
                root->insert("lewisnumbers", dep); // read after key source

                if (bind)
                {
                    root->get<LabeledArrayParameter>("a_dep")
                        .setKeySourceParameterPath("/fluid/species");
                    root->get<LabeledArrayParameter>("lewisnumbers")
                        .setKeySourceParameterPath("/fluid/species");
                }
                return root;
            };

            auto root = makeSet({}, true);
            for (const auto& n: {"a_dep", "lewisnumbers"})
            {
                insight::assertion(
                    root->get<LabeledArrayParameter>(n).keys()==std::set<std::string>({"A", "B"}),
                    std::string(n)+": keys not synchronized with key source after setup" );
            }

            std::string xml;
            makeSet({"A", "X"}, false)->saveToString(xml);

            root->readFromString(xml);
            insight::assertion(
                root->get<LabeledArrayParameter>("lewisnumbers").hasKey("X"),
                "expected stale key from file before re-sync" );

            root = ParameterSet::finalize(std::move(root));

            for (const auto& n: {"a_dep", "lewisnumbers"})
            {
                auto& dep = root->get<LabeledArrayParameter>(n);
                insight::assertion(
                    dep.keys()==std::set<std::string>({"A", "B"}),
                    std::string(n)+": keys not re-synchronized with key source after read" );
                insight::assertion(
                    dynamic_cast<const DoubleParameter&>(dep["A"])()==2.5,
                    std::string(n)+": value read from file was not preserved" );
            }

            std::cout<<"labeled array key re-sync after read test passed"<<std::endl;


            // --- row change notifications must be balanced and not nested
            //     (they are mapped to Qt's begin/end{Insert,Remove}Rows) ---
            int depth=0, maxDepth=0, nBefore=0, nDone=0;
            std::vector<boost::signals2::scoped_connection> conns;
            auto watch = [&](hierarchicalData::Element& e)
            {
                auto before = [&](int, int) { ++nBefore; maxDepth=std::max(maxDepth, ++depth); };
                auto done = [&](int, int) { ++nDone; --depth; };
                conns.emplace_back(e.beforeChildInsertion.connect(before));
                conns.emplace_back(e.childInsertionDone.connect(done));
                conns.emplace_back(e.beforeChildRemoval.connect(before));
                conns.emplace_back(e.childRemovalDone.connect(done));
            };
            watch(root->get<LabeledArrayParameter>("fluid/species"));
            watch(root->get<LabeledArrayParameter>("lewisnumbers"));
            watch(root->get<LabeledArrayParameter>("a_dep"));

            // source with different species: C replaces B
            auto other = makeSet({}, true);
            {
                auto& sp = other->get<LabeledArrayParameter>("fluid/species");
                sp.eraseValue("B");
                sp.insertValue("C", std::make_unique<DoubleParameter>(0.5, "fraction"));
            }

            root->assignFrom(*other);

            insight::assertion(
                nBefore>0 && nBefore==nDone && depth==0,
                str(boost::format("unbalanced row change notifications (before: %d, done: %d)")
                    % nBefore % nDone) );
            insight::assertion(
                maxDepth==1,
                str(boost::format("nested row change notifications (max depth %d)") % maxDepth) );

            for (const auto& n: {"a_dep", "lewisnumbers"})
            {
                auto& dep = root->get<LabeledArrayParameter>(n);
                insight::assertion(
                    dep.keys()==std::set<std::string>({"A", "C"}),
                    std::string(n)+": keys not synchronized after assignFrom" );
                insight::assertion(
                    dep.nChildren()==2,
                    std::string(n)+": unexpected number of children after assignFrom" );
            }

            std::cout<<"labeled array row change notification test passed"<<std::endl;
        }


        // --- test compatibility read of labeledArrayKeySelection from string parameter ---
        {
            auto makeSet = []()
            {
                auto root = ParameterSet::create("root");
                LabeledArrayParameter species(DoubleParameter(0., "fraction"), 0, "species");
                species.insertValue("A", std::make_unique<DoubleParameter>(0.5, "fraction"));
                species.insertValue("B", std::make_unique<DoubleParameter>(0.5, "fraction"));
                auto fluid = ParameterSet::create("fluid");
                fluid->insert("species", species);
                root->insert("fluid", *fluid);
                return root;
            };

            auto makeOldXML = [&](const std::string& v)
            {
                auto old = makeSet();
                // note: pass std::string, a literal would select the (description, bool isHidden) overload
                old->insert("sel", StringParameter(v, std::string("selected specie")));
                std::string xml;
                old->saveToString(xml);
                return xml;
            };

            auto root = makeSet();
            root->insert("sel",
                LabeledArrayKeySelectionParameter(
                    std::string("/fluid/species"), std::string("A"), std::string("selected specie") ) );

            root->readFromString(makeOldXML("B"));
            {
                auto& sel = root->get<LabeledArrayKeySelectionParameter>("sel");
                insight::assertion(
                    sel.selection()=="B",
                    "selection not read from string parameter, got \""+sel.selection()+"\"" );
                insight::assertion(
                    sel.arrayParameterPath()=="/fluid/species",
                    "array path changed during compatibility read" );
            }

            root->readFromString(makeOldXML("Z"));
            root = ParameterSet::finalize(std::move(root));
            insight::assertion(
                root->get<LabeledArrayKeySelectionParameter>("sel").selection().empty(),
                "invalid key read from string parameter was not cleared" );

            std::cout<<"labeledArrayKeySelection compatibility read test passed"<<std::endl;
        }

        return 0;
    }
    catch (std::exception& e)
    {
        cerr<<e.what()<<endl;
        return -1;
    }
}
