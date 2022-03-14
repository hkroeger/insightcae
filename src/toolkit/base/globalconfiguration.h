#ifndef INSIGHT_GLOBALCONFIGURATION_H
#define INSIGHT_GLOBALCONFIGURATION_H

#include <map>
#include <fstream>

#include "base/tools.h"
#include "base/boost_include.h"
#include <boost/range/adaptor/reversed.hpp>

#include "rapidxml/rapidxml_print.hpp"

namespace insight {




class InvalidConfigurationItem
: public std::exception
{
public:
    InvalidConfigurationItem();
};




template<class ConfigItem>
class GlobalConfiguration
        : public std::map<std::string, ConfigItem>
{
protected:
    boost::filesystem::path configFileName_;

    virtual void readAdditionalData(
            rapidxml::xml_document<>& doc,
            rapidxml::xml_node<> *root )
    {}

    virtual void writeAdditionalData(
            rapidxml::xml_document<>& doc,
            rapidxml::xml_node<> *root ) const
    {}

    virtual void readConfiguration()
    {
        SharedPathList paths;
        for ( const bfs_path& p: boost::adaptors::reverse(paths) ) // reverse: start with global, then per-user to possibly overwrite global
        {
            if ( exists(p) && is_directory ( p ) )
            {
                bfs_path file = bfs_path(p) / configFileName_;

                if ( exists(file) )
                {
                    insight::dbg()<<"reading configuration data from "<<file<<std::endl;

                    // read xml
                    std::string content;
                    readFileIntoString(file, content);

                    using namespace rapidxml;
                    xml_document<> doc;

                    try {
                        doc.parse<0>(&content[0]);
                    }
                    catch (...)
                    {
                        throw insight::Exception("Failed to parse XML from file "+file.string());
                    }

                    auto *rootnode = doc.first_node("root");
                    if (!rootnode)
                        throw insight::Exception("No valid root node found in XML!");

                    for (xml_node<> *e = rootnode->first_node(); e; e = e->next_sibling())
                    {
                        try
                        {
                           ConfigItem newitem(e);
                           this->insertItem(newitem);
                        }
                        catch (InvalidConfigurationItem& ici)
                        {
                            std::string reason(ici.what());
                            if (reason.empty())
                                insight::Warning(reason);
                        }
                    }

                    readAdditionalData(doc, rootnode);
                }
            }
        }
    }


    /**
     * @brief GlobalConfiguration
     * read from global config file
     */
    GlobalConfiguration(
            const boost::filesystem::path& configFileName
            )
        : configFileName_(configFileName)
    {}

public:
    GlobalConfiguration( const GlobalConfiguration& o)
        : std::map<std::string, ConfigItem>(o),
          configFileName_(o.configFileName_)
    {}

    std::pair<typename std::map<std::string, ConfigItem>::iterator, bool>
    insertItem(const ConfigItem& newitem)
    {
        return this->insert(std::make_pair(newitem.label(), newitem));
    }

    boost::filesystem::path firstWritableLocation() const
    {
        return insight::SharedPathList()
                .findFirstWritableLocation( configFileName_ );
    }


    void writeConfiguration(const boost::filesystem::path& file) const
    {
        using namespace rapidxml;

        if (!boost::filesystem::exists(file.parent_path()))
            boost::filesystem::create_directories(file.parent_path());

        xml_document<> doc;

        xml_node<>* decl = doc.allocate_node(node_declaration);
        decl->append_attribute(doc.allocate_attribute("version", "1.0"));
        decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
        doc.append_node(decl);

        xml_node<> *rootnode = doc.allocate_node(node_element, "root");
        for (const auto& rs: *this)
        {
            if (xml_node<> *node = rs.second.createNode(doc))
            {
                rootnode->append_node(node);
            }
        }

        writeAdditionalData(doc, rootnode);

        doc.append_node(rootnode);

        ofstream f(file.string());
        f << doc;
    }

    void reload() const
    {
        auto *ncthis = const_cast<GlobalConfiguration*>(this);
        ncthis->clear();
        ncthis->readConfiguration();
    }

};






template<class ConfigItem>
class GlobalConfigurationWithDefault
        : public GlobalConfiguration<ConfigItem>
{
protected:
    std::string defaultLabel_;

    void readAdditionalData(
                rapidxml::xml_document<>& doc,
                rapidxml::xml_node<> *root )
    {
        using namespace rapidxml;
        if (auto *dfl = root->first_node("defaultLabel"))
        {
            defaultLabel_=dfl->value();
            if (this->find(defaultLabel_)==this->end())
                throw insight::Exception("The default label \""+defaultLabel_+"\" does not exist!");
        }
        else
        {
            throw insight::Exception("There is no default entry label defined!");
        }
    }


    void writeAdditionalData(
            rapidxml::xml_document<>& doc,
            rapidxml::xml_node<> *root ) const override
    {
        using namespace rapidxml;
        xml_node<> *node = doc.allocate_node(node_element, "defaultLabel");
        node->value( doc.allocate_string(defaultLabel_.c_str()) );
        root->append_node(node);
    }


    /**
     * @brief GlobalConfiguration
     * read from global config file
     */
    GlobalConfigurationWithDefault(
            const boost::filesystem::path& configFileName
            )
        : GlobalConfiguration<ConfigItem>(configFileName),
          defaultLabel_()
    {}

public:
    GlobalConfigurationWithDefault( const GlobalConfigurationWithDefault& o)
        : GlobalConfiguration<ConfigItem>(o),
          defaultLabel_(o.defaultLabel_)
    {}


    typename std::map<std::string, ConfigItem>::const_iterator defaultItemIterator() const
    {
        return this->find(defaultLabel_);
    }


    const ConfigItem& defaultItem() const
    {
        auto r=defaultItemIterator();
        insight::assertion(
                    r!=this->end(),
                    "default template entry "+defaultLabel_+" does not exist!" );
        return r->second;
    }


    void setDefaultItem(const std::string& key)
    {
        auto r = this->find(key);
        insight::assertion(
                    r!=this->end(),
                    "designated default template entry "+key+" does not exist!" );
        defaultLabel_=key;
    }
};

} // namespace insight

#endif // INSIGHT_GLOBALCONFIGURATION_H
