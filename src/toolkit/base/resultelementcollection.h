#ifndef INSIGHT_RESULTELEMENTCOLLECTION_H
#define INSIGHT_RESULTELEMENTCOLLECTION_H

#include "base/resultelement.h"

#include <map>

namespace insight {

typedef std::map<std::string, std::unique_ptr<ResultElement> > ResultElementMap;

class ResultElementCollection
    : public ResultElement,
      private ResultElementMap
{

public:
    using ResultElement::ResultElement;

#ifndef SWIG
    ResultElement& insert (
        const std::string& key,
        std::unique_ptr<insight::ResultElement> elem );

    template<class RT, class ...Args>
    RT& insert(const std::string& key,
               Args&&... addArgs )
    {
        return dynamic_cast<RT&>(
            insert(key, std::make_unique<RT>(std::forward<Args>(addArgs)...))
            );
    }
#endif

    /**
     * insert elem into the set.
     * elem is cloned.
     */
    ResultElement& insert ( const std::string& key, const ResultElement& elem );

    /**
     * @brief copyFrom
     * insert clones of all result elements in another set
     * @param other
     */
    void copyFrom(const ResultElementCollection& other);

    std::string latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const override;

    template<class T>
    T& get ( const std::string& name );

    template<class T>
    const T& get ( const std::string& name ) const
    {
        return const_cast<ResultElementCollection&>(*this).get<T>(name);
    }

    /**
     * @brief contents
     * produces list of all direct and nested result element paths
     * @return
     */
    std::set<std::string> contents(bool onlyLeafs=true) const;

    template<class T>
    std::set<std::string> contentsOfType(bool onlyLeafs=true) const
    {
        auto all = contents(onlyLeafs);
        std::set<std::string> filtered;
        std::copy_if(
                    all.begin(), all.end(),
                    std::inserter(filtered, filtered.begin()),
                    [this](const std::string& path)
        {
            try { this->get<T>(path); return true; }
            catch(...) { return false; }
        }
        );
        return filtered;
    }

    double getScalar(const std::string& path) const;

    /**
     * append the result elements to the given xml node
     */
    rapidxml::xml_node<>* appendToNode (
        const std::string& name,
        rapidxml::xml_document<>& doc,
        rapidxml::xml_node<>& node,
        const OutputProperties& outProps ) const override;

    /**
     * restore the result elements from the given node
     */
    const rapidxml::xml_node<>*
    readFromNode (
        const std::string& name,
        const rapidxml::xml_node<>& node ) override;


    bool isEqual(const Element& op) const override;

    int nChildren() const override;

    std::string childElementName(
        int i,
        bool redirectArrayElementsToDefault=false ) const override;

    Element& childElementRef ( int i ) override;
    const Element& childElement( int i ) const override;

    void transfer ( ResultElementCollection& other );
};




typedef std::shared_ptr<ResultElementCollection> ResultElementCollectionPtr;




template<class T>
T& ResultElementCollection::get ( const std::string& name )
{
  using namespace boost;
  using namespace boost::algorithm;

  if ( boost::contains ( name, "/" ) )
    {
      std::string prefix = copy_range<std::string> ( *make_split_iterator ( name, first_finder ( "/" ) ) );
      std::string remain=name;
      erase_head ( remain, prefix.size()+1 );

      std::vector<std::string> path;
      boost::split ( path, name, boost::is_any_of ( "/" ) );

      return this->get<ResultElementCollection>( prefix ) .get<T> ( remain );
    }
  else
    {
      auto i = ResultElementMap::find ( name );
      if ( i==ResultElementMap::end() )
        {
          throw insight::Exception ( "Result "+name+" not found in result set" );
        }
      else
        {
          T* pt = dynamic_cast<T*>( i->second.get() );
          if ( pt )
          {
            return ( *pt );
          }
          else
          {
            throw insight::Exception ( "Parameter "+name+" not of requested type!" );
          }
        }
    }
}


} // namespace insight

#endif // INSIGHT_RESULTELEMENTCOLLECTION_H
