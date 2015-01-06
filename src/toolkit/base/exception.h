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


#ifndef INSIGHT_EXCEPTION_H
#define INSIGHT_EXCEPTION_H

#include <string>
#include <iostream>

namespace insight {
  
class Exception;
  
std::ostream& operator<<(std::ostream& os, const Exception& ex);

class Exception
{
  std::string message_;
  std::string strace_;
  
public:
    Exception(const std::string& msg, bool strace=true);
    virtual ~Exception();
    
    inline std::string as_string() const { return static_cast<std::string>(*this); }
    
    operator std::string() const;
    
    inline const std::string& message() const { return message_; }
    inline const std::string& strace() const { return strace_; }
    
    friend std::ostream& operator<<(std::ostream& os, const Exception& ex);
};

void Warning(const std::string& msg);

}

#endif // INSIGHT_EXCEPTION_H
