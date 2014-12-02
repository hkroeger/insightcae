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
 *
 */

#ifndef INSIGHT_ELECTROMAGNETICSCASEELEMENTS_H
#define INSIGHT_ELECTROMAGNETICSCASEELEMENTS_H

#include "base/linearalgebra.h"
#include "base/parameterset.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/boundaryconditioncaseelements.h"

#include <map>
#include "boost/utility.hpp"
#include "boost/variant.hpp"
#include "progrock/cppx/collections/options_boosted.h"

namespace insight 
{

class magnet
: public OpenFOAMCaseElement
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (name, std::string, "magnet")
    (permeability, double, 0.2)
    (remanence, double, 1000.0)
    (orientation, arma::mat, vec3(1,0,0) )
  )

protected:
  Parameters p_;
  
public:
  magnet(OpenFOAMCase& c, Parameters const& p = Parameters() );
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
  virtual void modifyCaseOnDisk(const OpenFOAMCase& cm, const boost::filesystem::path& location) const;
};


class FarFieldBC
: public BoundaryCondition
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
   (dummy, std::string, "")
  )
  
protected:
  Parameters p_;
  
public:
  FarFieldBC
  (
    OpenFOAMCase& c,
    const std::string& patchName, 
    const OFDictData::dict& boundaryDict, 
    Parameters const& p = Parameters()
  );
  virtual void addIntoFieldDictionaries(OFdicts& dictionaries) const;
};

}

#endif