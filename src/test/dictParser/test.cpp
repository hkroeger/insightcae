#include "openfoam/openfoamdict.h"

#include "openfoam/boundaryconditioncaseelements.h"

using namespace std;
using namespace insight;

struct Test_TurbulentVelocityInletBC
{
// 	boost::assign::list_of<ParameterSet::SingleEntry>
// 	("L", FieldData::defaultParameter(vec3(1,1,1), "Origin of the prescribed integral length scale field"))
// 	("R", FieldData::defaultParameter(arma::zeros(6), "Origin of the prescribed reynolds stress field"))
// 	.convert_to_container<ParameterSet::EntryList>()
//       ), 
//       "Inflow generator parameters"
      
  ISP_DEFINE_PARAMETERSET
  (
    Parameters,
    ISP_DEFINE_SIMPLEPARAMETER(Parameters, bool, uniformConvection, BoolParameter, (false, "Whether to use a uniform convection velocity instead of the local mean velocity"))
    ISP_DEFINE_SIMPLEPARAMETER(Parameters, double, volexcess, DoubleParameter, (2.0, "Volumetric overlapping of spots"))
    ISP_DEFINE_SIMPLEPARAMETER
    (
      Parameters, int, type, 
      SelectionParameter, 
      (
	0, 
	boost::assign::list_of<std::string>
	("inflowGenerator<hatSpot>")
	("inflowGenerator<gaussianSpot>")
	("inflowGenerator<decayingTurbulenceSpot>")
	("inflowGenerator<decayingTurbulenceVorton>")
	("inflowGenerator<anisotropicVorton>")
	("modalTurbulence"), 
	"Type of inflow generator"
      )
    )
  )
  
};

int main()
{
  try
  {
     Test_TurbulentVelocityInletBC::Parameters spp;
     spp.createInParameterSet();
     spp->saveToFile("test_spp.ist");
     
     ParameterSet p(*spp);
     p.saveToFile("test_p.ist");
     
     cout<<"created"<<endl;
         
     cout<<"sync from other:"<<endl;
     
     cout<<"BEFORE SYNC: "<<spp.volexcess<<endl;
     spp.syncFromOther(p);
     cout<<"AFTER SYNC: "<<spp.volexcess<<endl;

//      spp.pa=1.;
//      spp.pb=false;
//      spp.sub.pa=1.0;
//      spp.sub.path.c_str();
//      spp.sub.subsub.path.c_str();
  }
  catch (insight::Exception e)
  {
    cout<<e<<endl;
  }
 /*
 OFDictData::dict dictdata;
 std::ifstream f("transportProperties");
 readOpenFOAMDict(f, dictdata);
 std::cout<< dictdata <<std::endl;
 std::cout<< dictdata.getString("nu") <<std::endl;
 return 0;
 */
}
