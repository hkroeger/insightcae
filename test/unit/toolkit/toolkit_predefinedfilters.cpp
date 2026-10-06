#include "base/exception.h"
#include "base/rapidxml.h"
#include "base/analysistypehierarchy.h"
#include "base/predefinedfilters.h"

#include <iostream>
#include <sstream>

using namespace std;
using namespace insight;
using namespace insight::hierarchicalData;


// mimic analysis class hierarchy (no instances required)
struct BaseAnalysis { static const char* typeName_() { return "BaseAnalysis"; } };
struct IntermediateAnalysis : public BaseAnalysis { static const char* typeName_() { return "Intermediate Analysis"; } };
struct ConcreteAnalysis : public IntermediateAnalysis { static const char* typeName_() { return "Concrete Analysis"; } };
struct OtherAnalysis : public BaseAnalysis { static const char* typeName_() { return "Other Analysis"; } };

addToAnalysisTypeHierarchy(BaseAnalysis, "BaseAnalysis");
addToAnalysisTypeHierarchy(IntermediateAnalysis, IntermediateAnalysis::typeName_());

static AnalysisTypeHierarchy::Add<ConcreteAnalysis> addConcrete(ConcreteAnalysis::typeName_());
static AnalysisTypeHierarchy::Add<OtherAnalysis> addOther(OtherAnalysis::typeName_());

static PredefinedFilterRegistry::Add hardCodedFilter(
    "Intermediate Analysis", "Hard-coded",
    PredefinedFilter::Target::Parameters,
    Filter{ { std::string("a/b") } } );




int main(int argc, char* argv[])
{
    try
    {
        auto& h = AnalysisTypeHierarchy::global();

        insight::assertion( h.isDerivedFrom("Concrete Analysis", "Concrete Analysis"), "same type" );
        insight::assertion( h.isDerivedFrom("Concrete Analysis", "Intermediate Analysis"), "direct base" );
        insight::assertion( h.isDerivedFrom("Concrete Analysis", "BaseAnalysis"), "indirect base" );
        insight::assertion( !h.isDerivedFrom("Concrete Analysis", "Other Analysis"), "sibling" );
        insight::assertion( !h.isDerivedFrom("BaseAnalysis", "Concrete Analysis"), "derived is not base" );
        insight::assertion( !h.isDerivedFrom("Unknown", "BaseAnalysis"), "unknown analysis" );
        insight::assertion( !h.isDerivedFrom("Concrete Analysis", "Unknown"), "unknown base" );


        std::string cfg =
            "<predefinedFilters>"
            " <filter label=\"Z results\" analysis=\"BaseAnalysis\" target=\"results\">"
            "  <filterEntry type=\"string\" value=\"r/x\"/>"
            " </filter>"
            " <filter label=\"B params\" analysis=\"BaseAnalysis\" target=\"parameters\">"
            "  <filterEntry type=\"string\" value=\"run/machine\"/>"
            "  <filterEntry type=\"regex\" expression=\"fluid/numerics/.*\"/>"
            " </filter>"
            " <filter label=\"Other only\" analysis=\"Other Analysis\" target=\"parameters\">"
            "  <filterEntry type=\"string\" value=\"o\"/>"
            " </filter>"
            "</predefinedFilters>";
        XMLDocument doc(cfg.begin(), cfg.end(), "predefinedFilters");
        insight::assertion( doc.rootNode!=nullptr, "root node" );

        auto& reg = PredefinedFilterRegistry::global();
        reg.readFromNode(*doc.rootNode);
        insight::assertion( reg.filters().size()==4, "number of filters" );

        auto pf = reg.applicableFilters("Concrete Analysis", PredefinedFilter::Target::Parameters);
        insight::assertion( pf.size()==2, "number of applicable parameter filters: %d", int(pf.size()) );
        insight::assertion( pf[0]->label=="B params", "sorting" );
        insight::assertion( pf[1]->label=="Hard-coded", "sorting" );
        insight::assertion( pf[0]->filter.matches("run/machine"), "string entry" );
        insight::assertion( pf[0]->filter.matches("fluid/numerics/solver"), "regex entry" );
        insight::assertion( !pf[0]->filter.matches("fluid/geometry"), "no match" );

        auto rf = reg.applicableFilters("Concrete Analysis", PredefinedFilter::Target::Results);
        insight::assertion( rf.size()==1 && rf[0]->label=="Z results", "result filters" );

        auto of = reg.applicableFilters("Other Analysis", PredefinedFilter::Target::Parameters);
        insight::assertion( of.size()==2, "filters for other analysis: %d", int(of.size()) );


        std::string badCfg =
            "<predefinedFilters>"
            " <filter label=\"bad\" analysis=\"BaseAnalysis\" target=\"nothing\"/>"
            "</predefinedFilters>";
        XMLDocument badDoc(badCfg.begin(), badCfg.end(), "predefinedFilters");
        bool thrown=false;
        try { reg.readFromNode(*badDoc.rootNode); }
        catch (const insight::Exception&) { thrown=true; }
        insight::assertion( thrown, "invalid target not detected" );
    }
    catch (const std::exception& e)
    {
        cerr<<e.what()<<endl;
        return -1;
    }

    return 0;
}
