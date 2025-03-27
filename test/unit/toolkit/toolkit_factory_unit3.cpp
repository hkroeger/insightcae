
#include "toolkit_factory_unit3.h"

namespace insight
{


MyClassDerived3::MyClassDerived3(const std::string& v)
    : MyClass(v)
{}

defineType(MyClassDerived3);

addToFactoryTable2(
    MyClass, MyClassFactories, create, MyClassDerived3
    );

addToStaticFunctionTable2(
    MyClass, DescriptionFunctions, descriptions, MyClassDerived3,
    &text<MyClassDerived3> );

}
