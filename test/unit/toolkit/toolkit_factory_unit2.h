#ifndef TOOLKIT_FACTORY_UNIT2_H
#define TOOLKIT_FACTORY_UNIT2_H

#include "boost/noncopyable.hpp"
#include "base/factory.h"

namespace insight
{


/*
 * define some base class along with factory tables in its own, separate lib
 */


class MyClass;




struct MyClassDeleter
{
    void operator()(MyClass* ir);
};




extern std::string tab1;
extern std::string tab2;



class MyClass : public boost::noncopyable
{
public:
    declareType("MyClass");

    std::string something;

    MyClass(const std::string& v);
    ~MyClass();

    typedef std::unique_ptr<MyClass, MyClassDeleter> Ptr;


public:
    typedef FactoryWithDeleter<
        MyClass,
        typename Ptr::deleter_type,
        const std::string&
        >
        MyClassFactories;

    declareFactoryTableAccessFunction2(
        MyClassFactories, create );

    declareStaticFunctionTable2(
        DescriptionFunctions, descriptions,
        std::string
        );

    declareStaticFunctionTableAccessFunction2(
        DescriptionFunctions, st2
        );
};


template<class C>
std::string text()
{
    return C::typeName;
}

}

#endif // TOOLKIT_FACTORY_UNIT2_H
