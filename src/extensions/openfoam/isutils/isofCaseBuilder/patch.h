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

    insight::CADParameterSetModelVisualizer::VisualizerFunctions::Function
        getVisualizerFactoryFunction() override;

public:
    Patch(
        const std::string& patch_name,
        insight::MultiCADParameterSetVisualizer::SubVisualizerList& mvl,
        MultivisualizationGenerator* visGen,
        QObject* parent=nullptr
        );

    Patch(
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        boost::filesystem::path inputfilepath,
        insight::MultiCADParameterSetVisualizer::SubVisualizerList& mvl,
        MultivisualizationGenerator* visGen,
        QObject* parent=nullptr
        );

    virtual void set_bc_type(const std::string& type_name);
//    void updateText();

    void set_patch_name(const QString& newname);
    inline const std::string& patch_name() const { return patch_name_; }
    virtual bool insertElement(insight::OpenFOAMCase& ofc, insight::OFDictData::dict& boundaryDict) const;

    void appendToNode(rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, boost::filesystem::path inputfilepath);
};




class DefaultPatch
: public Patch
{
public:
    DefaultPatch(
        insight::MultiCADParameterSetVisualizer::SubVisualizerList& mvl,
        MultivisualizationGenerator* visGen,
        QObject* parent=nullptr
        );

    DefaultPatch(
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        boost::filesystem::path inputfilepath,
        insight::MultiCADParameterSetVisualizer::SubVisualizerList& mvl,
        MultivisualizationGenerator* visGen,
        QObject* parent=nullptr
        );

    virtual bool insertElement(insight::OpenFOAMCase& ofc, insight::OFDictData::dict& boundaryDict) const;

    const static QString defaultPatchName;
};




#endif // PATCH_H
