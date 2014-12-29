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
#include "feature.h"
#include "featurefilter.h"
#include "solidmodel.h"
#include "datum.h"
#include "sketch.h"
#include "meshing.h"
%}

%include "feature.h"
%include "featurefilter.h"
%include "solidmodel.h"
%include "datum.h"
%include "sketch.h"
%include "meshing.h"