#ifndef PATCH_H
#define PATCH_H

#include <QListWidgetItem>

#include <string>

#ifndef Q_MOC_RUN
#include "base/parameterset.h"
#include "openfoam/openfoamcase.h"
#endif

class Patch
: public QListWidgetItem
{
protected:
    std::string patch_name_;
    std::string bc_type_;
    insight::ParameterSet curp_;

public:
    Patch(QListWidget*, const std::string& patch_name);
    Patch(QListWidget*, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, boost::filesystem::path inputfilepath);

    virtual void set_bc_type(const std::string& type_name);

    inline const std::string& patch_name() const { return patch_name_; }
    inline const std::string& bc_type() const { return bc_type_; }
    inline insight::ParameterSet& parameters() { return curp_; }
    inline const insight::ParameterSet& parameters() const { return curp_; }
    virtual bool insertElement(insight::OpenFOAMCase& ofc, insight::OFDictData::dict& boundaryDict) const;
    void appendToNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, boost::filesystem::path inputfilepath);
};


class DefaultPatch
: public Patch
{
public:
    DefaultPatch(QListWidget*);
    DefaultPatch(QListWidget*, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, boost::filesystem::path inputfilepath);
    virtual bool insertElement(insight::OpenFOAMCase& ofc, insight::OFDictData::dict& boundaryDict) const;
};


#endif // PATCH_H
