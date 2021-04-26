#ifndef SUBSETPARAMETER_H
#define SUBSETPARAMETER_H

#include "base/parameter.h"
#include "base/parameterset.h"

namespace insight
{

class SubsetParameter
  : public Parameter,
    public ParameterSet,
    public SubParameterSet
{
public:
  typedef std::shared_ptr<SubsetParameter> Ptr;
  typedef ParameterSet value_type;

public:
  declareType ( "subset" );

  SubsetParameter();
  SubsetParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
  SubsetParameter ( const ParameterSet& defaultValue, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

//  inline void setParameterSet ( const ParameterSet& paramset )
//  {
//    this->setParameterSet ( paramset );
//  }

  bool isDifferent(const Parameter& p) const override;

  inline ParameterSet& operator() ()
  {
    return static_cast<ParameterSet&> ( *this );
  }

  inline const ParameterSet& operator() () const
  {
    return static_cast<const ParameterSet&> ( *this );
  }

  std::string latexRepresentation() const override;
  std::string plainTextRepresentation(int indent=0) const override;

  bool isPacked() const override;
  void pack() override;
  void unpack(const boost::filesystem::path& basePath) override;
  void clearPackedData() override;


  rapidxml::xml_node<>* appendToNode
  (
      const std::string& name,
      rapidxml::xml_document<>& doc,
      rapidxml::xml_node<>& node,
      boost::filesystem::path inputfilepath
  ) const override;

  void readFromNode
  (
      const std::string& name,
      rapidxml::xml_document<>& doc,
      rapidxml::xml_node<>& node,
      boost::filesystem::path inputfilepath
  ) override;

  Parameter* clone () const override;

  const ParameterSet& subset() const override;
};


}

#endif // SUBSETPARAMETER_H
