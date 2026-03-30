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
 *
 */

#ifndef MATRIXPARAMETER_H
#define MATRIXPARAMETER_H


#include "base/parameter.h"


namespace insight
{



class MatrixParameter
    : public Parameter
{
public:
    typedef arma::mat value_type;

protected:
    arma::mat value_;

public:
    declareType ( "matrix" );

    MatrixParameter (const rapidxml::xml_node<> & node);
    MatrixParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    MatrixParameter ( const arma::mat& defaultValue, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    bool isDifferent(const Parameter& p) const override;

    void set(const arma::mat& nv);
    const arma::mat& operator() () const;

    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;

    std::string plainTextRepresentation(int indent) const override;

    rapidxml::xml_node<>* appendToNode (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        const insight::hierarchicalData::Element::OutputProperties& outProps ) const override;

    const rapidxml::xml_node<>* readFromNode (
        const std::string& name,
        const rapidxml::xml_node<>& node ) override;

protected:
    std::unique_ptr<Element> cloneUninitialized() const override;
public:
    void assignFrom(const Element& p) override;
    bool isEqual(const Element& op) const override;

    int nChildren() const override;
};



}



#endif // MATRIXPARAMETER_H
