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
#include "base/parameters/selectionparameter.h"

namespace insight {


class SelectableSubsetParameter
    : public Parameter, public SelectionParameterInterface
{
public:
  typedef std::string key_type;
  typedef std::map<key_type, std::unique_ptr<ParameterSet> > ItemList;
  typedef ItemList value_type;

  typedef std::map< key_type, std::observer_ptr<ParameterSet> > EntryReferences;
  typedef std::map< key_type, std::unique_ptr<ParameterSet> > Entries;

protected:
  key_type selection_;
  ItemList value_;

public:
  declareType ( "selectableSubset" );

  SelectableSubsetParameter (
      const std::string& description,
      bool isHidden=false,
      bool isExpert=false,
      bool isNecessary=false,
      int order=0 );

  /**
   * Construct from components:
   * \param defaultSelection The key of the subset which is selected per default
   * \param defaultValue A map of key-subset pairs. Between these can be selected
   * \param description The description of the selection parameter
   */
  SelectableSubsetParameter (
      const key_type& defaultSelection,
      const EntryReferences& defaultValue,
      const std::string& description,
      bool isHidden=false,
      bool isExpert=false,
      bool isNecessary=false,
      int order=0 );

  SelectableSubsetParameter (
      const key_type& defaultSelection,
      Entries&& defaultValue,
      const std::string& description,
      bool isHidden=false,
      bool isExpert=false,
      bool isNecessary=false,
      int order=0 );

  void initialize() override;

  bool isDifferent(const Parameter& p) const override;

  std::vector<std::string> selectionKeys() const override;
  void setSelection(const key_type& nk) override;
  const key_type& selection() const override;

  // int indexOfSelection(const std::string& key) const;
  // int selectionIndex() const;
  // void setSelectionFromIndex(int);

  EntryReferences items() const;
  Entries copyItems() const;

  void addItem(key_type key, std::unique_ptr<ParameterSet>&& ps );

  inline ParameterSet& operator() ()
  {
    return * ( value_.find ( selection_ )->second );
  }

  inline const ParameterSet& operator() () const
  {
    return * ( value_.find ( selection_ )->second );
  }

  void setParametersForSelection(const key_type& key, const ParameterSet& ps);
  const ParameterSet& getParametersForSelection(const key_type& key) const;

  std::string latexRepresentation() const override;
  std::string plainTextRepresentation(int indent=0) const override;

  bool isPacked() const override;
  void pack() override;
  void unpack(const boost::filesystem::path& basePath) override;
  void clearPackedData() override;


  rapidxml::xml_node<>*
  appendToNode (
      const std::string& name,
      rapidxml::xml_document<>& doc,
      rapidxml::xml_node<>& node,
      boost::filesystem::path inputfilepath ) const override;

  void readFromNode (
      const std::string& name,
      rapidxml::xml_node<>& node,
      boost::filesystem::path inputfilepath ) override;

  std::unique_ptr<Parameter> clone (bool initialize) const override;
  void copyFrom(const Parameter& p) override;
  void operator=(const SelectableSubsetParameter& p);
  void extend ( const Parameter& op ) override;
  void merge ( const Parameter& other ) override;
#ifndef SWIG
  std::unique_ptr<Parameter> intersection(const Parameter &other) const override;
#endif


  int nChildren() const override;

  int childParameterIndex(
      const std::string& name ) const override;

  std::string childParameterName(
      int i,
      bool redirectArrayElementsToDefault=false ) const override;

  std::string childParameterName(
      const Parameter* childParam,
      bool redirectArrayElementsToDefault=false ) const override;

  Parameter& childParameterRef (
      int i ) override;

  const Parameter& childParameter(
      int i ) const override;

};

} // namespace insight

#endif // INSIGHT_SELECTABLESUBSETPARAMETER_H
