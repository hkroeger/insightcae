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


#ifndef INSIGHT_ANALYSIS_H
#define INSIGHT_ANALYSIS_H

#include "base/parameterset.h"
#include "base/factory.h"
#include "base/resultset.h"

#include <queue>

#include "boost/thread.hpp"

namespace insight
{

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
  declareFactoryTable(Analysis, NoParameters);
  
protected:
  std::string name_;
  std::string description_;

public:
  declareType("Analysis");
  
  //Analysis();
  Analysis(const NoParameters&);
  Analysis(const std::string& name, const std::string& description);
  virtual ~Analysis();
  
  inline const std::string& getName() const { return name_; }
  inline const std::string& getDescription() const { return description_; }

  virtual ParameterSet defaultParameters() const =0;
  
  virtual bool checkParameters(const ParameterSet& p);
  
  virtual ResultSetPtr operator()(const ParameterSet& p, ProgressDisplayer* displayer=NULL) =0;
  virtual void cancel() =0;
  

};

typedef boost::tuple<std::string, ParameterSetPtr, ResultSetPtr> AnalysisInstance;

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
    void enqueue(const AnalysisInstance& data);
    // Get data from the queue. Wait for data if not available
    AnalysisInstance dequeue();
    
    inline void clear() { m_queue=std::queue<AnalysisInstance>(); processed_.clear(); }
    inline bool isEmpty() { return m_queue.size()==0; }
    
    inline const std::vector<AnalysisInstance>& processed() const { return processed_; }
};


class AnalysisWorkerThread
: boost::noncopyable
{
protected:
  Analysis& analysis_;
  ProgressDisplayer* displayer_;
  SynchronisedAnalysisQueue* queue_;

public:
  AnalysisWorkerThread(SynchronisedAnalysisQueue* queue, Analysis& analysis, ProgressDisplayer* displayer=NULL);
  
  void operator()();
  inline void cancel() { analysis_.cancel(); }
};

class AnalysisLibraryLoader
{
public:
  AnalysisLibraryLoader();
};

extern AnalysisLibraryLoader loader;

}

#endif // INSIGHT_ANALYSIS_H
