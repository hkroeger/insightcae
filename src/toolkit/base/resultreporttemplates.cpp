#include "resultreporttemplates.h"

#include "base/casedirectory.h"
#include "base/tools.h"
#include "base/filecontainer.h"
#include "base/zipfile.h"

namespace insight {




void ResultReportTemplate::unpackAndRead(const boost::filesystem::path &filepath)
{
    namespace fs = boost::filesystem;
    namespace bp = boost::process;

    ZipFile zf(filepath);

    auto files = zf.uncompressFiles();

    std::set<boost::filesystem::path> templfs;
    for (const auto& f: files)
    {
        auto fp = f.first;
        auto ext = boost::to_lower_copy(fp.extension().string());

        if ( !(fp.empty() || fp.is_relative()) )
        {
            throw insight::Exception("only relative paths are allowed for additional files in template");
        }

        if (fp.parent_path().empty() && ext==".tex")
        {
            templfs.insert(fp);
        }
    }
    if (templfs.size()<1)
    {
        throw insight::Exception("The archive contains no tex file! There should be one tex file which comprises the template.");
    }
    if (templfs.size()>1)
    {
        throw insight::Exception("The archive contains multiple tex files! Don't know which one is the template.");
    }


    auto templ = *templfs.begin();

    for (const auto& f: files)
    {
        if (f.first==templ)
        {
            std::string::operator=(*f.second);
        }
        else
        {
            additionalFiles_[f.first]=f.second;
        }
    }

}




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
    insight::CurrentExceptionContext ex("reading result report template from file "+file.string());

    auto ext = boost::to_lower_copy(file.extension().string());
    if ( ext==".tex" )
    {
        readFileIntoString(file, *this);
    }
    else if ( ext==".irt" || ext==".zip" )
    {
        unpackAndRead(file);
    }
    else
    {
        throw insight::Exception("Unrecognized result report template format: "+file.string());
    }
}




ResultReportTemplate::ResultReportTemplate(rapidxml::xml_node<> *e)
{

    if ( std::string(e->name()) == "resultReport" )
    {
        using namespace rapidxml;
        label_ = e->first_attribute("label")->value();
        insight::CurrentExceptionContext ex("reading result report configuration "+label_);
        isImplicitlyDefined_=false;

        auto tn = e->first_node("template");
        if (!tn) throw insight::InvalidConfigurationItem();
        std::string::operator=(tn->value());

        for (xml_node<> *f = e->first_node(); f; f = f->next_sibling())
        {
            if (f->name()==std::string("additionalFile"))
            {
                boost::filesystem::path fp = f->first_attribute("path")->value();
                if ( !(fp.empty() || fp.is_relative()) )
                {
                    throw insight::Exception("only relative paths are allowed for additional files in template");
                }
                auto fc = std::make_shared<std::string>();
                base64_decode(f->value(), f->value_size(), fc);
                additionalFiles_[fp] = fc;
            }
        }
    }
    else
        throw insight::InvalidConfigurationItem();
}




void ResultReportTemplate::writeAdditionalFiles(const boost::filesystem::path &outputDirectory) const
{
    for (const auto& af: additionalFiles_)
    {
        writeStringIntoFile(af.second, outputDirectory/af.first);
    }
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
        xml_node<> *resultTemplateNode = doc.allocate_node(node_element, "resultReport");
        resultTemplateNode->append_attribute(doc.allocate_attribute
                               ("label",
                                doc.allocate_string(label_.c_str()) ) );

        for (const auto& af: additionalFiles_)
        {
            auto *fileContentNode = doc.allocate_node(node_element, "additionalFile");
            fileContentNode->append_attribute(doc.allocate_attribute(
                                     "path",
                                     doc.allocate_string(af.first.string().c_str()) ) );
            std::string efc = base64_encode(*af.second);
            fileContentNode->value(doc.allocate_string(efc.c_str()));
            resultTemplateNode->append_node(fileContentNode);
        }

        xml_node<> *templateNode = doc.allocate_node(node_element, "template");
        templateNode->value(doc.allocate_string(this->c_str()));
        resultTemplateNode->append_node(templateNode);

        return resultTemplateNode;
    }
}



std::string ResultReportTemplates::builtinLabel = "builtin";

void ResultReportTemplates::readConfiguration()
{
    std::string ovrDefaultLabel;

    auto dr = insertItem(
                ResultReportTemplate(
                    builtinLabel,
                    std::string(
                    "\\documentclass[a4paper,10pt]{scrartcl}\n"
                    "\\usepackage{hyperref}\n"
                    "\\usepackage{fancyhdr}\n"
                    "\\usepackage{longtable}\n"
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


void ResultReportTemplates::readAdditionalData(
        rapidxml::xml_document<> &doc,
        rapidxml::xml_node<> *root )
{
    try
    {
        GlobalConfigurationWithDefault<ResultReportTemplate>::readAdditionalData(doc, root);
    }
    catch (std::exception& e)
    {
        defaultLabel_=builtinLabel;
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
