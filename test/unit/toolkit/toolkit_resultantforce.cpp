#include <iostream>
#include <memory>
#include <vector>


#include "base/tools.h"

using namespace std;
using namespace insight;

int main()
{
    try
    {

        ResultantForce rf1(
            vec3(1., 0, 0),
            vec3(0, 1., 0)
            );

        insight::assertion(
            arma::norm(rf1.r-vec3Z(),2)<SMALL,
            "expected r_z=1");
        insight::assertion(
            arma::norm(rf1.Mr,2)<SMALL,
            "expected no residual moment");


        ResultantForce rf2(
            vec3(0, 1., 0),
            vec3(-1., 0, 0)
            );
        insight::assertion(
            arma::norm(rf2.r-vec3Z(),2)<SMALL,
            "expected r_z=1");
        insight::assertion(
            arma::norm(rf2.Mr,2)<SMALL,
            "expected no residual moment");

        ResultantForce rf3(
            vec3(1., 1., 0),
            vec3(-1., 1., 0)
            );
        insight::assertion(
            arma::norm(rf3.r-vec3Z(),2)<SMALL,
            "expected r_z=1");
        insight::assertion(
            arma::norm(rf3.Mr,2)<SMALL,
            "expected no residual moment");

        return 0;
    }
    catch (std::exception& e)
    {
        cerr<<e.what()<<endl;
        return -1;
    }
}
