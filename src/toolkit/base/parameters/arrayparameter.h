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

#ifndef ARRAYPARAMETER_H
#define ARRAYPARAMETER_H


#include "base/parameter.h"

namespace insight
{




class ArrayParameter
    : public Parameter,
      public ArrayParameterBase
{
public:
    typedef std::vector<ParameterPtr> value_type;

#ifndef SWIG
    boost::signals2::signal<void(ParameterPtr)> newItemAdded;
#endif

protected:
    ParameterPtr defaultValue_;
    int defaultSize_;
    std::vector<ParameterPtr> value_;

    std::map<Parameter*, std::shared_ptr<boost::signals2::scoped_connection> >
        valueChangedConnections_,
        childValueChangedConnections_;

public:
    declareType ( "array" );

    ArrayParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    ArrayParameter ( const Parameter& defaultValue, int size, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    bool isDifferent(const Parameter& p) const override;

    void setDefaultValue ( const Parameter& defP );
    const Parameter& defaultValue() const;

    int defaultSize() const;
    void resize(int newSize);
    void eraseValue ( int i );
    void appendValue ( const Parameter& np );
    void insertValue ( int i, const Parameter& np );
    void appendEmpty();
    Parameter& operator[] ( int i );
    const Parameter& operator[] ( int i ) const;

    const Parameter& element(int i) const override;

    int size() const override;

    int nChildren() const override;
    std::string childParameterName(int i) const override;
    Parameter& childParameterRef ( int i ) override;
    const Parameter& childParameter( int i ) const override;
    int childParameterIndex( const std::string& name ) const override;

    void clear();

    std::string latexRepresentation() const override;
    std::string plainTextRepresentation(int indent=0) const override;

    bool isPacked() const override;
    void pack() override;
    void unpack(const boost::filesystem::path& basePath) override;
    void clearPackedData() override;


    rapidxml::xml_node<>* appendToNode (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        boost::filesystem::path inputfilepath ) const override;
    void readFromNode (
        const std::string& name,
        rapidxml::xml_node<>& node,
        boost::filesystem::path inputfilepath ) override;

    Parameter* clone () const override;
    void copyFrom(const Parameter& p) override;
    void operator=(const ArrayParameter& p);
    void extend ( const Parameter& op ) override;
    void merge ( const Parameter& other ) override;
};



}




#endif // ARRAYPARAMETER_H
