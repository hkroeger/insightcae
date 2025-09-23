#ifndef GLOBALOBJECT_H
#define GLOBALOBJECT_H

#include "word.H"

namespace Foam
{

template<class Registry>
class globalObject
{
protected:
    word lbl_;
    bool registered_;

public:
    globalObject(const word& lbl, bool autoRegister=true)
    : lbl_(lbl)
    {
        if (autoRegister)
        {
            Registry::registry().registerObject(this);
            registered_=true;
        }
        else
        {
            registered_=false;
        }
    }

    virtual ~globalObject()
    {
        if (registered_)
        {
            Registry::registry().unregisterObject(this);
        }
    }

    const word& objectLabel() const
    {
        return lbl_;
    }
};

}

#endif // GLOBALOBJECT_H
