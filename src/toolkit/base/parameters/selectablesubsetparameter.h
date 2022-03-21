#ifndef INSIGHT_SELECTABLESUBSETPARAMETER_H
#define INSIGHT_SELECTABLESUBSETPARAMETER_H


#include "base/parameter.h"
#include "base/parameterset.h"


namespace insight {


class SelectableSubsetParameter
  : public Parameter,
    public SubParameterSet
{
public:
  typedef std::string key_type;
  typedef std::map<key_type, std::unique_ptr<ParameterSet> > ItemList;
  typedef ItemList value_type;

  typedef boost::tuple<key_type, ParameterSet*> SingleSubset;
  typedef std::vector< boost::tuple<key_type, ParameterSet*> > SubsetList;

protected:
  key_type selection_;
  ItemList value_;

public:
  declareType ( "selectableSubset" );

  SelectableSubsetParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
  /**
   * Construct from components:
   * \param defaultSelection The key of the subset which is selected per default
   * \param defaultValue A map of key-subset pairs. Between these can be selected
   * \param description The description of the selection parameter
   */
  SelectableSubsetParameter ( const key_type& defaultSelection, const SubsetList& defaultValue, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

  bool isDifferent(const Parameter& p) const override;

  inline key_type& selection()
  {
    return selection_;
  }

  inline const key_type& selection() const
  {
    return selection_;
  }

  inline const ItemList& items() const
  {
    return value_;
  }

  inline ItemList& items()
  {
    return value_;
  }

  void addItem ( key_type key, const ParameterSet& ps );

  inline ParameterSet& operator() ()
  {
    return * ( value_.find ( selection_ )->second );
  }

  inline const ParameterSet& operator() () const
  {
    return * ( value_.find ( selection_ )->second );
  }

  void setSelection(const key_type& key, const ParameterSet& ps);

  std::string latexRepresentation() const override;
  std::string plainTextRepresentation(int indent=0) const override;

  bool isPacked() const override;
  void pack() override;
  void unpack(const boost::filesystem::path& basePath) override;
  void clearPackedData() override;


  rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
      boost::filesystem::path inputfilepath ) const override;
  void readFromNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
                              boost::filesystem::path inputfilepath ) override;

  Parameter* clone () const override;
  void reset(const Parameter& p) override;

  const ParameterSet& subset() const override;
};

} // namespace insight

#endif // INSIGHT_SELECTABLESUBSETPARAMETER_H
