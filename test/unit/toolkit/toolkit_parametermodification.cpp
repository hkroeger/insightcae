#include "base/exception.h"
#include "base/parameterset.h"
#include "base/parameters/simpleparameter.h"
#include "base/parameters/arrayparameter.h"
#include "base/parameters/labeledarrayparameter.h"
#include "base/parameters/selectablesubsetparameter.h"

#include <iostream>

using namespace std;
using namespace insight;


// test of Parameter::isModified, which controls
// the bold display of modified parameters in the GUI

std::unique_ptr<ParameterSet> createTestSet()
{
    auto ps = ParameterSet::create();

    ps->insert<DoubleParameter>("dbl", 1.0, "a double");

    {
        auto& grp = dynamic_cast<ParameterSet&>(
            ps->insert("grp", *ParameterSet::create("a group")) );
        grp.insert<DoubleParameter>("inner", 2.0, "a double in a group");
    }

    {
        auto& ssp = ps->insert<SelectableSubsetParameter>(
            "sel", "a selectable subset");

        auto a = ParameterSet::create("alternative A");
        a->insert<DoubleParameter>("p", 3.0, "p in A");
        ssp.addItem("A", std::move(a));

        auto b = ParameterSet::create("alternative B");
        b->insert<DoubleParameter>("p", 3.0, "p in B");
        ssp.addItem("B", std::move(b));

        ssp.setSelection("A");
    }

    {
        // default array has one element, which differs from the prototype
        auto& arr = ps->insert<ArrayParameter>(
            "arr", DoubleParameter(0.0, "prototype"), 0, "an array");
        arr.appendValue(std::make_unique<DoubleParameter>(5.0, "element"));
    }

    ps->insert<LabeledArrayParameter>(
        "larr", DoubleParameter(0.0, "prototype"), 1, "a labeled array");

    return ps;
}


void check(
    const ParameterSet& ps,
    const ParameterSet& defaults,
    const std::string& path,
    bool expectModified,
    const std::string& scenario )
{
    bool m = ps.get<Parameter>(path).isModified(defaults);
    std::cout<<scenario<<": "<<path<<" isModified="<<m<<std::endl;
    insight::assertion(
        m==expectModified,
        "%s: expected parameter %s to be %s",
        scenario.c_str(), path.c_str(),
        expectModified ? "modified" : "unmodified" );
}


int main()
{
    try
    {
        auto defaults = createTestSet();

        {
            auto ps = defaults->cloneAs<ParameterSet>();
            for (auto p: {"dbl", "grp", "grp/inner", "sel", "sel/p", "arr", "arr/0", "larr"})
            {
                check(*ps, *defaults, p, false, "untouched");
            }
        }

        {
            auto ps = defaults->cloneAs<ParameterSet>();
            ps->get<DoubleParameter>("grp/inner").set(20.);
            check(*ps, *defaults, "grp/inner", true, "changed value");
            check(*ps, *defaults, "grp", true, "changed value");
            check(*ps, *defaults, "dbl", false, "changed value");

            ps->get<DoubleParameter>("grp/inner").set(2.);
            check(*ps, *defaults, "grp/inner", false, "reverted value");
            check(*ps, *defaults, "grp", false, "reverted value");
        }

        {
            // only the selection changes, child values are identical
            auto ps = defaults->cloneAs<ParameterSet>();
            ps->get<SelectableSubsetParameter>("sel").setSelection("B");
            check(*ps, *defaults, "sel", true, "selection switched");
            check(*ps, *defaults, "sel/p", false, "selection switched");

            ps->get<DoubleParameter>("sel/p").set(30.);
            check(*ps, *defaults, "sel/p", true, "value in switched selection changed");
        }

        {
            // appended element equals the prototype
            auto ps = defaults->cloneAs<ParameterSet>();
            ps->get<ArrayParameter>("arr").appendEmpty();
            check(*ps, *defaults, "arr", true, "array element added");
            check(*ps, *defaults, "arr/0", false, "array element added");
        }

        {
            auto ps = defaults->cloneAs<ParameterSet>();
            ps->get<ArrayParameter>("arr").appendEmpty();
            ps->get<ArrayParameter>("arr").eraseValue(0);
            check(*ps, *defaults, "arr", true, "array element removed");
        }

        {
            auto ps = defaults->cloneAs<ParameterSet>();
            ps->get<LabeledArrayParameter>("larr").appendEmpty();
            check(*ps, *defaults, "larr", true, "labeled array element added");
        }
    }
    catch (const std::exception& e)
    {
        cerr<<e.what()<<endl;
        return -1;
    }

    return 0;
}
