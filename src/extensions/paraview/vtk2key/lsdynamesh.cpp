
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

void LSDynaMesh::findShellsOfPart(std::set<int> &shellSet, int part_id) const
{
    insight::assertion(
        elementsAreNumbered,
        "Elements must have been numbered!" );

    for (const auto& e: tris_)
    {
        if (e.part_id==part_id)
            shellSet.insert(e.idx);
    }
    for (const auto& e: quads_)
    {
        if (e.part_id==part_id)
            shellSet.insert(e.idx);
    }
}




LSDynaMesh::IdSet& LSDynaMesh::nodeSet(int setId)
{
    return nodeSets_[setId];
}

LSDynaMesh::IdSet &LSDynaMesh::shellSet(int setId)
{
    return shellSets_[setId];
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


void LSDynaMesh::numberElements(
    const std::set<int>& parts2Skip
    )
{
    int ei=1;
    numberElements(ei, tris_, parts2Skip);
    numberElements(ei, quads_, parts2Skip);
    numberElements(ei, tets_, parts2Skip);
    elementsAreNumbered=true;
}

int LSDynaMesh::maxElementIdx() const
{
    insight::assertion(
        elementsAreNumbered,
        "Elements must have been numbered!" );

    auto le = std::find_if
    (
        tets_.rbegin(), tets_.rend(),
        [](const decltype(tets_)::value_type& e)
        {
            return e.idx>=0;
        }
    );
    if (le!=tets_.rend())
    {
        return le->idx;
    }
    else
    {
        auto le = std::find_if
            (
                quads_.rbegin(), quads_.rend(),
                [](const decltype(quads_)::value_type& e)
                {
                    return e.idx>=0;
                }
                );
        if (le!=quads_.rend())
        {
            return le->idx;
        }
        else
        {
            auto le = std::find_if
                (
                    tris_.rbegin(), tris_.rend(),
                    [](const decltype(tris_)::value_type& e)
                    {
                        return e.idx>=0;
                    }
                    );
            if (le!=tris_.rend())
            {
                return le->idx;
            }
            else
            {
                return -1;
            }
        }
    }
}

void LSDynaMesh::getCellCenters(vtkPoints *ctrs) const
{
    ctrs->SetNumberOfPoints(maxElementIdx());
    insertElementCenters(ctrs, tris_);
    insertElementCenters(ctrs, quads_);
    insertElementCenters(ctrs, tets_);
}



void LSDynaMesh::write(
    std::ostream& of ) const
{
    of<<"*NODE\n";
    for (const auto& n: nodes_)
    {
        of<<n.first<<", "<<n.second[0]<<", "<<n.second[1]<<", "<<n.second[2]<<"\n";
    }

    int ei=1;
    {
        std::ostringstream os;
        writeElementList(os, tris_, ", ");
        writeElementList(os, quads_, ", ");

        if (!os.str().empty())
        {
            of<<"*ELEMENT_SHELL\n";
            of<<os.str();
        }
    }

    {
        std::ostringstream os;
        writeElementList(os, tets_, "\n");
        if (!os.str().empty())
        {
            of<<"*ELEMENT_SOLID\n";
            of<<os.str();
        }
    }

    for (const auto& ns: nodeSets_)
    {
        if (ns.second.size())
        {
            of<<"*SET_NODE\n"<<ns.first<<"\n";
            ns.second.writeIds(of);
        }
    }

    for (const auto& ss: shellSets_)
    {
        if (ss.second.size())
        {
            of<<"*SET_SHELL\n"<<ss.first<<"\n";
            writeList(of, ss.second, 8);
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
