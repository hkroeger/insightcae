#ifndef INSIGHT_LABELEDARRAY_H
#define INSIGHT_LABELEDARRAY_H

#include "base/cppextensions.h"
#include "base/parameter.h"
#include <string>

namespace insight {

class LabeledArrayParameter
    : public Parameter
{
public:
    typedef std::map<std::string, std::unique_ptr<Parameter> > value_type;

#ifndef SWIG
    boost::signals2::signal<void(const std::string& key, std::observer_ptr<Parameter>)> newItemAdded;
    boost::signals2::signal<void(const std::string& key)> itemRemoved;
    boost::signals2::signal<void(const std::string& key, const std::string& newKey)> itemRelabeled;
#endif

protected:
    std::unique_ptr<Parameter> defaultValue_;
    value_type value_;

    std::string labelPattern_;

    /**
     * @brief keySourceParameterPath_
     * path to another parameter, to which the keys are synchronized
     */
    std::string keySourceParameterPath_;

    std::key_observer_map<Parameter, std::shared_ptr<boost::signals2::scoped_connection> >
        valueChangedConnections_,
        childValueChangedConnections_;



public:
    declareType ( "labeledarray" );

    LabeledArrayParameter (
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );

    LabeledArrayParameter (
        const Parameter& defaultValue,
        int size,
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );

    void initialize() override;

    bool isDifferent(const Parameter& p) const override;

    void setLabelPattern(const std::string& pat);
    void setKeySourceParameterPath(const std::string& pp);

    void setDefaultValue ( const Parameter& defP );
    const Parameter& defaultValue() const;

    std::string findUniqueNewKey() const;
    void eraseValue ( const std::string& label );
    void appendValue ( const Parameter& np );
    void insertValue ( const std::string& label, const Parameter& np );
    Parameter& getOrInsertDefaultValue ( const std::string& label );
    void appendEmpty();
    void insertWithDefaults(const std::string& label);
    void changeLabel ( const std::string& label, const std::string& newLabel );

    Parameter& operator[] ( const std::string& label);
    const Parameter& operator[] ( const std::string& label ) const;

    int size() const;

    int nChildren() const override;
    std::string childParameterName(
        int i,
        bool redirectArrayElementsToDefault=false ) const override;
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

    std::unique_ptr<Parameter> clone () const override;
    void copyFrom(const Parameter& p) override;
    void operator=(const LabeledArrayParameter& p);
    void extend ( const Parameter& op ) override;
    void merge ( const Parameter& other ) override;
};

} // namespace insight

#endif // INSIGHT_LABELEDARRAY_H
