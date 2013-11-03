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


#ifndef INSIGHT_FACTORY_H
#define INSIGHT_FACTORY_H

#include "boost/ptr_container/ptr_map.hpp"

namespace insight {
  
struct NoParameters
{
};

template<class baseT, class paramS>
class Factory
{

public:
    virtual baseT* operator()(const paramS& p) const =0;
};

template<class baseT, class specT, class paramS>
class SpecFactory
: public Factory<baseT, paramS>
{
public:
  
  virtual baseT* operator()(const paramS& p) const
  {
    return new specT(p);
  }
};

#define declareFactoryTable(baseT, paramS) \
 typedef boost::ptr_map<std::string, Factory<baseT, paramS> > FactoryTable; \
 static FactoryTable factories_;

#define defineFactoryTable(baseT, paramS) \
 boost::ptr_map<std::string, Factory<baseT, paramS> > baseT::factories_;

#define addToFactoryTable(entry, baseT, specT, paramS) \
struct add##specT##To##baseT##FactoryTable \
{\
  add##specT##To##baseT##FactoryTable()\
  {\
    std::string key(entry); \
    std::cout << "Adding entry " << key << " to " #baseT "FactoryTable" << std::endl; \
    baseT::factories_.insert(key, new SpecFactory<baseT, specT, paramS>() ); \
  }\
} v_add##specT##To##baseT##FactoryTable;

}

#endif // INSIGHT_FACTORY_H
