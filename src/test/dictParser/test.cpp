#include "openfoam/openfoamdict.h"

#include "openfoam/boundaryconditioncaseelements.h"

using namespace std;
using namespace insight;


#define ISP_DEFINE_ARRAYPARAMETER(PARAMCLASSNAME, NAME, DEFAULTDEF, DEFAULTSIZE, DESC) \
    ISP_BEGIN_DEFINE_PARAMETERSET(BOOST_PP_CAT(NAME,_default)) \
    DEFAULTDEF \
    ISP_END_DEFINE_PARAMETERSET(BOOST_PP_CAT(NAME,_default)) \
    typedef std::vector<NAME##_default::value_value_type> NAME##_value_type; \
    NAME##_value_type NAME; \
    struct NAME##Inserter : public insight::InserterBase \
    {\
      NAME##Inserter(PARAMCLASSNAME& s) : insight::InserterBase(&s, #NAME) {\
       s.inserters().push_back(this);\
      } \
      virtual void createInParameterSet() { if (!created_) { created_=true; createParentInParameterSet(); \
        std::cout<<"init SIMPLEPARAMETER "<<#NAME<<std::endl; \
        std::string key(#NAME); \
        ParameterSet defps = NAME##_default::makeWithDefaults(); \
        Parameter *defp=dynamic_cast<Parameter*>(&defps.at("value"));\
        std::cout<<defp<<std::endl;\
	(*dynamic_cast<PARAMCLASSNAME*>(parent()))->insert(key, new insight::ArrayParameter(*defp, DEFAULTSIZE, DESC) ); \
      }  else std::cout<<"no"<<std::endl; } \
      virtual void syncFromOther(const insight::ParameterSet& o) { \
       std::cout<<"sync: "<<fq_name()<<" <= "<<std::endl; /*o.get<PTYPE>(fq_name())()<<" (now "\
        <<(*dynamic_cast<PARAMCLASSNAME*>(parent())).NAME<<std::endl; \
        (*dynamic_cast<PARAMCLASSNAME*>(parent())).NAME = o.get<PTYPE>(fq_name())();*/ \
      } \
    } Impl_##NAME##Inserter = NAME##Inserter(*this); \


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
//     ISP_DEFINE_SIMPLEPARAMETER(Parameters, bool, uniformConvection, BoolParameter, (false, "Whether to use a uniform convection velocity instead of the local mean velocity"))
//     ISP_DEFINE_SIMPLEPARAMETER(Parameters, double, volexcess, DoubleParameter, (2.0, "Volumetric overlapping of spots"))
//     ISP_DEFINE_SIMPLEPARAMETER
//     (
//       Parameters, int, type, 
//       SelectionParameter, 
//       (
// 	0, 
// 	boost::assign::list_of<std::string>
// 	("inflowGenerator<hatSpot>")
// 	("inflowGenerator<gaussianSpot>")
// 	("inflowGenerator<decayingTurbulenceSpot>")
// 	("inflowGenerator<decayingTurbulenceVorton>")
// 	("inflowGenerator<anisotropicVorton>")
// 	("modalTurbulence"), 
// 	"Type of inflow generator"
//       )
//     )

// 	ISP_DEFINE_ARRAYPARAMETER
// 	(
// 	  Parameters, 
// 	  values,
// // 	  ISP_DEFINE_SUBSETPARAMETER(PARAMCLASSNAME, SUBCLASSNAME, NAME, DESCR, BODY)
// 	  ISP_DEFINE_SIMPLEPARAMETER(values_default, double, value, DoubleParameter, (3.0, "default value")),
// 	  3,
// 	  "blabla"
// 	)

    ISP_DEFINE_SELECTABLESUBSETPARAMETER
    (
      Parameters,
      R, 
      (uniform)(linearProfile)(fittedProfile), // selection
      uniform, // default
     
      ISP_DEFINE_SELECTABLESUBSET(uniform,
	ISP_DEFINE_ARRAYPARAMETER
	(
	  uniform, 
	  values,
// 	  ISP_DEFINE_SUBSETPARAMETER(PARAMCLASSNAME, SUBCLASSNAME, NAME, DESCR, BODY)
	  ISP_DEFINE_SIMPLEPARAMETER(values_default, double, value, DoubleParameter, (3.0, "default value")),
	  3,
	  "blabla"
	)
	ISP_DEFINE_SIMPLEPARAMETER(uniform, double, R_v1, DoubleParameter, (4.5, "Whether to use a uniform convection velocity instead of the local mean velocity"))
      )
      
      ISP_DEFINE_SELECTABLESUBSET(linearProfile,
	ISP_DEFINE_SIMPLEPARAMETER(linearProfile, bool, R_v2, BoolParameter, (false, "Whether to use a uniform convection velocity instead of the local mean velocity"))
      )
      
      ISP_DEFINE_SELECTABLESUBSET(fittedProfile,
	ISP_DEFINE_SIMPLEPARAMETER(fittedProfile, bool, R_v3, BoolParameter, (true, "Whether to use a uniform convection velocity instead of the local mean velocity"))
      )
      ,
     
      "R properties"
    )
  )
  
};

int main()
{
//   try
//   {
//      Test_TurbulentVelocityInletBC::Parameters spp =
//       Test_TurbulentVelocityInletBC::Parameters::makeWithDefaults();
// 
//      spp->saveToFile("test_spp.ist");
//      ParameterSet p(*spp);
//      p.saveToFile("test_p.ist");
//      
//      cout<<"created"<<endl;
//          
//      cout<<"sync from other:"<<endl;
//      
//      cout<<"BEFORE SYNC: "<<spp.volexcess<<endl;
//      spp.syncFromOther(p);
//      cout<<"AFTER SYNC: "<<spp.volexcess<<endl;
// 
// //      spp.pa=1.;
// //      spp.pb=false;
// //      spp.sub.pa=1.0;
// //      spp.sub.path.c_str();
// //      spp.sub.subsub.path.c_str();
//   }
//   catch (insight::Exception e)
//   {
//     cout<<e<<endl;
//   }
 /*
 OFDictData::dict dictdata;
 std::ifstream f("transportProperties");
 readOpenFOAMDict(f, dictdata);
 std::cout<< dictdata <<std::endl;
 std::cout<< dictdata.getString("nu") <<std::endl;
 return 0;
 */
}
