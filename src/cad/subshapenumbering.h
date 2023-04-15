#ifndef INSIGHT_CAD_SUBSHAPENUMBERING_H
#define INSIGHT_CAD_SUBSHAPENUMBERING_H

#include "TopTools_IndexedMapOfShape.hxx"
#include "TopTools_DataMapOfShapeInteger.hxx"
#include "TopTools_DataMapOfIntegerShape.hxx"

#include "TopoDS_Vertex.hxx"
#include "TopoDS_Edge.hxx"
#include "TopoDS_Wire.hxx"
#include "TopoDS_Face.hxx"
#include "TopoDS_Solid.hxx"
#include "TopoDS_Shell.hxx"

#include <set>

namespace insight {
namespace cad {

class SubshapeNumbering
{
    TopTools_IndexedMapOfShape _fmap, _emap, _vmap, _somap, _shmap, _wmap;

    TopTools_DataMapOfShapeInteger _vertexTag, _edgeTag, _faceTag, _solidTag;
    TopTools_DataMapOfIntegerShape _tagVertex, _tagEdge, _tagFace, _tagSolid;
    TopTools_DataMapOfShapeInteger _wireTag, _shellTag;
    TopTools_DataMapOfIntegerShape _tagWire, _tagShell;

private:
    // have the internals changed since the last synchronisation?
    bool _changed;
    int _maxTag[6];

    void bind(const TopoDS_Vertex &vertex, int tag, bool recursive = false);
    void bind(const TopoDS_Edge &edge, int tag, bool recursive = false);
    void bind(const TopoDS_Wire &wire, int tag, bool recursive = false);
    void bind(const TopoDS_Face &face, int tag, bool recursive = false);
    void bind(const TopoDS_Shell &shell, int tag, bool recursive = false);
    void bind(const TopoDS_Solid &solid, int tag, bool recursive = false);
    void bind(TopoDS_Shape shape, int dim, int tag, bool recursive = false);

    void _addShapeToMaps(const TopoDS_Shape &shape);

    // set/get max tag of entity for each dimension (0, 1, 2, 3), as well as
    // -2 for shells and -1 for wires
    void setMaxTag(int dim, int val);

public:
    SubshapeNumbering(const TopoDS_Shape& shape);

    int getMaxTag(int dim) const;

    const TopoDS_Vertex& vertexByTag(int tag) const;
    const TopoDS_Edge& edgeByTag(int tag) const;
    const TopoDS_Wire& wireByTag(int tag) const;
    const TopoDS_Face& faceByTag(int tag) const;
    const TopoDS_Shell& shellByTag(int tag) const;
    const TopoDS_Solid& solidByTag(int tag) const;

    int tagOfVertex(const TopoDS_Vertex& vertex) const;
    int tagOfEdge(const TopoDS_Edge& edge) const;
    int tagOfWire(const TopoDS_Wire& wire) const;
    int tagOfFace(const TopoDS_Face& face) const;
    int tagOfShell(const TopoDS_Shell& shell) const;
    int tagOfSolid(const TopoDS_Solid& solid) const;

    void insertAllVertexTags(std::set<int>& set) const;
    void insertAllEdgeTags(std::set<int>& set) const;
    void insertAllWireTags(std::set<int>& set) const;
    void insertAllFaceTags(std::set<int>& set) const;
    void insertAllShellTags(std::set<int>& set) const;
    void insertAllSolidTags(std::set<int>& set) const;

    int nVertexTags() const;
    int nEdgeTags() const;
    int nWireTags() const;
    int nFaceTags() const;
    int nShellTags() const;
    int nSolidTags() const;

};

} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_SUBSHAPENUMBERING_H
