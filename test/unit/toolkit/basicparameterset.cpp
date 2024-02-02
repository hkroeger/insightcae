#include "openfoam/openfoamdict.h"

#include <iostream>

using namespace std;
using namespace insight;

int main()
{
    try
    {
        OFDictData::dict dictdata;
        std::ifstream f("phaseProperties");
        readOpenFOAMDict(f, dictdata);
        std::cout<< dictdata <<std::endl;
        //std::cout<< dictdata.get("saturationModel") <<std::endl;
        return 0;
    }
    catch (std::exception& e)
    {
        cerr<<e.what()<<endl;
        return -1;
    }
}
