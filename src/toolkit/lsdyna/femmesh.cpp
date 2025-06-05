
#include "femmesh.h"


namespace insight {


const std::array<int, 3> FEMMesh::triNodeMapping = { 0, 1, 2 };
const std::array<int, 4> FEMMesh::quadNodeMapping = { 0, 1, 2, 3 };
const std::array<int, 4> FEMMesh::tetNodeMapping = { 0, 1, 2, 3 };



void writeList(std::ostream& os, const std::set<int>& data, int cols, int fixedWidth =-1)
{
    auto i = data.begin();
    for (int c=0; ; ++c)
    {
        if (i==data.end()) break;
        if (fixedWidth>=0) os<<std::setw(fixedWidth);
        os<<*i;
        ++i;
        if ( (c>=cols-1) || (i==data.end()) )
        {
            c=-1;
            os<<"\n";
        }
        else
            if (fixedWidth<0) os<<", ";
    }
}



void FEMMesh::IdSet::writeIds(std::ostream& os, int cols, int fixedWidth) const
{
    writeList(os, *this, cols, fixedWidth);
}




void FEMMesh::findNodesOfPart(std::set<int>& nodeSet, int part_id) const
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

void FEMMesh::findShellsOfPart(std::set<int> &shellSet, int part_id) const
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




FEMMesh::IdSet& FEMMesh::nodeSet(int setId)
{
    return nodeSets_[setId];
}

FEMMesh::IdSet &FEMMesh::shellSet(int setId)
{
    return shellSets_[setId];
}



void FEMMesh::addQuadElement(vtkDataSet* ds, vtkQuad* q, int part_id, vtkIdType nodeIdOfs)
{
    addElement(ds, q, part_id, quads_, nodeIdOfs);
}

void FEMMesh::addTriElement(vtkDataSet* ds, vtkTriangle* t, int part_id, vtkIdType nodeIdOfs)
{
    addElement(ds, t, part_id, tris_, nodeIdOfs);
}

void FEMMesh::addTetElement(vtkDataSet* ds, vtkTetra* t, int part_id, vtkIdType nodeIdOfs)
{
    addElement(ds, t, part_id, tets_, nodeIdOfs);
}

vtkIdType FEMMesh::maxNodeId() const
{
    if (nodes_.size()==0)
        return 0;
    else
        return (--nodes_.end())->first;
}


void FEMMesh::numberElements(
    const std::set<int>& parts2Skip
    )
{
    int ei=1;
    numberElements(ei, tris_, parts2Skip);
    numberElements(ei, quads_, parts2Skip);
    numberElements(ei, tets_, parts2Skip);
    elementsAreNumbered=true;
}

int FEMMesh::maxElementIdx() const
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

void FEMMesh::getCellCenters(vtkPoints *ctrs) const
{
    ctrs->SetNumberOfPoints(maxElementIdx());
    insertElementCenters(ctrs, tris_);
    insertElementCenters(ctrs, quads_);
    insertElementCenters(ctrs, tets_);
}



void FEMMesh::write(
    std::ostream& of,
    OutputFormat fmt ) const
{
    if (fmt==LSDyna)
        of<<"*NODE\n";
    else
        of<<"/NODE\n";
    for (const auto& n: nodes_)
    {
        of<<n.first<<", "<<n.second[0]<<", "<<n.second[1]<<", "<<n.second[2]<<"\n";
    }

    int ei=1;
    if (fmt==LSDyna)
    {
        std::ostringstream os;
        writeElementListLSDyna(os, tris_, ", ");
        writeElementListLSDyna(os, quads_, ", ");

        if (!os.str().empty())
        {
            of<<"*ELEMENT_SHELL\n";
            of<<os.str();
        }
    }
    else if (fmt==Radioss)
    {
        // if (tris_.size() || quads_.size())
        //     throw insight::Exception("not implemented");
    }

    if (fmt==LSDyna)
    {
        std::ostringstream os;
        writeElementListLSDyna(os, tets_, "\n");
        if (!os.str().empty())
        {
            of<<"*ELEMENT_SOLID\n";
            of<<os.str();
        }
    }
    else if (fmt==Radioss)
    {
        writeElementListRadioss(of, tets_, "TETRA4");
    }

    for (const auto& ns: nodeSets_)
    {
        if (ns.second.size())
        {
            if (fmt==LSDyna)
            {
                of<<"*SET_NODE\n"<<ns.first<<"\n";
                ns.second.writeIds(of);
            }
            else if (fmt==Radioss)
            {
                of<<"/GRNOD/NODE/"<<ns.first<<"\n"
                  <<"node group "<<ns.first<<"\n";
                ns.second.writeIds(of, 10, 10);
            }
        }
    }

    for (const auto& ss: shellSets_)
    {
        if (ss.second.size())
        {
            if (fmt==LSDyna)
            {
                of<<"*SET_SHELL\n"<<ss.first<<"\n";
                writeList(of, ss.second, 8);
            }
            else if (fmt==Radioss)
            {
                of<<"/GRSHEL/SHEL/"<<ss.first<<"\n"
                  <<"shell group "<<ss.first<<"\n";
                writeList(of, ss.second, 10, 10);
            }
        }
    }

    if (fmt==LSDyna)
        of << "*COMMENT\n";
}



void FEMMesh::printStatistics(ostream &os) const
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

FEMMesh::PartStatistics::PartStatistics()
    : nTris(0), nQuads(0), nTets(0)
{}

void FEMMesh::PartStatistics::print(ostream &os, int partId) const
{
    os << " * Part "<<partId<<" comprises "
       << "\t" << nTris << " triangles, "
       << "\t" << nQuads << " quads, "
       << "\t" << nTets << " tetrahedra."
       << endl;
}


} // namespace insight
