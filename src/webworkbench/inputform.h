#ifndef INPUTFORM_H
#define INPUTFORM_H

#include <Wt/WContainerWidget>
#include <Wt/WTreeView>

#include "insightsession.h"
#include "webparameterwrapper.h"

#include <memory>

namespace insight
{
namespace web
{



class InputForm
: public Wt::WContainerWidget
{
    SessionPtr session_;
    
public:
  InputForm(SessionPtr session, Wt::WContainerWidget *parent = 0);
  
  void createForm();
};

}
}


#endif
