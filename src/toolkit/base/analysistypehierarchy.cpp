#include "analysistypehierarchy.h"

namespace insight {


AnalysisTypeHierarchy::AnalysisTypeHierarchy()
{}




AnalysisTypeHierarchy &AnalysisTypeHierarchy::global()
{
    static AnalysisTypeHierarchy theHierarchy;
    return theHierarchy;
}




void AnalysisTypeHierarchy::registerType(
    const std::string &typeName,
    Thrower thrower,
    Catcher catcher )
{
    throwers_[typeName]=thrower;
    catchers_[typeName]=catcher;
}




bool AnalysisTypeHierarchy::isRegistered(const std::string &typeName) const
{
    return throwers_.count(typeName)>0;
}




bool AnalysisTypeHierarchy::isDerivedFrom(
    const std::string &analysisName,
    const std::string &baseTypeName ) const
{
    if (analysisName==baseTypeName)
        return true;

    auto t = throwers_.find(analysisName);
    auto c = catchers_.find(baseTypeName);
    if ( t==throwers_.end() || c==catchers_.end() )
        return false;

    return c->second(t->second);
}


} // namespace insight
