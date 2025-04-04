#include "analysisparameterpropositions.h"

#include "base/analysis.h"

#include <dlfcn.h>

#include "base/boost_include.h"
#include "boost/function.hpp"
#include "boost/python.hpp"

#include "base/pythoninterface.h"
#include "swigpyrun.h"
#define SWIG_as_voidptr(a) const_cast< void * >(static_cast< const void * >(a))

namespace insight
{



struct PythonFunction
{

    boost::filesystem::path scriptFile_;
    std::string functionName_;

    PythonFunction(const boost::filesystem::path& scriptFile, const std::string functionName)
        : scriptFile_(scriptFile), functionName_(functionName)
    {
        insight::assertion(
            boost::filesystem::exists(scriptFile_),
            "Script file "+scriptFile_.string()+" does not exist!" );
    }

    std::unique_ptr<ParameterSet> operator()(
        const std::string &parameterPath,
        const ParameterSet &currentParameterValues )
    {
        using namespace boost;
        using namespace boost::python;

        acquire_py_GIL locker;

        try {

            object main_module(handle<>(borrowed(PyImport_AddModule("__main__"))));
            object main_namespace = main_module.attr("__dict__");
            object toolkit = import("Insight.toolkit");

            static void *descr = nullptr;
            if (!descr) {
                descr = SWIG_TypeQuery("insight::ParameterSet *");    /* Get the type descriptor structure for Foo */
                assert(descr);
            }

            PyObject *currentParameterValuesObj;
            if (!(currentParameterValuesObj =
                  SWIG_NewPointerObj(SWIG_as_voidptr(&currentParameterValues), descr, 0 )))
            {
                throw insight::Exception("Could not create python parameter object!");
            }


            main_module.attr("parameterPath")=parameterPath;
            main_module.attr("currentParameterValues")=python::borrowed(currentParameterValuesObj);

            std::string cmd = boost::str( boost::format(
                "import imp;"
                "mod = imp.load_source('module', '%s');"
                //"result=mod.%s(parameterPath)"
                "result=mod.%s(parameterPath,currentParameterValues)"
                ) % scriptFile_.string() % functionName_
            );

            handle<> ignore(PyRun_String(
                cmd.c_str(),
                Py_file_input,
                main_namespace.ptr(),
                main_namespace.ptr() )
            );

            object o =  extract<object>(main_namespace["result"]);
            ParameterSet *res;
            descr=nullptr;
            if (!descr) {
                descr = SWIG_TypeQuery("insight::ParameterSet *");    /* Get the type descriptor structure for Foo */
                assert(descr);
            }
            if ((SWIG_ConvertPtr(o.ptr(), (void **) &res, descr, 0) == -1)) {
                throw insight::Exception("Could not convert return value!");
            }
            insight::assertion(
                res,
                "the function %s in python module %s has to return a ParameterSet object!",
                functionName_.c_str(), scriptFile_.c_str());
            return std::unique_ptr<ParameterSet>(res);
        }
        catch (const error_already_set &)
        {
            PyErr_Print();
            throw insight::Exception(
                "Failed to execute "+functionName_+" in script file "+scriptFile_.string()+".");
        }

        return nullptr;
    }
};



struct SharedLibraryFunction
: public AnalysisParameterPropositions::propositionGeneratorFunction
{
    void *libraryHandle_;

    SharedLibraryFunction(const boost::filesystem::path& libFile, const std::string functionName)
    {
#ifdef WIN32
        libraryHandle_ = LoadLibraryA(libFile.string().c_str());

        insight::assertion(
            libraryHandle_,
            "Failed to load library "+libFile.string()+"!");

        if (auto gfp=GetProcAddress(
                static_cast<HMODULE>(libraryHandle_), functionName.c_str()))
        {
            typedef std::unique_ptr<ParameterSet> (*FunctionPointer) (const std::string&, const ParameterSet&);
            this->AnalysisParameterPropositions::propositionGeneratorFunction::operator=(
                reinterpret_cast<FunctionPointer>(gfp) );
        }
        else
        {
            throw insight::Exception(
                "Failed to bind function "+functionName+" from library "+libFile.string()+"!");
        }
#else
        libraryHandle_ = dlopen (
            libFile.string().c_str(),
            RTLD_LAZY|RTLD_GLOBAL|RTLD_NODELETE );

        insight::assertion(
            libraryHandle_,
            "Failed to load library "+libFile.string()+"! Reason: "+dlerror() );

        if (auto gfp=dlsym(libraryHandle_, functionName.c_str()))
        {
            typedef std::unique_ptr<ParameterSet> (*FunctionPointer) (
                const std::string&, const ParameterSet&);

            this->AnalysisParameterPropositions::propositionGeneratorFunction::operator=(
                reinterpret_cast<FunctionPointer>(gfp) );
        }
        else
        {
            throw insight::Exception(
                "Failed to bind function "+functionName+" from library "+libFile.string()+"!");
        }
#endif



    }
};


AnalysisParameterPropositions::AnalysisParameterPropositions()
{
    boost::filesystem::path fp;
    try
    {
        fp =  SharedPathList::global().getSharedFilePath(
            "parameterPropositionSources.xml" );
    }
    catch (...)
    {
        return;
    }

    CurrentExceptionContext ex("reading external parameter proposition sources from "+fp.string());

    // read xml
    std::string content;
    readFileIntoString(fp, content);

    using namespace rapidxml;
    xml_document<> doc;

    try
    {
        doc.parse<0>(&content[0]);
    }
    catch (...)
    {
        throw insight::Exception("Failed to parse XML");
    }

    if (auto *rootnode = doc.first_node("root"))
    {
        for (xml_node<> *e = rootnode->first_node("analysis"); e; e = e->next_sibling("analysis"))
        {
                if (auto *l=e->first_attribute("label"))
                {
                    std::string analysisName(l->value());

                    if (Analysis::analyses().count(analysisName))
                    {
                        insight::Warning(
                            "Proposition sources for non-existing analysis "+analysisName
                            + " given in "+fp.string()+"! Ignored." );
                    }
                    else
                    {
                        for (auto *a = e->first_attribute(); a; a=a->next_attribute())
                        {
                            std::string attrname(a->name());
                            std::cout<<attrname<<" >>> "<<a->value()<<std::endl;
                            if (attrname=="pythonScript")
                            {
                                std::string arg(a->value());
                                std::vector<std::string> args;
                                boost::split(args, arg, boost::is_any_of(":"));

                                insight::assertion(
                                    (args.size()>=1) && (args.size()<=2),
                                    "pythonScript string must be of form <script file name>[:<function name>]!");

                                boost::filesystem::path fileName(args[0]);
                                std::string funcName="getPropositionsForParameter";
                                if (args.size()>1) funcName=args[1];

                                propositionGeneratorFunctions_[analysisName].push_back(
                                    PythonFunction(fileName, funcName)
                                    );
                            }
                            else if (attrname=="sharedLibrary")
                            {
                                std::string arg(a->value());
                                std::vector<std::string> args;
                                boost::split(args, arg, boost::is_any_of(":"));

                                insight::assertion(
                                    (args.size()>=1) && (args.size()<=2),
                                    "sharedLibrary string must be of form <library file name>[:<function name>]!");

                                boost::filesystem::path fileName(args[0]);
                                std::string funcName="getPropositionsForParameter";
                                if (args.size()>1) funcName=args[1];

                                propositionGeneratorFunctions_[analysisName].push_back(
                                    SharedLibraryFunction(fileName, funcName)
                                    );
                            }
                        }
                    }
                }
                else
                {
                    insight::Warning("Malformed node \"analysis\" in "+fp.string()+": no label attribute!");
                }
        }
    }
}



std::unique_ptr<AnalysisParameterPropositions> AnalysisParameterPropositions::instance;



std::unique_ptr<ParameterSet>
AnalysisParameterPropositions::getCombinedPropositionsForParameter(
    const std::string &analysisName,
    const std::string &parameterPath,
    const ParameterSet &currentParameterValues )
{
    auto props=ParameterSet::create();

    if (auto hardCodedProps =
          Analysis::propositionsForParameter()(
            analysisName, parameterPath, currentParameterValues))
    {
        props->extend(*hardCodedProps);
    }

    if (!instance)
        instance.reset(new AnalysisParameterPropositions);

    auto &pgfs = instance->propositionGeneratorFunctions_;

    auto pgf = pgfs.find(analysisName);
    if (pgf != pgfs.end())
    {
        for (auto &f: pgf->second)
        {
            auto props2 = f(parameterPath, currentParameterValues);
            props->extend(*props2);
        }
    }

    return props;
}


}
