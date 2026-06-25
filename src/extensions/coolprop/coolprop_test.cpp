#include "AbstractState.h"
#include "crossplatform_shared_ptr.h"

#include <iostream>


using namespace std;


int main(int argc, char* argv[])
{

  shared_ptr<CoolProp::AbstractState> state( CoolProp::AbstractState::factory(argv[1], argv[2]));

  double Tmin_, Tmax_, pmin_=10., pmax_;
  try {
    Tmin_ = state->Tmin();
  } catch (...) {
    cout << "Could not get minimum temperature value form CoolProp. Using built-in default." << endl;
  }
  try {
    Tmax_ = state->Tmax();
  } catch (...) {
    cout << "Could not get maximum temperature value form CoolProp. Using built-in default." << endl;
  }
  try {
    pmax_ = state->pmax();
  } catch (...) {
    cout << "Could not get maximum pressure value form CoolProp. Using built-in default." << endl;
  }


  cout << "Using cut-off limits:\n"
       << " - Tmin / Tmax = " << Tmin_ << " / " << Tmax_ << endl
       << " - pmin / pmax = " << pmin_ << " / " << pmax_ << endl;

  state->specify_phase(CoolProp::iphase_gas);

  auto testState = [&](double pIn, double TIn)
  {

    double p = std::min(pmax_, std::max(pmin_, pIn));
    double T = std::min(Tmax_, std::max(Tmin_, TIn));

    cout<<"p="<<pIn<<", T="<<TIn<<" ==>>  p="<<p<<", T="<<T<<endl;

    try {
      state->update(CoolProp::input_pairs::PT_INPUTS, p, T);
    }  catch (std::exception& e) {
      cout<<e.what()<<endl;
    }
    cout<<"hmass="<<state->hmass()<<endl;
    try {
      cout<<"visc="<<state->viscosity()<<endl;
    }  catch (std::exception& e) {
      cout<<e.what()<<endl;
    }
    try {
      cout<<"cond="<<state->conductivity()<<endl;
    }  catch (std::exception& e) {
      cout<<e.what()<<endl;
    }
  };

  testState(193511.98, 327.73031);
  testState(2.61729e+06, 355.276);


//  double p0 = 0.1e5, p1=12e5;
//  double T0 = 200., T1=600.;

//  int nT=10, np=100;
//  cout << "# p T h rho"<<endl;

//  for (int i=0; i<nT; i++)
//  {
//    double T=T0+(T1-T0)*double(i)/double(nT-1);

//    for (int j=0; j<np; j++)
//    {
//      double p=p0+(p1-p0)*double(j)/double(np-1);

//      state->update(CoolProp::input_pairs::PT_INPUTS, p, T);

//      cout << p<<" "<<T<<" "<<state->hmass()<<" "<<state->rhomass()<<endl;
//    }

//    cout <<"\n\n"<<endl;
//  }

}
