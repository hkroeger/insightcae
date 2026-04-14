%include "common.i"

%typemap(typecheck) insight::cad::FeatureSetList& 
{
    $1 = PySequence_Check($input) ? 1 : 0;
}

%typemap(in) insight::cad::FeatureSetList& (insight::cad::FeatureSetList vIn) 
{
    size_t iLen = PySequence_Length($input); 
    vIn.clear();
    for(unsigned int i = 0; i < iLen; i++) 
    {
	insight::cad::FeatureSet* p;
    
	int res1 = SWIG_ConvertPtr
	(
	  PySequence_GetItem($input, i), 
	  &p,
	  SWIGTYPE_p_insight__cad__FeatureSet, 1 
	);
	
	std::cout<<"res="<<res1<<std::endl;
	if (res1!=-1) //(!SWIG_IsOK(res1)) 
	{
	  std::cout<<"OK"<<std::endl;
	  vIn.push_back( insight::cad::FeatureSetPtr(p->clone()) );
	}
    }
    std::cout<<"size="<<vIn.size()<<std::endl;
    $1 = &vIn;
}

%module(directors="1") cad
%{
#include "base/factory.h"
#include "featureset.h"
#include "featurefilter.h"
#include "cadfeatures.h"
#include "datum.h"
#include "sketch.h"
#include "meshing.h"
using namespace insight;
using namespace insight::cad;
%}

// Suppress SWIG warnings for C++ patterns that don't need Python bindings
%warnfilter(325);  // Nested struct not currently supported (factory Add<>)
%warnfilter(317);  // Specialisation of non-template (is_arithmetic<arma::mat>)
%warnfilter(381);  // operator&& ignored
%warnfilter(361);  // operator! ignored
%warnfilter(362);  // operator= ignored
%warnfilter(401);  // Nothing known about base class (DependencySource, ASTBasedFeatureSet, …)
%warnfilter(503);  // Can't wrap conversion operator unless renamed
%warnfilter(504);  // CLONEABLE macro — function must have a return type

%include "base/factory.h"
%include "featureset.h"
%include "featurefilter.h"
%include "cadfeatures.h"
%include "datum.h"
%include "sketch.h"
%include "meshing.h"
