#ifndef SPLITSURFACE_H
#define SPLITSURFACE_H

#include "derivedfeature.h"
#include <vector>

namespace insight {
namespace cad {

class SplitSurface
: public DerivedFeature
{
    // struct Tool
    // {
    //     FeaturePtr tool_;
    //     std::string splitOffFacesName_;
    // };
    typedef boost::fusion::vector2<FeaturePtr, std::string> Tool;
    typedef std::vector<Tool> Tools;
    Tools tools_;

    size_t calcHash() const override;
    void build() override;

    SplitSurface(const SplitSurface&o, TreeCloneMap& tcm);
    SplitSurface(FeaturePtr featToSplit, Tools tools);


public:
    declareType("SplitSurface");
    void replaceDependency(const DependencyReplacement& repl) override;
    CREATE_FUNCTION(SplitSurface);
    CLONEABLE(SplitSurface);

    static void insertrule(parser::ISCADParser& ruleset);
};

} // namespace cad
} // namespace insight

#endif // SPLITSURFACE_H
