#ifndef INSIGHT_RESULTELEMENTCOLLECTION_H
#define INSIGHT_RESULTELEMENTCOLLECTION_H

#include "base/resultelement.h"

#include <map>

namespace insight {


class ResultElementCollection
    : public std::map<std::string, ResultElementPtr>
{

public:
//     ResultElementCollection(const boost::filesystem::path & file);
    virtual ~ResultElementCollection();

#ifndef SWIG
    /**
     * insert elem into the set.
     * elem is put into a shared_ptr but not clone. So don't delete it!
     */
    ResultElement& insert ( const std::string& key, ResultElement* elem );

//   void insert(const std::string& key, std::unique_ptr<ResultElement> elem);
    ResultElement& insert ( const std::string& key, ResultElementPtr elem );
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

    void writeLatexCodeOfElements ( std::ostream& f, const std::string&, int level, const boost::filesystem::path& outputfilepath ) const;

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
    virtual void appendElementsToNode ( rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node ) const;

    /**
     * restore the result elements from the given node
     */
    virtual void readElementsFromNode ( rapidxml::xml_node<>& node );

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
      iterator i = find ( name );
      if ( i==end() )
        {
          throw insight::Exception ( "Result "+name+" not found in result set" );
        }
      else
        {
          std::shared_ptr<T> pt
          (
            std::dynamic_pointer_cast<T>( i->second )
          );
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
