#ifndef DEPENDENCYREPLACEMENT_H
#define DEPENDENCYREPLACEMENT_H

#include "base/boost_include.h"
#include "base/exception.h"

#include "boost/spirit/include/qi.hpp"




namespace insight {
namespace cad {




class DependencySource;




class DependencyReplacement
    : public boost::static_visitor<void>
{
    const DependencySource* o;
    std::shared_ptr<DependencySource> n;

    void replaceDependencyIn(DependencySource& ds) const;

public:
    DependencyReplacement(
        const DependencySource* _o,
        std::shared_ptr<DependencySource> _n )
        : o(_o), n(_n)
    {}

    template<class T>
    void operator()(std::shared_ptr<const T>& t) const
    {
        if (dynamic_cast<const DependencySource*>(t.get())==o)
        {
            auto ns=std::dynamic_pointer_cast<const T>(n);
            insight::assertion(
                bool(ns), "attempted to replace CAD dependency with different type!");
            t=ns;
        }
        else
        {
            if (auto dep=dynamic_cast<const DependencySource*>(t.get()))
            {
                replaceDependencyIn(
                    const_cast<DependencySource&>(*dep) );
            }
        }
    }

    template<class T>
    void operator()(std::shared_ptr<T>& t) const
    {
        if (dynamic_cast<DependencySource*>(t.get())==o)
        {
            auto ns=std::dynamic_pointer_cast<T>(n);
            insight::assertion(
                bool(ns), "attempted to replace CAD dependency with different type!");
            t=ns;
        }
        else
        {
            if (auto dep=std::dynamic_pointer_cast<DependencySource>(t))
            {
                replaceDependencyIn(*dep);
            }
        }
    }

    void operator()(DependencySource& dsrc) const
    {
        replaceDependencyIn(dsrc);
    }

    // dispatch functions
    template<typename... Args>
    void operator()(boost::variant<Args...>& t) const
    {
        t.apply_visitor(*this);
    }

    template<class X, class Y>
    void operator()(std::pair<X,Y>& p) const
    {
        (*this)(p.first);
        (*this)(p.second);
    }

    template<class X>
    void operator()(std::vector<X>& v) const
    {
        std::for_each(v.begin(), v.end(), *this);
    }

    template<class X, class K=std::string>
    void operator()(std::map<K, X>& v) const
    {
        std::for_each(
            v.begin(), v.end(),
              [this](typename std::map<K, X>::value_type& e)
              {
                  (*this)(e.second);
              }
          );
        // for (auto& e: v)
        // {
        //     (*this)(e.second);
        // }
    }

    template<class T, class C=char>
    void operator()(boost::spirit::qi::symbols<C, T>& v) const
    {
        v.for_each(
            [this](const std::string& name, T& v)
            {
                (*this)(v);
            }
            );
    }
};




} // namespace cad
} // namespace insight

#endif // DEPENDENCYREPLACEMENT_H
