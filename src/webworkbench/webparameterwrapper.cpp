
#include "base/boost_include.h"
#include "webparameterwrapper.h"

#include <Wt/WText>

using namespace boost;


namespace insight
{
namespace web
{
    
    

void addWrapperToWidget
(
  insight::ParameterSet& pset, 
  Wt::WTreeNode *parentnode
)
{
    for
    (
        insight::ParameterSet::iterator i=pset.begin(); 
        i!=pset.end(); 
        i++ 
    ) 
    {
        try 
        {
            
        WebParameterWrapper *wrapper =
            WebParameterWrapper::lookup
            (
                i->second->type(),
                parentnode, 
                i->first, 
                *i->second/*, 
                detaileditwidget, 
                superform*/
            );
        }
        catch(...)
        {
            Wt::WTreeNode* n = new Wt::WTreeNode(i->first.c_str());
            parentnode->addChildNode(n);
        }

//         QObject::connect ( parentnode->treeWidget(), SIGNAL ( itemSelectionChanged() ),
//                            wrapper, SLOT ( onSelectionChanged() ) );
// 
//         if ( superform ) 
//         {
//             QObject::connect ( superform, SIGNAL ( apply() ), wrapper, SLOT ( onApply() ) );
//             QObject::connect ( superform, SIGNAL ( update() ), wrapper, SLOT ( onUpdate() ) );
//         }
    }
}



defineType(WebParameterWrapper);
defineFactoryTable
(
    WebParameterWrapper, 
    LIST(Wt::WTreeNode *parent, const std::string& name, insight::Parameter& p/*, QWidget*detailw, QWidget*superform*/),
    LIST(parent, name, p/*, detailw, superform*/)
);


// void WebParameterWrapper::focusInEvent(QFocusEvent* e)
// {
// //     QWidget::focusInEvent(e);
// //     std::cout<<p_.description()<<std::endl;
// }


WebParameterWrapper::WebParameterWrapper
(
    Wt::WTreeNode* parent, 
    const std::string& name, 
    insight::Parameter& p/*, 
    QWidget* detailw, 
    QWidget* superform*/
)
: Wt::WTreeNode(),
//   QObject(),
  name_(name),
  p_(p),
  /*detaileditwidget_(detailw),
  superform_(superform),*/
  widgetsDisplayed_(false)
{
  parent->addChildNode(this);
  label()->setText(name_);
//   setText(0, name_);
//   QFont f=font(1);
//   f.setItalic(true);
//   setFont(1, f);
}

WebParameterWrapper::~WebParameterWrapper()
{
}

void WebParameterWrapper::createWidgets()
{
  widgetsDisplayed_=true;
}

void WebParameterWrapper::removedWidgets()
{
  widgetsDisplayed_=false;
}

void WebParameterWrapper::onSelectionChanged()
{
//   QList<QTreeWidgetItem*> sel=treeWidget()->selectedItems();
//   ParameterWrapper* ptr=dynamic_cast<ParameterWrapper*>(sel[0]);
//   if ( (sel.size()==1) && ptr )
//   {
//     if (ptr==this)
//     {
//       onSelection();
//     }
//   }
}

void WebParameterWrapper::onSelection()
{
//   QList<QWidget*> widgets = detaileditwidget_->findChildren<QWidget*>();
//   foreach(QWidget* widget, widgets)
//   {
//     widget->deleteLater();
//   }
//   
//   if (detaileditwidget_->layout())
//   {
//     delete detaileditwidget_->layout();
//   }
// 
//   createWidgets();
}

void WebParameterWrapper::onDestruction()
{
//   removedWidgets();
}






defineType(WebIntParameterWrapper);
addToFactoryTable(WebParameterWrapper, WebIntParameterWrapper);

WebIntParameterWrapper::WebIntParameterWrapper
(
    Wt::WTreeNode* parent, 
    const std::string& name, 
    insight::Parameter& p/*, 
    QWidget* detailw, 
    QWidget* superform*/
)
: WebParameterWrapper(parent, name, p/*, detailw, superform*/)
{
  onUpdate();
}

void WebIntParameterWrapper::createWidgets()
{
  WebParameterWrapper::createWidgets();
  
//   QVBoxLayout *layout=new QVBoxLayout(detaileditwidget_);
//   
//   QLabel *nameLabel = new QLabel(name_, detaileditwidget_);
//   QFont f=nameLabel->font(); f.setBold(true); nameLabel->setFont(f);
//   layout->addWidget(nameLabel);
//   
//   QWebView *shortDescLabel = 
//     new QWebView( detaileditwidget_ );
//   shortDescLabel->setHtml( param().description().toHTML().c_str() );
//   layout->addWidget(shortDescLabel);
// 
//   QHBoxLayout *layout2=new QHBoxLayout(detaileditwidget_);
//   QLabel *promptLabel = new QLabel("Value:", detaileditwidget_);
//   layout2->addWidget(promptLabel);
//   le_=new QLineEdit(detaileditwidget_);
//   connect(le_, SIGNAL(destroyed(void)), this, SLOT(onDestruction(void)));
//   le_->setText(QString::number(param()()));
//   le_->setValidator(new QIntValidator());
//   connect(le_, SIGNAL(returnPressed()), this, SLOT(onApply()));
//   layout2->addWidget(le_);
//   layout->addLayout(layout2);
//   
//   QPushButton* apply=new QPushButton("&Apply", detaileditwidget_);
//   connect(apply, SIGNAL(pressed()), this, SLOT(onApply()));
//   layout->addWidget(apply);
//   
//   layout->addStretch();
// 
//   detaileditwidget_->setLayout(layout);
}

void WebIntParameterWrapper::onApply()
{
//   if (widgetsDisplayed_)
//   {
//     param()()=le_->text().toInt();
//     setText(1, QString::number(param()()));
//   }
}

void WebIntParameterWrapper::onUpdate()
{
//   setText(1, QString::number(param()()));
//   if (widgetsDisplayed_) le_->setText(QString::number(param()()));
}


}
}
