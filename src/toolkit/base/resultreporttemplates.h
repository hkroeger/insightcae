#ifndef INSIGHT_RESULTREPORTTEMPLATES_H
#define INSIGHT_RESULTREPORTTEMPLATES_H

#include "base/boost_include.h"

#include "base/globalconfiguration.h"
#include "rapidxml/rapidxml.hpp"

namespace insight {


class MultipleTemplateCandidatesException
        : public Exception
{
    std::vector<boost::filesystem::path> candidates_;
};


class ResultReportTemplate
        : public std::string
{
    std::string label_;
    bool isImplicitlyDefined_;

    /**
     * @brief additionalFiles_
     * a set of additional files which will be unpacked
     * along with the filled template latex file (e.g. logos)
     */
    std::map<boost::filesystem::path, std::shared_ptr<std::string> > additionalFiles_;

    void unpackAndRead(const boost::filesystem::path& filepath);

public:
    ResultReportTemplate();
    ResultReportTemplate(
            const std::string& label,
            const std::string& content,
            bool isImplicitlyDefined=false );
    ResultReportTemplate(
            const std::string& label,
            const boost::filesystem::path& file,
            bool isImplicitlyDefined=false );
    ResultReportTemplate(rapidxml::xml_node<> *e);

    void writeAdditionalFiles(const boost::filesystem::path& outputDirectory) const;

    const std::string& label() const;

    rapidxml::xml_node<>* createNode(rapidxml::xml_document<>& doc) const;
};




class ResultReportTemplates
        : public GlobalConfigurationWithDefault<ResultReportTemplate>
{
    static std::string builtinLabel;

    void readConfiguration() override;
    void readAdditionalData(
                rapidxml::xml_document<>& doc,
                rapidxml::xml_node<> *root ) override;

    ResultReportTemplates();

public:
    ResultReportTemplates(const ResultReportTemplates& o);

    static const ResultReportTemplates& globalInstance();
};

} // namespace insight

#endif // INSIGHT_RESULTREPORTTEMPLATES_H
