#include "resultelementcollection.h"

#include "base/resultelements/resultsection.h"
#include "base/resultelements/numericalresult.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace rapidxml;


namespace insight {




ResultElement& ResultElementCollection::insert (
    const string& key,
    std::unique_ptr<ResultElement> elem )
{
    auto res=
        ResultElementMap::insert (
            { key, std::move(elem) } );

    auto &inse=* ( *res.first ).second;

    inse.setParent(this);

    return inse;
}


ResultElement& ResultElementCollection::insert (
    const string& key,
    const ResultElement& elem )
{
    return insert(key, elem.cloneAs<ResultElement>() );
}


void ResultElementCollection::copyFrom(const ResultElementCollection& other)
{
    for (auto& oe: static_cast<const ResultElementMap&>(other))
    {
        insight::assertion( this->find(oe.first)==this->ResultElementMap::end(),
                            "inserting the element "+oe.first+" from other result set would overwrite existing entry in current!" );
        insert(oe.first, oe.second->cloneAs<ResultElement>());
    }
}

std::string ResultElementCollection::latexRepresentation(
        const std::string& name,
        int documentHierarchyLevel,
        const FileStorageInfo& fsi ) const
{
    std::vector<std::pair<key_type,ResultElement*> > items;

//   std::transform
//   (
//     begin(),
//     end(),
//     std::back_inserter(items),
//     boost::bind(&value_type, _1) // does not work...
//   );

    std::for_each
    (
        ResultElementMap::begin(),
        ResultElementMap::end(),
        [&items] ( const value_type& p ) {
            items.push_back ( {p.first, p.second.get()} );
        }
    );

    std::sort
    (
        items.begin(),
        items.end(),
        [] ( const decltype(items)::value_type &left, const decltype(items)::value_type &right ) {
              return left.second->order() < right.second->order();
          }
    );

    std::ostringstream f;
    for ( auto& re: items )
    {
        const ResultElement* r = & ( *re.second );

//         std::cout<<re.first<<" order="<<re.second->order() <<std::endl;

        std::string subelemname=re.first;
        if ( name!="" ) {
            subelemname=name+"__"+re.first;
        }


        if ( const ResultSection* se=dynamic_cast<const ResultSection*> ( r ) )
        {
            f << se->latexRepresentation ( subelemname, documentHierarchyLevel+1, fsi );
        }
        else
        {
            if (r->displayFullPage())
                f<<"\\newpage\n";

            f << latex_subsection ( documentHierarchyLevel+1 )
              << "{" << SimpleLatex( re.first ).toLaTeX() << "}\n";

            f << r->shortDescription().toLaTeX() << "\n\n";

            //     re.second->writeLatexCode(f, re.first, level+1, outputfilepath);
            f << r->latexRepresentation ( subelemname, documentHierarchyLevel+2, fsi );

            f << "\n\n" << r->longDescription().toLaTeX() << "\n\n";
            f << endl;
        }
    }
    return f.str();
}

std::set<string> ResultElementCollection::contents(bool onlyLeafs) const
{
    auto fullPath = [](const std::string& pp, const std::string& name)
    {
        return (pp.empty() ? "" : pp+"/") + name;
    };

    std::function<std::set<std::string>(const ResultElementCollection&, const std::string&)>
            listContents
            = [&](const ResultElementCollection& el, const std::string& parentPath) -> std::set<std::string>
    {
        std::set<std::string> result;
        for (auto& rel: static_cast<const ResultElementMap&>(el))
        {
          if (const auto* sub = dynamic_cast<const ResultElementCollection*>(rel.second.get()))
          {
            if (!onlyLeafs)
                result.insert( fullPath(parentPath, rel.first) ) ;

            auto subc = listContents( *sub, fullPath(parentPath, rel.first) );
            std::copy(subc.begin(), subc.end(),
                      std::inserter(result, result.begin()));
          }
          else
          {
              result.insert( fullPath(parentPath, rel.first) ) ;
          }
        }
        return result;
    };

    return listContents(*this, "");
}

double ResultElementCollection::getScalar(const std::string& path) const
{
    return this->get<NumericalResult<double> >(path).value();
}




rapidxml::xml_node<>* ResultElementCollection::appendToNode(
    const std::string& name,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_node<>& node ) const
{
    auto child = ResultElement::appendToNode ( name, doc, node );
    for ( auto& i: static_cast<const ResultElementMap&>(*this) )
    {
        i.second->appendToNode ( i.first, doc, *child );
    }
    return child;
}



const rapidxml::xml_node<>*
ResultElementCollection::readFromNode (
    const std::string& name,
    const rapidxml::xml_node<>& parentNode)
{
    auto *child=ResultElement::readFromNode(name, parentNode);
    for ( auto *e = child->first_node();
         e; e = e->next_sibling() )
    {
        std::string tname ( e->name() );
        std::string name ( getMandatoryAttribute(*e, "name") );

        std::unique_ptr<ResultElement> re(
            ResultElement::lookup(
                tname,
                "", "", "" ) );

        re->readFromNode ( std::string(), *e );
        insert( name, std::move(re) );
    }
    return child;
}

bool ResultElementCollection::isEqual(const Element &op) const
{
    if (auto *oa = dynamic_cast<const ResultElementCollection*>(&op))
    {
        if (size()!=oa->size()) return false;
        auto i=ResultElementMap::begin();
        auto j=oa->ResultElementMap::begin();
        while (i!=ResultElementMap::end())
        {
            if (i->first!=j->first)
                return false;
            if (!j->second->isEqual(*j->second))
                return false;

            ++i; ++j;
        }
        return true;
    }
    else
        return false;
}




int ResultElementCollection::nChildren() const
{
    return size();
}


std::string ResultElementCollection::childElementName(
    int i,
    bool redirectArrayElementsToDefault ) const
{
    auto iter=ResultElementMap::begin();
    std::advance(iter, i);
    return iter->first;
}



std::string ResultElementCollection::childElementName(
    const Element *p,
    bool redirectArrayElementsToDefault ) const
{
    return ResultElement::childElementName(p, redirectArrayElementsToDefault);
}



void ResultElementCollection::transfer(ResultElementCollection &other)
{
    auto& oc=static_cast<ResultElementMap&>(other);
    while (oc.size())
    {
        auto io=oc.begin();
        insert(io->first, std::move(io->second));
        oc.erase(io);
    }
}



hierarchicalData::Element& ResultElementCollection::childElementRef ( int i )
{
    auto iter=ResultElementMap::begin();
    std::advance(iter, i);
    return *iter->second;
}



const hierarchicalData::Element& ResultElementCollection::childElement( int i ) const
{
    auto iter=ResultElementMap::begin();
    std::advance(iter, i);
    return *iter->second;
}

} // namespace insight
