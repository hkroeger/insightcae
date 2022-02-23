#ifndef INSIGHT_CODEASTERMESHFILE_H
#define INSIGHT_CODEASTERMESHFILE_H

#include <iostream>
#include <memory>
#include <vector>

#include "base/boost_include.h"
#include "base/linearalgebra.h"

namespace insight {


class CodeAsterMeshFile
{
public:

    class Node : public arma::mat
    {
        friend class CodeAsterMeshFile;

        std::string nodeName_;
    public:
        Node(const arma::mat& x);
        const std::string& nodeName() const;
    };

    typedef std::shared_ptr<Node> NodePtr;

    class Subfile
    {
        std::string keyword_;

    protected:
        virtual void writeContent(std::ostream& f) const =0;

    public:
        Subfile(const std::string& keyword);
        void write(std::ostream& os) const;
    };

    typedef std::shared_ptr<Subfile> SubfilePtr;


    class Element : public Subfile
    {
        friend class CodeAsterMeshFile;

        std::string elementName_;

    public:
        Element(const std::string& elementTypeName);

        const std::string& elementName() const;
    };

    typedef std::shared_ptr<Subfile> ElementPtr;

protected:
    std::vector<NodePtr> nodes_;
    std::vector<SubfilePtr> subfiles_;

    void nameNodesAndElements();

public:
    CodeAsterMeshFile();

    NodePtr addNode(const arma::mat& x, bool merge=true);
    SubfilePtr addObject(SubfilePtr obj);

    void write(std::ostream& os) const;
    void write(const boost::filesystem::path& directory=".", int unit=20) const;
};


class PolylineMesh
{
    std::vector<CodeAsterMeshFile::NodePtr> nodes_;
    std::vector<CodeAsterMeshFile::ElementPtr> elements_;
public:
    PolylineMesh(CodeAsterMeshFile& mesh, std::vector<arma::mat>& pts, bool merge1, bool merge2);
};




class POI1 : public CodeAsterMeshFile::Element
{
    CodeAsterMeshFile::NodePtr node_;
protected:
    void writeContent(std::ostream& f) const override;
public:
    POI1(CodeAsterMeshFile::NodePtr n);
};




class SEG2 : public CodeAsterMeshFile::Element
{
    CodeAsterMeshFile::NodePtr node1_, node2_;
protected:
    void writeContent(std::ostream& f) const override;
public:
    SEG2(CodeAsterMeshFile::NodePtr n1, CodeAsterMeshFile::NodePtr n2);
};




class GROUP_MA : public CodeAsterMeshFile::Subfile
{
    std::string groupName_;
    std::set<std::shared_ptr<CodeAsterMeshFile::Element const> > elements_;
protected:
    void writeContent(std::ostream& f) const override;

public:
    GROUP_MA( const std::string& groupName,
              const std::set<std::shared_ptr<CodeAsterMeshFile::Element const> >& elements );
};




class GROUP_NO : public CodeAsterMeshFile::Subfile
{
    std::string groupName_;
    std::set<std::shared_ptr<CodeAsterMeshFile::Node const> > nodes_;
protected:
    void writeContent(std::ostream& f) const override;

public:
    GROUP_NO( const std::string& groupName,
              const std::set<std::shared_ptr<CodeAsterMeshFile::Node const> >& nodes );
};

} // namespace insight

#endif // INSIGHT_CODEASTERMESHFILE_H
