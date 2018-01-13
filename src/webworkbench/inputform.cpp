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
    
    Wt::WTree* tree=new Wt::WTree(this);
    tree->setSelectionMode(Wt::SingleSelection);

    WTreeNode* root=new WTreeNode("Parameters");
//     root->setStyleClass("example-tree");
    root->label()->setTextFormat(Wt::PlainText);
//     root->setImagePack("resources/");
    root->setLoadPolicy(Wt::WTreeNode::NextLevelLoading);

    addWrapperToWidget(session_->parameters_, root);
    
    tree->setTreeRoot(root);
    root->expand();
}

}
}
