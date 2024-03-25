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
#include "base/parameters.h"
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

#include "analysis__Analysis__Parameters_headers.h"

namespace insight
{


class ParameterSetVisualizer;


#define addToAnalysisFactoryTable(DerivedClass) \
 defineType(DerivedClass); \
 addToFactoryTable(Analysis, DerivedClass); \
 addToStaticFunctionTable(Analysis, DerivedClass, defaultParameters); \
 addToStaticFunctionTable(Analysis, DerivedClass, category); \
 addToStaticFunctionTable(Analysis, DerivedClass, compatibleOperatingSystems);

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
            const boost::filesystem::path& exePath,
            ProgressDisplayer& displayer = consoleProgressDisplayer
        ),
        LIST(ps, exePath, displayer)
    );
    
    declareStaticFunctionTable(defaultParameters, ParameterSet);
    declareStaticFunctionTable(category, std::string);
    declareStaticFunctionTable(compatibleOperatingSystems, OperatingSystemSet);
    declareStaticFunctionTable(validator, ParameterSet_ValidatorPtr);
    declareStaticFunctionTable(visualizer, std::shared_ptr<ParameterSetVisualizer>);
    declareStaticFunctionTableWithArgs(getPropositionsForParameter, ParameterSet,
                                       LIST(const std::string&, const ParameterSet&),
                                       LIST(const std::string& parameterPath, const ParameterSet& currentParameterValues)
                                       );

    /**
     * @brief analyses
     * get the list of defined analyses
     * @return
     */
    static std::set<std::string> availableAnalysisTypes();


#include "analysis__Analysis__Parameters.h"
/*
PARAMETERSET>>> Analysis Parameters

<<<PARAMETERSET
*/


protected:
    std::string name_;
    std::string description_;
    boost::filesystem::path executionPath_;
    bool removeExecutionPath_;
    bool enforceExecutionPathRemovalBehaviour_;

    void setExecutionPath ( const boost::filesystem::path& exePath );

public:
    declareType ( "Analysis" );
    
    static std::string category();
    static OperatingSystemSet compatibleOperatingSystems();

    static ParameterSet_ValidatorPtr validator();
    static std::shared_ptr<ParameterSetVisualizer> visualizer();

    /**
     * @brief getPropositionsForParameter
     * return proposed values for a selected parameter in the parameter set
     * @param parameterPath
     * path of the parameter in the set, for which propositions are to be returned
     * @param currentParameterValues
     * the current values of all the parameters in the set
     * @return
     * a parameterset which contains only entries with type of the parameter in question.
     * Each entry represents a proposed value and the name of the parameter the label of
     * the proposed value.
     * An empty set means there are no propositions.
     */
    static ParameterSet getPropositionsForParameter(
        const std::string& parameterPath,
        const ParameterSet& currentParameterValues );


    /**
     * create analysis from components.
     * @param name Analysis name
     * @param description Short description of the analysis intent
     * @param ps Analysis parameter set
     * @param exePath Path of working directory. Empty path "" requests a temporary storage directory. If the directory is not existing, it will be created and removed when the analysis object is deleted. This behaviour can be overridden by calling setKeepExecutionDirectory or setRemoveExecutionDirectory.
     */
    Analysis(
        const std::string& name,
        const std::string& description,
        const ParameterSet& ps,
        const boost::filesystem::path& exePath,
        ProgressDisplayer& displayer = consoleProgressDisplayer
        );
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

    virtual ParameterSet parameters() const =0;

    virtual ResultSetPtr operator() ( ProgressDisplayer& displayer = consoleProgressDisplayer ) =0;
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




}

#endif // INSIGHT_ANALYSIS_H
