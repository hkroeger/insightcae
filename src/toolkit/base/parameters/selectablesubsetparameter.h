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

#ifndef INSIGHT_SELECTABLESUBSETPARAMETER_H
#define INSIGHT_SELECTABLESUBSETPARAMETER_H


#include "base/parameter.h"
#include "base/parameterset.h"


namespace insight {


class SelectableSubsetParameter
  : public Parameter,
    public SubParameterSet
{
public:
  typedef std::string key_type;
  typedef std::map<key_type, std::unique_ptr<ParameterSet> > ItemList;
  typedef ItemList value_type;

  typedef boost::tuple<key_type, ParameterSet*> SingleSubset;
  typedef std::vector< boost::tuple<key_type, ParameterSet*> > SubsetList;

protected:
  key_type selection_;
  ItemList value_;

public:
  declareType ( "selectableSubset" );

  SelectableSubsetParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
  /**
   * Construct from components:
   * \param defaultSelection The key of the subset which is selected per default
   * \param defaultValue A map of key-subset pairs. Between these can be selected
   * \param description The description of the selection parameter
   */
  SelectableSubsetParameter ( const key_type& defaultSelection, const SubsetList& defaultValue, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

  bool isDifferent(const Parameter& p) const override;

  inline key_type& selection()
  {
    return selection_;
  }

  inline const key_type& selection() const
  {
    return selection_;
  }

  inline const ItemList& items() const
  {
    return value_;
  }

  inline ItemList& items()
  {
    return value_;
  }

  void addItem ( key_type key, const ParameterSet& ps );

  inline ParameterSet& operator() ()
  {
    return * ( value_.find ( selection_ )->second );
  }

  inline const ParameterSet& operator() () const
  {
    return * ( value_.find ( selection_ )->second );
  }

  void setSelection(const key_type& key, const ParameterSet& ps);

  std::string latexRepresentation() const override;
  std::string plainTextRepresentation(int indent=0) const override;

  bool isPacked() const override;
  void pack() override;
  void unpack(const boost::filesystem::path& basePath) override;
  void clearPackedData() override;


  rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
      boost::filesystem::path inputfilepath ) const override;
  void readFromNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
                              boost::filesystem::path inputfilepath ) override;

  Parameter* clone () const override;
  void reset(const Parameter& p) override;

  const ParameterSet& subset() const override;
};

} // namespace insight

#endif // INSIGHT_SELECTABLESUBSETPARAMETER_H
