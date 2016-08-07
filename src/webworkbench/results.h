#ifndef INSIGHT_WEB_RESULTS_H
#define INSIGHT_WEB_RESULTS_H


#include <Wt/WContainerWidget>

#include "insightsession.h"

namespace insight
{
namespace web
{

class Results
: public Wt::WContainerWidget
{
    SessionPtr session_;
    
public:
  Results(SessionPtr session, Wt::WContainerWidget *parent = 0);

  void create();
};

}
}

#endif
