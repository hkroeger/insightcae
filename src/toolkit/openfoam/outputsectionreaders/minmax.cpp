#include "minmax.h"
#include "base/tools.h"


namespace insight {
namespace OutputSectionReaders {




defineType(MinMax);
addToStaticFunctionTable2(
    OutputSectionReader,
    Readers, createIfMatches,
    MinMax, &MinMax::createIfMatches);




boost::regex re_intro("^fieldMinMax (.+) write:$");
boost::regex re_mima("^ *(min|max)\\((.+)\\) = (.+) in cell (.+) at location \\((.+) (.+) (.+)\\)");




MinMax::MinMax(const std::string& label)
    : label_(label)
{}




std::unique_ptr<OutputSectionReader> MinMax::createIfMatches(
    const std::string& line )
{
    boost::smatch match;
    if (boost::regex_search( line, match, re_intro, boost::match_default ) )
    {
        return std::unique_ptr<OutputSectionReader>(new MinMax(match[1]));
    }
    else
        return nullptr;
}




bool MinMax::parseNextLine(const std::string& line)
{
    if (line.empty())
    {
        return false;
    }
    else
    {
        boost::smatch match;
        if (boost::regex_search( line, match, re_mima, boost::match_default ) )
        {
            std::string varname(match[2]);
            if (match[1]=="min")
            {
                min_[varname] = insight::toNumber<double>(match[3]);
            }
            else if (match[1]=="max")
            {
                max_[varname] = insight::toNumber<double>(match[3]);
            }
            else
                return false;

            return true;
        }
        else
            return false;
    }
}




void MinMax::addProgressVariables(std::map<std::string, double>& progVars) const
{
    for(const auto&mi: min_)
    {
        progVars[label_+"_"+mi.first+"/min"]=mi.second;
    }
    for(const auto&ma: max_)
    {
        progVars[label_+"_"+ma.first+"/max"]=ma.second;
    }
}




} // namespace OutputSectionReaders
} // namespace insight
