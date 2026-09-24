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


#include "observer_ptr.h"
#include "base/exception.h"

namespace std
{


void observer_ptr_base::register_at_observable()
{
    if (valid())
        observed_->register_observer(this);
}


void observer_ptr_base::unregister_at_observable()
{
    if (valid())
        observed_->unregister_observer(this);
}

observer_ptr_base::observer_ptr_base()
    : observed_(nullptr)
{}


observer_ptr_base::observer_ptr_base(const observer_ptr_base &o)
    : observed_(o.observed_)
{
    register_at_observable();
}



observer_ptr_base::observer_ptr_base(observable *o)
    : observed_(o)
{
    register_at_observable();
}



observer_ptr_base::observer_ptr_base(const observable *o)
    : observed_(const_cast<observable*>(o))
{
    register_at_observable();
}



observer_ptr_base::~observer_ptr_base()
{
    unregister_at_observable();
}



void observer_ptr_base::operator=(const observer_ptr_base& o)
{
    unregister_at_observable();
    observed_=o.observed_;
    register_at_observable();
}



invalid_observer_ptr::invalid_observer_ptr()
    : insight::Exception("attempt to access an invalid pointer")
{}




void observable::register_observer(observer_ptr_base* obs)
{
    observers_.insert(obs);
}




void observable::unregister_observer(observer_ptr_base* obs)
{
    observers_.erase(obs);
}




observable::~observable()
{
    auto obs=observers_; // loop over copy, since observers will remove itself and modify list
    for (auto& o: obs)
    {
        o->invalidate();
    }
}




}
