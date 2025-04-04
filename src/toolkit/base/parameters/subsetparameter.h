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




class ParameterNotFoundException
    : public Exception
{
public:
    ParameterNotFoundException(const std::string& msg);
};




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
        const std::string& description,
        bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    ParameterSet(
        Entries &&defaultValue,
        const std::string& description,
        bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    ParameterSet(
        const EntryReferences &defaultValue,
        const std::string& description,
        bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

public:
  declareType ( "subset" );;

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



  int nChildren() const override;

  std::string childParameterName(
      int i,
      bool redirectArrayElementsToDefault=false ) const override;

  std::string childParameterName(
      const Parameter *p,
      bool redirectArrayElementsToDefault=false ) const override;

  Parameter& childParameterRef ( int i ) override;

  const Parameter& childParameter( int i ) const override;


  size_t size() const;

  bool hasParameter( std::string path ) const;
  insight::Parameter& getParameter( std::string path );

  template<class T>
  T& get ( const std::string& name );

  template<class T>
  const T& get ( const std::string& name ) const
  {
      return const_cast<ParameterSet&>(*this).get<T>(name);
  }

  template<class T>
  const typename T::value_type& getOrDefault ( const std::string& name, const typename T::value_type& defaultValue ) const
  {
      try
      {
          return this->get<T> ( name ) ();
      }
      catch ( const ParameterNotFoundException& /*e*/ )
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

  std::unique_ptr<Parameter> clone(bool initialize) const override;

  inline std::unique_ptr<ParameterSet> cloneParameterSet() const
  {
      return std::dynamic_unique_ptr_cast<ParameterSet>(clone(true));
  }

  void copyFrom(const Parameter& o) override;
  void operator=(const ParameterSet& spo);
  void extend ( const Parameter& op ) override;
  void merge ( const Parameter& other ) override;
  void clear();
#ifndef SWIG
  std::unique_ptr<Parameter> intersection(const Parameter& other) const override;
#endif
};



#ifndef SWIG
std::ostream& operator<<(std::ostream& os, const ParameterSet& ps);
#endif



template<class T = insight::Parameter>
T& ParameterSet::get ( const std::string& name )
{
  typedef T PT;


  auto& p = this->getParameter(name);

  if ( PT* const pt=dynamic_cast<PT* const>(&p) )
  {
      return *pt;
  }
  else
  {
      throw insight::ParameterNotFoundException(
          "Parameter "+name+" not of requested type!"
                                " (actual type is "+p.type()+")"
          );
  }
}

}

#endif // SUBSETPARAMETER_H
