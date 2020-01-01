#include "resultelementcollection.h"

#include "base/resultelements/resultsection.h"
#include "base/resultelements/numericalresult.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;


namespace insight {




ResultElementCollection::~ResultElementCollection()
{}


ResultElement& ResultElementCollection::insert ( const string& key, ResultElement* elem )
{
    std::pair< iterator, bool > res=
        std::map<std::string, ResultElementPtr>::insert ( ResultElementCollection::value_type ( key, ResultElementPtr ( elem ) ) );
    return * ( *res.first ).second;
}

// void ResultSet::insert(const string& key, unique_ptr< ResultElement > elem)
// {
//   this->insert(ResultSet::value_type(key, ResultElementPtr(elem.release())));
// }


ResultElement& ResultElementCollection::insert ( const string& key, ResultElementPtr elem )
{
    std::pair< iterator, bool > res=
        std::map<std::string, ResultElementPtr>::insert ( ResultElementCollection::value_type ( key, elem ) );
    return * ( *res.first ).second;
}


ResultElement& ResultElementCollection::insert ( const string& key, const ResultElement& elem )
{
    std::pair< iterator, bool > res=
        std::map<std::string, ResultElementPtr>::insert ( ResultElementCollection::value_type ( key, elem.clone() ) );
    return * ( *res.first ).second;
}


void ResultElementCollection::writeLatexCodeOfElements
(
    std::ostream& f,
    const string& name,
    int level,
    const boost::filesystem::path& outputfilepath
) const
{
    std::vector<std::pair<key_type,mapped_type> > items;

//   std::transform
//   (
//     begin(),
//     end(),
//     std::back_inserter(items),
//     boost::bind(&value_type, _1) // does not work...
//   );

    std::for_each
    (
        begin(),
        end(),
        [&items] ( const value_type& p ) {
            items.push_back ( p );
        }
    );

    std::sort
    (
        items.begin(),
        items.end(),
        [] ( const value_type &left, const value_type &right ) {
              return left.second->order() < right.second->order();
          }
    );

    for ( const value_type& re: items ) {
        const ResultElement* r = & ( *re.second );

//         std::cout<<re.first<<" order="<<re.second->order() <<std::endl;

        std::string subelemname=re.first;
        if ( name!="" ) {
            subelemname=name+"__"+re.first;
        }


        if ( const ResultSection* se=dynamic_cast<const ResultSection*> ( r ) )
        {
            se->writeLatexCode ( f, subelemname, level+1, outputfilepath );
        }
        else
        {
            f << latex_subsection ( level+1 ) << "{" << SimpleLatex( re.first ).toLaTeX() << "}\n";

            f << r->shortDescription().toLaTeX() << "\n\n";

            //     re.second->writeLatexCode(f, re.first, level+1, outputfilepath);
            r->writeLatexCode ( f, subelemname, level+2, outputfilepath );

            f << "\n\n" << r->longDescription().toLaTeX() << "\n\n";
            f << endl;
        }
    }
}

double ResultElementCollection::getScalar(const std::string& path) const
{
    return this->get<NumericalResult<double> >(path).value();
}

void ResultElementCollection::appendElementsToNode ( rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node ) const
{
    for ( const_iterator i=begin(); i!= end(); i++ ) {
        i->second->appendToNode ( i->first, doc, node );
    }
}

void ResultElementCollection::readElementsFromNode ( rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node )
{
    for ( xml_node<> *e = node.first_node(); e; e = e->next_sibling() ) {
        std::string tname ( e->name() );
        std::string name ( e->first_attribute ( "name" )->value() );
//         std::cout<<"reading "<<name<<" of type "<<tname<<std::endl;

        ResultElementPtr re
        (
            ResultElement::lookup
            (
                tname,
                "", "", ""
            )
        );

        re->readFromNode ( name, doc, *e );
        insert ( name, re );
    }
//   for( iterator i=begin(); i!= end(); i++)
//   {
//     i->second->readFromNode(i->first, doc, node);
//   }
}



} // namespace insight
