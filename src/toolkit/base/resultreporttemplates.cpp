#include "resultreporttemplates.h"

#include "base/tools.h"

namespace insight {



ResultReportTemplate::ResultReportTemplate()
    : std::string(),
      label_(),
      isImplicitlyDefined_(true)
{}

ResultReportTemplate::ResultReportTemplate(
        const std::string &label,
        const std::string &file,
        bool isImplicitlyDefined )
    : std::string(file),
      label_(label),
      isImplicitlyDefined_(isImplicitlyDefined)
{}

ResultReportTemplate::ResultReportTemplate(
        const std::string& label,
        const boost::filesystem::path& file,
        bool isImplicitlyDefined )
    : label_(label),
      isImplicitlyDefined_(isImplicitlyDefined)
{
    readFileIntoString(file, *this);
}

ResultReportTemplate::ResultReportTemplate(rapidxml::xml_node<> *e)
{
    if ( std::string(e->name()) == "resultReport" )
    {
        using namespace rapidxml;
        label_ = e->first_attribute("label")->value();
        std::string::operator=(e->value());
        isImplicitlyDefined_=false;
    }
    else
        throw insight::InvalidConfigurationItem();
}

const std::string &ResultReportTemplate::label() const
{
    return label_;
}

rapidxml::xml_node<> *ResultReportTemplate::createNode(rapidxml::xml_document<> &doc) const
{
    if (isImplicitlyDefined_)
    {
        return nullptr;
    }
    else
    {
        using namespace rapidxml;
        xml_node<> *node = doc.allocate_node(node_element, "resultReport");
        node->append_attribute(doc.allocate_attribute
                               ("label",
                                doc.allocate_string(label_.c_str()) ) );
        node->value(doc.allocate_string(this->c_str()));
        return node;
    }
}




void ResultReportTemplates::readConfiguration()
{
    std::string builtinLabel = "builtin", ovrDefaultLabel;

    auto dr = insertItem(
                ResultReportTemplate(
                    builtinLabel,
                    std::string(
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
                    "\\end{document}\n"),
                    true )
                );

    std::string envvarname="INSIGHT_REPORT_TEMPLATE";
    if ( char *TEMPL=getenv ( envvarname.c_str() ) )
    {
        boost::filesystem::path tp(TEMPL);
        std::string key = tp.filename().stem().string();
        ovrDefaultLabel=insertItem(
                    ResultReportTemplate(
                        key, tp, true )
                    ).first->second.label();
    }

    GlobalConfigurationWithDefault<ResultReportTemplate>::readConfiguration();

    if (defaultLabel_.empty())
    {
        defaultLabel_=builtinLabel;
    }
    if (!ovrDefaultLabel.empty())
    {
        defaultLabel_=ovrDefaultLabel;
    }
}


ResultReportTemplates::ResultReportTemplates()
    : GlobalConfigurationWithDefault<ResultReportTemplate>("reportTemplates.list")
{}




ResultReportTemplates::ResultReportTemplates(const ResultReportTemplates &o)
    : GlobalConfigurationWithDefault<ResultReportTemplate>(o)
{}



const ResultReportTemplates &ResultReportTemplates::globalInstance()
{
    insight::CurrentExceptionContext ex("creating report template data");

    static ResultReportTemplates resultReportTemplates;
    resultReportTemplates.readConfiguration();
    return resultReportTemplates;
}



} // namespace insight
