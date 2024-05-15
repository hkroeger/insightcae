#ifndef INSIGHT_PROPERTYLIBRARY_H
#define INSIGHT_PROPERTYLIBRARY_H

#include <map>
#include <memory>
#include <vector>
#include <string>

#include "base/boost_include.h"
#include "base/exception.h"
#include "base/tools.h"

#include "rapidxml/rapidxml.hpp"

namespace insight
{




class PropertyLibraryBase
{
protected:
    std::string libraryName_;

public:
    PropertyLibraryBase(const std::string& libraryName);

    virtual std::vector<std::string> entryList() const =0;

    virtual std::string icon(const std::string&) const
    {
        return std::string();
    }
};



template<class PropertyLibraryEntry>
class MapPropertyLibrary
    : public PropertyLibraryBase,
      public std::map<std::string, std::shared_ptr<PropertyLibraryEntry> >
{
public:
    typedef PropertyLibraryEntry value_type;

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




template<class PropertyLibraryEntry, const boost::filesystem::path* subDir = nullptr>
class PropertyLibrary
    : public MapPropertyLibrary<PropertyLibraryEntry>
{
public:
    typedef PropertyLibraryEntry value_type;

public:
    PropertyLibrary(const std::string& libraryName = "")
        : MapPropertyLibrary<PropertyLibraryEntry>(libraryName)
    {
        CurrentExceptionContext ex("reading property library %s", libraryName.c_str());

        if (libraryName.empty())
        {
            libraryName = value_type::typeName;
            insight::assertion(
                        !libraryName.empty(),
                        "the property library entry must not have empty type names!" );
        }

        boost::filesystem::path subDirectory;
        if (subDir) subDirectory = *subDir;

        auto sp=subDirectory / (libraryName+"Library.xml");

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
              throw insight::Exception("Failed to parse XML from file "+fp.string());
            }

            if (auto *rootnode = doc.first_node("root"))
            {
              for (xml_node<> *e = rootnode->first_node(); e; e = e->next_sibling())
              {
                std::string nodeName(e->name());
                if (nodeName=="entry")
                {
                    if (auto *l=e->first_attribute("label"))
                    {
                        std::string label(l->value());
                        insight::CurrentExceptionContext ex("reading library entry "+label);

                        if (this->find(label) != this->end())
                        {
                            insight::Warning(
                                        "Replacing previously read entry "+label
                                        + " in library "+libraryName
                                        + " with that from "+fp.string() );
                        }

                        this->insert(std::make_pair(label, std::make_shared<value_type>(*e)));

                        anythingRead=true;
                    }
                    else
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




template<class PropertyLibraryEntry, const boost::filesystem::path* subDir = nullptr>
class PropertyLibraryWithIcons
    : public PropertyLibrary<PropertyLibraryEntry, subDir>
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




} // namespace insight

#endif // INSIGHT_PROPERTYLIBRARY_H
