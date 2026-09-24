/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */


#ifndef INSIGHT_STL_CONTAINER_EXTENSIONS_H
#define INSIGHT_STL_CONTAINER_EXTENSIONS_H

#include <functional>
#include <iterator>
#include <memory>
#include <set>
#include <tuple>
#include <type_traits>
#include <vector>

#include "base/exception.h"

namespace std
{



template<typename T>
std::vector<T>& append(std::vector<T>& dst, const std::vector<T>& src)
{
    dst.insert(
        dst.end(),
        src.begin(), src.end() );

    return dst;
}

template<typename T, typename... Args>
std::unique_ptr<T> make_unique_aggr(Args&&... args)
{
    return std::unique_ptr<T>(new T{ std::forward<Args>(args)... });
}


template<typename T, typename... Args>
std::shared_ptr<T> make_shared_aggr(Args&&... args)
{
    return std::shared_ptr<T>(new T{ std::forward<Args>(args)... });
}



template<class Container, class Iterator, class Converter>
Container transform_copy(
    Iterator b, Iterator e, Converter c)
{
    Container res;
    std::transform(b, e, std::back_inserter(res), c);
    return res;
}



template<class Container, class Container1, class Converter>
Container transform_copy(
    Container1 c1, Converter c)
{
    Container res;
    std::transform(c1.begin(), c1.end(), std::back_inserter(res), c);
    return res;
}

// taken from https://stackoverflow.com/a/3611374

template<typename T, typename = void>
struct has_push_back : std::false_type {};

template<typename T>
struct has_push_back<T, std::void_t<
    decltype(std::declval<T&>().push_back(std::declval<typename T::value_type>()))
>> : std::true_type {};

template<class Container>
class last_inserter_iterator
/*: public std::_Outit*/
: public std::iterator<std::output_iterator_tag,
                           void, void, void, void>
{
public:
    typedef last_inserter_iterator<Container> _Myt;
    typedef Container container_type;
    typedef typename Container::const_reference const_reference;
    typedef typename Container::value_type _Valty;

    last_inserter_iterator(Container& cont)
        : container(cont)
    {
    }

    _Myt& operator=(const _Valty& _Val)
    {
        container.insert(get_insert_hint(), _Val);
        return (*this);
    }

    _Myt& operator=(_Valty&& _Val)
    {
        container.insert(get_insert_hint(), std::forward<_Valty>(_Val));
        return (*this);
    }

    _Myt& operator*()
    {
        return (*this);
    }

    _Myt& operator++()
    {
        return (*this);
    }

    _Myt& operator++(int)
    {
        return (*this);
    }

protected:
    Container& container;

    typename Container::iterator get_insert_hint() const
    {
        if constexpr (has_push_back<Container>::value)
        {
            // Sequence containers (vector, deque, ...): insert(pos, val) inserts
            // *before* pos, so end() appends correctly in all cases.
            return container.end();
        }
        else
        {
            // Associative containers (set, map, ...): insert(hint, val) uses hint
            // as a search hint. Last element is the right hint for ordered appending.
            if (container.empty())
                return container.end();
            return --container.end();
        }
    }
};

template<typename Container>
inline last_inserter_iterator<Container> last_inserter(Container& cont)
{
    return last_inserter_iterator<Container>(cont);
}





template<class Container, class Container1>
Container container_type_cast(
    Container1 c1)
{
    Container res;
    std::transform(
        c1.begin(), c1.end(),
        std::last_inserter(res),
        [](const typename Container::value_type& e) { return e; } );
    return res;
}

template<class T, class Container1>
std::vector<T> vector_cast(
    Container1 c1)
{
    return std::container_type_cast<std::vector<T> >(c1);
}

template<class Map>
std::set<typename Map::key_type>
map_keys(const Map& m)
{
    std::set<typename Map::key_type> result;
    std::transform(
        m.begin(), m.end(),
        std::last_inserter(result),
        [](const typename Map::value_type& v){ return v.first; }
        );
    return result;
}


template<class Map>
std::vector<typename Map::mapped_type>
map_items(const Map& m)
{
    std::vector<typename Map::mapped_type> result;
    std::transform(
        m.begin(), m.end(),
        std::last_inserter(result),
        [](const typename Map::value_type& v){ return v.second; }
        );
    return result;
}


template<class TargetType, class SourceType>
std::unique_ptr<TargetType> dynamic_unique_ptr_cast(std::unique_ptr<SourceType> src)
{
  if (TargetType* to = dynamic_cast<TargetType*>(src.get()))
  {
    src.release();
    return std::unique_ptr<TargetType>(to);
  }
  else
    throw insight::Exception("Could not cast unique_ptr!");
}


template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}



template<class E>
struct hash<std::vector<E> >
{
    size_t operator()(const std::vector<E>& v) const
    {
        size_t h=std::hash<size_t>()(v.size());
        for (const auto& e: v)
        {
            std::hash_combine(h, e);
        }
        return h;
    }
};






// https://stackoverflow.com/a/51591178
template<class T>
bool operator==(const std::shared_ptr<T> &lhs, const std::weak_ptr<T> &rhs)
{
    return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
}

template<class T>
class comparable_weak_ptr : public std::weak_ptr<T>
{
public:
    template<class ...Args>
    comparable_weak_ptr(Args&&... addArgs)
        : std::weak_ptr<T>(std::forward<Args>(addArgs)...)
    {}

    // https://stackoverflow.com/a/51591178
    bool operator==(const std::shared_ptr<T>& lhs) const
    {
        return !lhs.owner_before(*this) && !this->owner_before(lhs);
    }

    bool operator<(const std::weak_ptr<T>& rhs) const
    {
        return std::owner_less<std::weak_ptr<T> >()(*this, rhs);
    }

};





template<class Map>
typename Map::const_iterator find_mapped_value(const Map& m, const typename Map::mapped_type& value)
{
    return std::find_if(
        m.begin(), m.end(),
        [&](const typename Map::value_type& entry) {
            return entry.second==value;
        });
}


// the following is taken from sigidagi (https://stackoverflow.com/a/6401663)

// ------------- UTILITY---------------
template<int...> struct index_tuple{};

template<int I, typename IndexTuple, typename... Types>
struct make_indexes_impl;

template<int I, int... Indexes, typename T, typename ... Types>
struct make_indexes_impl<I, index_tuple<Indexes...>, T, Types...>
{
    typedef typename make_indexes_impl<I + 1, index_tuple<Indexes..., I>, Types...>::type type;
};

template<int I, int... Indexes>
struct make_indexes_impl<I, index_tuple<Indexes...> >
{
    typedef index_tuple<Indexes...> type;
};

template<typename ... Types>
struct make_indexes : make_indexes_impl<0, index_tuple<>, Types...>
{};

// ----------- FOR EACH -----------------
template<typename Func, typename Last>
void for_each_impl(Func&& f, Last&& last)
{
    f(last);
}

template<typename Func, typename First, typename ... Rest>
void for_each_impl(Func&& f, First&& first, Rest&&...rest)
{
    f(first);
    for_each_impl( std::forward<Func>(f), rest...);
}

template<typename Func, int ... Indexes, typename ... Args>
void for_each_helper( Func&& f, index_tuple<Indexes...>, std::tuple<Args...>&& tup)
{
    for_each_impl( std::forward<Func>(f), std::forward<Args>(std::get<Indexes>(tup))...);
}

template<typename Func, typename ... Args>
void for_each( std::tuple<Args...>& tup, Func&& f)
{
    for_each_helper(std::forward<Func>(f),
                    typename make_indexes<Args...>::type(),
                    std::forward<std::tuple<Args...>>(tup) );
}

template<typename Func, typename ... Args>
void for_each( std::tuple<Args...>&& tup, Func&& f)
{
    for_each_helper(std::forward<Func>(f),
                    typename make_indexes<Args...>::type(),
                    std::forward<std::tuple<Args...>>(tup) );
}


}

#endif // INSIGHT_STL_CONTAINER_EXTENSIONS_H
