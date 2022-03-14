#ifndef INSIGHT_RESULTREPORTTEMPLATES_H
#define INSIGHT_RESULTREPORTTEMPLATES_H

#include "base/boost_include.h"

#include "base/globalconfiguration.h"
#include "rapidxml/rapidxml.hpp"

namespace insight {


class ResultReportTemplate
        : public std::string
{
    std::string label_;
    bool isImplicitlyDefined_;

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

    const std::string& label() const;

    rapidxml::xml_node<>* createNode(rapidxml::xml_document<>& doc) const;
};


class ResultReportTemplates
        : public GlobalConfigurationWithDefault<ResultReportTemplate>
{

    void readConfiguration() override;

    ResultReportTemplates();

public:
    ResultReportTemplates(const ResultReportTemplates& o);

    static const ResultReportTemplates& globalInstance();
};

} // namespace insight

#endif // INSIGHT_RESULTREPORTTEMPLATES_H
