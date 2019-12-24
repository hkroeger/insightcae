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

}
