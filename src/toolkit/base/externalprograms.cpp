#include "externalprograms.h"

#include "base/tools.h"

#include "rapidxml/rapidxml_print.hpp"
#include <boost/range/adaptor/reversed.hpp>

namespace insight {

std::vector<std::string> requiredPrograms = {
    "paraview",
    "pdflatex",
    "gnuplot",
    "iscad",
#ifdef WIN32
    "plink"
#else
    "ssh"
#endif
};

ExternalPrograms::ExternalPrograms()
{
    for (const auto& exe: requiredPrograms)
    {
        auto p = boost::process::search_path(exe);
        emplace(exe, p);
    }

    auto paths = SharedPathList::global();
    for ( const bfs_path& p: boost::adaptors::reverse(paths) ) // reverse: start with global, then per-user to possibly overwrite global
    {
        if ( exists(p) && is_directory ( p ) )
        {
            bfs_path file = bfs_path(p) / "externalPrograms.list";

            if ( exists(file) )
            {
                insight::dbg()<<"reading external programs from "<<file<<std::endl;
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
                    throw insight::Exception("No valid \"remote\" node found in XML!");

                for (xml_node<> *e = rootnode->first_node(); e; e = e->next_sibling())
                {
                    if (e->name()==std::string("externalProgram"))
                    {
                        auto* exe = e->first_attribute("executable");
                        auto* path = e->first_attribute("path");
                        if (exe && path)
                        {
                            insight::dbg()<<"setting path for "<<exe->value()<<" to \""<<path->value()<<"\""<<std::endl;
                            (*this)[exe->value()]=boost::filesystem::path(path->value());
                        }
                        else
                        {
                            insight::Warning("mal-formed entry in "+file.string()+": required attribute \"executable\" or \"path\" not found." );
                        }
                    }
                }
            }
        }
    }
}

ExternalPrograms::ExternalPrograms(const ExternalPrograms &o)
    : std::map<std::string, boost::filesystem::path>(o)
{}

boost::filesystem::path ExternalPrograms::firstWritableLocation() const
{
    return insight::SharedPathList::global()
            .findFirstWritableLocation( "externalPrograms.list" );
}

void ExternalPrograms::writeConfiguration(const boost::filesystem::path &file)
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
      xml_node<> *exenode = doc.allocate_node(node_element, "externalProgram");
      exenode->append_attribute(doc.allocate_attribute
                                   ("executable",
                                     doc.allocate_string(rs.first.c_str()) ) );
      exenode->append_attribute(doc.allocate_attribute
                                   ("path",
                                     doc.allocate_string(rs.second.string().c_str()) ) );
      rootnode->append_node(exenode);
    }

    doc.append_node(rootnode);

    ofstream f(file.string());
    f << doc;
}



std::vector<std::string> ExternalPrograms::missingPrograms() const
{
    std::vector<std::string> mp;
    for (const auto& p: *this)
    {
        if (p.second.empty())
            mp.push_back(p.first);
    }
    return mp;
}


ExternalPrograms& ExternalPrograms::globalInstance()
{
    static ExternalPrograms externalPrograms;
    return externalPrograms;
}

boost::filesystem::path ExternalPrograms::path(const std::string &exeName)
{
    auto i=globalInstance().find(exeName);
    if (i!=globalInstance().end())
        return i->second;
    else
        throw insight::Exception("No path known to executable "+exeName+"! Please check external programs configuration!");
}

} // namespace insight
