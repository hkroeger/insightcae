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


#include "base/caseelement.h"
#include "base/case.h"

namespace insight
{


defineType(CaseElement);
defineStaticFunctionTableWithArgs(
    CaseElement,
    isInConflict, bool,
    LIST(const insight::CaseElement& e),
    LIST(e)
    );

CaseElement::CaseElement(Case& c, ParameterSetInput ip)
: case_(c),
    p_(ip.forward<Parameters>())
{

}

CaseElement::CaseElement(const CaseElement& other)
: case_(other.case_),
    p_(other.p_.clone())
{

}

CaseElement::~CaseElement()
{}



// void CaseElement::rename(const std::string& name)
// {
//   name_=name;
// }


// const ParameterSet& CaseElement::parameters() const
// {
//   return ps_;
// }

const Case& CaseElement::get_case() const
{
    return case_;
}


Case& CaseElement::caseRef()
{
    return case_;
}


bool CaseElement::isInConflict(const CaseElement&)
{
  return false;
}

}
