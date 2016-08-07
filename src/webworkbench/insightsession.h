#ifndef INSIGHT_SESSION_H
#define INSIGHT_SESSION_H

#include "base/boost_include.h"

#include "base/analysis.h"

namespace insight
{
namespace web
{
 
class Session
{
    boost::filesystem::path dir_;
    
public:
    Session();
    ~Session();
    
    const boost::filesystem::path& dir() const { return dir_; }
    
    insight::AnalysisPtr analysis_;
    insight::ParameterSet parameters_;
};

typedef boost::shared_ptr<Session> SessionPtr;

}
}

#endif
