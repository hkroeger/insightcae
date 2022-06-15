
#include <iostream>

#include "base/intervals.h"

using namespace insight;

int main(int argc, char* argv[])
{
    try
    {
        Interval i{2., 3.};

        OverlappingIntervals oi
                ( {
                      { 0,      std::make_shared<Interval>(0, 5)      },
                      { 0.5,    std::make_shared<Interval>(4, 4.9)    },
                      { 1,      std::make_shared<Interval>(3.8, 10)   }
                  } );

        std::cout<<oi<<std::endl;

        oi.clipIntervals();

        std::cout<<oi<<std::endl;

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }
}
