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


double Compound::mass(double density_ovr, double aw_ovr) const
{
  checkForBuildDuringAccess();
  std::vector<FeaturePtr> sfs;
  BOOST_FOREACH(const CompoundFeatureMap::value_type& c, components_)
  {
    sfs.push_back(c.second);
  }
  double grho=density_ovr, gaw=aw_ovr;
  if ( density_ && (density_ovr<0) ) grho=density_->value();
  if ( areaWeight_ && (aw_ovr<0) ) gaw=areaWeight_->value();
  
  std::cout<<"Compound mass map:"<<std::endl;
  std::cout<<"=================="<<std::endl;
  double m=0.0;
  BOOST_FOREACH(const CompoundFeatureMap::value_type& c, components_)
  {
    double mc = c.second->mass(density_ovr, aw_ovr);
    m += mc;
    std::cout<<c.first<<":\t m="<<mc<<std::endl;
  }
  std::cout<<std::endl;
//   MassAndCoG mco=compoundProps(sfs, grho, gaw);
  
  return m;
}


arma::mat Compound::modelCoG(double density_ovr) const
{
  checkForBuildDuringAccess();
  std::vector<FeaturePtr> sfs;
  BOOST_FOREACH(const CompoundFeatureMap::value_type& c, components_)
  {
    sfs.push_back(c.second);
  }
  double grho=-1, gaw=-1;
  if ( density_  && (density_ovr<0) ) grho=density_->value();
  if ( areaWeight_ /*&& (aw_ovr<0)*/ ) gaw=areaWeight_->value();
  Mass_CoG_Inertia mco=compoundProps(sfs, grho, gaw);
  return boost::fusion::at_c<1>(mco);
}


arma::mat Compound::modelInertia(double density_ovr) const
{
  checkForBuildDuringAccess();
  std::vector<FeaturePtr> sfs;
  BOOST_FOREACH(const CompoundFeatureMap::value_type& c, components_)
  {
    sfs.push_back(c.second);
  }
  double grho=-1, gaw=-1;
  if ( density_  && (density_ovr<0) ) grho=density_->value();
  if ( areaWeight_ /*&& (aw_ovr<0)*/ ) gaw=areaWeight_->value();
  Mass_CoG_Inertia mco=compoundProps(sfs, grho, gaw);
  return boost::fusion::at_c<2>(mco);
}

}
}
