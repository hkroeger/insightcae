/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef UNITS_H
#define UNITS_H


#include <boost/units/systems/si/energy.hpp>
#include <boost/units/systems/si/plane_angle.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/force.hpp>
#include <boost/units/systems/si/length.hpp>
#include <boost/units/systems/si/area.hpp>
#include <boost/units/systems/si/volume.hpp>
#include <boost/units/systems/si/electric_potential.hpp>
#include <boost/units/systems/si/current.hpp>
#include <boost/units/systems/si/resistance.hpp>
#include <boost/units/systems/si/io.hpp>
#include "boost/units/systems/si/pressure.hpp"
#include "boost/units/systems/si/prefixes.hpp"
#include <boost/units/base_units/metric/knot.hpp>
#include <boost/units/base_units/metric/ton.hpp>
#include "boost/units/physical_dimensions.hpp"
#include "boost/units/systems/si/codata/physico-chemical_constants.hpp"

#include "base/linearalgebra.h"



namespace boost { namespace units { namespace si {


 typedef unit<specific_heat_capacity_dimension,si::system> specific_heat_capacity;
 typedef unit<thermal_conductivity_dimension,si::system> thermal_conductivity;

 BOOST_UNITS_STATIC_CONSTANT(joule_per_kilogram_kelvin,specific_heat_capacity);
 BOOST_UNITS_STATIC_CONSTANT(watt_per_square_meter,thermal_conductivity);

 // some commonly used units and aliases

 static const auto megapascal = mega*pascals;
 static const auto millimeters = milli*meters;
 static const auto millimeter = milli*meter;
 static const auto mps = meter/second;

 static const auto dimless = si::dimensionless();

 typedef metric::knot_base_unit::unit_type knot_unit;
 typedef quantity<knot_unit> knot_quantity;
 static const knot_unit knot;

 typedef metric::ton_base_unit::unit_type metric_ton_unit;
 typedef quantity<metric_ton_unit> metric_ton_quantity;
 static const metric_ton_unit metric_ton;


 typedef degree::plane_angle::unit_type angle_deg_unit;
 typedef quantity<angle_deg_unit> angle_deg_quantity;
 static const angle_deg_unit angle_deg;

 typedef si::plane_angle::unit_type angle_rad_unit;
 typedef quantity<angle_rad_unit> angle_rad_quantity;
 static const angle_rad_unit angle_rad;

 static const auto kelvin_per_meter = si::kelvin/si::meter;



 template<class Dimension, class Type, class Unit>
 Type toValue(const quantity<Dimension,Type>& q, const Unit& u)
 {
   return ( static_cast<quantity<Unit, Type> >(q) / u).value();
 }



 template<class Unit, class Type>
 class matQuantity
     : public quantity<Unit, Type>
 {
 public:
   typedef quantity<Unit, typename Type::elem_type> dimensioned_elem_type;

   matQuantity() : quantity<Unit, Type>() {}

   matQuantity(const dimensioned_elem_type&x, const dimensioned_elem_type&y, const dimensioned_elem_type&z)
     : quantity<Unit,Type>(insight::vec3(toValue(x, Unit()), toValue(y, Unit()), toValue(z, Unit()))*Unit())
   {}

   template<class P1>
   matQuantity(P1 p1) : quantity<Unit,Type>(p1) {}

   template<class P1, class P2>
   matQuantity(P1 p1, P2 p2) : quantity<Unit,Type>(p1, p2) {}


   inline dimensioned_elem_type operator()(arma::uword i) const
   { return dimensioned_elem_type( this->value()(i)*Unit() ); }

   inline dimensioned_elem_type operator()(arma::uword i, arma::uword j) const
   { return dimensioned_elem_type( this->value()(i,j)*Unit() ); }

   inline dimensioned_elem_type operator[](arma::uword i) const
   { return dimensioned_elem_type( this->value()[i]*Unit() ); }

 };



 typedef matQuantity<dimensionless, arma::mat> DimlessVector;

 typedef quantity<length, double> Length;
 typedef matQuantity<length, arma::mat> LengthVector;

 typedef quantity<time, double> Time;

 typedef quantity<area, double> Area;

 typedef quantity<velocity, double> Velocity;

 typedef quantity<plane_angle, double> Angle;

 typedef quantity<pressure, double> Pressure;

 typedef quantity<temperature, double> Temperature;

 typedef quantity<power, double> Power;

 typedef quantity<decltype(kelvin_per_meter)::unit_type, double> TemperatureGradient;


}}}

namespace si = boost::units::si;

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

extern double Pa;
extern double bar;

}

#endif // UNITS_H
