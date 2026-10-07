#include "openfoamcasedirectoryparameter.h"

#include "base/boost_include.h"
#include "base/exception.h"
#include "base/rapidxml.h"
#include "base/temporaryfile.h"
#include "base/tools.h"
#include "base/warningdispatcher.h"
#include "base/zipfile.h"
#include "openfoam/oftimedirectories.h"

#include "boost/filesystem/operations.hpp"

using namespace boost::filesystem;
namespace fs = boost::filesystem;

namespace insight
{




defineType(OpenFOAMCaseDirectoryParameter);
addParameterFactories(OpenFOAMCaseDirectoryParameter);




OpenFOAMCaseDirectoryParameter::OpenFOAMCaseDirectoryParameter(
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order)
: DirectoryParameter(description, isHidden, isExpert, isNecessary, order)
{}




OpenFOAMCaseDirectoryParameter::OpenFOAMCaseDirectoryParameter(
    const boost::filesystem::path& value,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order)
: DirectoryParameter(value, description, isHidden, isExpert, isNecessary, order)
{}




OpenFOAMCaseDirectoryParameter::OpenFOAMCaseDirectoryParameter(
    const FileContainer& fc,
    const std::string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order)
: DirectoryParameter(fc, description, isHidden, isExpert, isNecessary, order)
{}




OpenFOAMCaseDirectoryParameter::OpenFOAMCaseDirectoryParameter(
    const rapidxml::xml_node<> &node)
: DirectoryParameter(node)
{}




namespace
{

/**
 * same criterion as used by mapFields()
 */
bool isOpenFOAMCase(const fs::path& p)
{
    return !p.empty() && is_regular_file(p/"system"/"controlDict");
}

/**
 * name for the directory, into which a packed case is extracted.
 * Derived from the (possibly relative) path, falls back to "case",
 * if no name can be derived (e.g. for "..").
 */
fs::path extractedCaseDirectoryName(const fs::path& p)
{
    auto np = p.lexically_normal();
    np.remove_trailing_separator();
    auto fn = np.filename();
    if (fn.empty() || fn=="." || fn=="..")
    {
        return "case";
    }
    return fn;
}

void addFilesRecursively(
    const fs::path& caseDir,
    const fs::path& relDir,
    std::set<fs::path>& files )
{
    if (is_directory(caseDir/relDir))
    {
        for (recursive_directory_iterator it(caseDir/relDir), end; it!=end; ++it)
        {
            if (is_regular_file(it->status()))
            {
                files.insert(
                    relDir / make_relative(caseDir/relDir, it->path()) );
            }
        }
    }
}

}




std::set<fs::path> OpenFOAMCaseDirectoryParameter::requiredFiles(
    const fs::path& caseDir )
{
    std::set<fs::path> files;

    // controlDict, fvSchemes, fvSolution etc.
    addFilesRecursively(caseDir, "system", files);

    // mesh, boundaryData, physical properties
    addFilesRecursively(caseDir, "constant", files);

    auto timeDirs = listTimeDirectories(caseDir);
    if (timeDirs.size())
    {
        // fields, uniform/, lagrangian/, region subdirectories
        auto latestTimeName = timeDirs.rbegin()->second.filename();
        addFilesRecursively(caseDir, latestTimeName, files);

        // mesh files may reside in earlier time directories
        // (e.g. after topological changes). Emulate Time::findInstance.
        std::set<fs::path> regions{""};
        auto addRegionsIn = [&regions](const fs::path& d)
        {
            if (is_directory(d))
            {
                for (directory_iterator it(d), end; it!=end; ++it)
                {
                    if (is_directory(it->path()/"polyMesh"))
                        regions.insert(it->path().filename());
                }
            }
        };
        addRegionsIn(caseDir/"constant");
        for (const auto& td: timeDirs)
            addRegionsIn(td.second);

        for (const auto& region: regions)
        {
            auto meshDir = region/"polyMesh";

            std::set<fs::path> meshFileNames;
            for (const auto& td: timeDirs)
            {
                if (is_directory(td.second/meshDir))
                {
                    for (directory_iterator it(td.second/meshDir), end; it!=end; ++it)
                    {
                        if (is_regular_file(it->status()))
                            meshFileNames.insert(it->path().filename());
                    }
                }
            }

            for (const auto& fn: meshFileNames)
            {
                for (auto td=timeDirs.rbegin(); td!=timeDirs.rend(); ++td)
                {
                    if (is_regular_file(td->second/meshDir/fn))
                    {
                        files.insert(td->second.filename()/meshDir/fn);
                        break;
                    }
                }
            }
        }
    }

    return files;
}




fs::path OpenFOAMCaseDirectoryParameter::accessibleCaseDirectory(
    boost::optional<fs::path> overrideBaseDirectory ) const
{
    fs::path fp = expandedFilePath(true);

    // the referenced path might exist but not be the case
    // (e.g. a relative path like ".." on a remote host)
    if ( !isOpenFOAMCase(fp) && hasFileContent() )
    {
        auto baseDir = baseDirectory();
        if (overrideBaseDirectory)
            baseDir=*overrideBaseDirectory;

        fs::path extractDir;
        if (!baseDir)
        {
            extractDir = GlobalTemporaryDirectory::path();
        }
        else
        {
            extractDir = *baseDir / "embeddedFiles";
        }
        extractDir /= toString( std::hash<std::string>()(
            filePath().generic_string() ) );

        fp = extractDir / extractedCaseDirectoryName(filePath());

        if (!isOpenFOAMCase(fp))
        {
            CurrentExceptionContext ex(
                "extracting packed OpenFOAM case %s into %s",
                filePath().c_str(), fp.c_str() );

            create_directories(extractDir);

            // extract into temporary location first,
            // to avoid reuse of incomplete extractions
            auto tmpDir = unique_path(extractDir/"extract-%%%%%%");
            try
            {
                TemporaryFile tf("case-%%%%%%.zip", extractDir);
                copyTo(tf.path());
                ZipFile(tf.path()).uncompressTo(tmpDir);
            }
            catch (...)
            {
                remove_all(tmpDir);
                throw;
            }

            if (!isOpenFOAMCase(tmpDir))
            {
                remove_all(tmpDir);
                throw insight::Exception(
                    "The packed data of parameter for OpenFOAM case %s"
                    " does not contain an OpenFOAM case (no system/controlDict)."
                    " Please re-pack the parameter.",
                    filePath().c_str() );
            }

            remove_all(fp); // possible remains of invalid extraction
            rename(tmpDir, fp);
        }
    }

    return fp;
}




void OpenFOAMCaseDirectoryParameter::pack()
{
    if (isValid())
    {
        auto caseDir = expandedFilePath();

        if (!isOpenFOAMCase(caseDir))
        {
            throw insight::Exception(
                "cannot pack OpenFOAM case %s:"
                " directory does not exist or does not contain system/controlDict!",
                caseDir.c_str() );
        }

        CurrentExceptionContext ex(
            "packing required files of OpenFOAM case %s",
            caseDir.c_str() );

        // archive root is the case root
        std::map<fs::path, fs::path> entries;
        for (const auto& f: requiredFiles(caseDir))
        {
            entries[f] = caseDir/f;
        }

        TemporaryFile tf("case-%%%%%%.zip");
        writeZipFile(tf.path(), entries);
        replaceContent(tf.path());
    }
}




void OpenFOAMCaseDirectoryParameter::unpack(const fs::path &basePath)
{
    accessibleCaseDirectory(basePath);
}




const rapidxml::xml_node<>* OpenFOAMCaseDirectoryParameter::readFromNode
(
    const std::string& name,
    const rapidxml::xml_node<>& node
)
{
    auto* child = Parameter::readFromNode(name, node);
    bool legacyPath=false;

    if (!child && !name.empty())
    {
        // compatibility: read from former parameter types
        if ( (child = findNode(node, name, PathParameter::typeName)) )
        {
            legacyPath=true;
        }
        else
        {
            child = findNode(node, name, DirectoryParameter::typeName);
        }

        if (child)
        {
            if (auto o=getOptionalAttribute<double>(*child, "order"))
            {
                setOrder(*o);
            }
        }
    }

    if (child)
    {
        FileContainer::readFromNode(*child);

        if (legacyPath && hasFileContent())
        {
            insight::Warning(
                "Parameter %s: packed content of former path parameter"
                " cannot be used as OpenFOAM case and is discarded.",
                name.c_str() );
            FileContainer::clearPackedData();
        }

        triggerValueChanged();
    }
    else
    {
        insight::Warning(
            "No xml node found with type '%s' and name '%s', default value '%s' is used.",
            type().c_str(), name.c_str(), filePath().c_str()
            );
    }
    return child;
}




bool OpenFOAMCaseDirectoryParameter::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const OpenFOAMCaseDirectoryParameter*>(&op))
    {
        return PathParameter::isEqual(*oa);
    }
    else
        return false;
}




std::unique_ptr<hierarchicalData::Element>
OpenFOAMCaseDirectoryParameter::doCloneUninitialized() const
{
    return std::make_unique<OpenFOAMCaseDirectoryParameter>(
        *this,
        description().simpleLatex(),
        isHidden(), isExpert(), isNecessary(), order() );
}




std::unique_ptr<OpenFOAMCaseDirectoryParameter>
OpenFOAMCaseDirectoryParameter::cloneOpenFOAMCaseDirectoryParameter() const
{
    return cloneAs<OpenFOAMCaseDirectoryParameter>();
}




} // namespace insight
