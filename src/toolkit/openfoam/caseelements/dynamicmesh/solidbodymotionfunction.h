#ifndef SOLIDBODYMOTIONFUNCTION_H
#define SOLIDBODYMOTIONFUNCTION_H


#include "openfoam/openfoamdict.h"
#include "solidbodymotionfunction__solidBodyMotionFunction__Parameters_headers.h"


namespace insight {



class solidBodyMotionFunction
{
public:
#include "solidbodymotionfunction__solidBodyMotionFunction__Parameters.h"
/*
PARAMETERSET>>> solidBodyMotionFunction Parameters
skipDefaultBase

motion = selectablesubset
{{

 rotation
 set {
  origin = vector (0 0 0) "origin point"
  axis = vector (0 0 1) "rotation axis"
  rpm = double 1000 "rotation rate"
 }

 translation
 set {
  velocity = vector (1 0 0) "motion velocity"
 }

 oscillatingRotating
 set {
  origin = vector (0 0 0) "origin point"
  amplitude = vector (0 0 1) "[deg] amplitude"
  omega = double 1 "[rad/sec] rotation frequency"
 }

}} rotation "type of motion"

<<<PARAMETERSET
*/
    insight::solidBodyMotionFunction::Parameters mp_;

    solidBodyMotionFunction(
        const insight::solidBodyMotionFunction::Parameters& p);

    void addIntoDictionary(OFDictData::dict& dictionary) const;
};


} // namespace insight

#endif // SOLIDBODYMOTIONFUNCTION_H
