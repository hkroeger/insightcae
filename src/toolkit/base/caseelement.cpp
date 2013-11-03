/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  hannes <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "base/caseelement.h"
#include "base/case.h"

namespace insight
{

CaseElement::CaseElement(Case& c, const std::string& name)
: name_(name),
  case_(c)
{

}

CaseElement::CaseElement(const CaseElement& other)
: name_(other.name_),
  case_(other.case_)
{

}

CaseElement::~CaseElement()
{

}

const ParameterSet& CaseElement::params() const
{ 
  return case_.params(); 
}

}
