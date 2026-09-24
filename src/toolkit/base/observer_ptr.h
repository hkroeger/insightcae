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


#ifndef INSIGHT_OBSERVER_PTR_H
#define INSIGHT_OBSERVER_PTR_H

#include <map>
#include <memory>
#include <set>

#include "base/exception.h"

namespace std
{


class observer_ptr_base;




class observable
{
    friend class observer_ptr_base;

    std::set<observer_ptr_base*> observers_;

    void register_observer(observer_ptr_base* obs);
    void unregister_observer(observer_ptr_base* obs);

public:
    ~observable();

    inline int numberOfObservers() const
    {
        return observers_.size();
    }
};




class observer_ptr_base
: public observable
{
    friend class observable;

protected:
    observable *observed_;

    virtual void invalidate()
    {
        observed_=nullptr;
    }

    void register_at_observable();
    void unregister_at_observable();

public:
    observer_ptr_base();
    observer_ptr_base(const observer_ptr_base& o);
    observer_ptr_base(const observable *o);
    observer_ptr_base(observable *o);
    virtual ~observer_ptr_base();

    void operator=(const observer_ptr_base& o);

    inline bool valid() const
    {
        return observed_!=nullptr;
    }
};




class invalid_observer_ptr
    : public insight::Exception
{
public:
    invalid_observer_ptr();
};




template<class T>
class observer_ptr : public observer_ptr_base
{

    inline T* observed()
    {
        if (!valid())
            throw invalid_observer_ptr();
        return static_cast<T*>(observed_);
    }

    inline const T* observed() const
    {
        if (!valid())
            throw invalid_observer_ptr();
        return static_cast<const T*>(observed_);
    }

public:
    observer_ptr()
        : observer_ptr_base()
    {}

    observer_ptr(T* observed)
        : observer_ptr_base(observed)
    {}

    observer_ptr(const std::unique_ptr<T>& observed)
        : observer_ptr_base(observed.get())
    {}


    inline operator bool() const
    {
        return valid();
    }

    inline T* get() { return observed(); }
    inline const T* get() const { return observed(); }


    inline operator T&() { return *observed(); }
    inline operator const T&() const { return *observed(); }

    inline T& operator*() { return *observed(); }
    inline T* operator->() { return observed(); }

    inline const T& operator*() const { return *observed(); }
    inline const T* operator->() const { return observed(); }

    bool operator==(const T* ptr) const
    {
        return ptr==static_cast<const T*>(observed_);
    }

    bool operator==(const observer_ptr& op) const
    {
        return observed_==op.observed_;
    }

    bool operator<(const T* ptr) const
    {
        return static_cast<const T*>(observed_) < ptr;
    }

    bool operator<(const observer_ptr& op) const
    {
        return observed_<op.observed_;
    }
};





template<class K, class T>
class key_observer_map;



template<class K, class T>
class map_key_observer_ptr
    : public observer_ptr<K>
{
    typedef key_observer_map<K,T> map_base;

    map_base& the_map;
    T t_;

    void invalidate() override
    {
        the_map.erase(this->get());
    }

public:
    map_key_observer_ptr(K* observed, T&& t, map_base& map )
        : observer_ptr<K>(observed), the_map(map), t_(std::move(t))
    {}

    ~map_key_observer_ptr()
    {}

    const T& getContained() const { return t_; }
    T& getContained() { return t_; }
};




template<class Key, class T>
class key_observer_map
    : private std::map<Key*, std::unique_ptr<map_key_observer_ptr<Key, T> > >
{
    friend class map_key_observer_ptr<Key, T>;

    typedef std::map<Key*, std::unique_ptr<map_key_observer_ptr<Key, T> > > map_base;

public:
    typedef std::pair<const Key&, T> value_type;
    typedef std::pair<const Key&, T&> reference_type;
    typedef std::pair<const Key&, const T&> const_reference_type;

public:

    class iterator
    {
        typename map_base::iterator baseIterator_;

    public:

        typedef typename map_base::size_type size_type;
        typedef typename map_base::difference_type difference_type;
        typedef key_observer_map::value_type value_type;
        typedef key_observer_map::reference_type reference;
        typedef key_observer_map::reference_type* pointer;
        typedef std::bidirectional_iterator_tag iterator_category;

        iterator()
        {}

        iterator(typename map_base::iterator baseIterator)
            : baseIterator_(baseIterator)
        {}

        iterator(const iterator& oi)
            : baseIterator_(oi.baseIterator_)
        {}

        ~iterator()
        {}

        iterator& operator=(const iterator& oi)
        {
            baseIterator_=oi.baseIterator_;
            return *this;
        }

        bool operator==(const iterator& oi) const
        {
            return (baseIterator_==oi.baseIterator_);
        }

        bool operator!=(const iterator& oi) const
        {
            return (baseIterator_!=oi.baseIterator_);
        }

        iterator& operator++()
        {
            baseIterator_++;
            return *this;
        }

        iterator& operator--()
        {
            baseIterator_--;
            return *this;
        }


        reference_type operator*()
        {
            return { *baseIterator_->first, baseIterator_->second->getContained() };
        }
    };


    class const_iterator
    {
        typename map_base::const_iterator baseIterator_;

    public:

        typedef typename map_base::size_type size_type;
        typedef typename map_base::difference_type difference_type;
        typedef key_observer_map::value_type value_type;
        typedef key_observer_map::const_reference_type reference;
        typedef key_observer_map::const_reference_type* pointer;
        typedef std::bidirectional_iterator_tag iterator_category;

        const_iterator()
        {}

        const_iterator(typename map_base::const_iterator baseIterator)
            : baseIterator_(baseIterator)
        {}

        const_iterator(const const_iterator& oi)
            : baseIterator_(oi.baseIterator_)
        {}

        ~const_iterator()
        {}

        const_iterator& operator=(const const_iterator& oi)
        {
            baseIterator_=oi.baseIterator_;
            return *this;
        }

        bool operator==(const const_iterator& oi) const
        {
            return (baseIterator_==oi.baseIterator_);
        }

        bool operator!=(const const_iterator& oi) const
        {
            return (baseIterator_!=oi.baseIterator_);
        }

        const_iterator& operator++()
        {
            baseIterator_++;
            return *this;
        }

        const_iterator& operator--()
        {
            baseIterator_--;
            return *this;
        }


        const_reference_type operator*()
        {
            return { *baseIterator_->first, baseIterator_->second->getContained() };
        }
    };

    inline iterator begin() { return iterator(map_base::begin()); }
    inline const_iterator begin() const { return const_iterator(map_base::cbegin()); }
    inline const_iterator cbegin() const { return const_iterator(map_base::cbegin()); }

    inline iterator end() { return iterator(map_base::end()); }
    inline const_iterator end() const { return const_iterator(map_base::cend()); }
    inline const_iterator cend() const{ return const_iterator(map_base::cend()); }

    void insert(Key* k, T&& t)
    {
        map_base::insert(
            {
             k,
             std::make_unique<map_key_observer_ptr<Key,T> >(
                 k, std::move(t), *this )
            });
    }

    void erase(Key* k)
    {
        map_base::erase(k);
    }


    void clear()
    {
        while (size())
        {
            erase(map_base::begin()->first);
        }
    }

    size_t size() const { return map_base::size(); }
};


}

#endif // INSIGHT_OBSERVER_PTR_H
