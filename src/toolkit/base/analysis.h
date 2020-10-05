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

#include "base/progressdisplayer.h"
#include "base/parameterset.h"
#include "base/factory.h"
#include "base/resultset.h"
#include "base/analysisstepcontrol.h"
#include "base/tools.h"
#include "boost/chrono/duration.hpp"

#include "base/progressdisplayer/textprogressdisplayer.h"

#include <queue>
#include <thread>

#include "base/boost_include.h"
#include "boost/thread.hpp"

namespace insight
{




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
    declareStaticFunctionTable(validator, ParameterSet_ValidatorPtr);
    declareStaticFunctionTable(visualizer, ParameterSet_VisualizerPtr);

protected:
    std::string name_;
    std::string description_;
    boost::filesystem::path executionPath_;
    bool removeExecutionPath_;
    bool enforceExecutionPathRemovalBehaviour_;
    ParameterSet parameters_;


    SharedPathList sharedSearchPath_;

    void extendSharedSearchPath ( const std::string& name );

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

    static ParameterSet_ValidatorPtr validator();
    static ParameterSet_VisualizerPtr visualizer();

    /**
     * create analysis from components.
     * @param name Analysis name
     * @param description Short description of the analysis intent
     * @param ps Analysis parameter set
     * @param exePath Path of working directory. Empty path "" requests a temporary storage directory. If the directory is not existing, it will be created and removed when the analysis object is deleted. This behaviour can be overridden by calling setKeepExecutionDirectory or setRemoveExecutionDirectory.
     */
    Analysis(const std::string& name, const std::string& description, const ParameterSet& ps, const boost::filesystem::path& exePath );
    virtual ~Analysis();

    virtual boost::filesystem::path setupExecutionEnvironment();

    virtual boost::filesystem::path executionPath() const;
    boost::filesystem::path createExecutionPathIfNonexistent();

    inline void setKeepExecutionDirectory(bool keep = true)
    {
        enforceExecutionPathRemovalBehaviour_=true;
        removeExecutionPath_=!keep;
    }
    
    inline void setRemoveExecutionDirectory(bool remove = true)
    {
        enforceExecutionPathRemovalBehaviour_=true;
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

    virtual ResultSetPtr operator() ( ProgressDisplayer& displayer = consoleProgressDisplayer ) =0;

    virtual boost::filesystem::path getSharedFilePath ( const boost::filesystem::path& file );

    virtual Analysis* clone() const;

};





typedef std::shared_ptr<Analysis> AnalysisPtr;





struct AnalysisInstance
{
  std::string name;
  AnalysisPtr analysis;
  ResultSetPtr results;
  std::exception_ptr exception;
};

typedef std::vector<AnalysisInstance> AnalysisInstanceList;


// Queue class that has thread synchronisation
class SynchronisedAnalysisQueue
{

private:
    std::queue<AnalysisInstance> m_queue; // Use STL queue to store data
    boost::mutex m_mutex; // The mutex to synchronise on
    boost::condition_variable m_cond; // The condition to wait for
    AnalysisInstanceList processed_;

public:

    // Add data to the queue and notify others
    void enqueue ( const AnalysisInstance& data );

    // Get data from the queue. Wait for data if not available
    AnalysisInstance dequeue();

    inline size_t n_instances() const
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

    inline const AnalysisInstanceList& processed() const
    {
        return processed_;
    }
};



/**
 * @brief The AnalysisWorkerThread class
 * Objects of this class work together with the SynchronizedAnalysisQueue.
 * The latter holds a pool of Analyses to process.
 * For each processor, an AnalysisWorkerThread object is created.
 * It grabs an Analysis form the queue, processes it and grabs the next until none is left.
 */
class AnalysisWorkerThread
    : boost::noncopyable
{
protected:
    ProgressDisplayer* displayer_;
    SynchronisedAnalysisQueue* queue_;

    /**
     * @brief exception
     * stores the exception, if any occurred
     */
    std::exception_ptr exception_;

    WarningDispatcher* mainThreadWarningDispatcher_;

public:
    /**
     * @brief AnalysisWorkerThread
     * @param queue
     * @param displayer
     * Constructs the worker.
     * This is expected to be executed in the main thread.
     */
    AnalysisWorkerThread ( SynchronisedAnalysisQueue* queue, ProgressDisplayer* displayer=nullptr );

    /**
     * @brief operator ()
     * Executes the job.
     * This function runs in a separate thread.
     */
    void operator() ();

    void rethrowIfNeeded() const;
};




/**
 * @brief The AnalysisThread class
 * This class wraps the execution of an analysis in a separate thread.
 * It is made sure, the warnings and progress info is properly propagated
 * to the main thread.
 */
class AnalysisThread
{

  boost::thread thread_;

protected:
  /**
   * @brief results_
   * stores the result set
   */
  ResultSetPtr results_;

  /**
   * @brief exception
   * stores the exception, if any occurred
   */
  std::exception_ptr exception_;

  std::function<void(std::exception_ptr)> exceptionHandler_;

  void launch(std::function<void(void)> action);

public:
  AnalysisThread(
      AnalysisPtr analysis,
      ProgressDisplayer* pd
#ifndef SWIG
      ,
      std::function<void(void)> preAction = []()->void {},
      std::function<void(void)> postAction = []()->void {},
      std::function<void(std::exception_ptr)> exHdlr = std::function<void(std::exception_ptr)>()
#endif
  );

#ifndef SWIG
  AnalysisThread(
      std::function<void(void)> action,
      std::function<void(std::exception_ptr)> exHdlr = std::function<void(std::exception_ptr)>()
  );
#endif

  void interrupt();

  /**
   * @brief join
   * join thread and rethrow any exception, if there was no handler set
   * @return
   */
  ResultSetPtr join();

  template <class Rep, class Period>
  bool try_join_for(const boost::chrono::duration<Rep, Period>& rel_time)
  {
    return thread_.try_join_for(rel_time);
  }
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
