#include "inputform.h"

#include <Wt/WAnimation>
#include <Wt/WPanel>
#include <Wt/WText>
#include <Wt/WTree>
#include <Wt/WTreeNode>

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
    Wt::WAnimation animation(Wt::WAnimation::SlideInFromTop,
			 Wt::WAnimation::EaseOut,
			 100);
    
    new WText("<h2>Input Parameters</h2>", this);
    WTree* tree=new WTree(this);
    WTreeNode* root=new WTreeNode("Parameters");
    tree->setTreeRoot(root);
}

}
}
