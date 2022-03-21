#include "textprogressdisplayer.h"
#include <iostream>

using namespace std;

namespace insight
{

void TextProgressDisplayer::update ( const ProgressState& pi )
{
    const ProgressVariableList& pvl=pi.second;

    for ( const ProgressVariableList::value_type& i: pvl )
    {
        const std::string& name = i.first;

        cout << name << "\t";
    }
    cout << endl;
    for ( const ProgressVariableList::value_type& i: pvl )
    {
        double value = i.second;

        cout << value << "\t";
    }
    cout << endl;

    cout<<pi.logMessage_<<endl;
}

void TextProgressDisplayer::logMessage(const std::string &line)
{
    cout<<line<<std::endl;
}

void TextProgressDisplayer::setActionProgressValue(const string &path, double value)
{
  cout << path << " progress: " << 100.*value << "%" << endl;
}

void TextProgressDisplayer::setMessageText(const string &path, const string &message)
{
  cout<< path << ": " << message << endl;
}

void TextProgressDisplayer::finishActionProgress(const string &)
{
}

void TextProgressDisplayer::reset()
{}

TextProgressDisplayer consoleProgressDisplayer;

}
