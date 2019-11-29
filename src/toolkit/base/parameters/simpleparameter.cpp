#include "simpleparameter.h"

namespace insight
{


char DoubleName[] = "double";
char IntName[] = "int";
char BoolName[] = "bool";
char VectorName[] = "vector";
char StringName[] = "string";
//char PathName[] = "pathbase";






template<> defineType(DoubleParameter);
template<> defineType(IntParameter);
template<> defineType(BoolParameter);
template<> defineType(VectorParameter);
template<> defineType(StringParameter);
//typedef SimpleParameter<boost::filesystem::path, PathName> PathParameterBase;
//template<> defineType(PathParameterBase);





addToFactoryTable(Parameter, DoubleParameter);
addToFactoryTable(Parameter, IntParameter);
addToFactoryTable(Parameter, BoolParameter);
addToFactoryTable(Parameter, VectorParameter);
addToFactoryTable(Parameter, StringParameter);


}
