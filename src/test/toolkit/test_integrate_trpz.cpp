#include <functional>
#include "base/linearalgebra.h"

using namespace insight;



int main(int /*argc*/, char*/*argv*/[])
{
  try
  {

        double a=0., b=1.;

        arma::mat x = arma::linspace(a, b, 100);

        arma::mat y = 5.*pow(x, 2) + 6.*x;
        auto F=[&](double x) {
            return 5.*(1./3.)*pow(x,3)+6.*(1./2.)*pow(x,2);
        };

//        arma::mat y = 0.*x +1.;
//        auto F=[&](double x) {
//            return x;
//        };

        double Iana=F(b)-F(a);
        Interpolator f(x, y, true);

        auto test = [&](int n)
        {
            double I = integrate_trpz<double>(
                        std::bind(&Interpolator::y, &f, std::placeholders::_1, 0, nullptr),
                        a, b,
                        n
                        );

            std::cout<<I<<" "<<Iana<<" ("<<(I-Iana)/Iana*100.<<"%)"<<std::endl;
        };

        std::vector<double> ns = { 50, 100, 200, 400, 1000 };
        for (const auto& n: ns)
        {
            test(n);
        }
  }
  catch (const std::exception& e)
  {
    std::cerr<<e.what()<<std::endl;
    return -1;
  }

  return 0;
}
