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


#ifndef CASE_H
#define CASE_H

#include "base/parameterset.h"
#include "base/caseelement.h"

#include "boost/ptr_container/ptr_vector.hpp"

namespace insight
{

class Case
{
  
protected:
  ParameterSet parameters_;
  boost::ptr_vector<CaseElement> elements_;

public:
    Case();
    Case(const Case& other);
    virtual ~Case();
    
    void insert(CaseElement* elem);
    
    template<class T>
    T* get(const std::string& name)
    {
      return NULL;
    }
    
    inline const ParameterSet& params() const
    { return parameters_; }
    
    virtual void createOnDisk(const boost::filesystem::path& location) =0;
};

}

#endif // CASE_H
