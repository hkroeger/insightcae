#ifndef INSIGHT_NUMERICALRESULTS_H
#define INSIGHT_NUMERICALRESULTS_H


#include "base/resultelement.h"


namespace insight {

template<class T>
class NumericalResult
    : public ResultElement
{
protected:
    T value_;

public:

    NumericalResult ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit )
        : ResultElement ( shortdesc, longdesc, unit )
    {}

    NumericalResult ( const T& value, const std::string& shortDesc, const std::string& longDesc, const std::string& unit )
        : ResultElement ( shortDesc, longDesc, unit ),
          value_ ( value )
    {}

    inline void setValue ( const T& value )
    {
        value_=value;
    }

    inline const T& value() const
    {
        return value_;
    }

    void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const override
    {
        boost::filesystem::path fname ( outputdirectory/ ( name+".dat" ) );
        std::ofstream f ( fname.c_str() );
        f<<value_<<std::endl;
    }

    /**
     * append the contents of this element to the given xml node
     */
    rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const override
    {
        using namespace rapidxml;
        xml_node<>* child = ResultElement::appendToNode ( name, doc, node );

        child->append_attribute ( doc.allocate_attribute
                                  (
                                      "value",
                                      doc.allocate_string ( boost::lexical_cast<std::string> ( value_ ).c_str() )
                                  ) );

        return child;
    }

    void readFromNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) override
    {
      using namespace rapidxml;
//      xml_node<>* child = findNode ( node, name, type() );
//      if ( child ) {
          stringToValue ( node.first_attribute ( "value" )->value(), value_ );
//      }
    }

    inline operator const T& () const
    {
        return value();
    }
    inline const T& operator() () const
    {
        return value();
    }

};



} // namespace insight

#endif // INSIGHT_NUMERICALRESULTS_H
