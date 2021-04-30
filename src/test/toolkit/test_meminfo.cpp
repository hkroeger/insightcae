

#include "base/tools.h"

using namespace std;
using namespace insight;

int main()
{
    MemoryInfo mi;
    cout<<mi.memFree_<<"/"<<mi.memTotal_<<endl;
    return 0;
}
