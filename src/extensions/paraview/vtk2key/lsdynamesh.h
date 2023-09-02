#ifndef INSIGHT_LSDYNAMESH_H
#define INSIGHT_LSDYNAMESH_H

#include "base/linearalgebra.h"

#include <array>
#include <vector>
#include <map>
#include <set>

#include "vtkDataSet.h"
#include "vtkQuad.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"

#include "boost/lexical_cast.hpp"
#include "boost/algorithm/string.hpp"

namespace insight {

class LSDynaMesh
{
public:


    template<int N, const std::array<int,N>& Mapping>
    struct Element
    {
        const std::array<int,N>& nodeMap = Mapping;
        int part_id;
        std::array<vtkIdType, N> n;
    };

    static const std::array<int,3> triNodeMapping;
    typedef Element<3, triNodeMapping> Tri;

    static const std::array<int,4> quadNodeMapping;
    typedef Element<4, quadNodeMapping> Quad;

    static const std::array<int,4> tetNodeMapping;
    typedef Element<4, tetNodeMapping> Tetraeder;

    class IdSet : public std::set<int>
    {
    public:
        void writeIds(std::ostream& os, int cols=8) const;
    };


    struct PartStatistics
    {
        int nTris, nQuads, nTets;

        PartStatistics();
        void print(std::ostream& os, int partId) const;
    };

private:
    std::map<vtkIdType, arma::mat> nodes_;

    std::vector<Tri> tris_;
    std::vector<Quad> quads_;
    std::vector<Tetraeder> tets_;

    std::map<int, IdSet> nodeSets_;

public:
    template<class VTKCell, class TargetElement>
    void addElement(vtkDataSet* ds, VTKCell* q, int part_id, std::vector<TargetElement>& cellList, vtkIdType nodeIdOfs=0)
    {
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

    void findNodesOfPart(std::set<int>& nodeSet, int part_id) const;

    IdSet& nodeSet(int setId);



    template<class TargetElement>
    void writeElementList(
        std::ostream& os, int& ei,
        const std::vector<TargetElement>& cellList,
        const std::set<int>& parts2Skip,
        const std::string& nodeIdListSeperator=", ") const
    {
        for (const auto& c: cellList)
        {
            if (parts2Skip.count(c.part_id)==0)
            {
                std::vector<std::string> nodeIds;
                for (auto& ni: c.n)
                    nodeIds.push_back(boost::lexical_cast<std::string>(ni));
#warning dirty hack for triangles
                if (c.n.size()==3)
                    nodeIds.push_back(nodeIds.back());
                auto nodeIdList = boost::join(nodeIds, ", ");

                os <<(ei++) << ", " << c.part_id;
                os << nodeIdListSeperator << nodeIdList << "\n";
            }
        }
    }

    void addQuadElement(vtkDataSet* ds, vtkQuad* q, int part_id, vtkIdType nodeIdOfs=0);
    void addTriElement(vtkDataSet* ds, vtkTriangle* t, int part_id, vtkIdType nodeIdOfs=0);
    void addTetElement(vtkDataSet* ds, vtkTetra* t, int part_id, vtkIdType nodeIdOfs=0);

    vtkIdType maxNodeId() const;

    void write(
        std::ostream& of,
        const std::set<int>& parts2Skip ) const;

    void printStatistics(std::ostream& os) const;
};

} // namespace insight

#endif // INSIGHT_LSDYNAMESH_H
