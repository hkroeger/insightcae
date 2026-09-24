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


#ifndef INSIGHT_ONDEMAND_H
#define INSIGHT_ONDEMAND_H

#include <functional>
#include <memory>

#include "vtkSmartPointer.h"

namespace insight {

template<class T, class SmartPtr, typename ...Args>
class OnDemandBase
{
public:
    typedef std::function<SmartPtr(Args...)> InitFunction;

private:
    InitFunction initFunction_;
    mutable SmartPtr value_;

public:
    OnDemandBase(InitFunction inif)
        : initFunction_(inif)
    {}

    SmartPtr ptr(Args...args) const
    {
        if (!value_)
        {
            value_=initFunction_(args...);
        }
        return value_;
    }

    T& operator()(Args...args)
    {
        return *ptr(args...);
    }

    const T& operator()(Args...args) const
    {
        return *ptr(args...);
    }

    void reset()
    {
        value_.reset();
    }
};




template<class T, typename ...Args>
class OnDemand
    : public OnDemandBase<T, std::shared_ptr<T>, Args...>
{
public:
    using OnDemandBase<T, std::shared_ptr<T>, Args...>::OnDemandBase;
};


template<class T, typename ...Args>
class vtkOnDemand
    : public OnDemandBase<T, vtkSmartPointer<T>, Args...>
{
public:
    using OnDemandBase<T, vtkSmartPointer<T>, Args...>::OnDemandBase;
};

}

#endif // INSIGHT_ONDEMAND_H
