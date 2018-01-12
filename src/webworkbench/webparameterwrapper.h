#ifndef WEBPARAMETERWRAPPER_H
#define WEBPARAMETERWRAPPER_H

#include <string>

#include "base/parameterset.h"

#include <Wt/WTreeNode>

namespace insight
{
namespace web
{
    
    

void addWrapperToWidget
(
  insight::ParameterSet& pset, 
  Wt::WTreeNode *parentnode/*, 
  Wt::QWidget *detaileditwidget,
  Wt::QWidget *superform*/
);



class WebParameterWrapper 
: public Wt::WTreeNode
{
  
public:
  
  declareFactoryTable
  (
      WebParameterWrapper, 
        LIST(Wt::WTreeNode* parent, const std::string& name, insight::Parameter& p/*, QWidget* detailw, QWidget* superform*/),
        LIST(parent, name, p/*, detailw, superform*/)
  );  

protected:
  std::string name_;
  insight::Parameter& p_;
//   QWidget* detaileditwidget_;
//   QWidget* superform_;
  
  bool widgetsDisplayed_;
  
//   virtual void focusInEvent( QFocusEvent* );
  
public:
  declareType("WebParameterWrapper");
  WebParameterWrapper
  (
      Wt::WTreeNode* parent, 
      const std::string& name, 
      insight::Parameter& p/*, 
      QWidget* detailw, 
      QWidget* superform*/
  );
  virtual ~WebParameterWrapper();
    
  virtual void createWidgets();
  virtual void removedWidgets();
  
// public slots:
    virtual void onApply() =0;
    virtual void onUpdate() =0;
    virtual void onSelectionChanged();
    virtual void onSelection();
    virtual void onDestruction();
};




class WebIntParameterWrapper
: public WebParameterWrapper
{
protected:
//   QLineEdit *le_;

public:
  declareType(insight::IntParameter::typeName_());
  
  WebIntParameterWrapper(Wt::WTreeNode* parent, const std::string& name, insight::Parameter& p/*, QWidget* detailw, QWidget* superform*/);
  virtual void createWidgets();
  inline insight::IntParameter& param() { return dynamic_cast<insight::IntParameter&>(p_); }
  
// public slots:
  virtual void onApply();
  virtual void onUpdate();
};




}
}

#endif
