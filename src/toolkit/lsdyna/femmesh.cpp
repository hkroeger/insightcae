
#include "femmesh.h"

#include "vtkGenericDataObjectReader.h"
#include "vtkCellData.h"

namespace insight {


const std::array<int, 3> FEMMesh::triNodeMapping = { 0, 1, 2 };
const std::array<int, 4> FEMMesh::quadNodeMapping = { 0, 1, 2, 3 };
const std::array<int, 4> FEMMesh::tetNodeMapping = { 0, 1, 2, 3 };
const std::array<int, 2> FEMMesh::lineNodeMapping = { 0, 1 };



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
    for (const auto& l: lines_)
    {
        if (l.part_id==part_id)
            nodeSet.insert(l.n.begin(), l.n.end());
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




int FEMMesh::findNodeAt(const arma::mat &x, double tol) const
{
    for (auto& n: nodes_)
    {
        if (arma::norm(n.second-x,2)<=tol)
        {
            return n.first;
        }
    }
    throw insight::Exception("no node found at location (%g %g %g)",
                             x(0), x(1), x(2));
    return -1;
}




FEMMesh::IdSet& FEMMesh::nodeSet(int setId)
{
    return nodeSets_[setId];
}

FEMMesh::IdSet &FEMMesh::shellSet(int setId)
{
    return shellSets_[setId];
}



void FEMMesh::addVTK(const boost::filesystem::path &fn, int partId)
{
    auto reader = vtkSmartPointer<vtkGenericDataObjectReader>::New();
    reader->SetFileName(fn.string().c_str());
    reader->Update();

    std::map<int, int> unhandledCells;
    vtkIdType nodeIdsOfs=maxNodeId();
    if (auto *smesh = vtkDataSet::SafeDownCast(reader->GetOutput()))
    {
        auto cei = smesh->GetCellData()->GetArray("CellEntityIds");

        for (int i=0; i<smesh->GetNumberOfCells(); ++i)
        {

            int effectivePartId=partId;
            if (cei)
            {
                effectivePartId+=cei->GetTuple1(i);
            }

            auto *c=smesh->GetCell(i);
            if (auto *q = vtkQuad::SafeDownCast(c))
            {
                addQuadElement(smesh, q, effectivePartId, nodeIdsOfs);
            }
            else if (auto *t = vtkTriangle::SafeDownCast(c))
            {
                addTriElement(smesh, t, effectivePartId, nodeIdsOfs);
            }
            else if (auto *t = vtkTetra::SafeDownCast(c))
            {
                addTetElement(smesh, t, effectivePartId, nodeIdsOfs);
            }
            else if (auto *l = vtkLine::SafeDownCast(c))
            {
                addLineElement(smesh, l, effectivePartId, nodeIdsOfs);
            }
            else
            {
                int ct=c->GetCellType();
                if (unhandledCells.find(ct)==unhandledCells.end())
                    unhandledCells[ct]=1;
                else
                    unhandledCells[ct]++;
            }
        }
    }
    else
    {
        throw insight::Exception("Unhandled data set type:", reader->GetOutput()->GetDataObjectType());
    }

    if (unhandledCells.size()>0)
    {
        std::cerr<<"Unhandled cells:\n";
        for (const auto& uhc: unhandledCells)
        {
            std::cerr<<" type "<<uhc.first<<": "<<uhc.second<<"\n";
        }
    }
}

void FEMMesh::partToSet(int partId, int setId)
{
    insight::assertion(
        elementsAreNumbered,
        "elements need to be numbered first");

    findNodesOfPart( nodeSet(setId), partId );
    findShellsOfPart( shellSet(setId), partId );
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

void FEMMesh::addLineElement(vtkDataSet *ds, vtkLine *l, int part_id, vtkIdType nodeIdOfs)
{
    addElement(ds, l, part_id, lines_, nodeIdOfs);
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
    numberElements(ei, lines_, parts2Skip);
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
                auto lie = std::find_if
                    (
                        lines_.rbegin(), lines_.rend(),
                        [](const decltype(lines_)::value_type& e)
                        {
                            return e.idx>=0;
                        }
                        );
                if (lie!=lines_.rend())
                {
                    return lie->idx;
                }
                else
                {
                    return -1;
                }
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
    insertElementCenters(ctrs, lines_);
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


    if (fmt==LSDyna)
    {
        std::ostringstream os;
        writeElementListLSDyna(os, lines_, ", ");
        if (!os.str().empty())
        {
            of<<"*ELEMENT_BEAM\n";
            of<<os.str();
        }
    }
    else if (fmt==Radioss)
    {
        writeElementListRadioss(of, lines_, "LINE2");
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
    for (const auto& e: lines_)
    {
        stat[e.part_id].nLines++;
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
       << "\t" << nTets << " tetrahedra, "
       << "\t" << nLines << " lines."
       << endl;
}


} // namespace insight
