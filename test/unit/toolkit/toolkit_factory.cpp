
#include "base/exception.h"
#include "toolkit_factory_unit2.h"

#include <iostream>
#include <iterator>

#include <dlfcn.h>
#include <stdexcept>

namespace insight
{

/*
 * add some other entry, locally defined
 */
class MyClassDerived2 : public MyClass
{
public:
    declareType("Derived_Int");

    MyClassDerived2(const std::string& v) : MyClass(v) {};
};

defineType(MyClassDerived2);

addToFactoryTable2(
    MyClass, MyClassFactories, create, MyClassDerived2
    );

addToStaticFunctionTable2(
    MyClass, DescriptionFunctions, st2, MyClassDerived2,
    &text<MyClassDerived2> );

};


using namespace std;
using namespace insight;



int main()
{
    std::string libFile="libtoolkit_factory_lib_ext.so";
    void *handle = dlopen ( libFile.c_str(), RTLD_LAZY|RTLD_GLOBAL|RTLD_NODELETE );
    if ( !handle )
    {
        std::cerr<<"Could not load module library "<<libFile<<"!\n"
                    "Reason: " << dlerror() << std::endl;
    }

    try
    {
        std::cout<<"Factories:";
        for (auto& e: MyClass::create())
        {
            std::cout<<" "<<e.first;
        }
        std::cout<<std::endl;

        std::cout<<"ST1:";
        for (auto& e: MyClass::descriptions())
        {
            std::cout<<" "<<e.first;
        }
        std::cout<<std::endl;

        std::cout<<"ST2:";
        for (auto& e: MyClass::st2())
        {
            std::cout<<" "<<e.first;
        }
        std::cout<<std::endl;

        auto c1 = MyClass::create()("Derived_Int", "blubb");
        if (!c1)
            throw std::runtime_error("could not lookup derived class which was defined in the same translation unit!");

        auto c2 = MyClass::create()("Derived_Ext", "blubb");
        if (!c2)
            throw std::runtime_error("could not lookup derived class which was defined in other library!");

        return 0;
    }
    catch (std::exception& e)
    {
        cerr<<e.what()<<endl;
        return -1;
    }
}
