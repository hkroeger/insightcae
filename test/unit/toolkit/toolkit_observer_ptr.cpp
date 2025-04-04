
#include "base/cppextensions.h"
#include "base/exception.h"

#include <iostream>
#include <iterator>

using namespace std;
using namespace insight;

class MyClass : public boost::noncopyable, public observable
{
public:
    std::string something;

    MyClass(const std::string& v) : something(v) {}
    ~MyClass()
    {
        std::cout<<"destruct MyClass"<<std::endl;
    }
};

typedef observer_ptr<MyClass> MyClassPtr;


int main()
{
    try
    {

        std::unique_ptr<observer_ptr<MyClassPtr> > observerObserver;
        {
            typedef std::map<std::string, std::unique_ptr<MyClass> > Container;

            Container container;

            container.insert({"x", make_unique<MyClass>("xx")});
            container.insert({"y", make_unique<MyClass>("yy")});
            container.insert({"z", make_unique<MyClass>("zz")});

            auto cc = make_unique<MyClass>("cc");

            MyClassPtr xe(container["x"]);
            observerObserver=std::make_unique<observer_ptr<MyClassPtr> >(&xe);

            insight::assertion(
                observerObserver->valid(),
                "observerObserver not valid"
                );
            insight::assertion(
                xe.numberOfObservers()==1,
                "unexpected number of obs (xe)" );

            insight::assertion(
                container.at("x")->numberOfObservers()==1,
                "expect one observer" );

            {
                auto xe2=xe;
                insight::assertion(
                    container.at("x")->numberOfObservers()==2,
                    "expect 2 observers" );
                MyClassPtr xe3(xe2);
                insight::assertion(
                    container.at("x")->numberOfObservers()==3,
                    "expect 3 observers" );
            }
            insight::assertion(
                container.at("x")->numberOfObservers()==1,
                "expect one observer again" );

            key_observer_map<MyClass, std::string > refc;
            refc.insert(xe.get(), "xx");
            refc.insert(cc.get(), "cc");

            std::cout<<"loop "<<refc.size()<<std::endl;
            for (auto r: refc)
            {
                std::cout<<r.first.something<<" "<<r.second<<std::endl;
            }

            auto ic = decltype(refc)::iterator::iterator_category{};
            auto ic2=std::iterator_traits<decltype(refc)>();

            auto ri = std::find_if(
                refc.begin(), refc.end(),
                [&](decltype(refc)::const_reference_type v)
                {
                    return &v.first==cc.get();
                }
                );
            insight::assertion(
                (*ri).second=="cc",
                "unexpected search result" );


            std::cout<<"ref size="<<refc.size()<<std::endl;

            std::cout<<"xe is valid: "<<bool(xe);
            if (xe) std::cout<<", xe: "<<static_cast<MyClass&>(xe).something;
            std::cout<<std::endl;

            std::cout<<container.at("y")->something<<std::endl;

            auto i=container.find("x");
            std::cout<<"'x' is contained: "<<(i!=container.end())<<std::endl;

            container.erase(i);

            std::cout<<"xe is valid: "<<bool(xe)<<std::endl;

            for (auto& e: container)
            {
                std::cout<<e.first<<" => "<<e.second->something<<std::endl;
            }

            std::cout<<"ref size="<<refc.size()<<std::endl;

        }

        insight::assertion(
            !observerObserver->valid(),
            "observerObserver expected to be invalid"
            );


        {
            typedef std::vector<std::unique_ptr<MyClass> > Container;

            Container container;

            container.push_back(make_unique<MyClass>("xx"));
            container.push_back(make_unique<MyClass>("yy"));

            auto &x=*container[1];

            std::cout<<container.at(1)->something<<std::endl;

            for (auto& e: container)
            {
                std::cout<<e->something<<std::endl;
            }
        }

        return 0;
    }
    catch (std::exception& e)
    {
        cerr<<e.what()<<endl;
        return -1;
    }
}
