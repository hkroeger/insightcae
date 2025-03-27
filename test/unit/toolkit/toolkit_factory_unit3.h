#ifndef TOOLKIT_FACTORY_UNIT3_H
#define TOOLKIT_FACTORY_UNIT3_H

#include "toolkit_factory_unit2.h"

/*
 * create a derived class and add to factory table which was defined in another lib
 */

namespace insight
{

class MyClassDerived3 : public MyClass
{
public:
    declareType("Derived_Ext");

    MyClassDerived3(const std::string& v);
};

}

#endif // TOOLKIT_FACTORY_UNIT3_H
