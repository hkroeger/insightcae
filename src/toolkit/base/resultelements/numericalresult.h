#ifndef INSIGHT_NUMERICALRESULTS_H
#define INSIGHT_NUMERICALRESULTS_H


#include "base/rapidxml.h"
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

    void exportDataToFile (
        const std::string& name,
        const boost::filesystem::path& outputdirectory ) const override
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
        auto* child = ResultElement::appendToNode ( name, doc, node );

        appendAttribute(doc, *child, "value", value_ );

        return child;
    }

    const rapidxml::xml_node<>* readFromNode
    (
        const std::string& name,
        const rapidxml::xml_node<>& node
    ) override
    {
        auto *child = ResultElement::readFromNode(name, node);
        value_ = getMandatoryAttribute<T>(*child, "value");
        return child;
    }

    int nChildren() const override
    {
        return 0;
    }

    inline operator const T& () const
    {
        return value();
    }
    inline const T& operator() () const
    {
        return value();
    }

    bool isEqual(const Element& op) const override
    {
        if (auto *oa = dynamic_cast<const NumericalResult*>(&op))
        {
            return value_==oa->value_;
        }
        else
            return false;
    }

};



} // namespace insight

#endif // INSIGHT_NUMERICALRESULTS_H
