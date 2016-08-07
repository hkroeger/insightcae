#include "inputform.h"

#include <Wt/WText>

using namespace Wt;

namespace insight
{
namespace web
{
    
InputForm::InputForm(insight::web::SessionPtr session, Wt::WContainerWidget* parent)
: Wt::WContainerWidget(parent),
  session_(session)
{
//     setContentAlignment(AlignCenter);
    createForm();
}


void InputForm::createForm()
{
    clear();
    new WText("<h2>Input Parameters</h2>", this);
}

}
}
