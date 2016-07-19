/*
 * Copyright (C) 2008 Emweb bvba, Heverlee, Belgium.
 *
 * See the LICENSE file for terms of use.
 */

#include <Wt/WApplication>
#include <Wt/WBreak>
#include <Wt/WContainerWidget>
#include <Wt/WLineEdit>
#include <Wt/WPushButton>
#include <Wt/WText>

// c++0x only, for std::bind
// #include <functional>

using namespace Wt;

/*
 * A simple hello world application class which demonstrates how to react
 * to events, read input, and give feed-back.
 */
class WebWorkbenchApplication
: public WApplication
{
public:
  WebWorkbenchApplication(const WEnvironment& env);

private:
  void loadAnalysis();
  void newAnalysis();
};

/*
 * The env argument contains information about the new session, and
 * the initial request. It must be passed to the WApplication
 * constructor so it is typically also an argument for your custom
 * application constructor.
*/
WebWorkbenchApplication::WebWorkbenchApplication(const WEnvironment& env)
  : WApplication(env)
{
  setTitle("Insight WebWorkbench");                               // application title

  WPushButton *btn_upl = new WPushButton("Upload IST file...", root());              // create a button
  WPushButton *btn_new = new WPushButton("New Analysis...", root());              // create a button
//   button->setMargin(5, Left);                            // add 5 pixels margin

  root()->addWidget(new WBreak());                       // insert a line break

  btn_upl->clicked().connect(this, &WebWorkbenchApplication::loadAnalysis);
  btn_new->clicked().connect(this, &WebWorkbenchApplication::newAnalysis);

  /*
   * - using an arbitrary function object (binding values with boost::bind())
   */
//   nameEdit_->enterPressed().connect
//     (boost::bind(&WebWorkbenchApplication::greet, this));

  /*
   * - using a c++0x lambda:
   */
  // button->clicked().connect(std::bind([=]() { 
  //       greeting_->setText("Hello there, " + nameEdit_->text());
  // }));
}

void WebWorkbenchApplication::loadAnalysis()
{
  /*
   * Update the text, using text input into the nameEdit_ field.
   */
//   greeting_->setText("Hello there, " + nameEdit_->text());
}

void WebWorkbenchApplication::newAnalysis()
{
}


WApplication *createApplication(const WEnvironment& env)
{
  /*
   * You could read information from the environment to decide whether
   * the user has permission to start a new application
   */
  return new WebWorkbenchApplication(env);
}

int main(int argc, char **argv)
{
  /*
   * Your main method may set up some shared resources, but should then
   * start the server application (FastCGI or httpd) that starts listening
   * for requests, and handles all of the application life cycles.
   *
   * The last argument to WRun specifies the function that will instantiate
   * new application objects. That function is executed when a new user surfs
   * to the Wt application, and after the library has negotiated browser
   * support. The function should return a newly instantiated application
   * object.
   */
  return WRun(argc, argv, &createApplication);
}
 
