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

#ifndef SUBSETPARAMETER_H
#define SUBSETPARAMETER_H

#include "base/parameter.h"
#include "base/parameterset.h"

namespace insight
{

class SubsetParameter
  : public Parameter,
    public ParameterSet,
    public SubParameterSet
{
public:
  typedef std::shared_ptr<SubsetParameter> Ptr;
  typedef ParameterSet value_type;

public:
  declareType ( "subset" );

  SubsetParameter();
  SubsetParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
  SubsetParameter ( const ParameterSet& defaultValue, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

//  inline void setParameterSet ( const ParameterSet& paramset )
//  {
//    this->setParameterSet ( paramset );
//  }

  bool isDifferent(const Parameter& p) const override;

  inline ParameterSet& operator() ()
  {
    return static_cast<ParameterSet&> ( *this );
  }

  inline const ParameterSet& operator() () const
  {
    return static_cast<const ParameterSet&> ( *this );
  }

  std::string latexRepresentation() const override;
  std::string plainTextRepresentation(int indent=0) const override;

  bool isPacked() const override;
  void pack() override;
  void unpack(const boost::filesystem::path& basePath) override;
  void clearPackedData() override;


  rapidxml::xml_node<>* appendToNode
  (
      const std::string& name,
      rapidxml::xml_document<>& doc,
      rapidxml::xml_node<>& node,
      boost::filesystem::path inputfilepath
  ) const override;

  void readFromNode
  (
      const std::string& name,
      rapidxml::xml_node<>& node,
      boost::filesystem::path inputfilepath
  ) override;

  Parameter* clone () const override;

  const ParameterSet& subset() const override;
  void merge(const SubParameterSet& other, bool allowInsertion) override;
  Parameter* intersection(const SubParameterSet& other) const override;
};


}

#endif // SUBSETPARAMETER_H
