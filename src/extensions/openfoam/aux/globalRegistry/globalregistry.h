#ifndef GLOBALREGISTRY_H
#define GLOBALREGISTRY_H

#include "error.H"

#include <set>
#include <algorithm>

namespace Foam
{



template<class T>
class globalRegistry
        : private std::set<T*>
{

    globalRegistry()
    {}

public:
    void registerObject(T* o)
    {
        if (this->find(o)==this->end())
        {
            insert(o);
        }
        else
        {
            WarningIn("void globalRegistry::registerObject")
                    << "attempt to register object twice!"
                    << endl;
        }
    }

    void unregisterObject(T* o)
    {
        if (this->find(o)!=this->end())
        {
            this->erase(o);
        }
        else
        {
            WarningIn("void globalRegistry::unregisterObject")
                    << "attempt to unregister object which was not registered!"
                    << endl;
        }
    }

    bool exists(const word& lbl) const
    {
        auto fi = std::find_if(
                    this->begin(), this->end(),
                    [&lbl](const typename std::set<T*>::value_type& i)
                    {
                        return i->objectLabel()==lbl;
                    }
        );
        return fi!=this->end();
    }

    T* get(const word& lbl) const
    {
        auto fi = std::find_if(
                    this->begin(), this->end(),
                    [&lbl](const typename std::set<T*>::value_type& i)
                    {
                        return i->objectLabel()==lbl;
                    }
        );
        if (fi==this->end())
        {

            std::string entries="(";
            for (const auto& e: *this)
            {
                entries+=" "+e->objectLabel();
            }
            entries+=" )";

            FatalErrorIn("objectRegistry::get")
                    <<"Did not found force object "<<lbl<<" in registry!"
                   << "Registered objects are :"<<entries
                   <<abort(FatalError);
        }
        return *fi;
    }

    static globalRegistry<T>& registry()
    {
        static globalRegistry<T> reg;
        return reg;
    }
};



}

#endif // GLOBALREGISTRY_H
