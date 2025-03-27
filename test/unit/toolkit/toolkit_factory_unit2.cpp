
#include "toolkit_factory_unit2.h"

namespace insight
{

std::string tab1("tab1");
std::string tab2("tab2");




void MyClassDeleter::operator()(MyClass* ir)
{
    delete ir;
}




defineType(MyClass);

defineFactoryTable2(MyClass, MyClassFactories, create);

defineStaticFunctionTable2(
    "descriptions",
    MyClass, DescriptionFunctions, descriptions
    );

defineStaticFunctionTableAccessFunction2(
    "some other text",
    MyClass, DescriptionFunctions, st2
    );

MyClass::MyClass(const std::string& v) : something(v)
{
    std::cout<<"construct MyClass "<<something<<std::endl;
}

MyClass::~MyClass()
{
    std::cout<<"destruct MyClass"<<std::endl;
}



}
