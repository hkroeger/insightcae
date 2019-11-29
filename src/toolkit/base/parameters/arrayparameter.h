#ifndef ARRAYPARAMETER_H
#define ARRAYPARAMETER_H


#include "base/parameter.h"


namespace insight
{




class ArrayParameter
    : public Parameter
{
public:
    typedef std::vector<ParameterPtr> value_type;

protected:
    ParameterPtr defaultValue_;
    int defaultSize_;
    std::vector<ParameterPtr> value_;

public:
    declareType ( "array" );

    ArrayParameter ( const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );
    ArrayParameter ( const Parameter& defaultValue, int size, const std::string& description,  bool isHidden=false, bool isExpert=false, bool isNecessary=false, int order=0 );

    //inline void setParameterSet(const ParameterSet& paramset) { value_.reset(paramset.clone()); }
    inline void setDefaultValue ( const Parameter& defP )
    {
        defaultValue_.reset ( defP.clone() );
    }
    inline const Parameter& defaultValue() const
    {
      return *defaultValue_;
    }
    inline int defaultSize() const
    {
      return defaultSize_;
    }
    inline void eraseValue ( int i )
    {
        value_.erase ( value_.begin()+i );
    }
    inline void appendValue ( const Parameter& np )
    {
        value_.push_back ( ParameterPtr( np.clone() ) );
    }
    inline void appendEmpty()
    {
        value_.push_back ( ParameterPtr( defaultValue_->clone() ) );
    }
    inline Parameter& operator[] ( int i )
    {
        return *(value_[i]);
    }
    inline const Parameter& operator[] ( int i ) const
    {
        return *(value_[i]);
    }
    inline int size() const
    {
        return value_.size();
    }
    inline void clear()
    {
        value_.clear();
    }

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
};



}




#endif // ARRAYPARAMETER_H
