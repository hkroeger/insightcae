#ifndef ANALYSIS_SELECTOR_H
#define ANALYSIS_SELECTOR_H

#include <Wt/WContainerWidget>

#include "insightsession.h"

namespace Wt
{
    class WSelectionBox;
    class WFileUpload;
}

namespace insight
{
namespace web
{
    
class AnalysisSelector
: public Wt::WContainerWidget
{
    SessionPtr session_;
    
    Wt::WSelectionBox *asb_;
    Wt::WFileUpload *upload_;
    
public:
    AnalysisSelector(SessionPtr session, Wt::WContainerWidget *parent = 0);
    
    void newAnalysisSelected();
    void uploadAnalysisConfiguration();
    
    Wt::Signal<void> analysisSelected;
};

}
}

#endif
