#include "insightanalysis.h"
#include "analysisselector.h"
#include "inputform.h"
#include "results.h"

#include <Wt/WAnchor>
#include <Wt/WText>
#include <Wt/WStackedWidget>
#include <Wt/WVBoxLayout>
#include <Wt/WHBoxLayout>
#include <Wt/WApplication>
#include <Wt/Auth/AuthWidget>


using namespace Wt;

namespace insight
{
namespace web
{




InsightAnalysis::InsightAnalysis(WContainerWidget *parent)
: WContainerWidget(parent),
  session_(new Session),
  inputForm_(0),
  results_(0)
{
  WText *title = new WText("<h1>Insight Web Frontend</h1>");
  addWidget(title);

  mainStack_ = new WStackedWidget();
  mainStack_->setStyleClass("insightanalysisstack");
  addWidget(mainStack_);

  links_ = new WContainerWidget();
  links_->setStyleClass("links");
  addWidget(links_);

  inputFormAnchor_ = new WAnchor("/input", "Input Parameters", links_);
  inputFormAnchor_->setLink(WLink(WLink::InternalPath, "/input"));

  resultsAnchor_ = new WAnchor("/results", "Results", links_);
  resultsAnchor_->setLink(WLink(WLink::InternalPath, "/results"));

  WApplication::instance()->internalPathChanged()
    .connect(this, &InsightAnalysis::handleInternalPath);
    
//   handleInternalPath(WApplication::instance()->internalPath());

  links_->hide();
  AnalysisSelector *sel = new AnalysisSelector(session_, mainStack_);
  sel->analysisSelected.connect(this, &InsightAnalysis::analysisSelectionFinished);
  mainStack_->setCurrentWidget(sel);
}




void InsightAnalysis::handleInternalPath(const std::string &internalPath)
{
    if (internalPath == "/input")
        showInputForm();
    else if (internalPath == "/results")
        showResults();
    else
        WApplication::instance()->setInternalPath("/input",  true);
}




void InsightAnalysis::analysisSelectionFinished()
{
    links_->show();
    showInputForm();
}




void InsightAnalysis::showInputForm()
{
    if (!inputForm_)
    {
        inputForm_=new InputForm(session_, mainStack_);
    }
    mainStack_->setCurrentWidget(inputForm_);
}




void InsightAnalysis::showResults()
{
    if (!results_)
    {
        results_=new Results(session_, mainStack_);
    }
    mainStack_->setCurrentWidget(results_);
}

}
}
