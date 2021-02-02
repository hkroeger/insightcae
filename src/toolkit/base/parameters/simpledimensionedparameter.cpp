#include "simpledimensionedparameter.h"

namespace insight {


#define defineDimensionedParameter(baseType, baseTypeName, dimensionType, dimensionTypeName) \
  char baseTypeName##dimensionTypeName[] = #baseTypeName#dimensionTypeName; \
  template<> defineType(baseTypeName##dimensionTypeName##Parameter);\
  addToFactoryTable(Parameter, baseTypeName##dimensionTypeName##Parameter)


#define defineDimensionedParameters(baseType, baseTypeName) \
  defineDimensionedParameter(baseType, baseTypeName, boost::units::si::length, Length); \
  defineDimensionedParameter(baseType, baseTypeName, boost::units::si::velocity, Velocity)


defineDimensionedParameters(double, scalar);
defineDimensionedParameters(arma::mat, vector);

} // namespace insight
