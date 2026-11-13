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
#ifndef INSIGHT_ANALYSISTYPEHIERARCHY_H
#define INSIGHT_ANALYSISTYPEHIERARCHY_H

#include <functional>
#include <map>
#include <string>

namespace insight {


/**
 * @brief The AnalysisTypeHierarchy class
 * Provides a runtime check, whether an analysis type (identified by its type name)
 * is derived from some other analysis type, without creating an instance.
 *
 * Each registered type provides a function, which throws a null pointer of its type,
 * and a function, which checks, whether a thrown pointer can be caught as a pointer of its type.
 * The C++ exception handling will match pointers to derived classes against handlers
 * for pointers to (public, unambiguous) base classes.
 */
class AnalysisTypeHierarchy
{
public:
    typedef std::function<void()> Thrower;
    typedef std::function<bool(const Thrower&)> Catcher;

private:
    std::map<std::string, Thrower> throwers_;
    std::map<std::string, Catcher> catchers_;

    AnalysisTypeHierarchy();

public:
    static AnalysisTypeHierarchy& global();

    void registerType(
        const std::string& typeName,
        Thrower thrower,
        Catcher catcher );

    template<class T>
    void registerType(const std::string& typeName = T::typeName_())
    {
        registerType(
            typeName,
            []() { throw static_cast<T*>(nullptr); },
            [](const Thrower& t) -> bool
            {
                try { t(); }
                catch (T*) { return true; }
                catch (...) { return false; }
                return false;
            } );
    }

    bool isRegistered(const std::string& typeName) const;

    /**
     * @brief isDerivedFrom
     * @return true, if both names are equal or if analysisName is registered and
     * derived from the registered type baseTypeName
     */
    bool isDerivedFrom(
        const std::string& analysisName,
        const std::string& baseTypeName ) const;

    template<class T>
    struct Add
    {
        Add(const std::string& typeName)
        {
            AnalysisTypeHierarchy::global().registerType<T>(typeName);
        }
    };
};


/**
 * register a type, which is not registered as an analysis itself (e.g. an abstract base class),
 * under the given name, so that it can be used as a base type in AnalysisTypeHierarchy::isDerivedFrom.
 * (Registered analyses are added automatically by Analysis::Add.)
 */
#define addToAnalysisTypeHierarchy(T, typeName) \
 static insight::AnalysisTypeHierarchy::Add<T> addToAnalysisTypeHierarchy##T(typeName)


} // namespace insight

#endif // INSIGHT_ANALYSISTYPEHIERARCHY_H
