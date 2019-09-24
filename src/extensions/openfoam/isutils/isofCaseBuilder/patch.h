#ifndef PATCH_H
#define PATCH_H

#include <QListWidgetItem>

#include <string>

#ifndef Q_MOC_RUN
#include "base/parameterset.h"
#include "openfoam/openfoamcase.h"
#endif

#include "insertedcaseelement.h"

class Patch
: public CaseElementData
{
protected:
    std::string patch_name_;
//    std::string bc_type_;

public:
    Patch(QListWidget*, const std::string& patch_name, ParameterSetDisplay* d);
    Patch(QListWidget*, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, boost::filesystem::path inputfilepath, ParameterSetDisplay* d);

    virtual void set_bc_type(const std::string& type_name);
    void updateText();

    void set_patch_name(const QString& newname);
    inline const std::string& patch_name() const { return patch_name_; }
//    inline const std::string& bc_type() const { return bc_type_; }
    virtual bool insertElement(insight::OpenFOAMCase& ofc, insight::OFDictData::dict& boundaryDict) const;

    void appendToNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, boost::filesystem::path inputfilepath);
};



class DefaultPatch
: public Patch
{
public:
    DefaultPatch(QListWidget*, ParameterSetDisplay* d);
    DefaultPatch(QListWidget*, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, boost::filesystem::path inputfilepath, ParameterSetDisplay* d);
    virtual bool insertElement(insight::OpenFOAMCase& ofc, insight::OFDictData::dict& boundaryDict) const;

    const static QString defaultPatchName;
};


#endif // PATCH_H
