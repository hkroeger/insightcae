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


#ifndef INSIGHT_PARAMETERSET_H
#define INSIGHT_PARAMETERSET_H

#include <memory>
#include <map>
#include <vector>
#include <iostream>
#include <algorithm>

#include "base/exception.h"
#include "base/parameter.h"
#include "base/parameters/arrayparameter.h"
#include "base/progressdisplayer.h"
#include "base/progressdisplayer/textprogressdisplayer.h"

#include "boost/tuple/tuple.hpp"
#include "boost/fusion/tuple.hpp"
#include "boost/algorithm/string.hpp"

#include "rapidxml/rapidxml.hpp"


class QoccViewWidget;
class QModelTree;
class QIcon;


namespace insight {
  



class SubsetParameter;
class SelectableSubsetParameter;




/**
 * @brief The SubParameterSet class
 * common base class for parameters that contain a set of parameters
 */
class SubParameterSet
{
public:
  virtual ~SubParameterSet();

  ParameterSet& subsetRef();
  virtual const ParameterSet& subset() const =0;
};



std::string splitOffFirstParameter(std::string& path, int& nRemaining);



class ParameterSet
  : public std::map<std::string, std::unique_ptr<Parameter> >
{

public:
  typedef std::shared_ptr<ParameterSet> Ptr;
  typedef boost::tuple<std::string, Parameter*> SingleEntry;
  typedef std::vector< boost::tuple<std::string, Parameter*> > EntryList;

protected:
  SimpleLatex parameterSetDescription_;

public:
  ParameterSet();
  ParameterSet ( const ParameterSet& o );
  ParameterSet ( const EntryList& entries );
  virtual ~ParameterSet();

  bool isDifferent(const ParameterSet& p) const;

  void setParameterSetDescription(const std::string& desc);
  const SimpleLatex& parameterSetDescription() const;

  void operator=(const ParameterSet& o);

  EntryList entries() const;

  /**
   * insert values from entries, that are not present.
   * Do not overwrite entries!
   */
  void extend ( const EntryList& entries );

  /**
   * insert values from other, overwrite where possible.
   * return a non-const reference to this PS to anable call chains like PS.merge().merge()...
   */
  ParameterSet& merge ( const ParameterSet& other );

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
    catch ( const std::exception& /*e*/ )
      {
        return defaultValue;
      }
  }

  inline bool contains ( const std::string& name ) const
  {
    const_iterator i = find ( name );
    return ( i!=end() );
  }

  int& getInt ( const std::string& name );
  double& getDouble ( const std::string& name );
  bool& getBool ( const std::string& name );
  std::string& getString ( const std::string& name );
  arma::mat& getVector ( const std::string& name );
  arma::mat& getMatrix ( const std::string& name );
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

  inline void replace ( const std::string& key, Parameter* newp )
  {
    using namespace boost;
    using namespace boost::algorithm;

    if ( boost::contains ( key, "/" ) )
      {
        std::string prefix = copy_range<std::string> ( *make_split_iterator ( key, first_finder ( "/" ) ) );
        std::string remain = key;
        erase_head ( remain, prefix.size()+1 );
        return this->getSubset ( prefix ).replace ( remain, newp );
      }
    else
      {
        this->find(key)->second.reset(newp);
      }
  }

//  /**
//   * set selection of selectable subset parameter to typename of T and its dictionary to the parameters of p
//   */
//  template<class T>
//  ParameterSet& setSelectableSubset(const std::string& key, const typename T::Parameters& p);


  virtual std::string latexRepresentation() const;
  virtual std::string plainTextRepresentation(int indent=0) const;

  virtual ParameterSet* cloneParameterSet() const;

  virtual void appendToNode ( rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
                              boost::filesystem::path inputfilepath ) const;
  virtual void readFromNode ( rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
                              boost::filesystem::path inputfilepath );


  void packExternalFiles();
  void removePackedData();

  virtual void saveToStream(std::ostream& os, const boost::filesystem::path& parentPath, std::string analysisName = std::string() ) const;
  void saveToFile ( const boost::filesystem::path& file, std::string analysisType = std::string() ) const;
  virtual std::string readFromFile ( const boost::filesystem::path& file );

};


//class AtomicParameterSet
//{
//  mutable boost::mutex mx_;
//  ParameterSet ps_;

//public:
//  void operator=(const ParameterSet& ps);

//  template<class StaticParameters>
//  std::unique_ptr<StaticParameters> get() const
//  {
//    boost::mutex::scoped_lock lck(mx_);
//    return std::unique_ptr<StaticParameters>(new StaticParameters(ps_));
//  }

//  operator ParameterSet&()
//  {
//    return ps_;
//  }
//};



#ifndef SWIG
std::ostream& operator<<(std::ostream& os, const ParameterSet& ps);
#endif


typedef std::shared_ptr<ParameterSet> ParameterSetPtr;

//#define PSINT(p, subdict, key) int key = p[subdict].getInt(#key);
//#define PSDBL(p, subdict, key) double key = p[subdict].getDouble(#key);
//#define PSSTR(p, subdict, key) std::string key = p[subdict].getString(#key);
//#define PSBOOL(p, subdict, key) bool key = p[subdict].getBool(#key);
//#define PSPATH(p, subdict, key) boost::filesystem::path key = p[subdict].getPath(#key);





//template<class T>
//ParameterSet& ParameterSet::setSelectableSubset(const std::string& key, const typename T::Parameters& p)
//{
//    SelectableSubsetParameter& ssp = this->get<SelectableSubsetParameter>(key);
//    ssp.selection()=p.type();
//    ssp().merge(p);
//    return *this;
//}




class ParameterSet_Validator
{
public:
    typedef std::map<std::string, std::string> WarningList;
    typedef std::map<std::string, std::string> ErrorList;

protected:
    ParameterSet ps_;


    WarningList warnings_;
    ErrorList errors_;

public:
    virtual ~ParameterSet_Validator();

    /**
     * @brief update
     * @param ps
     * checks a parameter set (needs to be customized by derivation for that)
     * Stores a copy of the parameter set and updates the warnings and errors list.
     * Needs to be called first in derived classes.
     */
    virtual void update(const ParameterSet& ps);

    virtual bool isValid() const;
    virtual const WarningList& warnings() const;
    virtual const ErrorList& errors() const;
};


typedef std::shared_ptr<ParameterSet_Validator> ParameterSet_ValidatorPtr;





class ParameterSet_Visualizer
{

  // not linked to CAD; don't use any non-forward definitions from CAD module
private:
    TextProgressDisplayer defaultProgressDisplayer_;
    std::unique_ptr<ParameterSet> scheduledParameters_, visualizedParameters_;

protected:
    ProgressDisplayer* progress_;

public:
    virtual bool hasScheduledParameters() const;
    virtual const ParameterSet& currentParameters() const;
    virtual bool selectScheduledParameters();
    virtual void clearScheduledParameters();

    ParameterSet_Visualizer();
    virtual ~ParameterSet_Visualizer();

    /**
     * @brief update
     * @param ps
     * updates the parameterset which is to visualize.
     * This triggers recomputation of visualization features (from insight::cad) for several parameters.
     */
    virtual void update(const ParameterSet& ps);

    virtual void setIcon(QIcon* icon);

    void setProgressDisplayer(ProgressDisplayer* pd);
};

typedef std::shared_ptr<ParameterSet_Visualizer> ParameterSet_VisualizerPtr;




template<class T>
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
    throw insight::Exception(
          "Parameter "+name+" not of requested type!"
          " (actual type is "+p.type()+")"
          );
  }
}

}


#endif // INSIGHT_PARAMETERSET_H
