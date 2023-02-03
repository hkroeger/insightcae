#include "codeastermeshfile.h"


#include "base/exception.h"

using namespace std;
using namespace boost;


namespace insight {


CodeAsterMeshFile::Node::Node(const arma::mat &x)
    : arma::mat(x)
{}

const string &CodeAsterMeshFile::Node::nodeName() const
{
    return nodeName_;
}


CodeAsterMeshFile::Subfile::Subfile(const std::string &keyword)
    : keyword_(keyword)
{}




void CodeAsterMeshFile::Subfile::write(std::ostream &os) const
{
    os << keyword_ << endl;
    writeContent(os);
    os << "FINSF" << endl;
}



void CodeAsterMeshFile::nameNodesAndElements()
{
    int id=1;
    for (auto& n: nodes_)
    {
        n->nodeName_ =
                str( format("N%d") % (id++) );
    }

    id=1;
    for (auto& sf: subfiles_)
    {
        if (auto e = dynamic_pointer_cast<Element>(sf))
        {
            e->elementName_ = str( format("M%d") % (id++) );
        }
    }
}




CodeAsterMeshFile::CodeAsterMeshFile()
{}


CodeAsterMeshFile::NodePtr CodeAsterMeshFile::addNode(const arma::mat &x, bool merge)
{
    NodePtr node;

    if (merge)
    {
        auto presentNode =
                std::find_if(
                    nodes_.begin(), nodes_.end(),
                    [&](NodePtr n)
                    {
                        return (arma::norm( x - static_cast<arma::mat>(*n), 2) < SMALL);
                    }
                );
        if (presentNode != nodes_.end())
        {
            node=*presentNode;
        }
    }

    if (!node)
    {
        node = std::make_shared<Node>(x);
        nodes_.push_back(node);
    }

    return node;
}

CodeAsterMeshFile::SubfilePtr CodeAsterMeshFile::addObject(SubfilePtr obj)
{
    subfiles_.push_back(obj);
    return obj;
}



void CodeAsterMeshFile::write(std::ostream &os) const
{
    const_cast<CodeAsterMeshFile*>(this) -> nameNodesAndElements();

    if (nodes_.size())
    {
        os << "COOR_3D" << endl;
        for (const auto& n: nodes_)
        {
            os << n->nodeName() << " "
               << (*n)(0) << " " << (*n)(1) << " " << (*n)(2) << endl;
        }
        os << "FINSF" << endl;
    }

    for (const auto& sf: subfiles_)
    {
        sf->write(os);
    }

    os << "FIN" << endl;
}

void CodeAsterMeshFile::write(const boost::filesystem::path& outfile) const
{
    std::ofstream f( outfile.string() );
    write(f);
}

void CodeAsterMeshFile::write(const boost::filesystem::path &directory, int unit) const
{
    write( directory / str(format("fort.%d")%unit) );
}



CodeAsterMeshFile::Element::Element(const std::string &elementTypeName)
    : Subfile(elementTypeName)
{}


const std::string& CodeAsterMeshFile::Element::elementName() const
{
    return elementName_;
}



void POI1::writeContent(std::ostream& f) const
{
    f << elementName() << " " << node_->nodeName() << endl;
}

POI1::POI1(CodeAsterMeshFile::NodePtr n)
    : CodeAsterMeshFile::Element("POI1"),
      node_(n)
{}


void SEG2::writeContent(std::ostream& f) const
{
    f << elementName() << " " << node1_->nodeName() << " " << node2_->nodeName() << endl;
}

SEG2::SEG2(CodeAsterMeshFile::NodePtr n1, CodeAsterMeshFile::NodePtr n2)
    : CodeAsterMeshFile::Element("SEG2"),
      node1_(n1), node2_(n2)
{}


void GROUP_MA::writeContent(std::ostream &f) const
{
    f << groupName_ << endl;
    for (auto& e: elements_)
    {
        f<<e->elementName()<<endl;
    }
}

GROUP_MA::GROUP_MA(
        const std::string& groupName,
        const std::set<std::shared_ptr<const CodeAsterMeshFile::Element> > &elements )
    : Subfile("GROUP_MA"),
      groupName_(groupName),
      elements_(elements)
{}




void GROUP_NO::writeContent(std::ostream& f) const
{
    f << groupName_ << endl;
    for (auto& n: nodes_)
    {
        f<<n->nodeName()<<endl;
    }
}


GROUP_NO::GROUP_NO(
        const std::string& groupName,
        const std::set<std::shared_ptr<CodeAsterMeshFile::Node const> >& nodes )
    : Subfile("GROUP_NO"),
      groupName_(groupName),
      nodes_(nodes)
{

}

PolylineMesh::PolylineMesh(CodeAsterMeshFile& mesh, std::vector<arma::mat> &pts, bool merge1, bool merge2)
{
    insight::assertion(pts.size()>=2,
                       "at least two node locations have to be provided!");

    nodes_.push_back( mesh.addNode(pts[0], merge1) );
    for (size_t i=1; i<pts.size()-1; ++i)
        nodes_.push_back( mesh.addNode(pts[i], false) );
    nodes_.push_back( mesh.addNode(pts.back(), merge2) );

    for (size_t i=0; i<nodes_.size()-1; ++i)
    {
        elements_.push_back(
                    mesh.addObject(
                        std::make_shared<SEG2>(nodes_[i], nodes_[i+1]) )
                );
    }
}



} // namespace insight
