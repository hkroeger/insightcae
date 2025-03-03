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

#include "base/exception.h"
#include "base/progressdisplayer.h"
#include "base/parameters.h"
#include "base/parameterset.h"
#include "base/factory.h"
#include "base/resultset.h"
#include "base/analysisstepcontrol.h"
#include "base/tools.h"
#include "boost/chrono/duration.hpp"
#include "boost/range/algorithm/transform.hpp"

#include "base/progressdisplayer/textprogressdisplayer.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <queue>
#include <thread>

#include "base/boost_include.h"
#include "boost/thread.hpp"

#include "analysis__AnalysisWithParameters__Parameters_headers.h"

namespace insight
{



struct AnalysisDescription {
    std::string name;
    std::string description;
};

/**
 * An analysis is the basic container in InsightCAE.
 * It take a set of parameters and its designation is to computes from it a set of results.
 * An analysis always has an associated working directory for outputting intermediate files and result data.
 */
class Analysis
{


public:
    declareType ( "Analysis" );


    typedef
        insight::StaticFunctionTable<
            &typeName,
            std::unique_ptr<supplementedInputDataBase>,
            ParameterSetInput&&, const boost::filesystem::path&, ProgressDisplayer&
        >
        supplementedInputDataFactory;

    declareStaticFunctionTable2(supplementedInputDataFactory, createSupplementedInputDataFor);

    typedef
        insight::Factory<
            &typeName, Analysis,
            const std::shared_ptr<supplementedInputDataBase>&
        >
        AnalysisFactory;

    declareStaticFunctionTable2(AnalysisFactory, createAnalysis);


    typedef
        insight::StaticFunctionTable<
            &typeName,
            std::unique_ptr<ParameterSet>
            >
        defaultParametersFunctions;

    declareStaticFunctionTable2(defaultParametersFunctions, defaultParametersFor);


    typedef
        insight::StaticFunctionTable<
            &typeName,
            std::string
        >
        categoryFunctions;

    declareStaticFunctionTable2(categoryFunctions, categoryFor);


    typedef
        insight::StaticFunctionTable<
            &typeName,
            OperatingSystemSet
        >
        compatibleOperatingSystemsFunctions;

    declareStaticFunctionTable2(compatibleOperatingSystemsFunctions, compatibleOperatingSystemsFor);


    typedef
        insight::StaticFunctionTable<
            &typeName,
            ParameterSet_ValidatorPtr
        >
        validatorFunctions;

    declareStaticFunctionTable2(validatorFunctions, validatorFor);



    typedef
        insight::StaticFunctionTable<
            &typeName,
            std::unique_ptr<ParameterSet>,
            const std::string&, const ParameterSet&
        >
        getPropositionsForParameterFunctions;

    declareStaticFunctionTable2(getPropositionsForParameterFunctions, getPropositionsForParameterFor);


    typedef
        insight::StaticFunctionTable<
            &typeName, AnalysisDescription
        >
        DescriptionFunctions;

    declareStaticFunctionTable2(DescriptionFunctions, descriptionFor);



    template<class AnalysisInstance>
    struct Add
    {
        Add()
        {
            addToStaticFunctionTable2(
                Analysis, supplementedInputDataFactory, createSupplementedInputDataFor,
                AnalysisInstance,
                (&std::make_unique<
                    typename AnalysisInstance::supplementedInputData,
                    ParameterSetInput&&,
                    const boost::filesystem::path&,
                    ProgressDisplayer&>) );

            addToFactoryTable2(
                Analysis, AnalysisFactory, createAnalysis,
                AnalysisInstance );

            addToStaticFunctionTable2(
                Analysis, defaultParametersFunctions, defaultParametersFor,
                AnalysisInstance, &AnalysisInstance::defaultParameters );

            addToStaticFunctionTable2(
                Analysis, categoryFunctions, categoryFor,
                AnalysisInstance,
                &AnalysisInstance::category );

            addToStaticFunctionTable2(
                Analysis, compatibleOperatingSystemsFunctions, compatibleOperatingSystemsFor,
                AnalysisInstance, &AnalysisInstance::compatibleOperatingSystems );

            addToStaticFunctionTable2(
                Analysis, getPropositionsForParameterFunctions, getPropositionsForParameterFor,
                AnalysisInstance, &AnalysisInstance::getPropositionsForParameter);

            addToStaticFunctionTable2(
                Analysis, DescriptionFunctions, descriptionFor,
                AnalysisInstance,  &AnalysisInstance::description);
        }
    };

    /**
     * @brief analyses
     * get the list of defined analyses
     * @return
     */
    static std::set<std::string> availableAnalysisTypes(
        const std::set<std::string>& restrictToCategories = {} );


private:
    std::shared_ptr<supplementedInputDataBase> sp_;

protected:
    void resetParameters(
        std::shared_ptr<supplementedInputDataBase> sid );

public:
    
    static std::string category();
    static OperatingSystemSet compatibleOperatingSystems();

    static ParameterSet_ValidatorPtr validator();

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
    static std::unique_ptr<ParameterSet> getPropositionsForParameter(
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
        const std::shared_ptr<supplementedInputDataBase>& sp );

    virtual ~Analysis();

    static AnalysisDescription description();

    virtual boost::filesystem::path executionPath() const;
    virtual const ParameterSet& parameters() const;

    inline const supplementedInputDataBase& spBase() const
    {
        return *sp_;
    }

    virtual ResultSetPtr createResultSet() const;

    virtual ResultSetPtr operator() (
        ProgressDisplayer& displayer = consoleProgressDisplayer ) =0;
};





typedef std::shared_ptr<Analysis> AnalysisPtr;




class AnalysisWithParameters
    : public Analysis
{
public:

#include "analysis__AnalysisWithParameters__Parameters.h"
/*
PARAMETERSET>>> AnalysisWithParameters Parameters

userInformation = set {
 cause = string "" "Cause of the analysis."
 notes = string "" "Additional notes"
 version = int 1 ""
 modification = int 1 ""
}

<<<PARAMETERSET
*/

public:
    typedef supplementedInputDataDerived<Parameters> supplementedInputData;
    addParameterMembers_SupplementedInputData(AnalysisWithParameters::Parameters);

public:
    AnalysisWithParameters(
        const std::shared_ptr<supplementedInputDataBase>& sp
        );

    inline const supplementedInputDataFromParameters& spPBase() const
    {
        return
            dynamic_cast<const supplementedInputDataFromParameters&>(
              spBase() );
    }



};





struct AnalysisInstance
{
  std::string name;
  std::shared_ptr<ParameterSet> parameters_; // needs to exist, until analysis is finished
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
