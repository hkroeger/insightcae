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

#ifndef SELECTIONPARAMETER_H
#define SELECTIONPARAMETER_H


#include "base/parameters/simpleparameter.h"
#include <string>


namespace insight
{


class SelectionParameterInterface
{
public:
    typedef std::string key_type;

    virtual ~SelectionParameterInterface();

    virtual std::vector<std::string> selectionKeys() const =0;
    virtual void setSelection(const key_type& nk) =0;
    virtual const key_type& selection() const =0;

    virtual bool contains(const std::string &value) const;
    virtual int indexOfSelection(const std::string& key) const;
    virtual int selectionIndex() const;
    virtual void setSelectionFromIndex(int idx);
    virtual std::string iconPathForKey(const std::string& key) const;
};



class SelectionParameter
    : public IntParameter,
      public SelectionParameterInterface
{
public:
    typedef std::vector<std::string> ItemList;

protected:
    ItemList items_;

public:
    declareType ( "selection" );

    SelectionParameter(const rapidxml::xml_node<> & node);
    SelectionParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    SelectionParameter ( const int& value, const ItemList& items, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    SelectionParameter ( const std::string& key, const ItemList& items, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    bool isDifferent(const Parameter& p) const override;

//    inline ItemList& items()
//    {
//        return items_;
//    }

    void resetItems(const ItemList& newItems);
    virtual const ItemList& items() const;

    std::vector<std::string> selectionKeys() const override;
    void setSelection ( const std::string& sel ) override;
    const std::string& selection() const override;

    // inline int selection_id ( const std::string& key ) const
    // {
    //     return  std::find ( items_.begin(), items_.end(), key ) - items_.begin();
    // }

    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;

    std::string plainTextRepresentation(
        int indent) const override;


    rapidxml::xml_node<>* appendToNode (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        const insight::hierarchicalData::Element::OutputProperties& outProps ) const override;

    const rapidxml::xml_node<>* readFromNode (
        const std::string& name,
        const rapidxml::xml_node<>& node ) override;

    std::unique_ptr<Element> clone() const override;
    void assignFrom(const Element& p) override;

    int nChildren() const override;
};


}



#endif // SELECTIONPARAMETER_H
