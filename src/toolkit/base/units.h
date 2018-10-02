#ifndef UNITS_H
#define UNITS_H

namespace SI
{

// the units below represent the conversion factor from the unit under consideration into the corresponding SI standard unit
// e.g.:   <length in meters> = <length in millimeters> * SI:mm;  (i.e. SI:mm == 1e-3)

// mass

extern double ton;
extern double kg;
extern double gram;
extern double lb;

// length
extern double microns;
extern double mm;
extern double cm;
extern double m;
extern double km;
extern double in;

// time
extern double sec;
extern double min;
extern double hr;
extern double day;

// angle
extern double rad;
extern double deg;

// velocity
extern double mps;
extern double kmh;
extern double kt;

}

#endif // UNITS_H
