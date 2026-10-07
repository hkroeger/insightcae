#ifndef INSIGHT_OPENFOAMCASEDIRECTORYPARAMETER_H
#define INSIGHT_OPENFOAMCASEDIRECTORYPARAMETER_H

#include "base/parameters/pathparameter.h"

#include <set>

namespace insight
{




/**
 * @brief The OpenFOAMCaseDirectoryParameter class
 * references an existing OpenFOAM case directory, from which
 * a solution shall be mapped (using mapFields -sourceTime latestTime).
 *
 * When packed, only the files which are required by mapFields are embedded:
 * the system and constant directories, the latest time directory and
 * mesh files which reside in earlier time directories.
 *
 * For backwards compatibility, it can be read from XML nodes of type
 * "path" or "directory".
 */
class OpenFOAMCaseDirectoryParameter
    : public DirectoryParameter
{
public:
    declareType ( "openfoamCaseDirectory" );

    OpenFOAMCaseDirectoryParameter (
        const rapidxml::xml_node<> & node );

    OpenFOAMCaseDirectoryParameter (
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );

    OpenFOAMCaseDirectoryParameter (
        const boost::filesystem::path& value,
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );

    OpenFOAMCaseDirectoryParameter (
        const FileContainer& fc,
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );

    /**
     * @brief requiredFiles
     * determine the files of a case, which mapFields reads when
     * called with "-sourceTime latestTime"
     * @param caseDir
     * @return
     * file paths relative to caseDir
     */
    static std::set<boost::filesystem::path> requiredFiles(
        const boost::filesystem::path& caseDir );

    /**
     * @brief accessibleCaseDirectory
     * returns the path to the case directory. If it does not exist on
     * the filesystem but packed content is available, the content is extracted
     * below the base directory.
     * @param overrideBaseDirectory
     * @return
     */
    boost::filesystem::path accessibleCaseDirectory(
        boost::optional<boost::filesystem::path> overrideBaseDirectory
        = boost::optional<boost::filesystem::path>() ) const;

    void pack() override;
    void unpack(const boost::filesystem::path& basePath) override;

    const rapidxml::xml_node<>* readFromNode (
        const std::string& name,
        const rapidxml::xml_node<>& node) override;

    bool isEqual(const insight::hierarchicalData::Element& op) const override;

protected:
    std::unique_ptr<insight::hierarchicalData::Element> doCloneUninitialized() const override;
public:
    std::unique_ptr<OpenFOAMCaseDirectoryParameter> cloneOpenFOAMCaseDirectoryParameter() const;
};




} // namespace insight

#endif // INSIGHT_OPENFOAMCASEDIRECTORYPARAMETER_H
