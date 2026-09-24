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

#ifndef INSIGHT_PROPERTYLIBRARYSELECTIONPARAMETER_H
#define INSIGHT_PROPERTYLIBRARYSELECTIONPARAMETER_H

#include "base/parameters/selectionparameter.h"
#include "base/propertylibrary.h"

namespace insight {

class PropertyLibrarySelectionParameter
    : public StringParameter,
      public SelectionParameterInterface
{

protected:
    const PropertyLibraryBase* propertyLibrary_;

    typedef std::map<std::string, std::unique_ptr<ParameterSet> > InstanceParametersMap;

    /**
     * @brief instanceParameters_
     * one (editable) ParameterSet per library entry that has been visited
     * (selected at least once) during this parameter's lifetime. Mirrors
     * SelectableSubsetParameter::value_: switching the selection never
     * destroys a previously-built entry's parameters, only switches which
     * one is currently exposed as this parameter's children - so edits made
     * while a different entry was selected are preserved when switching back.
     * Built lazily (declared mutable so const accessors can populate it too).
     */
    mutable InstanceParametersMap instanceParameters_;

    /**
     * @brief instanceParametersFor
     * return the ParameterSet for the given library entry, building it via
     * propertyLibrary_->defaultParameters(sel) and wiring it up (see
     * wireEntry()) the first time this entry is visited; returns the
     * already-built, already-live instance on every subsequent call.
     * @param sel
     */
    ParameterSet& instanceParametersFor(const std::string& sel) const;

    /**
     * @brief wireEntry
     * establish an entry ParameterSet's parent pointer and forward its
     * valueChanged/childValueChanged signals to this parameter's
     * childValueChanged. Called exactly once per entry, the moment it is
     * created (lazily on first visit, or cloned/copied from another
     * instance).
     */
    void wireEntry(ParameterSet& ps);

public:
    declareType ( "librarySelection" );

    PropertyLibrarySelectionParameter(
        const rapidxml::xml_node<> & node);

    PropertyLibrarySelectionParameter (
            const std::string& description,
            bool isHidden=false,
            bool isExpert=false,
            bool isNecessary=false, int order=0 );

    PropertyLibrarySelectionParameter (
        const PropertyLibraryBase& lib,
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );

    PropertyLibrarySelectionParameter (
            const std::string& value,
            const PropertyLibraryBase& lib,
            const std::string& description,
            bool isHidden=false,
            bool isExpert=false,
            bool isNecessary=false,
            int order=0 );

    void initializeHierarchy() override;

    bool isDifferent(const Parameter& p) const override;

    const PropertyLibraryBase* propertyLibrary() const;

    /**
     * @brief instanceParameters
     * the (editable) parameter set of the currently selected library entry -
     * shorthand for instanceParametersFor(selection()).
     */
    ParameterSet& instanceParameters();
    const ParameterSet& instanceParameters() const;

    // std::vector<std::string> items() const; // now "selectionKeys"

    std::vector<std::string> selectionKeys() const override;
    void setSelection ( const std::string& sel ) override;
    const std::string& selection() const override;
    std::string iconPathForKey(const std::string& key) const override;


    rapidxml::xml_node<>* appendToNode(
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        const insight::hierarchicalData::Element::OutputProperties& outProps ) const override;

    const rapidxml::xml_node<>* readFromNode(
        const std::string& name,
        const rapidxml::xml_node<>& node
    ) override;

    void resolveRelativePaths(
        const boost::filesystem::path &baseDirectory) override;
    bool isPacked() const override;
    void pack() override;
    void unpack(const boost::filesystem::path& basePath) override;
    void clearPackedData() override;

protected:
    std::unique_ptr<insight::hierarchicalData::Element> doCloneUninitialized() const override;

public:
    void assignFrom(const Element& p) override;
    void copyMatching(const Element& p) override;
    void extend(const Element& op) override;
    bool isEqual(const insight::hierarchicalData::Element& op) const override;

    // The child-index space exposed by this class is the concatenation of two ranges,
    // mirroring SelectableSubsetParameter's childElement* split:
    //  [0, nChildren())                          -> the live parameters of the *currently
    //                                                selected* entry (delegated to
    //                                                instanceParametersFor(selection())).
    //  [nChildren(), nChildren()+instanceParameters_.size()) -> one synthetic pseudo-child
    //                                                per *visited* entry, named "<key>",
    //                                                giving read-only tree access to the
    //                                                stored values of entries that aren't
    //                                                currently selected.

    int nChildren() const override;

    std::string childElementName(
        int i,
        bool redirectArrayElementsToDefault=false ) const override;

    std::string childElementName(
        const Element* childParam,
        bool redirectArrayElementsToDefault=false ) const override;

    int childElementIndex(
        const std::string& name ) const override;

    Element& childElementRef ( int i ) override;

    const Element& childElement( int i ) const override;
};

} // namespace insight

#endif // INSIGHT_PROPERTYLIBRARYSELECTIONPARAMETER_H
