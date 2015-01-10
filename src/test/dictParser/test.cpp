#include "openfoam/openfoamdict.h"

#include "openfoam/boundaryconditioncaseelements.h"

using namespace std;
using namespace insight;

struct Test
{
 
struct InserterBase
{
  volatile bool created_;
  std::string name_;
  InserterBase* parent_;
  InserterBase(InserterBase* parent, const std::string& name) : created_(false), name_(name), parent_(parent) {}
  virtual std::string fq_name() { if (parent()) return parent()->fq_name()+name_; else return name_; }
  virtual void createInParameterSet() =0;
  virtual void createParentInParameterSet() { if (parent()) { cout<<name_<<"CIPS"<<endl; parent()->createInParameterSet(); } else cout<<name_<<" : NOP"<<endl; }
  
  virtual void syncFromOther(const ParameterSet&) =0;
  virtual InserterBase* parent() { return parent_; };
  virtual InserterBase* topParent() { parent()->topParent(); };
  virtual std::vector<InserterBase*>& inserters() { return topParent()->inserters(); };
};

struct TopInserterBase : public InserterBase
{
  std::vector<InserterBase*> inserters_;
  TopInserterBase() : InserterBase(NULL, "") {}
  virtual void createInParameterSet() 
  {
    if (!created_) { created_=true;
      cout<<"TOP"<<endl;
      BOOST_FOREACH(InserterBase* ins, inserters_)
      {
	ins->createInParameterSet();
      }
    }
  };
  virtual void syncFromOther(const ParameterSet& o) 
  {
    BOOST_FOREACH(InserterBase* ins, inserters_) 
    {
      ins->syncFromOther(o);
    }
  }
  virtual InserterBase* parent() { return NULL; };
  virtual InserterBase* topParent() { return this; };
  virtual std::string fq_name() { return ""; }
  virtual std::vector<InserterBase*>& inserters() { return inserters_; };
};


#define ISP_BEGIN_DEFINE_PARAMETERSET(PARAMCLASSNAME) \
  struct PARAMCLASSNAME : public ParameterSet::Ptr, public TopInserterBase \
  { \
    PARAMCLASSNAME() : ParameterSet::Ptr(new ParameterSet()) {} \

#define ISP_END_DEFINE_PARAMETERSET(PARAMCLASSNAME) \
  }
  
// #define

#define ISP_DEFINE_SIMPLEPARAMETER(PARAMCLASSNAME, TYPE, NAME, PTYPE, CONSTR) \
    TYPE NAME; \
    struct NAME##Inserter : public InserterBase \
    {\
      NAME##Inserter(PARAMCLASSNAME& s) : InserterBase(&s, #NAME) {\
       s.inserters().push_back(this);\
      } \
      virtual void createInParameterSet() { if (!created_) { created_=true; createParentInParameterSet(); \
        std::cout<<"init SIMPLEPARAMETER "<<#NAME<<std::endl; \
        std::string key(#NAME); \
	(*dynamic_cast<PARAMCLASSNAME*>(parent()))->insert(key, new PTYPE CONSTR ); \
      }  else std::cout<<"no"<<std::endl; } \
      virtual void syncFromOther(const ParameterSet&) { std::cout<<"sync: "<<fq_name()<<std::endl; } \
    } Impl_##NAME##Inserter = NAME##Inserter(*this) \
  
 
#define ISP_BEGIN_DEFINE_SUBSETPARAMETER(PARAMCLASSNAME, SUBCLASSNAME, NAME, DESCR) \
struct SUBCLASSNAME : public wrap_ptr<SubsetParameter> { \
 SUBCLASSNAME(PARAMCLASSNAME* p) : wrap_ptr<SubsetParameter>(p, std::string(#NAME)+"/") {\
      p->inserters().push_back(this); \
    }\
     virtual void createInParameterSet() { if (!created_) { created_=true; createParentInParameterSet();\
       std::cout<<"init SUBSETPARAMETER "<<#NAME<<std::endl; \
       std::string key(#NAME); \
       reset(new SubsetParameter(DESCR)); \
       (*dynamic_cast<PARAMCLASSNAME*>(parent()))->insert(key, get()); \
     } else std::cout<<"no"<<std::endl; } \
     virtual void syncFromOther(const ParameterSet&) {} \


#define ISP_END_DEFINE_SUBSETPARAMETER(PARAMCLASSNAME, SUBCLASSNAME, NAME) \
} NAME = SUBCLASSNAME(this); \
   
  template<class T>
  struct wrap_ptr : public InserterBase
  {
    T* ps_;
    wrap_ptr(InserterBase* p, const std::string& name) : InserterBase(p, name) {}
    inline T* get() { return ps_; }
    inline T* operator*() { return ps_; }
    inline T* operator->() const { return ps_; }
    inline void reset(T* p) { ps_=p; }
    virtual void syncFromOther(const ParameterSet&) { }
    virtual void createInParameterSet() { createParentInParameterSet(); }
  };
  
  ISP_BEGIN_DEFINE_PARAMETERSET(SParam)
    
    ISP_DEFINE_SIMPLEPARAMETER(SParam, double, pa, DoubleParameter, (1.2, "Test"));
    ISP_DEFINE_SIMPLEPARAMETER(SParam, bool, pb, BoolParameter, (true, "Test"));

    ISP_BEGIN_DEFINE_SUBSETPARAMETER(SParam, Ssub, sub, "blubb")
      
      ISP_BEGIN_DEFINE_SUBSETPARAMETER(Ssub, Ssubsub, subsub, "another sub")
	
	ISP_DEFINE_SIMPLEPARAMETER(Ssubsub, 
			      boost::filesystem::path, path, 
			      PathParameter, ("/home/bla.txt", "Test"));
	
      ISP_END_DEFINE_SUBSETPARAMETER(Ssub, Ssubsub, subsub);
      
      ISP_DEFINE_SIMPLEPARAMETER(Ssub, 
			     boost::filesystem::path, path, 
			     PathParameter, ("/home/hannes/tmp/blubb.xy", "Test"));
      
      ISP_DEFINE_SIMPLEPARAMETER(Ssub, 
			     double, pa, 
			     DoubleParameter, (1.5, "Test"));
      
      ISP_DEFINE_SIMPLEPARAMETER(Ssub, 
			     std::string, sel, 
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
			    );
      
    ISP_END_DEFINE_SUBSETPARAMETER(SParam, Ssub, sub);
    
  ISP_END_DEFINE_PARAMETERSET(SParam);
};

int main()
{
  
     ParameterSet p;
     Test::SParam spp;
     cout<<"created"<<endl;
     cout<<"build:"<<endl;
     spp.createInParameterSet();
     spp->saveToFile("test.ist");
     cout<<"sync from other:"<<endl;
     spp.syncFromOther(p);
     spp.pa=1.;
     spp.pb=false;
     spp.sub.pa=1.0;
     spp.sub.path.c_str();
     spp.sub.subsub.path.c_str();
     
 /*
 OFDictData::dict dictdata;
 std::ifstream f("transportProperties");
 readOpenFOAMDict(f, dictdata);
 std::cout<< dictdata <<std::endl;
 std::cout<< dictdata.getString("nu") <<std::endl;
 return 0;
 */
}
