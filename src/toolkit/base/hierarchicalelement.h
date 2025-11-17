#ifndef HIERARCHICALELEMENT_H
#define HIERARCHICALELEMENT_H


#include <string>

#include "base/cppextensions.h"
#include "base/boost_include.h"
#include "base/filestorageinfo.h"

#include "rapidxml/rapidxml.hpp"


namespace insight {

namespace hierarchicalData {

namespace elementPath {

std::string
join(const std::string& p1, const std::string& p2);

std::string
join(const std::vector<std::string>& ps);

}




class Ordering
{
    double ordering_, step_;
public:
    Ordering(double ordering_base=1., double ordering_step_fraction=0.001);

    double next();
};



class LaTeXRepresentableValue
{
    bool displayFullPage_;

public:
    LaTeXRepresentableValue();
    virtual ~LaTeXRepresentableValue();

    void setDisplayFullPage(bool displayFullPage);
    bool displayFullPage() const;

    virtual void insertLatexHeaderCode ( std::set<std::string>& headerCode ) const ;



    /**
     * @brief latexRepresentation
     * return the elements value in LaTeX source
     * @return
     */
    virtual std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const =0;

};




class PlaintextRepresentableValue
{
public:
    virtual ~PlaintextRepresentableValue();
    /**
     * @brief plainTextRepresentation
     * return the value described in plain text
     * @param indent
     * number of spaces to prepend
     * @return
     */
    virtual std::string plainTextRepresentation(int indent) const =0;
};






class Element
    : public boost::noncopyable,
      public std::observable,
      public LaTeXRepresentableValue,
      public PlaintextRepresentableValue
{
public:

#ifndef SWIG
    declareFactoryTable2(
        Element,
        ElementFactories, elements );

    boost::signals2::signal<void()> valueChanged, childValueChanged;
    boost::signals2::signal<void(int, int)> beforeChildInsertion, childInsertionDone;
    boost::signals2::signal<void(int, int)> beforeChildRemoval, childRemovalDone;
#endif


    typedef Element& reference;
    typedef const Element& const_reference;
    typedef Element* pointer;
    typedef const Element* const_pointer;
    typedef size_t size_type;
    typedef int difference_type;

    class const_iterator;

    class iterator
    {
        friend class const_iterator;

        Element* p_;
        int iChild_;
    public:
        typedef Element::reference reference;
        typedef Element::pointer pointer;
        typedef Element::size_type size_type;
        typedef Element::difference_type difference_type;
        typedef std::random_access_iterator_tag iterator_category;

        iterator();
        iterator(Element&, int i=0);
        iterator(const iterator&);
        ~iterator();

        iterator& operator=(const iterator&);
        bool operator==(const iterator&) const;
        bool operator!=(const iterator&) const;

        iterator& operator++();

        reference operator*() const;
        pointer operator->() const;
        pointer get_pointer() const;
        std::string name() const;
    };

    class const_iterator
    {
        const Element* p_;
        int iChild_;
    public:
        typedef Element::const_reference reference;
        typedef Element::const_pointer pointer;
        typedef Element::size_type size_type;
        typedef Element::difference_type difference_type;
        typedef std::random_access_iterator_tag iterator_category;

        const_iterator();
        const_iterator(const Element&, int i=0);
        const_iterator(const iterator&);
        const_iterator(const const_iterator&);
        ~const_iterator();

        const_iterator& operator=(const const_iterator&);
        bool operator==(const const_iterator&) const;
        bool operator!=(const const_iterator&) const;

        const_iterator& operator++();

        reference operator*() const;
        pointer operator->() const;
        pointer get_pointer() const;
        std::string name() const;
    };

    typedef std::reverse_iterator<iterator> reverse_iterator; //optional
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator; //optional


private:
    bool valueChangeSignalBlocked_;

    std::observer_ptr<Element> parent_;

    /**
     * numerical quantity which determines order relative to
     * other elements at the same hierarchy level
     */
    double order_;

    /**
     * @brief workingDirectory_
     * the directory, to which relative paths of parameters
     * (this one and possible children) refer to
     */
    mutable boost::optional<boost::filesystem::path> workingDirectory_;

    virtual bool hasWorkingDirectory() const;
    virtual boost::filesystem::path workingDirectory() const;
    virtual void setWorkingDirectory(const boost::filesystem::path& wd) const;

    bool requiresInit_;

public: // needs to be accessible from ResultElementCollection
    virtual void setParent(Element* parent);

protected:
    void markAsInitialized();
    void ensureInitialization() const;
    void resetInitialization();

    /**
     * @brief initialize
     * called after entire hierarchy of objects is readily constructed
     * so that interdependencies are available.
     * does nothing by default.
     * required by synchronized parameters (LabeledArrayParameter) that get a selection from other parameters
     */
    virtual void initialize();

    inline bool valueChangeSignalBlocked() const
    {
        return valueChangeSignalBlocked_;
    }

public:
    declareType ( "hierarchicalDataElement" );

    Element(int order);
    virtual ~Element();


    bool isInitialized() const;


    virtual bool canSetDataFromString() const;
    virtual void setDataFromString(const std::string& newValue, bool* ok = nullptr);


    virtual bool isBooleanData() const;
    virtual bool canSetFromBoolean() const;
    virtual bool getAsBoolean() const;
    virtual void setBoolean(bool newValue);

    std::string plainTextRepresentation(int indent=0) const override;

    bool hasParent() const;
    Element& parent();
    const Element& parent() const;

    std::string path(bool redirectArrayElementsToDefault=false) const;
    std::string name(bool redirectArrayElementsToDefault=false) const;

    inline Element& setOrder ( double o ) { order_=o; return *this; }
    inline double order() const { return order_; }

    // IO

    /**
     * @brief appendToNode
     * creates a node for the current element with the given name under
     * the parentNode, saves the contents and returns the pointer to the child node
     * (the one to which the data was saved)
     * @param name
     * @param doc
     * @param parentNode
     * @param inputFilePath
     * @return
     */
    virtual rapidxml::xml_node<>* appendToNode
    (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& parentNode
    ) const;


    /**
     * @brief readFromNode
     * finds the node for the given name under the parentNode,
     * reads the contents and returns the pointer to the child node
     * (the one from which the data was read)
     * @param name
     * @param parentNode
     * @param inputFilePath
     * @return
     */
    virtual const rapidxml::xml_node<>* readFromNode
    (
        const std::string& name,
        const rapidxml::xml_node<>& parentNode
    );



    void saveToNode(
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& rootNode ) const;


    void saveToStream(
        std::ostream& os ) const;

    void saveToFile (
        const boost::filesystem::path& file
        ) const;

    void saveToString (
        std::string& s ) const;



    virtual void readFromRootNode(
        const rapidxml::xml_node<>& rootNode,
        const std::string& startAtSubnode = std::string() );

    void readFromStream(
        std::istream& is );

    virtual void readFromFile(
        const boost::filesystem::path& file,
        const std::string& startAtSubnode = std::string() );

    void readFromString(
        const std::string& contents,
        const std::string& startAtSubnode = std::string() );


    // use fully qualified type for SWIG
    virtual std::unique_ptr<insight::hierarchicalData::Element> clone() const =0;

    template<class T>
    std::unique_ptr<T> cloneAs() const
    {
        return std::dynamic_unique_ptr_cast<T>(this->clone());
    }


    /**
     * @brief assignFrom
     * like assignment operator.
     * This element remains at the same address.
     * Matching children are assigned, non-existing children added, children not existing in rhs are removed.
     * Thus contents in this element is the same as in rhs afterwards.
     * @param e
     */
    virtual void assignFrom( const Element& rhs );

    /**
     * @brief copyMatching
     * copy the values of elements from rhs, which are also present in this.
     * Ignore all non-matching.
     *
     * @param rhs
     */
    virtual void copyMatching( const Element& rhs );

    /**
     * @brief extend
     * insert entries into current subset, that are not yet present.
     * Existing parameters will not be touched!
     * @param op
     */
    virtual void extend( const Element& op );


    /**
     * @brief isEqual
     * check, if the content is equal
     * @param op
     * @return
     */
    virtual bool isEqual(const Element& op) const =0;

    // void operator=(const Element& rhs);

    // Hierarchy

    virtual int nChildren() const =0;

    virtual std::string childElementName(
        int i,
        bool redirectArrayElementsToDefault=false ) const;

    virtual std::string childElementName(
        const Element* childParam,
        bool redirectArrayElementsToDefault=false ) const;

    virtual Element& childElementRef ( int i );

    virtual const Element& childElement( int i ) const;

    virtual int childElementIndex( const std::string& name ) const;

    virtual int childElementIndex( const Element* childParam ) const;

    Element& childElementByNameRef ( const std::string& name );

    const Element& childElementByName ( const std::string& name ) const;

    std::vector<std::string> childElementNameList() const;
    std::vector<std::string> childElementFullPathList() const;


    iterator begin();
    const_iterator begin() const;
    const_iterator cbegin() const;
    iterator end();
    const_iterator end() const;
    const_iterator cend() const;

    struct UpdateValueSignalBlockage
    {
        Element& blockedElement;
        UpdateValueSignalBlockage(Element& p);
        ~UpdateValueSignalBlockage();
    };

    std::unique_ptr<insight::hierarchicalData::Element::UpdateValueSignalBlockage>
    blockUpdateValueSignal();

    virtual void setUpdateValueSignalBlockage(bool block=true);

    void triggerValueChanged();
    void triggerChildValueChanged();

    bool hasPath( std::string path ) const;
    insight::hierarchicalData::Element& getByPath( std::string path );

    template<class T>
    T& get ( const std::string& name )
    {
        typedef T PT;

        auto& p = this->getByPath(name);

        if ( PT* const pt=dynamic_cast<PT* const>(&p) )
        {
            return *pt;
        }
        else
        {
            throw insight::ElementNotFoundException(
                    str(boost::format("Element %s not of requested type!"
                        " (actual type is %s)")
                    % name % p.type())
                );
        }
    }

    template<class T>
    const T& get ( const std::string& name ) const
    {
        return const_cast<hierarchicalData::Element&>(*this)
            .get<T>(name);
    }

};




} // namespace hierarchicalData
} // namespace insight

#endif // HIERARCHICALELEMENT_H
