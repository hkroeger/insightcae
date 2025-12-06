#ifndef TREECLONEMAP_H
#define TREECLONEMAP_H

#include <map>
#include "base/exception.h"
#include "base/boost_include.h"
#include "boost/spirit/include/qi.hpp"


namespace insight {
namespace cad {

class DependencyReplacement;
class DependencySource;

class TreeCloneMap
    : public std::map<const DependencySource*, std::shared_ptr<DependencySource> >
{

    std::shared_ptr<DependencySource>
    doShallowClone(const DependencySource*);

    std::set<const DependencySource*> triggerDeps_;
    bool skipCloning(const DependencySource*) const;

public:
    /**
     * @brief constrainCloning
     * clone only entities which directly or indirectly depends on given entities
     * @param deps
     * a dependency on this set of entities triggers cloning
     */
    void constrainCloning(const std::set<const DependencySource*>& triggerDeps);

    template<class T>
    std::shared_ptr<T> clone(const std::shared_ptr<T>& old)
    {
        if (auto oldptr=dynamic_cast<const DependencySource*>(old.get()))
        {
            if (skipCloning(oldptr))
                return old;

            auto i=this->find(oldptr);

            if (i!=end())
            {
                // already cloned elsewhere, replace with created clone
                auto nv=std::dynamic_pointer_cast<T>(i->second);
                insight::assertion(
                    bool(nv),
                    "unexpected type of previously cloned dependency");
                return nv;
            }
            else
            {
                // create clone
                auto cl = doShallowClone(oldptr);
                emplace(oldptr, cl);
                auto nv=std::dynamic_pointer_cast<T>(cl);
                insight::assertion(
                    bool(nv),
                    "unexpected type of previously cloned dependency");
                return nv;
            }
        }
        else
            return nullptr;
    }



    template<class PT, class C=char>
    void clone(
        const boost::spirit::qi::symbols<C, PT>& v,
        boost::spirit::qi::symbols<C, PT>& out
        )
    {
        out.clear();
        v.for_each(
            [&out,this](const std::string& name, const PT& v)
            {
                out.add(name, this->clone(v));
            }
            );
    }

};



} // namespace cad
} // namespace insight

#endif // TREECLONEMAP_H
