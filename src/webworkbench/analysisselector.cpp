#include "analysisselector.h"

#include <Wt/WText>
#include <Wt/WPushButton>
#include <Wt/WSelectionBox>
#include <Wt/WPanel>
#include <Wt/WFileUpload>

#include "boost/algorithm/string.hpp"

using namespace Wt;

namespace insight
{
namespace web
{
    
AnalysisSelector::AnalysisSelector(SessionPtr session, Wt::WContainerWidget *parent)
: Wt::WContainerWidget(parent),
  session_(session)
{
    new WText("<h2>Upload Configuration or Select Analysis Type</h2>", this);
    
    Wt::WPanel *panel1 = new Wt::WPanel(this);
    panel1->setTitle("<h3>Select an analysis type:</h3>");
    asb_ = new Wt::WSelectionBox(this);
    
    for (insight::Analysis::FactoryTable::const_iterator i = insight::Analysis::factories_->begin();
       i != insight::Analysis::factories_->end(); i++)
    {
        asb_->addItem(i->first.c_str());
    }
    asb_->setCurrentIndex(0); // Select 'medium' by default.
    asb_->setMargin(10, Wt::Right);
    panel1->setCentralWidget(asb_);
    
    WPushButton *btn = new WPushButton("Create New Analysis...", this);
    btn->clicked().connect(this, &AnalysisSelector::newAnalysisSelected);

//     new WText("or:", this);
    
    Wt::WPanel *panel2 = new Wt::WPanel(this);
    panel2->setTitle("<h3>or upload analysis configuration:</h3>");
    upload_ = new Wt::WFileUpload(this);
    upload_->setFileTextSize(40);
    panel2->setCentralWidget(upload_);
    // Provide a button
    Wt::WPushButton *uploadButton = new Wt::WPushButton("Upload and proceed...", this);
    // Upload when the button is clicked.
    uploadButton->clicked().connect(upload_, &Wt::WFileUpload::upload);
    uploadButton->clicked().connect(uploadButton, &Wt::WPushButton::disable);
    
    upload_->changed().connect(upload_, &WFileUpload::upload);
    upload_->changed().connect(uploadButton, &Wt::WPushButton::disable);
    
    // React to a succesfull upload.
    upload_->uploaded().connect(this, &AnalysisSelector::uploadAnalysisConfiguration);

}

void AnalysisSelector::newAnalysisSelected()
{
    session_.reset(new Session);
    
    std::string analysisName = asb_->currentText().toUTF8();
    
//     session_->analysis_.reset ( insight::Analysis::lookup(analysisName) );
//     session_->analysis_->setDefaults();
    session_->parameters_ = insight::Analysis::defaultParameters(analysisName); /*session_->analysis_->defaultParameters();*/
    
    analysisSelected.emit();
}

void AnalysisSelector::uploadAnalysisConfiguration()
{
    session_.reset(new Session);
    
    std::vector<Http::UploadedFile> fns = upload_->uploadedFiles();
    boost::filesystem::path maininputfile;
    BOOST_FOREACH(const Http::UploadedFile& uf, fns)
    {
        boost::filesystem::path fp = session_->dir()/uf.clientFileName();
        boost::filesystem::copy
        (
            uf.spoolFileName(), 
            fp
        );
        std::string ext=fp.extension().string();
        boost::algorithm::to_lower(ext);
        if ( ext == "ist" )
        {
            if (!maininputfile.empty())
            {
                throw insight::Exception("Multiple candidates for input file!");
            }
            else
            {
                maininputfile=fp;
            }
        }
    }
    
    using namespace rapidxml;
    
    std::ifstream in(maininputfile.c_str());
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();

    xml_document<> doc;
    doc.parse<0>(&contents[0]);

    xml_node<> *rootnode = doc.first_node("root");

    std::string analysisName;
    xml_node<> *analysisnamenode = rootnode->first_node("analysis");
    if (analysisnamenode)
    {
        analysisName = analysisnamenode->first_attribute("name")->value();
    }

//     session_->analysis_.reset ( insight::Analysis::lookup(analysisName) );
//     session_->analysis_->setDefaults();
    session_->parameters_ = insight::Analysis::defaultParameters(analysisName); /*session_->analysis_->defaultParameters()*/;
    session_->parameters_.readFromNode(doc, *rootnode, maininputfile.parent_path());
//     session_->analysis_->setExecutionPath(session_->dir());
    
    analysisSelected.emit();
}

}
}
