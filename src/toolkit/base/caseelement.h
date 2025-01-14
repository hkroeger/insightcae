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


#ifndef CASEELEMENT_H
#define CASEELEMENT_H

#include <string>

#include "base/softwareenvironment.h"
#include "base/parameterset.h"
#include "base/parameters.h"
#include <boost/typeof/typeof.hpp>
#include "caseelement__CaseElement__Parameters_headers.h"

namespace insight
{
  
class Case;


#define addToCaseElementFactoryTable(DerivedClass) \
 addToStaticFunctionTable(CaseElement, DerivedClass, isInConflict ); \


class CaseElement
{

public:

  declareStaticFunctionTableWithArgs(
        isInConflict, bool,
        LIST(const insight::CaseElement&),
        LIST(const insight::CaseElement& e) );

  declareType ( "CaseElement" );
  
public:
#include "caseelement__CaseElement__Parameters.h"
/*
PARAMETERSET>>> CaseElement Parameters

name = string "" "Name of the case element"

createGetter
<<<PARAMETERSET
*/
private:
  Case& case_;

public:
    CaseElement(Case& c, ParameterSetInput ip = ParameterSetInput() );
    CaseElement(const CaseElement& other);
    virtual ~CaseElement();

    inline const std::string& name() const { return p().name; };
    // virtual void rename(const std::string& name);

    // const ParameterSet& parameters() const;

    template<class P = Parameters>
    P& tweakParameters()
    {
        return dynamic_cast<P&>(p_.tweakParameters());
    }

    const Case& get_case() const;
    Case& caseRef();

    static bool isInConflict(const CaseElement& other);

};

}

#endif // CASEELEMENT_H
