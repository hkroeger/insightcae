
#include "lsdynamesh.h"


namespace insight {


const std::array<int, 3> LSDynaMesh::triNodeMapping = { 0, 1, 2 };
const std::array<int, 4> LSDynaMesh::quadNodeMapping = { 0, 1, 2, 3 };
const std::array<int, 4> LSDynaMesh::tetNodeMapping = { 0, 1, 2, 3 };



void writeList(std::ostream& os, const std::set<int>& data, int cols)
{
    auto i = data.begin();
    for (int c=0; ; ++c)
    {
        if (i==data.end()) break;
        os<<*i;
        ++i;
        if ( (c>=cols-1) || (i==data.end()) )
        {
            c=-1;
            os<<"\n";
        }
        else
            os<<", ";
    }
}



void LSDynaMesh::IdSet::writeIds(std::ostream& os, int cols) const
{
    writeList(os, *this, cols);
}




void LSDynaMesh::findNodesOfPart(std::set<int>& nodeSet, int part_id) const
{
    for (const auto& e: tris_)
    {
        if (e.part_id==part_id)
            nodeSet.insert(e.n.begin(), e.n.end());
    }
    for (const auto& e: quads_)
    {
        if (e.part_id==part_id)
            nodeSet.insert(e.n.begin(), e.n.end());
    }
    for (const auto& e: tets_)
    {
        if (e.part_id==part_id)
            nodeSet.insert(e.n.begin(), e.n.end());
    }
}




LSDynaMesh::IdSet& LSDynaMesh::nodeSet(int setId)
{
    return nodeSets_[setId];
}



void LSDynaMesh::addQuadElement(vtkDataSet* ds, vtkQuad* q, int part_id, vtkIdType nodeIdOfs)
{
    addElement(ds, q, part_id, quads_, nodeIdOfs);
}

void LSDynaMesh::addTriElement(vtkDataSet* ds, vtkTriangle* t, int part_id, vtkIdType nodeIdOfs)
{
    addElement(ds, t, part_id, tris_, nodeIdOfs);
}

void LSDynaMesh::addTetElement(vtkDataSet* ds, vtkTetra* t, int part_id, vtkIdType nodeIdOfs)
{
    addElement(ds, t, part_id, tets_, nodeIdOfs);
}

vtkIdType LSDynaMesh::maxNodeId() const
{
    if (nodes_.size()==0)
        return 0;
    else
        return (--nodes_.end())->first;
}

void LSDynaMesh::write(
    std::ostream& of,
    const std::set<int>& parts2Skip,
    const std::set<int>& parts2ElementGroup ) const
{
    of<<"*NODE\n";
    for (const auto& n: nodes_)
    {
        of<<n.first<<", "<<n.second[0]<<", "<<n.second[1]<<", "<<n.second[2]<<"\n";
    }

    std::map<int,std::set<int> > shellIDsPerPart;

    int ei=1;
    {
        std::ostringstream os;
        writeElementList(os, ei, tris_, parts2Skip, ", ", &shellIDsPerPart);
        writeElementList(os, ei, quads_, parts2Skip, ", ", &shellIDsPerPart);

        if (!os.str().empty())
        {
            of<<"*ELEMENT_SHELL\n";
            of<<os.str();
        }
    }

    {
        std::ostringstream os;
        writeElementList(os, ei, tets_, parts2Skip, "\n");
        if (!os.str().empty())
        {
            of<<"*ELEMENT_SOLID\n";
            of<<os.str();
        }
    }

    for (const auto& ns: nodeSets_)
    {
        of<<"*SET_NODE\n"<<ns.first<<"\n";
        ns.second.writeIds(of);
    }
    for (const auto& ns: shellIDsPerPart)
    {
        if (parts2ElementGroup.count(ns.first)>0)
        {
            of<<"*SET_SHELL\n"<<ns.first<<"\n";
            writeList(of, ns.second, 8);
        }
    }

    of << "*COMMENT\n";
}



void LSDynaMesh::printStatistics(ostream &os) const
{
    std::map<int, PartStatistics> stat;

    for (const auto& e: tris_)
    {
        stat[e.part_id].nTris++;
    }
    for (const auto& e: quads_)
    {
        stat[e.part_id].nQuads++;
    }
    for (const auto& e: tets_)
    {
        stat[e.part_id].nTets++;
    }

    for (const auto& p: stat)
    {
        p.second.print(os, p.first);
    }
}

LSDynaMesh::PartStatistics::PartStatistics()
    : nTris(0), nQuads(0), nTets(0)
{}

void LSDynaMesh::PartStatistics::print(ostream &os, int partId) const
{
    os << " * Part "<<partId<<" comprises "
       << "\t" << nTris << " triangles, "
       << "\t" << nQuads << " quads, "
       << "\t" << nTets << " tetrahedra."
       << endl;
}


} // namespace insight
