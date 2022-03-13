#ifndef INSIGHT_RESULTREPORTTEMPLATES_H
#define INSIGHT_RESULTREPORTTEMPLATES_H

#include "base/boost_include.h"

namespace insight {

class ResultReportTemplates
        : public std::map<std::string, std::string>
{

    std::string defaultTemplateLabel_;

    ResultReportTemplates();

public:
    ResultReportTemplates(const ResultReportTemplates& o);

    const_iterator defaultTemplateIterator() const;
    const std::string& defaultTemplate() const;

    const_iterator addTemplateFromFile(const std::string& key, const boost::filesystem::path& fp);

    void setDefaultTemplate(const std::string& key);

    static const ResultReportTemplates& globalInstance();
};

} // namespace insight

#endif // INSIGHT_RESULTREPORTTEMPLATES_H
