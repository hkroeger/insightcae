
#include "base/exception.h"
#include "base/tools.h"
#include "base/rapidxml.h"
#include "openfoam/openfoamcasedirectoryparameter.h"
#include "base/zipfile.h"

#include "rapidxml/rapidxml_print.hpp"

using namespace insight;
using namespace boost::filesystem;

void touch(const path& f, const std::string& content="x")
{
    create_directories(f.parent_path());
    std::ofstream(f.string())<<content<<std::endl;
}

std::set<path> listFiles(const path& dir)
{
    std::set<path> files;
    for (recursive_directory_iterator it(dir), end; it!=end; ++it)
    {
        if (is_regular_file(it->status()))
            files.insert(make_relative(dir, it->path()));
    }
    return files;
}

int main(int /*argc*/, char*/*argv*/[])
{
    try
    {
        auto testDir = unique_path(temp_directory_path()/"ofcasedirparam-%%%%%%");
        auto caseDir = testDir/"sourceCase";

        touch(caseDir/"system"/"controlDict");
        touch(caseDir/"system"/"fvSchemes");
        for (auto f: {"points", "faces", "owner", "neighbour", "boundary"})
        {
            touch(caseDir/"constant"/"polyMesh"/f);
            touch(caseDir/"constant"/"solid"/"polyMesh"/f);
        }
        touch(caseDir/"constant"/"transportProperties");
        touch(caseDir/"0"/"U");
        for (auto f: {"points", "faces", "owner", "neighbour", "boundary"})
        {
            touch(caseDir/"5"/"polyMesh"/f);
        }
        touch(caseDir/"5"/"U");
        touch(caseDir/"10"/"U", "latestU");
        touch(caseDir/"10"/"polyMesh"/"points");
        touch(caseDir/"10"/"uniform"/"time");
        touch(caseDir/"processor0"/"10"/"U");
        touch(caseDir/"processor0"/"constant"/"polyMesh"/"points");

        // ================================================================
        // selection of required files

        auto rf = OpenFOAMCaseDirectoryParameter::requiredFiles(caseDir);
        for (const auto& f: rf) std::cout<<f.generic_string()<<std::endl;

        std::set<path> expected = {
            "system/controlDict",
            "system/fvSchemes",
            "constant/transportProperties",
            "10/U",
            "10/polyMesh/points",
            "10/uniform/time",
            "5/polyMesh/faces",
            "5/polyMesh/owner",
            "5/polyMesh/neighbour",
            "5/polyMesh/boundary"
        };
        for (auto f: {"points", "faces", "owner", "neighbour", "boundary"})
        {
            expected.insert(path("constant")/"polyMesh"/f);
            expected.insert(path("constant")/"solid"/"polyMesh"/f);
        }

        insight::assertion(rf==expected, "unexpected selection of required files");

        // ================================================================
        // pack, save, remove original, load and unpack

        std::string xml;
        {
            OpenFOAMCaseDirectoryParameter p(caseDir, "test");
            p.pack();
            insight::assertion(p.isPacked(), "parameter is not packed");

            rapidxml::xml_document<> doc;
            auto* root = doc.allocate_node(rapidxml::node_element, "root");
            doc.append_node(root);
            p.appendToNode("mapFrom", doc, *root, hierarchicalData::Element::OutputProperties());
            rapidxml::print(std::back_inserter(xml), doc, 0);
        }

        // ================================================================
        // relative path "..": packed locally, executed "remotely" in a
        // directory whose parent exists but is not the case

        {
            std::string relxml;
            {
                create_directories(caseDir/"run");
                OpenFOAMCaseDirectoryParameter p(path(".."), "test");
                p.resolveRelativePaths(caseDir/"run");
                insight::assertion(
                    equivalent(p.expandedFilePath(), caseDir),
                    "unexpected expansion of relative path" );
                p.pack();

                rapidxml::xml_document<> doc;
                auto* root = doc.allocate_node(rapidxml::node_element, "root");
                doc.append_node(root);
                p.appendToNode("mapFrom", doc, *root, hierarchicalData::Element::OutputProperties());
                rapidxml::print(std::back_inserter(relxml), doc, 0);
                remove_all(caseDir/"run");
            }

            auto remoteExec = testDir/"remote"/"exec";
            create_directories(remoteExec);

            rapidxml::xml_document<> doc;
            doc.parse<0>(&relxml[0]);
            OpenFOAMCaseDirectoryParameter p("test");
            p.readFromNode("mapFrom", *doc.first_node("root"));
            p.resolveRelativePaths(remoteExec);

            auto ucd = p.accessibleCaseDirectory(remoteExec);
            std::cout<<"unpacked relative case to "<<ucd<<std::endl;
            insight::assertion(
                ucd.filename()=="case"
                && path_contains_file(remoteExec/"embeddedFiles", ucd),
                "unexpected unpack location of relative case" );
            insight::assertion(
                listFiles(ucd)==expected,
                "unexpected unpacked files of relative case" );

            std::set<path> remoteContents;
            for (directory_iterator it(testDir/"remote"), end; it!=end; ++it)
                remoteContents.insert(it->path().filename());
            insight::assertion(
                remoteContents==std::set<path>{"exec"},
                "files were written outside the execution directory" );
        }

        // ================================================================
        // packing a directory which is not an OpenFOAM case fails

        {
            create_directories(testDir/"notacase");
            OpenFOAMCaseDirectoryParameter p(testDir/"notacase", "test");
            bool thrown=false;
            try { p.pack(); } catch (const insight::Exception&) { thrown=true; }
            insight::assertion(thrown, "packing a non-case directory did not fail");
        }

        // ================================================================
        // archive entries pointing outside the target are rejected

        {
            auto zsDir = testDir/"zipslip";
            touch(zsDir/"src");
            writeZipFile(zsDir/"evil.zip", {{"../evil", zsDir/"src"}});
            bool thrown=false;
            try
            {
                ZipFile(zsDir/"evil.zip").uncompressTo(zsDir/"target");
            }
            catch (const insight::Exception&) { thrown=true; }
            insight::assertion(thrown, "unsafe archive entry was not rejected");
            insight::assertion(!exists(zsDir/"evil"), "unsafe archive entry was extracted");
        }

        remove_all(caseDir);

        {
            rapidxml::xml_document<> doc;
            doc.parse<0>(&xml[0]);
            OpenFOAMCaseDirectoryParameter p("test");
            p.readFromNode("mapFrom", *doc.first_node("root"));

            auto ucd = p.accessibleCaseDirectory(testDir/"exec");
            std::cout<<"unpacked to "<<ucd<<std::endl;
            insight::assertion(
                ucd.filename()=="sourceCase"
                && path_contains_file(testDir/"exec"/"embeddedFiles", ucd),
                "unexpected unpack location" );

            insight::assertion(listFiles(ucd)==expected, "unexpected unpacked files");

            std::ifstream f((ucd/"10"/"U").string());
            std::string line;
            getline(f, line);
            insight::assertion(line=="latestU", "unexpected content of unpacked file");

            // second access reuses extraction
            insight::assertion(p.accessibleCaseDirectory(testDir/"exec")==ucd, "extraction not reused");
        }

        // ================================================================
        // compatibility: read from former "path" and "directory" nodes

        for (std::string legacyType: {"path", "directory"})
        {
            std::string lxml =
                "<root><"+legacyType+" name=\"mapFrom\" value=\"/some/case\"/></root>";
            rapidxml::xml_document<> doc;
            doc.parse<0>(&lxml[0]);

            OpenFOAMCaseDirectoryParameter p("test");
            p.readFromNode("mapFrom", *doc.first_node("root"));
            insight::assertion(
                p.filePath()=="/some/case",
                "could not read from legacy node of type "+legacyType );
        }

        remove_all(testDir);
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
