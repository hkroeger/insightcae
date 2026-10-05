#ifndef INSIGHT_PREDEFINEDFILTERS_H
#define INSIGHT_PREDEFINEDFILTERS_H

#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>

#include "base/hierarchicaldatafilter.h"

namespace insight {

namespace hierarchicalData {


struct PredefinedFilter
{
    enum class Target { Parameters, Results };

    std::string label;

    /**
     * the filter applies to this analysis type and all types derived from it
     */
    std::string analysisType;

    Target target;

    Filter filter;
};




/**
 * @brief The PredefinedFilterRegistry class
 * collects predefined filters for parameter sets and result sets,
 * either hard-coded (see PredefinedFilterRegistry::Add)
 * or read from a configuration file of the form
 *
 * <predefinedFilters>
 *   <filter label="Hide numerics" analysis="OpenFOAMAnalysis" target="parameters">
 *     <filterEntry type="string" value="run/machine"/>
 *     <filterEntry type="regex" expression="fluid/numerics/.*"/>
 *   </filter>
 * </predefinedFilters>
 */
class PredefinedFilterRegistry
{
    std::vector<PredefinedFilter> filters_;

    PredefinedFilterRegistry();

public:
    static PredefinedFilterRegistry& global();

    void add(PredefinedFilter filter);

    const std::vector<PredefinedFilter>& filters() const;

    /**
     * @brief applicableFilters
     * @return all filters for the given target, which apply to analysisName, sorted by label
     */
    std::vector<const PredefinedFilter*> applicableFilters(
        const std::string& analysisName,
        PredefinedFilter::Target target ) const;

    void readFromNode(const rapidxml::xml_node<>& node);
    void readFromFile(const boost::filesystem::path& file);

    /**
     * hard-coded registration of a filter for the analysis type analysisType and its descendants.
     * The analysis type has to be known to AnalysisTypeHierarchy
     * (registered analyses are added by Analysis::Add, others by addToAnalysisTypeHierarchy).
     */
    struct Add
    {
        Add(
            const std::string& analysisType,
            const std::string& label,
            PredefinedFilter::Target target,
            const Filter& filter )
        {
            PredefinedFilterRegistry::global().add(
                { label, analysisType, target, filter } );
        }
    };
};


}

} // namespace insight

#endif // INSIGHT_PREDEFINEDFILTERS_H
