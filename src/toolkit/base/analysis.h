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


#ifndef INSIGHT_ANALYSIS_H
#define INSIGHT_ANALYSIS_H

#include "base/parameterset.h"
#include "base/factory.h"
#include "base/resultset.h"
#include "base/analysisstepcontrol.h"
#include "base/tools.h"

#include <queue>

#include "boost/thread.hpp"

namespace insight
{
  
    
    
    
class ProgressDisplayer;




typedef boost::shared_ptr<ProgressDisplayer> ProgressDisplayerPtr;
typedef std::map<std::string, double> ProgressVariableList;
typedef std::pair<double, ProgressVariableList> ProgressState;  




class ProgressDisplayer
{
public:
    virtual ~ProgressDisplayer();

    virtual void update ( const ProgressState& pi ) =0;

    virtual bool stopRun() const;
};




class CombinedProgressDisplayer
    : public ProgressDisplayer
{
public:
    typedef enum {AND, OR} Ops;
protected:
    std::vector<ProgressDisplayer*> displayers_;
    Ops op_;
public:
    CombinedProgressDisplayer ( Ops op );
    void add ( ProgressDisplayer* );
    virtual void update ( const ProgressState& pi );
    virtual bool stopRun() const;
};
 



class TextProgressDisplayer
    : public ProgressDisplayer
{
public:
    virtual void update ( const ProgressState& pi );
};




class ConvergenceAnalysisDisplayer
  : public ProgressDisplayer
{
  std::string progvar_;
  std::vector<double> trackedValues_;

  int istart_, co_;
  double threshold_;

  bool converged_;

public:
  ConvergenceAnalysisDisplayer ( const std::string &progvar, double threshold = 1e-5 );

  virtual void update ( const ProgressState &pi );

  virtual bool stopRun() const;
};




typedef boost::shared_ptr<ConvergenceAnalysisDisplayer> ConvergenceAnalysisDisplayerPtr;

#define addToAnalysisFactoryTable(DerivedClass) \
 defineType(DerivedClass); \
 addToFactoryTable(Analysis, DerivedClass); \
 addToStaticFunctionTable(Analysis, DerivedClass, defaultParameters); \
 addToStaticFunctionTable(Analysis, DerivedClass, category);

/**
 * An analysis is the basic container in InsightCAE.
 * It take a set of parameters and its designation is to computes from it a set of results.
 * An analysis always has an associated working directory for outputting intermediate files and result data.
 */
class Analysis
{

public:
    declareFactoryTable 
    (
        Analysis,
        LIST(
            const ParameterSet& ps,
            const boost::filesystem::path& exePath
        ),
        LIST(ps, exePath)
    );
    
    declareStaticFunctionTable(defaultParameters, ParameterSet);
    declareStaticFunctionTable(category, std::string);


protected:
    std::string name_;
    std::string description_;
    boost::filesystem::path executionPath_;
    bool removeExecutionPath_;
    ParameterSet parameters_;


    SharedPathList sharedSearchPath_;
    void extendSharedSearchPath ( const std::string& name );

    virtual boost::filesystem::path setupExecutionEnvironment();
    virtual void setExecutionPath ( const boost::filesystem::path& exePath );
    virtual void setParameters ( const ParameterSet& p );
    
    /**
     * check parameters
     * returns true, of parameters are valid
     */
    virtual bool checkParameters () const;

public:
    declareType ( "Analysis" );
    
    static std::string category();

    /**
     * create analysis from components.
     * @param name Analysis name
     * @param description Short description of the analysis intent
     * @param ps Analysis parameter set
     * @param exePath Path of working directory. Empty path "" requests a temporary storage directory. If the directory is not existing, it will be created and removed when the analysis object is deleted. This behaviour can be overridden by calling setKeepExecutionDirectory or setRemoveExecutionDirectory.
     */
    Analysis(const std::string& name, const std::string& description, const ParameterSet& ps, const boost::filesystem::path& exePath );
    virtual ~Analysis();

    virtual boost::filesystem::path executionPath() const;
    
    inline void setKeepExecutionDirectory(bool keep = true)
    {
        removeExecutionPath_=!keep;
    }
    
    inline void setRemoveExecutionDirectory(bool remove = true)
    {
        removeExecutionPath_=remove;
    }

    inline const std::string& getName() const
    {
        return name_;
    }
    inline std::string& name()
    {
        return name_;
    }
    inline std::string safe_name() const
    {
        std::string n ( getName() );
        boost::replace_all ( n, " ", "_" );
        boost::replace_all ( n, "/", "-" );
        return n;
    }

    inline const std::string& getDescription() const
    {
        return description_;
    }
    inline std::string& description()
    {
        return description_;
    }

    inline const ParameterSet& parameters() const
    {
        return parameters_;
    }

    virtual ResultSetPtr operator() ( ProgressDisplayer* displayer=NULL ) =0;
    virtual void cancel();

    virtual boost::filesystem::path getSharedFilePath ( const boost::filesystem::path& file );

    virtual Analysis* clone() const;

};





typedef boost::shared_ptr<Analysis> AnalysisPtr;




typedef boost::tuple<std::string, AnalysisPtr, ResultSetPtr> AnalysisInstance;




// Queue class that has thread synchronisation
class SynchronisedAnalysisQueue
{

private:
    std::queue<AnalysisInstance> m_queue; // Use STL queue to store data
    boost::mutex m_mutex; // The mutex to synchronise on
    boost::condition_variable m_cond; // The condition to wait for
    std::vector<AnalysisInstance> processed_;

public:
    // Add data to the queue and notify others
    void enqueue ( const AnalysisInstance& data );
    // Get data from the queue. Wait for data if not available
    AnalysisInstance dequeue();
    inline int n_instances() const
    {
        return m_queue.size();
    }

    inline void clear()
    {
        m_queue=std::queue<AnalysisInstance>();
        processed_.clear();
    }
    inline bool isEmpty()
    {
        return m_queue.size() ==0;
    }
    inline AnalysisInstance& front()
    {
        return m_queue.front();
    }

    void cancelAll();

    inline const std::vector<AnalysisInstance>& processed() const
    {
        return processed_;
    }
};




class CollectingProgressDisplayer
    : public ProgressDisplayer
{
    std::string id_;
    ProgressDisplayer* receiver_;

public:
    CollectingProgressDisplayer ( const std::string& id, ProgressDisplayer* receiver );
    virtual void update ( const ProgressState& pi );
};




class AnalysisWorkerThread
    : boost::noncopyable
{
protected:
    ProgressDisplayer* displayer_;
    SynchronisedAnalysisQueue* queue_;

public:
    AnalysisWorkerThread ( SynchronisedAnalysisQueue* queue, ProgressDisplayer* displayer=NULL );

    void operator() ();
//   void cancel(); // { analysis_.cancel(); }
};




class AnalysisLibraryLoader
{
protected:
    std::vector<void*> handles_;

public:
    AnalysisLibraryLoader();
    ~AnalysisLibraryLoader();
    
    void addLibrary(const boost::filesystem::path& lib);
};




extern AnalysisLibraryLoader loader;




}

#endif // INSIGHT_ANALYSIS_H
