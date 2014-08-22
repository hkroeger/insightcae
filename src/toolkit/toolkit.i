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
 
%module(directors="1") toolkit

%{
#include "base/analysis.h"
#include "base/parameter.h"
#include "base/parameterset.h"
%}

%feature("director") Analysis;

namespace insight
{

/*
  
class SubsetParameter;

class ParameterSet
: public boost::ptr_map<std::string, Parameter>
{
  
public:
  typedef boost::tuple<std::string, Parameter*> SingleEntry;
  typedef std::vector< boost::tuple<std::string, Parameter*> > EntryList;

public:
    ParameterSet();
    ParameterSet(const EntryList& entries);
    virtual ~ParameterSet();

    void extend(const EntryList& entries);

    inline bool contains(const std::string& name) const
    {
      const_iterator i = find(name);
      return (i!=end());
    }

    inline int& getInt(const std::string& name) { return this->get<IntParameter>(name)(); }
    inline double& getDouble(const std::string& name) { return this->get<DoubleParameter>(name)(); }
    inline bool& getBool(const std::string& name) { return this->get<BoolParameter>(name)(); }
    inline std::string& getString(const std::string& name) { return this->get<StringParameter>(name)(); }
    inline boost::filesystem::path& getPath(const std::string& name) 
    { return this->get<PathParameter>(name)(); }
    ParameterSet& getSubset(const std::string& name);

    inline const int& getInt(const std::string& name) const { return this->get<IntParameter>(name)(); }
    inline const double& getDouble(const std::string& name) const { return this->get<DoubleParameter>(name)(); }
    inline const bool& getBool(const std::string& name) const { return this->get<BoolParameter>(name)(); }
    inline const std::string& getString(const std::string& name) const { return this->get<StringParameter>(name)(); }
    inline const boost::filesystem::path& getPath(const std::string& name) const
    { return this->get<PathParameter>(name)(); }
    const ParameterSet& getSubset(const std::string& name) const;
    inline const ParameterSet& operator[](const std::string& name) const { return getSubset(name); }
    
    inline void replace(const std::string& key, Parameter* newp)
    {
      using namespace boost;
      using namespace boost::algorithm;
      
      if (boost::contains(key, "/"))
      {
	std::string prefix = copy_range<std::string>( *make_split_iterator(key, first_finder("/")) );
	std::string remain = key;
	erase_head(remain, prefix.size()+1);
	std::cout<<prefix<<" >> "<<remain<<std::endl;
	return this->getSubset(prefix).replace(remain, newp);
      }
      else
      {
	boost::ptr_map<std::string, Parameter>::replace(this->find(key), newp);
      }
    }
    
    virtual std::string latexRepresentation() const;

    virtual ParameterSet* clone() const;

    virtual void appendToNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const;
    virtual void readFromNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node);
    
    virtual void saveToFile(const boost::filesystem::path& file, std::string analysisType = std::string() ) const;
    virtual std::string readFromFile(const boost::filesystem::path& file);

};

typedef boost::shared_ptr<ParameterSet> ParameterSetPtr;

#define PSINT(p, subdict, key) int key = p[subdict].getInt(#key);
#define PSDBL(p, subdict, key) double key = p[subdict].getDouble(#key);
#define PSSTR(p, subdict, key) std::string key = p[subdict].getString(#key);
#define PSBOOL(p, subdict, key) bool key = p[subdict].getBool(#key);
#define PSPATH(p, subdict, key) boost::filesystem::path key = p[subdict].getPath(#key);


class SubsetParameter
: public Parameter
{
protected:
  boost::shared_ptr<ParameterSet> value_;
  
public:
  declareType("subset");
  
  SubsetParameter(const std::string& description);
  SubsetParameter(const ParameterSet& defaultValue, const std::string& description);
  
  inline void setParameterSet(const ParameterSet& paramset) { value_.reset(paramset.clone()); }
  inline ParameterSet& operator()() { return *value_; }
  inline const ParameterSet& operator()() const { return *value_; }
  
  virtual std::string latexRepresentation() const;
  
  virtual Parameter* clone () const;

  virtual rapidxml::xml_node<>* appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node) const;
  virtual void readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node);
};


typedef std::map<std::string, double> ProgressVariableList;
typedef std::pair<double, ProgressVariableList> ProgressState;  
  
class ProgressDisplayer
{
public:
  virtual void update(const ProgressState& pi) =0;
};

class Analysis
{
  
public:
  //declareFactoryTable(Analysis, NoParameters);
  
  typedef std::vector<boost::filesystem::path> PathList;
  
protected:
  std::string name_;
  std::string description_;
  DirectoryParameter executionPath_;
  ParameterSetPtr parameters_;
  
  PathList sharedSearchPath_;
  void extendSharedSearchPath(const std::string& name);
  
  virtual boost::filesystem::path setupExecutionEnvironment();

public:
//  declareType("Analysis");
  
  //Analysis();
  Analysis(const NoParameters&);
  Analysis(const std::string& name, const std::string& description);
  virtual ~Analysis();
  
  void setDefaults();
  virtual void setExecutionPath(boost::filesystem::path& exePath);
  virtual void setParameters(const ParameterSet& p);
  virtual boost::filesystem::path executionPath() const;
  
  inline DirectoryParameter& executionPathParameter() { return executionPath_; }
  
  inline const std::string& getName() const { return name_; }
  inline const std::string& getDescription() const { return description_; }
  inline std::string& description() { return description_; }
  
  inline const ParameterSet& p() const { return *parameters_; }

  virtual ParameterSet defaultParameters() const =0;
  
  virtual bool checkParameters(const ParameterSet& p);
  
  virtual ResultSetPtr operator()(ProgressDisplayer* displayer=NULL) =0;
  virtual void cancel() =0;
  
  virtual boost::filesystem::path getSharedFilePath(const boost::filesystem::path& file);
  
  virtual Analysis* clone() const;

};

typedef boost::shared_ptr<Analysis> AnalysisPtr;
*/
}