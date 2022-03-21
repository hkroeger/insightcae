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
};




template<class PropertyLibraryEntry, const boost::filesystem::path* subDir = nullptr>
class PropertyLibrary
      : public PropertyLibraryBase,
        public std::map<std::string, std::shared_ptr<PropertyLibraryEntry> >
{
public:
    typedef PropertyLibraryEntry value_type;

public:
    PropertyLibrary(const std::string& libraryName = "")
        : PropertyLibraryBase(libraryName)
    {
        CurrentExceptionContext ex("reading property library "+libraryName_);

        if (libraryName_.empty())
        {
            libraryName_ = value_type::typeName;
            insight::assertion(
                        !libraryName_.empty(),
                        "the property library entry must not have empty type names!" );
        }

        boost::filesystem::path subDirectory;
        if (subDir) subDirectory = *subDir;
        auto fp =  SharedPathList().getSharedFilePath(
                    subDirectory / (libraryName_+"Library.xml") );

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

                    if (this->find(label) != this->end())
                    {
                        insight::Warning(
                                    "Replacing previously read entry "+label
                                    + " in library "+libraryName_
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

    static const PropertyLibrary& library()
    {
        static PropertyLibrary theLibrary;
        return theLibrary;
    }
};



} // namespace insight

#endif // INSIGHT_PROPERTYLIBRARY_H
