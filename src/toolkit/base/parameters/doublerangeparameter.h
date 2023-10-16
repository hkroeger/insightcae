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

#ifndef DOUBLERANGEPARAMETER_H
#define DOUBLERANGEPARAMETER_H


#include "base/parameters/simpleparameter.h"



namespace insight
{


template<class Container>
std::string toStringList(const Container& vals, const std::string& fmt, const std::string& sep = "; " )
{
    std::vector<std::string> strVals;
    std::transform(
                vals.begin(), vals.end(),
                std::back_inserter(strVals),
                [&fmt](const typename Container::value_type& v) { return str(boost::format(fmt)%v); }
    );
    return boost::join(strVals, sep);
}



class DoubleRangeParameter
    : public Parameter
{
public:
    typedef std::set<double> RangeList;

protected:
    RangeList values_;

public:
    typedef RangeList value_type;

    declareType ( "doubleRange" );

    DoubleRangeParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    DoubleRangeParameter ( const RangeList& value, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    DoubleRangeParameter ( double defaultFrom, double defaultTo, int defaultNum, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    ~DoubleRangeParameter() override;

    bool isDifferent(const Parameter& p) const override;

    inline void insertValue ( double v )
    {
        values_.insert ( v );
        valueChanged();
    }
//    inline RangeList::iterator operator() ()
//    {
//        return values_.begin();
//    }
    inline RangeList::const_iterator operator() () const
    {
        return values_.begin();
    }

//    inline RangeList& values()
//    {
//        return values_;
//    }
    void resetValues(const RangeList& nvs);

    inline const RangeList& values() const
    {
        return values_;
    }

    void clear();

    std::string latexRepresentation() const override;
    std::string plainTextRepresentation(int indent=0) const override;

    DoubleParameter* toDoubleParameter ( RangeList::const_iterator i ) const;


    rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const override;
    void readFromNode (
        const std::string& name,
        rapidxml::xml_node<>& node,
        boost::filesystem::path inputfilepath ) override;

    Parameter* clone() const override;
    void reset(const Parameter& p) override;
};




}


#endif // DOUBLERANGEPARAMETER_H
