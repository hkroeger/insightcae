/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef INSIGHT_CAD_MODEL_H
#define INSIGHT_CAD_MODEL_H

#include "cadtypes.h"
#include "base/exception.h"
#include "cadparameters.h"
#include "mapkey_parser.h"
#include "astbase.h"

#include <boost/spirit/include/qi.hpp>

#include <vtkSmartPointer.h>
#include <vtkDataObject.h>

#include <map>
#include <string>




namespace insight 
{
namespace cad 
{

namespace parser {
class SyntaxElementDirectory;
typedef std::shared_ptr<SyntaxElementDirectory> SyntaxElementDirectoryPtr;
}
    
    
class Model
: public ASTBase
{
public:

    typedef std::map<std::string, ScalarPtr> 	ScalarTableContents;
    typedef std::map<std::string, VectorPtr> 	VectorTableContents;
    typedef std::map<std::string, DatumPtr> 	DatumTableContents;
    typedef std::map<std::string, FeaturePtr> 	ModelstepTableContents;
    typedef std::map<std::string, ModelPtr> 	ModelTableContents;

    typedef std::map<std::string, FeatureSetPtr> 	VertexFeatureTableContents;
    typedef std::map<std::string, FeatureSetPtr> 	EdgeFeatureTableContents;
    typedef std::map<std::string, FeatureSetPtr> 	FaceFeatureTableContents;
    typedef std::map<std::string, FeatureSetPtr> 	SolidFeatureTableContents;
    typedef std::map<std::string, PostprocActionPtr> 	PostprocActionTableContents;
    typedef std::map<std::string, vtkSmartPointer<vtkDataObject> > 	DatasetTableContents;

    typedef boost::spirit::qi::symbols<char, ScalarPtr> 	ScalarTable;
    typedef boost::spirit::qi::symbols<char, VectorPtr> 	VectorTable;
    typedef boost::spirit::qi::symbols<char, DatumPtr>          DatumTable;
    typedef boost::spirit::qi::symbols<char, FeaturePtr> 	ModelstepTable;
    typedef boost::spirit::qi::symbols<char, ModelPtr>          ModelTable;
    typedef std::set<std::string>                               ComponentSet;

    typedef boost::spirit::qi::symbols<char, FeatureSetPtr> 	VertexFeatureTable;
    typedef boost::spirit::qi::symbols<char, FeatureSetPtr> 	EdgeFeatureTable;
    typedef boost::spirit::qi::symbols<char, FeatureSetPtr> 	FaceFeatureTable;
    typedef boost::spirit::qi::symbols<char, FeatureSetPtr> 	SolidFeatureTable;
    typedef boost::spirit::qi::symbols<char, PostprocActionPtr> PostprocActionTable;
    typedef DatasetTableContents                                DatasetTable;

protected:
    std::string                 description_;
    double                      cost_;

    ScalarTable 		scalars_;
    VectorTable 		points_, directions_;
    DatumTable                  datums_;
    ModelstepTable              modelsteps_;
    ComponentSet		components_;
    VertexFeatureTable          vertexFeatures_;
    EdgeFeatureTable            edgeFeatures_;
    FaceFeatureTable            faceFeatures_;
    SolidFeatureTable           solidFeatures_;
    ModelTable                  models_;
    PostprocActionTable         postprocActions_;
    DatasetTable                datasets_;

    insight::cad::parser::SyntaxElementDirectoryPtr syn_elem_dir_;
    boost::filesystem::path modelfile_;


    void defaultVariables();
    void copyVariables(const ModelVariableTable& vars);

    virtual size_t calcHash() const;
    virtual void build();

public:

    Model(const ModelVariableTable& vars = ModelVariableTable());
    Model(const std::string& modelname, const ModelVariableTable& vars = ModelVariableTable());
    Model(const boost::filesystem::path& modelfile, const ModelVariableTable& vars = ModelVariableTable());

    void setDescription(const std::string& description);
    void setCost(double cost);

    const ScalarTable& 	scalarSymbols() const;
    const VectorTable&	pointSymbols() const;
    const VectorTable&	directionSymbols() const;
    const DatumTable&	datumSymbols() const;
    const ModelstepTable&	modelstepSymbols() const;
    const VertexFeatureTable&	vertexFeatureSymbols() const;
    const EdgeFeatureTable&	edgeFeatureSymbols() const;
    const FaceFeatureTable& 	faceFeatureSymbols() const;
    const SolidFeatureTable& 	solidFeatureSymbols() const;
    const ModelTable& 	modelSymbols() const;
    const PostprocActionTable& 	postprocActionSymbols() const;
    const DatasetTable& 	datasets() const;


    /**
     * @brief addScalar
     * add or replace existing
     * @param name
     * @param value
     */
    void addScalar(const std::string& name, ScalarPtr value);

    /**
     * @brief addScalarIfNotPresent
     * Only adds, if symbol is not present. Else it don't does nothing (not replace).
     * @param name
     * @param value
     */
    void addScalarIfNotPresent(const std::string& name, ScalarPtr value);
    void addPoint(const std::string& name, VectorPtr value);
    void addPointIfNotPresent(const std::string& name, VectorPtr value);
    void addDirection(const std::string& name, VectorPtr value);
    void addDirectionIfNotPresent(const std::string& name, VectorPtr value);
    void addDatum(const std::string& name, DatumPtr value);
    void addDatumIfNotPresent(const std::string& name, DatumPtr value);
    void addModelstep(const std::string& name, FeaturePtr value, bool isComponent, const std::string& featureDescription = std::string() );
    void addModelstepIfNotPresent(const std::string& name, FeaturePtr value, bool isComponent, const std::string& featureDescription = std::string() );

    void removeScalar(const std::string& name);
    void removePoint(const std::string& name);
    void removeDirection(const std::string& name);
    void removeDatum(const std::string& name);
    void removeModelstep(const std::string& name);

    void addVertexFeature(const std::string& name, FeatureSetPtr value);
    void addEdgeFeature(const std::string& name, FeatureSetPtr value);
    void addFaceFeature(const std::string& name, FeatureSetPtr value);
    void addSolidFeature(const std::string& name, FeatureSetPtr value);
    void addModel(const std::string& name, ModelPtr value);
    void addPostprocAction(const std::string& name, PostprocActionPtr value);
    std::string addPostprocActionUnnamed(PostprocActionPtr value);

    void removePostprocAction(const std::string& name);

    void addDataset(const std::string& name, vtkSmartPointer<vtkDataObject> value);

    void removeDataset(const std::string& name);

    ScalarPtr lookupScalar(const std::string& name) const;
    VectorPtr lookupPoint(const std::string& name) const;
    VectorPtr lookupDirection(const std::string& name) const;
    DatumPtr lookupDatum(const std::string& name) const;
    FeaturePtr lookupModelstep(const std::string& name) const;
    FeatureSetPtr lookupVertexFeature(const std::string& name) const;
    FeatureSetPtr lookupEdgeFeature(const std::string& name) const;
    FeatureSetPtr lookupFaceFeature(const std::string& name) const;
    FeatureSetPtr lookupSolidFeature(const std::string& name) const;
    ModelPtr lookupModel(const std::string& name) const;
    PostprocActionPtr lookupPostprocActionSymbol(const std::string& name) const;

    const ComponentSet& components() const 	
    {
        return components_;
    }

    bool isComponent(const std::string& name) const;
    
    const std::string description() const;

    /**
     * @brief cost
     * @return
     * cost of this model only
     */
    double cost() const;

    /**
     * @brief totalCost
     * @return
     * cost of this model plus all submodel costs
     */
    double totalCost() const;

    ScalarTableContents scalars() const;
    VectorTableContents	points() const;
    VectorTableContents	directions() const;
    DatumTableContents	datums() const;
    ModelstepTableContents	modelsteps() const;
    VertexFeatureTableContents 	vertexFeatures() const;
    EdgeFeatureTableContents edgeFeatures() const;
    FaceFeatureTableContents faceFeatures() const;
    SolidFeatureTableContents solidFeatures() const;
    ModelTableContents models() const;
    PostprocActionTableContents postprocActions() const;

};



}
}

#endif // INSIGHT_CAD_MODEL_H
