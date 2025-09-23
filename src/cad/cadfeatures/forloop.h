#ifndef FORLOOP_H
#define FORLOOP_H

#include "modelfeature.h"

namespace insight {
namespace cad {


class ForLoop
    : public Compound
{

    std::string loopVarName_;
    mutable ModelFeaturePtr loopBodyTemplate_;
    ScalarPtr i0_, imax_, increment_;
    FeaturePtr lastInstance_;

    void createInstances();

    ForLoop(const ForLoop& o, TreeCloneMap& tcm);
    ForLoop (
        const std::string& varname,
        ModelFeaturePtr loopBodyTemplate,
        ScalarPtr i0, ScalarPtr imax,
        ScalarPtr increment = cad::scalarconst(1.) );

    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "for" );
#ifndef SWIG
    DEPENDS_W_BASE(Compound, (i0_, imax_, increment_));
#endif
    template <typename... T>
    static std::shared_ptr<ForLoop> create(T...args)
    {
        auto o= std::shared_ptr<ForLoop>(
            new ForLoop(::std::forward<T>(args)...));
        if (o->loopBodyTemplate_)
            o->createInstances();
        return o;
    }
    CLONEABLE(ForLoop);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();
};


} // namespace cad
} // namespace insight

#endif // FORLOOP_H
