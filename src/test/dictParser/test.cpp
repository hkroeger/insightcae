#include "openfoam/openfoamdict.h"

#include "openfoam/boundaryconditioncaseelements.h"

using namespace std;
using namespace insight;

// #define ISP_DEFINE_SELECTABLESUBSETPARAMETER(PARAMCLASSNAME, SUBCLASSNAME, NAME, DESCR, BODY) \
// struct SUBCLASSNAME : public insight::wrap_ptr<insight::SelectableSubsetParameter> { \
//  SUBCLASSNAME(PARAMCLASSNAME* p) : insight::wrap_ptr<insight::SelectableSubsetParameter>(p, std::string(#NAME)+"/") {\
//       p->inserters().push_back(this); \
//     }\
//      virtual void createInParameterSet() { if (!created_) { created_=true; createParentInParameterSet();\
//        std::cout<<"init SELECTABLESUBSETPARAMETER "<<#NAME<<std::endl; \
//        std::string key(#NAME); \
//        reset(new insight::SubsetParameter(DESCR)); \
//        (*dynamic_cast<PARAMCLASSNAME*>(parent()))->insert(key, get()); \
//      } else std::cout<<"no"<<std::endl; } \
//      virtual void syncFromOther(const insight::ParameterSet&) {} \
// BODY\
// } NAME = SUBCLASSNAME(this); \

#define ISP_DEFINE_SELECTABLESUBSET(SUBSELKEY, BODY) \
 ISP_DEFINE_PARAMETERSET(SUBSELKEY, BODY)

#define ISP_INSERT_KEY(r,DATA,KEY) KEY,

#define ISP_CREATE_DEF(r,DATA,KEY) \
 std::cout<<"make "<<BOOST_PP_STRINGIZE(KEY)<<std::endl;\
 defs.push_back(insight::SelectableSubsetParameter::SingleSubset(BOOST_PP_STRINGIZE(KEY), KEY::makeWithDefaults()->cloneParameterSet()));

#define ISP_DEFINE_SELECTABLESUBSETPARAMETER(PARAMCLASSNAME, NAME, KEYS, DEFAULTKEY, DEFS, DESCR) \
 DEFS \
 typedef boost::variant< BOOST_PP_SEQ_FOR_EACH(ISP_INSERT_KEY, _, KEYS) void*> NAME##_value_type; \
 NAME##_value_type NAME; \
 \
    struct NAME##Inserter : public insight::InserterBase \
    {\
      NAME##Inserter(PARAMCLASSNAME& s) : insight::InserterBase(&s, #NAME) {\
       s.inserters().push_back(this);\
      } \
      virtual void createInParameterSet() { if (!created_) { created_=true; createParentInParameterSet(); \
        std::cout<<"init SELSUBSETPARAMETER "<<#NAME<<std::endl; \
        std::string key(#NAME); \
        insight::SelectableSubsetParameter::SubsetList defs;\
        BOOST_PP_SEQ_FOR_EACH(ISP_CREATE_DEF, _, KEYS) \
	(*dynamic_cast<PARAMCLASSNAME*>(parent()))->insert(key, new insight::SelectableSubsetParameter(\
	 #DEFAULTKEY, defs, DESCR) );\
	 std::cout<<"inserted"<<std::endl;\
      }  else std::cout<<"no"<<std::endl; } \
      virtual void syncFromOther(const insight::ParameterSet& o) { \
       std::cout<<"sync: "<<fq_name()<<std::endl; /*" <= "<<o.get<PTYPE>(fq_name())()<<" (now "\
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
    ISP_DEFINE_SELECTABLESUBSETPARAMETER
    (
      Parameters,
      R, (uniform)(linearProfile), uniform,
     
      ISP_DEFINE_SELECTABLESUBSET(uniform,
	ISP_DEFINE_SIMPLEPARAMETER(uniform, bool, test, BoolParameter, (false, "Whether to use a uniform convection velocity instead of the local mean velocity"))
      )
      ISP_DEFINE_SELECTABLESUBSET(linearProfile,
	ISP_DEFINE_SIMPLEPARAMETER(linearProfile, bool, test2, BoolParameter, (false, "Whether to use a uniform convection velocity instead of the local mean velocity"))
      ),
      "R properties"
    )
  )
  
};

int main()
{
  try
  {
     Test_TurbulentVelocityInletBC::Parameters spp =
      Test_TurbulentVelocityInletBC::Parameters::makeWithDefaults();

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
