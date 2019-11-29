#ifndef SELECTIONPARAMETER_H
#define SELECTIONPARAMETER_H


#include "base/parameters/simpleparameter.h"


namespace insight
{



class SelectionParameter
    : public IntParameter
{
public:
    typedef std::vector<std::string> ItemList;

protected:
    ItemList items_;

public:
    declareType ( "selection" );

    SelectionParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    SelectionParameter ( const int& value, const ItemList& items, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    SelectionParameter ( const std::string& key, const ItemList& items, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    ~SelectionParameter() override;

    inline ItemList& items()
    {
        return items_;
    }

    virtual const ItemList& items() const;

    inline void setSelection ( const std::string& sel )
    {
        value_=selection_id ( sel );
    }

    inline const std::string& selection() const
    {
        return items_[value_];
    }

    inline int selection_id ( const std::string& key ) const
    {
        return  std::find ( items_.begin(), items_.end(), key ) - items_.begin();
    }

    std::string latexRepresentation() const override;
    std::string plainTextRepresentation(int indent=0) const override;


    rapidxml::xml_node<>* appendToNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath ) const override;
    void readFromNode ( const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node,
                                boost::filesystem::path inputfilepath ) override;

    Parameter* clone() const override;
    void reset(const Parameter& p) override;
};


}



#endif // SELECTIONPARAMETER_H
