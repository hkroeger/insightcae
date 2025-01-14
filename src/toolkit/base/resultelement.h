#ifndef INSIGHT_RESULTELEMENT_H
#define INSIGHT_RESULTELEMENT_H

#include "base/parameterset.h"

namespace insight {




class Ordering
{
  double ordering_, step_;
public:
  Ordering(double ordering_base=1., double ordering_step_fraction=0.001);

  double next();
};

std::string latex_subsection ( int level );


class ResultElement
    : public boost::noncopyable
{
public:
    declareFactoryTable
    (
        ResultElement,
        LIST ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit ),
        LIST ( shortdesc, longdesc, unit )
    );

protected:
  /**
   * short description of result quantity in LaTeX format
   */
    SimpleLatex shortDescription_;

  /**
   * detailed description of result quantity in LaTeX format
   */
    SimpleLatex longDescription_;

  /**
   * unit of result quantity in LaTeX format
   */
    SimpleLatex unit_;

    /**
     * numerical quantity which determines order relative to other result elements
     */
    double order_;

public:
    declareType ( "ResultElement" );

    ResultElement ( const std::string& shortdesc, const std::string& longdesc, const std::string& unit );
    virtual ~ResultElement();

  /**
   * short description of result quantity
   */
  const SimpleLatex& shortDescription() const;

  /**
   * detailed description of result quantity
   */
  const SimpleLatex& longDescription() const;

  /**
   * unit of result quantity
   */
  const SimpleLatex& unit() const;

  inline ResultElement& setOrder ( double o ) { order_=o; return *this; }
    inline double order() const { return order_; }

    virtual void insertLatexHeaderCode ( std::set<std::string>& headerCode ) const;
    virtual void writeLatexCode ( std::ostream& f, const std::string& name, int level, const boost::filesystem::path& outputfilepath ) const;
    virtual void exportDataToFile ( const std::string& name, const boost::filesystem::path& outputdirectory ) const;

    /**
     * append the contents of this element to the given xml node
     */
    virtual rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node
    ) const;

    void readBaseAttributesFromNode
        (
            const std::string& name,
            rapidxml::xml_node<>& node
        );
    /**
     * restore the contents of this element from the given node
     */
    virtual void readFromNode
    (
        const std::string& name,
        rapidxml::xml_node<>& node
    );

    /**
     * convert this result element into a parameter
     * returns an invalid pointer per default
     * Since not all Results can be converted into parameters, a check for validity is required before using the pointer.
     */
    virtual std::unique_ptr<Parameter> convertIntoParameter() const;

    virtual std::shared_ptr<ResultElement> clone() const =0;
};


typedef std::shared_ptr<ResultElement> ResultElementPtr;



inline ResultElement* new_clone(const ResultElement& e)
{
  return e.clone().get();
}


} // namespace insight

#endif // INSIGHT_RESULTELEMENT_H
