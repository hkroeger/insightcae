#ifndef DOUBLERANGEPARAMETER_H
#define DOUBLERANGEPARAMETER_H


#include "base/parameters/simpleparameter.h"



namespace insight
{



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
    }
    inline RangeList::iterator operator() ()
    {
        return values_.begin();
    }
    inline RangeList::const_iterator operator() () const
    {
        return values_.begin();
    }

    inline RangeList& values()
    {
        return values_;
    }
    inline const RangeList& values() const
    {
        return values_;
    }

    std::string latexRepresentation() const override;
    std::string plainTextRepresentation(int indent=0) const override;

    DoubleParameter* toDoubleParameter ( RangeList::const_iterator i ) const;


    rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const override;
    void readFromNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
                                boost::filesystem::path inputfilepath ) override;

    Parameter* clone() const override;
    void reset(const Parameter& p) override;
};




}


#endif // DOUBLERANGEPARAMETER_H
