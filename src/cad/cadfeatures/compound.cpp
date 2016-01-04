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

#include "cadfeature.h"
#include "compound.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{
  

defineType(Compound);
addToFactoryTable(Feature, Compound, NoParameters);

Compound::Compound(const NoParameters& nop)
: Feature(nop)
{}


Compound::Compound(const CompoundFeatureList& m1)
{
  for (size_t i=0; i<m1.size(); i++)
    components_[str( format("component%d") % (i+1) )] = m1[i];
}

Compound::Compound(const CompoundFeatureMap& m1)
: components_(m1)
{}


void Compound::build()
{
  BRep_Builder bb;
  TopoDS_Compound result;
  bb.MakeCompound(result);

  BOOST_FOREACH(const CompoundFeatureMap::value_type& c, components_)
  {
    std::string name=c.first;
    FeaturePtr p=c.second;
    
    bb.Add(result, *p);
    p->unsetLeaf();
    copyDatums(*p, name+"_");
    
    providedSubshapes_[c.first]=c.second;
  }
  setShape(result);
}

void Compound::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Compound",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ( ruleset.r_solidmodel_expression % ',' ) > ')' ) 
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Compound>(qi::_1)) ]
      
    ))
  );
}

arma::mat Compound::modelCoG() const
{
  if (explicitCoG_.get())
  {
    return *explicitCoG_;
  }
  else
  {
    double mtot=0.0;
    arma::mat cog=vec3(0,0,0);
    
    BOOST_FOREACH(const CompoundFeatureMap::value_type& c, components_)
    {
      std::string name=c.first;
      FeaturePtr p=c.second;
      const Feature& m = *p;
      
      mtot+=m.mass();
      cog += m.modelCoG()*m.mass();
    }
    
    cout<<"total mass="<<mtot<<endl;
    
    if (mtot<1e-10)
      throw insight::Exception("Total mass is zero!");
    
    cog/=mtot;
    
    cout<<"CoG="<<cog<<endl;
    
    return cog;
  }
}

double Compound::mass() const
{
  if (explicitMass_)
  {
    return *explicitMass_;
  }
  else
  {
    double mtot=0.0;
    
    BOOST_FOREACH(const CompoundFeatureMap::value_type& c, components_)
    {
      std::string name=c.first;
      FeaturePtr p=c.second;
      const Feature& m = *p;
      mtot+=m.mass();
    }
    return mtot;
  }
}


}
}
