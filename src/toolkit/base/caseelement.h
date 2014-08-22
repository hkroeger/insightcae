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
#include <boost/typeof/typeof.hpp>

/*
#define DECL_ELEM_PARAMETERS() \
public: \
template <RT> struct TParameters

#define DEF_ELEM_PARAMETERS(classname) \
public: \
typdef TParameters<classname> Parameters; \
protected: \
Parameters p_;

#define ADD_ELEM_PARAMETER(type, name) \
 boost::value_initialized< type > name; \
 inline RT& set_##name(const type& value) { name.data()=value; return static_cast<RT>(*this); }
 */

namespace insight
{
  
class Case;

class CaseElement
{
  
protected:
  std::string name_;
  Case& case_;

public:
    CaseElement(Case& c, const std::string& name);
    CaseElement(const CaseElement& other);
    virtual ~CaseElement();

    inline const std::string& name() const { return name_; };
    const ParameterSet& params() const;

};

}

#endif // CASEELEMENT_H
