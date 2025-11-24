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
    std::set<boost::signals2::scoped_connection> syncConnections_;

    std::key_observer_map<Parameter, std::shared_ptr<boost::signals2::scoped_connection> >
        valueChangedConnections_,
        childValueChangedConnections_;

    void eraseValueImpl(const std::string& label );
    void insertWithDefaultsImpl(const std::string& label);
    void insertValueImpl(
        const std::string& label,
        std::unique_ptr<Parameter>&& np );
    void changeLabelImpl( const std::string& label, const std::string& newLabel );
    Parameter& getOrInsertDefaultValueImpl( const std::string& label );

    void initialize() override;

public:
    declareType ( "labeledarray" );

    LabeledArrayParameter(const rapidxml::xml_node<> & node);

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


    bool isDifferent(const Parameter& p) const override;

    void setLabelPattern(const std::string& pat);

    void setKeySourceParameterPath(const std::string& pp);
    void unsetKeySourceParameterPath();
    bool keysAreLocked() const;

    std::set<std::string> keys() const;
    bool hasKey(const std::string& label) const;
    value_type& value();
    const value_type& value() const;

    void setDefaultValue ( std::unique_ptr<Parameter>&& defP );
    const Parameter& defaultValue() const;

    std::string findUniqueNewKey() const;
    void eraseValue ( const std::string& label );
    void appendValue ( std::unique_ptr<Parameter>&& np );
    void insertValue (
        const std::string& label,
        std::unique_ptr<Parameter>&& np );

    Parameter& getOrInsertDefaultValue ( const std::string& label );

    void appendEmpty();
    void insertWithDefaults(const std::string& label);
    void changeLabel ( const std::string& label, const std::string& newLabel );

    Parameter& operator[] ( const std::string& label);
    const Parameter& operator[] ( const std::string& label ) const;

    int size() const;

    int nChildren() const override;
    std::string childElementName(
        int i,
        bool redirectArrayElementsToDefault=false ) const override;
    Element& childElementRef ( int i ) override;
    const Element& childElement( int i ) const override;
    int childElementIndex( const std::string& name ) const override;

    void clear();

    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;
    std::string plainTextRepresentation(int indent) const override;

    bool isPacked() const override;
    void pack() override;
    void unpack(const boost::filesystem::path& basePath) override;
    void clearPackedData() override;


    rapidxml::xml_node<>* appendToNode (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        const insight::hierarchicalData::Element::OutputProperties& outProps ) const override;

    const rapidxml::xml_node<>* readFromNode (
        const std::string& name,
        const rapidxml::xml_node<>& node ) override;

    std::unique_ptr<Element> clone () const override;

    void assignFrom( const Element& rhs ) override;
    void copyMatching( const Element& rhs ) override;
    void extend( const Element& op ) override;
    bool isEqual(const Element& op) const override;
};

} // namespace insight

#endif // INSIGHT_LABELEDARRAY_H
