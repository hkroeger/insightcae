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


#include "exceptionhandling.h"
#include "base/warningdispatcher.h"
#include "base/translations.h"

#include <climits>
#include <functional>
#include <sstream>
#include <system_error>
#include <thread>

#include "boost/format.hpp"
#include "boost/stacktrace.hpp"

using namespace std;

namespace insight
{


void UnhandledExceptionHandling::handler()
{
  std::cerr<<"Unhandled exception occurred!"<<std::endl;
  std::cerr << boost::stacktrace::stacktrace();
  exit(1);
}

UnhandledExceptionHandling::UnhandledExceptionHandling()
{
    std::set_terminate( handler );
}



string ExceptionHandler::title() const
{
    std::ostringstream t;
    t<<"*** ERROR ["<< std::this_thread::get_id() <<"] ***";
    return t.str();
}



ExceptionHandler::ExceptionHandler(int priority)
{
    exceptionHandlers().insert({priority, this});
}

ExceptionHandler::~ExceptionHandler()
{}

ErrorDescriptionPtr ExceptionHandler::describeProblem() const
{
    throw;
    return nullptr;
}


std::multimap<int, ExceptionHandler*>&
ExceptionHandler::exceptionHandlers()
{
    static std::multimap<int, ExceptionHandler*> theExceptionHandlers;
    return theExceptionHandlers;
}


class InsightExceptionHandler
    : public ExceptionHandler
{
public:
    InsightExceptionHandler() : ExceptionHandler(INT_MAX) {}

    ErrorDescriptionPtr describeProblem() const override
    {
        try { throw; }

        catch (insight::Exception& e)
        {
            return e.description();
        }

        catch (std::system_error& e)
        {
            return std::make_shared<ErrorDescription>(
                str(boost::format("%s (std::system_error)")
                    % e.what() ) );
        }

        catch (std::exception& e)
        {
            return std::make_shared<ErrorDescription>(
                str(boost::format("%s (std::exception)")
                    % e.what() ) );
        }

        catch (...)
        {
            auto desc =
                std::make_unique<insight::ErrorDescription>(
                    "unknown error" );

            return desc;
        }
    }
} insightExceptionHandler;




ErrorDescriptionPtr describeCurrentException()
{
    auto handler =
        ExceptionHandler::exceptionHandlers().begin();

    std::function<ErrorDescriptionPtr()> tryNextHandler;

    tryNextHandler = [&]() {
        try
        {
            return handler->second->describeProblem();
        }
        catch (...)
        {
            // not handled

            // try next
            handler++;
            if (handler !=
                ExceptionHandler::exceptionHandlers().end())
            {
                return tryNextHandler();
            }
            else
            {
                // no more
                throw; // give up
            }
        }
    };

    return tryNextHandler();
}




void printCurrentException(std::ostream& os)
{
    auto desc=describeCurrentException();

    displayFramed(
        "*** ERROR ["+desc->thread_id_+"] ***",
        (*desc) + (
            !desc->errorDetails_.empty() ?
             (std::string("\n") + _("Further details:") + "\n" + desc->errorDetails_) :
             std::string("")
        ),
        '=', os
    );
}


}
