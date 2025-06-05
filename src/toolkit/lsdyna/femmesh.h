#ifndef INSIGHT_LSDYNAMESH_H
#define INSIGHT_LSDYNAMESH_H

#include "base/linearalgebra.h"
#include "base/boost_include.h"

#include <array>
#include <vector>
#include <map>
#include <set>

#include "vtkDataSet.h"
#include "vtkQuad.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkLine.h"
#include "vtkPoints.h"

#include "boost/algorithm/string.hpp"

namespace insight {

class FEMMesh
{
public:


    enum OutputFormat { LSDyna, Radioss };

    template<int N, const std::array<int,N>& Mapping>
    struct Element
    {
        vtkIdType idx = -1;
        const std::array<int,N>& nodeMap = Mapping;
        int part_id = -1;
        std::array<vtkIdType, N> n;
    };

    static const std::array<int,2> lineNodeMapping;
    typedef Element<2, lineNodeMapping> Line;

    static const std::array<int,3> triNodeMapping;
    typedef Element<3, triNodeMapping> Tri;

    static const std::array<int,4> quadNodeMapping;
    typedef Element<4, quadNodeMapping> Quad;

    static const std::array<int,4> tetNodeMapping;
    typedef Element<4, tetNodeMapping> Tetraeder;

    class IdSet : public std::set<int>
    {
    public:
        void writeIds(std::ostream& os, int cols=8, int fixedWidth=-1) const;
    };


    struct PartStatistics
    {
        int nTris, nQuads, nTets, nLines;

        PartStatistics();
        void print(std::ostream& os, int partId) const;
    };

private:
    std::map<vtkIdType, arma::mat> nodes_;

    std::vector<Tri> tris_;
    std::vector<Quad> quads_;
    std::vector<Tetraeder> tets_;
    std::vector<Line> lines_;
    bool elementsAreNumbered;

    std::map<int, IdSet> nodeSets_, shellSets_;

public:
    FEMMesh()
        : elementsAreNumbered(false)
    {}

    void addVTK(
        const boost::filesystem::path& f,
        int partId
        );

    void partToSet(int partId, int setId);

    template<class VTKCell, class TargetElement>
    void addElement(vtkDataSet* ds, VTKCell* q, int part_id, std::vector<TargetElement>& cellList, vtkIdType nodeIdOfs=0)
    {
        insight::assertion(!elementsAreNumbered,
                           "Elements are already numbered. No more addition allowed!" );

        TargetElement s;
        s.part_id=part_id;

        for (int c=0; c<s.n.size(); ++c)
        {
            vtkIdType vtkId = q->GetPointId(s.nodeMap[c]);

            int myId = 1+vtkId+nodeIdOfs;
            s.n[c]=myId;
            nodes_[myId]=vec3FromComponents(ds->GetPoint(vtkId));
        };

        cellList.push_back(s);
    }

    template<class Element>
    void insertElementCenters(vtkPoints *ctrs, const std::vector<Element>& cellList) const
    {
        for (const auto& c: cellList)
        {
            arma::mat ctr=vec3Zero();
            for (int i=0; i<c.n.size(); ++i)
            {
                ctr+=nodes_[c.n[i]];
            }
            ctr/=double(c.n.size());
            ctrs->SetPoint(c.idx-1, ctr.memptr());
        }
    }


    void addQuadElement(vtkDataSet* ds, vtkQuad* q, int part_id, vtkIdType nodeIdOfs=0);
    void addTriElement(vtkDataSet* ds, vtkTriangle* t, int part_id, vtkIdType nodeIdOfs=0);
    void addTetElement(vtkDataSet* ds, vtkTetra* t, int part_id, vtkIdType nodeIdOfs=0);
    void addLineElement(vtkDataSet* ds, vtkLine* l, int part_id, vtkIdType nodeIdOfs=0);

    template<class TargetElement>
    void numberElements(
        int& ei,
        std::vector<TargetElement>& cellList,
        const std::set<int>& parts2Skip
        )
    {
        for (auto& c: cellList)
        {
            if (parts2Skip.count(c.part_id)==0)
            {
                c.idx=ei++;
            }
        }
    }


    void numberElements(
        const std::set<int>& parts2Skip
        );

    int maxElementIdx() const;
    void getCellCenters(vtkPoints* pts) const;


    void findNodesOfPart(std::set<int>& nodeSet, int part_id) const;
    void findShellsOfPart(std::set<int>& shellSet, int part_id) const;
    int findNodeAt(const arma::mat& x, double tol = insight::SMALL) const;

    IdSet& nodeSet(int setId);
    IdSet& shellSet(int setId);



    template<class TargetElement>
    void writeElementListLSDyna(
        std::ostream& os,
        const std::vector<TargetElement>& cellList,
        const std::string& nodeIdListSeperator=", " ) const
    {
        for (const auto& c: cellList)
        {
            if (c.idx>0)
            {
                std::vector<std::string> nodeIds;
                for (auto& ni: c.n)
                    nodeIds.push_back(boost::lexical_cast<std::string>(ni));
#warning dirty hack for triangles
                if (c.n.size()==3)
                    nodeIds.push_back(nodeIds.back());
                auto nodeIdList = boost::join(nodeIds, ", ");

                os <<c.idx << ", " << c.part_id;
                os << nodeIdListSeperator << nodeIdList << "\n";
            }
        }
    }

    template<class TargetElement>
    void writeElementListRadioss(
        std::ostream& os,
        const std::vector<TargetElement>& cellList,
        const std::string& keyword ) const
    {
        TargetElement *lastelem=nullptr;
        for (const auto& c: cellList)
        {
            if (c.idx>0)
            {
                if (!lastelem || lastelem->part_id!=c.part_id)
                {
                    os<<"/"<<keyword<<"/"<<c.part_id<<endl;
                }

                std::vector<std::string> nodeIds;
                for (auto& ni: c.n)
                    nodeIds.push_back(boost::lexical_cast<std::string>(ni));
                if (c.n.size()==3)
                    nodeIds.push_back(nodeIds.back());
                auto nodeIdList = boost::join(nodeIds, ", ");

                os <<c.idx << ", " << nodeIdList << "\n";
            }
            lastelem=&c;
        }
    }


    vtkIdType maxNodeId() const;

    void write(
        std::ostream& of,
        OutputFormat fmt ) const;

    void printStatistics(std::ostream& os) const;
};

} // namespace insight

#endif // INSIGHT_LSDYNAMESH_H
