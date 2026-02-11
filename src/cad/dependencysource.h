#ifndef DEPENDENCYSOURCE_H
#define DEPENDENCYSOURCE_H

#include <algorithm>
#include <set>
#include <string>
#include <functional>
#include <stdio.h>

#include "base/boost_include.h"
#include "base/factory.h"
#include "base/exception.h"

#include "boost/variant/static_visitor.hpp"
#include "boost/spirit/include/qi.hpp"



namespace insight {
namespace cad {




class DependencySource;
class DependencyReplacement;
class TreeCloneMap;




class DOT
    : public std::reference_wrapper<std::ostream>
{
    std::set<const DependencySource*> alreadyProcessed_;

public:
    DOT(std::ostream& os);
    ~DOT();

    bool needsOutput(const DependencySource* ds);
};





class DependencySource
{

public:
    declareType("DependencySource");

    virtual std::string label() const;

    virtual ~DependencySource();

    typedef std::map<const DependencySource*, std::string> DependencyList;

    DependencyList dependencies() const;

    virtual void addDependencies(DependencyList& dl) const =0;
    virtual void replaceDependency(const DependencyReplacement& repl) =0;

    bool directlyDependsOn(const std::set<const DependencySource*> &deps2chk) const;
    bool indirectlyDependsOn(const std::set<const DependencySource*> &deps2chk) const;
    void replaceAllDependencies(const TreeCloneMap& tcm);


    class DepListInserter
        : public boost::static_visitor<void>
    {
        DependencyList& dl_;
        const std::string& label_;

    public:
        DepListInserter(DependencyList& dl, const std::string& label);

        void operator()(const DependencySource& s) const;

        template<class T>
        void operator()(std::shared_ptr<const T> t) const
        {
            if (auto ds=std::dynamic_pointer_cast<const DependencySource>(t))
            {
                (*this)(*ds);
            }
        }

        template<class T>
        void operator()(std::shared_ptr<T> t) const
        {
            if (auto ds=std::dynamic_pointer_cast<DependencySource>(t))
            {
                (*this)(*ds);
            }
        }

        // dispatch functions
        template<typename... Args>
        void operator()(const boost::variant<Args...>& t) const
        {

            t.apply_visitor(*this);
        }

        template<class X, class Y>
        void operator()(const std::pair<X,Y>& p) const
        {
            (*this)(p.first);
            (*this)(p.second);
        }

        template<class X>
        void operator()(const std::vector<X>& v) const
        {
            for (auto x: boost::adaptors::index(v))
            {
                DepListInserter(dl_, label_+"["+toString(x.index())+"]")
                (x.value());
            }
        }

        template<class X, class K=std::string>
        void operator()(const std::map<K, X>& v) const
        {
            std::for_each(
                v.begin(), v.end(),
                [this](const typename std::map<K, X>::value_type& v)
                {
                    DepListInserter(dl_, label_+"."+v.first)(v.second);
                }
                );
        }

        template<class T, class C=char>
        void operator()(const boost::spirit::qi::symbols<C, T>& v) const
        {
            v.for_each(
                [this](const std::string& name, const T& v)
                {
                    DepListInserter(dl_, label_+"."+name)(v);
                }
                );
        }

    };



    virtual void printDependencies( DOT& dot ) const;

    virtual std::shared_ptr<DependencySource>
    shallowClone(TreeCloneMap& tcm) const =0;

    std::unique_ptr<TreeCloneMap> createTCM(
        const std::set<const DependencySource*>& deps = {} ) const;

    template<class T=DependencySource>
    std::shared_ptr<T> deepClone(
        const std::set<const DependencySource*>& constrainCloningToDepOn = {} ) const
    {
        auto tcm=createTCM(constrainCloningToDepOn);

        auto sc=shallowClone(*tcm);
        sc->replaceAllDependencies(*tcm);

        auto tp=std::dynamic_pointer_cast<T>(sc);
        insight::assertion(
            bool(tp), "shallow clone return unexpected type");
        return tp;
    }
};


} // namespace cad
} // namespace insight

#endif // DEPENDENCYSOURCE_H
