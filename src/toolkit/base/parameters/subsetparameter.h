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

#include "base/cppextensions.h"
#include "base/parameter.h"



namespace insight
{




class ParameterSet
  : public Parameter
{
// #warning should be removed
  friend class Parameter; // for constructor table

public:
  typedef std::map<std::string, std::unique_ptr<Parameter> > value_type;

  typedef std::map<std::string, std::unique_ptr<Parameter> > Entries;
  typedef std::map<std::string, std::observer_ptr<Parameter> > EntryReferences;

private:
  value_type value_;

    std::key_observer_map<
      Parameter,
      std::shared_ptr<boost::signals2::scoped_connection>
      >
        valueChangedConnections_,
        childValueChangedConnections_;

protected:
    ParameterSet();


    ParameterSet(
        Entries &&defaultValue,
        const std::string& description,
        bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    ParameterSet(
        const EntryReferences &defaultValue,
        const std::string& description,
        bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

public:
  declareType ( "subset" );

  ParameterSet(
        const rapidxml::xml_node<> & node );

  ParameterSet(
      const std::string& description,
      bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

  static std::unique_ptr<ParameterSet> create();

  static std::unique_ptr<ParameterSet> create(
      const std::string& description,
      bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

  static std::unique_ptr<ParameterSet> create_uninitialized(
      const std::string& description,
      bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

  static std::unique_ptr<ParameterSet> create(
      Entries &&defaultValue,
      const std::string& description,
      bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

  static std::unique_ptr<ParameterSet> create(
      const EntryReferences &defaultValue,
      const std::string& description,
      bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

  static std::unique_ptr<ParameterSet> create_uninitialized(
      const EntryReferences &defaultValue,
      const std::string& description,
      bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );


  EntryReferences entries() const;
  Entries copyEntries() const;

  bool isDifferent(const Parameter& p) const override;


  void insert(const std::string &name, std::unique_ptr<Parameter>&& p);
  void remove(const std::string& name);

  // for interchangeability in arrays with selectablesubset
  inline ParameterSet& operator() ()
  {
      return *this;
  }
  inline const ParameterSet& operator() () const
  {
      return *this;
  }

  std::string latexRepresentation(
      const std::string& name,
      int documentHierarchyLevel,
      const FileStorageInfo& fsi ) const override;

  std::string plainTextRepresentation(int indent) const override;

  bool isPacked() const override;
  void pack() override;
  void unpack(const boost::filesystem::path& basePath) override;
  void clearPackedData() override;


  rapidxml::xml_node<>* appendToNode
  (
      const std::string& name,
      rapidxml::xml_document<>& doc,
      rapidxml::xml_node<>& node
  ) const override;

  const rapidxml::xml_node<>* readFromNode
  (
      const std::string& name,
      const rapidxml::xml_node<>& node
  ) override;



  int nChildren() const override;

  std::string childElementName(
      int i,
      bool redirectArrayElementsToDefault=false ) const override;

  std::string childElementName(
      const Element *p,
      bool redirectArrayElementsToDefault=false ) const override;

  Element& childElementRef ( int i ) override;

  const Element& childElement( int i ) const override;

  size_t size() const;

  template<class T>
  const typename T::value_type& getOrDefault ( const std::string& name, const typename T::value_type& defaultValue ) const
  {
      try
      {
          return this->get<T> ( name ) ();
      }
      catch ( const ElementNotFoundException& /*e*/ )
      {
          return defaultValue;
      }
  }

  bool contains ( const std::string& name ) const;

  std::istream& getFileStream ( const std::string& name );

  ParameterSet& setInt ( const std::string& name, int v );
  ParameterSet& setDouble ( const std::string& name, double v );
  ParameterSet& setBool ( const std::string& name, bool v );
  ParameterSet& setString ( const std::string& name, const std::string& v );
  ParameterSet& setVector ( const std::string& name, const arma::mat& v );
  ParameterSet& setMatrix ( const std::string& name, const arma::mat& m );
  ParameterSet& setOriginalFileName ( const std::string& name, const boost::filesystem::path& fp);

  ParameterSet& getSubset ( const std::string& name );

  const int& getInt ( const std::string& name ) const;
  const double& getDouble ( const std::string& name ) const;
  const bool& getBool ( const std::string& name ) const;
  const std::string& getString ( const std::string& name ) const;
  const arma::mat& getVector ( const std::string& name ) const;
  const boost::filesystem::path getPath ( const std::string& name, const boost::filesystem::path& basePath = "" ) const;
  const ParameterSet& getSubset ( const std::string& name ) const;

  const ParameterSet& operator[] ( const std::string& name ) const;

  void replace ( const std::string& key, std::unique_ptr<Parameter> newp );

  std::unique_ptr<Element> clone() const override;

  void assignFrom( const Element& rhs ) override;
  void copyMatching( const Element& o ) override;
  void extend ( const Element& op ) override;
  bool isEqual(const Element& op) const override;

  void clear();

#ifndef SWIG
  std::unique_ptr<Parameter> intersection(const Parameter& other) const override;
#endif
};



#ifndef SWIG
std::ostream& operator<<(std::ostream& os, const ParameterSet& ps);
#endif




}

#endif // SUBSETPARAMETER_H
