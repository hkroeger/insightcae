
#ifndef INSIGHTANALYSIS_H
#define INSIGHTANALYSIS_H

#include <Wt/WContainerWidget>

#include "insightsession.h"



namespace Wt {
  class WStackedWidget;
  class WAnchor;
}

namespace insight
{
namespace web
{
    
class InputForm;
class Results;

class InsightAnalysis
: public Wt::WContainerWidget
{
  Wt::WStackedWidget *mainStack_;
  
  Wt::WContainerWidget *links_;
  Wt::WAnchor *inputFormAnchor_;
  Wt::WAnchor *resultsAnchor_;
  
  SessionPtr session_;
  
  InputForm *inputForm_;
  Results *results_;

public:
  InsightAnalysis(Wt::WContainerWidget *parent = 0);
  void handleInternalPath(const std::string &internalPath);

  void analysisSelectionFinished();
  void showInputForm();
  void showResults();
};

}
}

#endif //INSIGHT_ANALYSIS_H
