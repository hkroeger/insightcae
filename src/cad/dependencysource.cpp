
#include "dependencysource.h"
#include "treeclonemap.h"
#include "dependencyreplacement.h"
#include <algorithm>
#include <string>
#include <functional>

namespace insight {
namespace cad {



DOT::DOT(std::ostream& os)
    : std::reference_wrapper<std::ostream>(os)
{
    this->get() << "digraph theGraph {" << std::endl;
}

DOT::~DOT()
{
    this->get() << "}" << std::endl;
}

bool DOT::needsOutput(const DependencySource* ds)
{
    if (alreadyProcessed_.count(ds))
    {
        return false;
    }
    else
    {
        alreadyProcessed_.insert(ds);
        return true;
    }
}




defineType(DependencySource);

std::string DependencySource::label() const
{
    return std::string();
}


DependencySource::~DependencySource()
{}


DependencySource::DependencyList DependencySource::dependencies() const
{
    DependencyList dl;
    addDependencies(dl);
    return dl;
}

bool DependencySource::directlyDependsOn(const std::set<const DependencySource*> &deps2chk) const
{
    auto deps = dependencies();

    return std::any_of(
        deps2chk.begin(), deps2chk.end(),
        [&deps](const DependencySource* ds2chk)
        { return deps.count(ds2chk)>0; });
}

bool DependencySource::indirectlyDependsOn(const std::set<const DependencySource*> &deps2chk) const
{
    if (directlyDependsOn(deps2chk))
        return true;

    auto deps = dependencies();

    return std::any_of(
        deps.begin(), deps.end(),
        [&deps2chk](const decltype(deps)::value_type& dep)
        {
            return
                (dep.first!=nullptr)
              && dep.first->indirectlyDependsOn(deps2chk);
        });
}


void DependencySource::replaceAllDependencies(const TreeCloneMap &tcm)
{
    for (auto& dep: tcm)
    {
        DependencyReplacement dr(dep.first, dep.second);
        this->replaceDependency(dr);
    }
}



DependencySource::DepListInserter::DepListInserter(
    DependencyList &dl, const std::string &label)
    : dl_(dl), label_(label)
{}

void DependencySource::DepListInserter::operator()(const DependencySource& s) const
{
    dl_.emplace(&s, label_);
}

// void DependencySource::printDependency(
//     DOT& dot, const std::string& label, const DependencySource& s) const
// {
//     dot.get() << "x"<<this << " -> " << "x"<<&s << "[label=\""<<label<<"\"];" << std::endl;
//     if (dot.needsOutput(&s)) s.printDependencies(dot);
// }




void DependencySource::printDependencies( DOT& dot ) const
{
    auto lbl=label();
    if (!lbl.empty()) lbl+=" ";
    dot.get() << "x"<<this << "[label=\""<<lbl<<"("<<type()<<")\";shape=box];\n";

    auto deps=dependencies();
    for (auto& d: deps)
    {
        dot.get() << "x"<<this << " -> " << "x"<<d.first << "[label=\""<<d.second<<"\"];\n";
        if (dot.needsOutput(d.first))
        {
            d.first->printDependencies(dot);
        }
    }
}


std::unique_ptr<TreeCloneMap>
DependencySource::createTCM(
    const std::set<const DependencySource*>& deps ) const
{
    auto tcm=std::make_unique<TreeCloneMap>();
    tcm->constrainCloning(deps);
    return tcm;
}


} // namespace cad
} // namespace insight
