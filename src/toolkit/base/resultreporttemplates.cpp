#include "resultreporttemplates.h"

#include "base/tools.h"

namespace insight {

ResultReportTemplates::ResultReportTemplates()
{
    auto dr=insert(std::make_pair("builtin",
        "\\documentclass[a4paper,10pt]{scrartcl}\n"
        "\\usepackage{hyperref}\n"
        "\\usepackage{fancyhdr}\n"
        "\\pagestyle{fancy}\n"
        "###HEADER###\n"
        "\\begin{document}\n"
        "\\title{###TITLE###\\\\\n"
        "\\vspace{0.5cm}\\normalsize{###SUBTITLE###}}\n"
        "\\date{###DATE###}\n"
        "\\author{###AUTHOR###}\n"
        "\\maketitle\n"
        "\\tableofcontents\n"
        "###CONTENT###\n"
        "\\end{document}\n")
           );
    if (dr.second)
    {
        defaultTemplateLabel_=dr.first->first;
    }

    std::string envvarname="INSIGHT_REPORT_TEMPLATE";
    if ( char *TEMPL=getenv ( envvarname.c_str() ) )
    {
        boost::filesystem::path tp(TEMPL);
        std::string key = tp.filename().stem().string();
        defaultTemplateLabel_=addTemplateFromFile(key, tp)->first;
    }
}

ResultReportTemplates::ResultReportTemplates(const ResultReportTemplates &o)
    : std::map<std::string, std::string>(o),
      defaultTemplateLabel_(o.defaultTemplateLabel_)
{}

ResultReportTemplates::const_iterator ResultReportTemplates::defaultTemplateIterator() const
{
    auto r=find(defaultTemplateLabel_);
    insight::assertion(
                r!=end(),
                "default template entry "+defaultTemplateLabel_+" does not exist!" );
    return r;
}

const std::string& ResultReportTemplates::defaultTemplate() const
{
    return defaultTemplateIterator()->second;
}

ResultReportTemplates::const_iterator ResultReportTemplates::addTemplateFromFile(
        const std::string &key,
        const boost::filesystem::path &fp )
{
    auto r = insert(make_pair(key, std::string()));
    insight::assertion(
                r.second,
                "could not insert template "+key);

    readFileIntoString(fp, r.first->second);
    return r.first;
}

void ResultReportTemplates::setDefaultTemplate(const std::string &key)
{
    auto r=find(key);
    insight::assertion(
                r!=end(),
                "designated default template entry "+key+" does not exist!" );
    defaultTemplateLabel_=key;
}

const ResultReportTemplates &ResultReportTemplates::globalInstance()
{
    static ResultReportTemplates resultReportTemplates;
    return resultReportTemplates;
}

} // namespace insight
