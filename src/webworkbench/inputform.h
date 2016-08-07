#ifndef INPUTFORM_H
#define INPUTFORM_H

#include <Wt/WContainerWidget>

#include "insightsession.h"

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
