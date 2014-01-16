/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  hannes <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef INSIGHT_OPENFOAMCASE_H
#define INSIGHT_OPENFOAMCASE_H

#include "base/case.h"
#include "openfoam/openfoamdict.h"
#include "base/softwareenvironment.h"

#include "boost/ptr_container/ptr_map.hpp"


namespace insight {

class ProgressDisplayer;
  
struct OFdicts
: public boost::ptr_map<std::string, OFDictData::dict> 
{
  OFDictData::dict& addDictionaryIfNonexistent(const std::string& key);
  OFDictData::dict& lookupDict(const std::string& key);
};

class OpenFOAMCase;

class OFEnvironment
: public SoftwareEnvironment
{
protected:
  int version_;
  boost::filesystem::path bashrc_;
  
public:
  OFEnvironment(int version, const boost::filesystem::path& bashrc);
  
  virtual int version() const;
  virtual const boost::filesystem::path& bashrc() const;
  //virtual int executeCommand(const std::vector<std::string>& args) const;
};


class OpenFOAMCaseElement
: public CaseElement
{

public:
  OpenFOAMCaseElement(OpenFOAMCase& c, const std::string& name);
 
  const OpenFOAMCase& OFcase() const;
  int OFversion() const;
  virtual void addIntoDictionaries(OFdicts& dictionaries) const =0;
};


enum FieldType
{
  scalarField,
  vectorField,
  symmTensorField
};

enum FieldGeoType
{
  volField,
  pointField,
  tetField
};


class SolverOutputAnalyzer
{
  
protected:
  ProgressDisplayer& pdisp_;
  
  double curTime_;
  std::map<std::string, double> curProgVars_;
  
public:
  SolverOutputAnalyzer(ProgressDisplayer& pdisp);
  
  void update(const std::string& line);
};


extern const OFDictData::dimensionSet dimPressure;
extern const OFDictData::dimensionSet dimKinPressure;
extern const OFDictData::dimensionSet dimKinEnergy;
extern const OFDictData::dimensionSet dimVelocity;
extern const OFDictData::dimensionSet dimLength;
extern const OFDictData::dimensionSet dimDensity;

typedef std::vector<double> FieldValue;
typedef boost::fusion::tuple<FieldType, OFDictData::dimensionSet, FieldValue> FieldInfo;

typedef std::map<std::string, FieldInfo> FieldList;

class OpenFOAMCase 
: public Case
{
protected:
  const OFEnvironment& env_;
  FieldList fields_;
  
public:
    OpenFOAMCase(const OFEnvironment& env);
    OpenFOAMCase(const OpenFOAMCase& other);
    virtual ~OpenFOAMCase();
    
    void addField(const std::string& name, const FieldInfo& field);

    void parseBoundaryDict(const boost::filesystem::path& location, OFDictData::dict& boundaryDict);
    void addRemainingBCs(OFDictData::dict& boundaryDict);
    inline int OFversion() const { return env_.version(); }
    
    boost::shared_ptr<OFdicts> createDictionaries() const;
    
    virtual void createOnDisk(const boost::filesystem::path& location, boost::shared_ptr<OFdicts> dictionaries );
    virtual void createOnDisk(const boost::filesystem::path& location );
    
    virtual bool meshPresentOnDisk( const boost::filesystem::path& location ) const;
    virtual bool outputTimesPresentOnDisk( const boost::filesystem::path& location ) const;
    
    std::string cmdString
    (
      const boost::filesystem::path& location, 
      const std::string& cmd,
      std::vector<std::string> argv
    )
    const;

    int executeCommand
    (
      const boost::filesystem::path& location, 
      const std::string& cmd,
      std::vector<std::string> argv = std::vector<std::string>(),
      std::vector<std::string>* output = NULL
    ) const;
    
    int runSolver
    (
      const boost::filesystem::path& location, 
      SolverOutputAnalyzer& analyzer,
      std::string solverName,
      bool *stopFlag = NULL
    );
    
    template<class stream>
    void forkCommand
    (
      stream& p_in,      
      const boost::filesystem::path& location, 
      const std::string& cmd, 
      std::vector<std::string> argv = std::vector<std::string>()
    ) const
    {
      env_.forkCommand(p_in, cmdString(location, cmd, argv), std::vector<std::string>());
    }
    
    inline const FieldList& fields() const
    {
      return fields_;
    }  
    inline FieldList& fields()
    {
      return fields_;
    }  
};

}

#endif // INSIGHT_OPENFOAMCASE_H
