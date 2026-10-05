#include "predefinedfilters.h"
#include "base/exception.h"
#include "base/rapidxml.h"
#include "base/analysistypehierarchy.h"

#include <algorithm>

namespace insight {

namespace hierarchicalData {


PredefinedFilterRegistry::PredefinedFilterRegistry()
{}




PredefinedFilterRegistry &PredefinedFilterRegistry::global()
{
    static PredefinedFilterRegistry theRegistry;
    return theRegistry;
}




void PredefinedFilterRegistry::add(PredefinedFilter filter)
{
    filters_.push_back(std::move(filter));
}




const std::vector<PredefinedFilter> &PredefinedFilterRegistry::filters() const
{
    return filters_;
}




std::vector<const PredefinedFilter *>
PredefinedFilterRegistry::applicableFilters(
    const std::string &analysisName,
    PredefinedFilter::Target target ) const
{
    std::vector<const PredefinedFilter *> result;
    for (const auto& f: filters_)
    {
        if ( f.target==target
            && AnalysisTypeHierarchy::global().isDerivedFrom(
                analysisName, f.analysisType ) )
        {
            result.push_back(&f);
        }
    }
    std::stable_sort(
        result.begin(), result.end(),
        [](const PredefinedFilter* a, const PredefinedFilter* b)
        {
            return a->label < b->label;
        } );
    return result;
}




void PredefinedFilterRegistry::readFromNode(const rapidxml::xml_node<> &node)
{
    for (auto *e = node.first_node("filter");
         e; e = e->next_sibling("filter"))
    {
        PredefinedFilter pf;
        pf.label = getMandatoryAttribute(*e, "label");
        pf.analysisType = getMandatoryAttribute(*e, "analysis");

        auto target = getMandatoryAttribute(*e, "target");
        if (target=="parameters")
            pf.target = PredefinedFilter::Target::Parameters;
        else if (target=="results")
            pf.target = PredefinedFilter::Target::Results;
        else
            throw insight::Exception(
                "unknown target \"%s\" of predefined filter \"%s\""
                " (expected \"parameters\" or \"results\")",
                target.c_str(), pf.label.c_str() );

        pf.filter.readFromNode(*e);

        add(std::move(pf));
    }
}




void PredefinedFilterRegistry::readFromFile(const boost::filesystem::path &file)
{
    CurrentExceptionContext ex("reading predefined filters from file "+file.string());

    XMLDocument doc(file, "predefinedFilters");
    insight::assertion(
        doc.rootNode!=nullptr,
        "no node \"predefinedFilters\" found" );
    readFromNode(*doc.rootNode);
}


}

} // namespace insight
