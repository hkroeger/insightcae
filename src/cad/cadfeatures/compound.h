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

#ifndef INSIGHT_CAD_COMPOUND_H
#define INSIGHT_CAD_COMPOUND_H

#include "cadfeature.h"

namespace insight 
{
namespace cad 
{


    
    
typedef std::vector<FeaturePtr> CompoundFeatureList;
typedef std::map<std::string, FeaturePtr> CompoundFeatureMap;




class Compound
    : public Feature
{
protected:
    CompoundFeatureMap components_;

    Compound ( const CompoundFeatureList& m1 );
    Compound ( const CompoundFeatureMap& m1 );

public:
    declareType ( "Compound" );
    Compound ( const NoParameters& nop = NoParameters() );
    
    static FeaturePtr create( const CompoundFeatureList& m1 );
    static FeaturePtr create_map( const CompoundFeatureMap& m1 );

    virtual void build();
    virtual void insertrule ( parser::ISCADParser& ruleset ) const;
    virtual FeatureCmdInfoList ruleDocumentation() const;

    virtual double mass ( double density_ovr=-1., double aw_ovr=-1. ) const;
    virtual arma::mat modelCoG ( double density_ovr=-1. ) const;
    virtual arma::mat modelInertia ( double density_ovr=-1. ) const;
};




}
}

#endif // INSIGHT_CAD_COMPOUND_H
