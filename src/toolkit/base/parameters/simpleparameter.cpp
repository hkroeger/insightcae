#include "simpleparameter.h"

namespace insight
{


char DoubleName[] = "double";
char IntName[] = "int";
char BoolName[] = "bool";
char VectorName[] = "vector";
char StringName[] = "string";
char DateName[] = "date";
char DateTimeName[] = "datetime";






template<> defineType(DoubleParameter);
template<> defineType(IntParameter);
template<> defineType(BoolParameter);
template<> defineType(VectorParameter);
template<> defineType(StringParameter);
template<> defineType(DateParameter);
template<> defineType(DateTimeParameter);
//typedef SimpleParameter<boost::filesystem::path, PathName> PathParameterBase;
//template<> defineType(PathParameterBase);





addParameterFactories(DoubleParameter);
addParameterFactories(IntParameter);
addParameterFactories(BoolParameter);
addParameterFactories(VectorParameter);
addParameterFactories(StringParameter);
addParameterFactories(DateParameter);
addParameterFactories(DateTimeParameter);


}
