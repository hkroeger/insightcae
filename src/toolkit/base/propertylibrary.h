#ifndef INSIGHT_PROPERTYLIBRARY_H
#define INSIGHT_PROPERTYLIBRARY_H

#include <map>
#include <memory>
#include <vector>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "base/exception.h"
#include "base/tools.h"
#include "base/rapidxml.h"

#include "rapidxml/rapidxml.hpp"

#include "base/parameterset.h"
#include "base/parametersetinput.h"


namespace insight
{


class PropertyLibraryBase
{
protected:
    std::string libraryName_;

public:
    PropertyLibraryBase(const std::string& libraryName);

    virtual std::vector<std::string> entryList() const =0;

    virtual std::string icon(const std::string&) const;

    virtual std::unique_ptr<insight::ParameterSet>
    defaultParameters(const std::string& label) const;
};



template<class PropertyLibraryEntry, class PropertyLibraryInstance = PropertyLibraryEntry >
class MapPropertyLibrary
    : public PropertyLibraryBase,
      public std::map<std::string, std::shared_ptr<PropertyLibraryEntry> >
{
public:
    typedef PropertyLibraryEntry value_type;
    typedef PropertyLibraryInstance instance_type;

public:
    MapPropertyLibrary(const std::string& libraryName = "")
        : PropertyLibraryBase(libraryName)
    {}

public:
    std::vector<std::string> entryList() const override
    {
        std::vector<std::string> entries;
        for (const auto& e: *this)
        {
            entries.push_back(e.first);
        }
        return entries;
    }

    const value_type& lookup(const std::string& label) const
    {
        auto i = this->find(label);
        if (i == this->end())
        {
            throw insight::Exception(
                "There is no entry "+label+" in the property library!\n"
                                               "Known entries: "+boost::join(entryList(), " ") );
        }
        return *i->second;
    }

    /**
     * @brief createInstance
     * Default (non-parameterized) manufacturing: since instance_type defaults to
     * value_type, simply hand back the library-owned entry itself, ignoring the
     * supplied parameters. PropertyLibraryWithParameters overrides this for libraries
     * whose entries are actual generators that need to be manufactured from a
     * ParameterSet.
     */
    std::shared_ptr<instance_type> createInstance(const std::string& label, const insight::ParameterSet&) const
    {
        auto i = this->find(label);
        if (i == this->end())
        {
            throw insight::Exception(
                "There is no entry "+label+" in the property library!\n"
                                               "Known entries: "+boost::join(entryList(), " ") );
        }
        return i->second;
    }

    std::string labelOf(const PropertyLibraryEntry& entry) const
    {
        std::string label;

        auto i = std::find_if(
            this->begin(), this->end(),
            [&](const std::pair<std::string, std::shared_ptr<PropertyLibraryEntry> >& v)
            {
                return v.second.get()==&entry;
            }
            );
        if (i!=this->end()) label=i->first;

        return label;
    }
};




template<
    class PropertyLibraryEntry,
    const boost::filesystem::path* subDir = nullptr,
    class PropertyLibraryInstance = PropertyLibraryEntry>
class PropertyLibrary
    : public MapPropertyLibrary<PropertyLibraryEntry, PropertyLibraryInstance>
{

public:
    PropertyLibrary(const std::string& libraryName = "")
        : MapPropertyLibrary<PropertyLibraryEntry, PropertyLibraryInstance>(libraryName)
    {
        CurrentExceptionContext ex("reading property library %s", this->libraryName_.c_str());

        if (this->libraryName_.empty())
        {
            this->libraryName_ =
                MapPropertyLibrary<PropertyLibraryEntry, PropertyLibraryInstance>::value_type::typeName;
            insight::assertion(
                        !this->libraryName_.empty(),
                        "the property library entry must not have empty type names!" );
        }

        boost::filesystem::path subDirectory;
        if (subDir) subDirectory = *subDir;

        auto sp=subDirectory / (this->libraryName_+"Library.xml");

        bool found;
        auto fp =  SharedPathList::global().getSharedFilePath(
                    sp, &found );


        if (!found)
        {
            insight::Warning(
                "Shared library database file %s not found!"
                " Please check your installation!"
                " Library remains empty.",
                sp.string().c_str() );
        }
        else
        {
            CurrentExceptionContext ex("reading property library file %s", sp.string().c_str());

            bool anythingRead=false;

            // read xml
            XMLDocument doc(fp);

            if (auto *rootnode = doc.rootNode)
            {
              for (auto *e = rootnode->first_node(); e; e = e->next_sibling())
              {
                std::string nodeName(e->name());
                if (nodeName=="entry")
                {
                    // create multiple entries with same value and different labels,
                    // if multiple label attributes are present
                    int nc=0;
                    for (auto *l=e->first_attribute("label"); l; l=l->next_attribute("label"))
                    {
                        nc++;
                        std::string label(l->value());
                        insight::CurrentExceptionContext ex("reading library entry "+label);

                        if (this->find(label) != this->end())
                        {
                            insight::Warning(
                                        "Replacing previously read entry "+label
                                        + " in library "+libraryName
                                        + " with that from "+fp.string() );
                        }

                        this->insert(std::make_pair(
                            label,
                            std::make_shared<
                                typename MapPropertyLibrary<PropertyLibraryEntry, PropertyLibraryInstance>
                                    ::value_type>( *e )));

                        anythingRead=true;
                    }

                    if (nc==0)
                    {
                        insight::Warning("Malformed entry node in "+fp.string()+": no label attribute!");
                    }
                }
                else
                  insight::Warning("Ignoring unrecognized XML node \""+nodeName+"\" in file "+fp.string());
              }
            }
            else
              throw insight::Exception("No valid \"pads\" node found in file \""+fp.string()+"\"!");

            if (!anythingRead)
              insight::Warning("Could not any read valid data from "+fp.string());
        }

    }

public:

    static const PropertyLibrary& library()
    {
        static PropertyLibrary theLibrary;
        return theLibrary;
    }
};




template<class BaseClass>
class PropertyLibraryWithIcons
 : public BaseClass
{
public:
    std::string icon(const std::string& label) const override
    {
        return this->lookup(label).icon();
    }

    static const PropertyLibraryWithIcons& library()
    {
        static PropertyLibraryWithIcons theLibrary;
        return theLibrary;
    }
};




template<class BaseClass>
class PropertyLibraryWithParameters
    : public BaseClass
{

public:

    typedef typename BaseClass::instance_type instance_type;

    // keep the plain catalog-entry lookup(label) visible: used by defaultParameters() and
    // createInstance() below, which fetch the entry (value_type, the "generator") in order
    // to ask it for its default parameters or to manufacture an actual instance from them.
    using BaseClass::lookup;

    std::unique_ptr<insight::ParameterSet>
    defaultParameters(const std::string& label) const override
    {
        return BaseClass::lookup(label).defaultParameters();
    }

    /**
     * @brief createInstance
     * Manufacture the actual instance for the given library entry, using the
     * consumer-supplied parameters (typically obtained from
     * PropertyLibrarySelectionParameter::instanceParameters(), seeded from
     * defaultParameters() above and possibly edited by the consumer).
     * Delegates to the entry class's own createInstance(const ParameterSet&), which
     * every entry used with PropertyLibraryWithParameters must implement.
     */
    std::shared_ptr<instance_type>
    createInstance(const std::string& label, const insight::ParameterSet& parameters) const
    {
        return BaseClass::lookup(label).createInstance(parameters);
    }

    static const PropertyLibraryWithParameters& library()
    {
        static PropertyLibraryWithParameters theLibrary;
        return theLibrary;
    }
};



} // namespace insight

#endif // INSIGHT_PROPERTYLIBRARY_H
