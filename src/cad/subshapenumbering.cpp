#include "subshapenumbering.h"

#include "TopExp_Explorer.hxx"
#include "TopoDS.hxx"

#include "base/exception.h"

#include <vector>



namespace insight {
namespace cad {


void SubshapeNumbering::setMaxTag(int dim, int val)
{
  if(dim < -2 || dim > 3) return;
  _maxTag[dim + 2] = std::max(_maxTag[dim + 2], val);
}

int SubshapeNumbering::getMaxTag(int dim) const
{
  if(dim < -2 || dim > 3) return 0;
  return _maxTag[dim + 2];
}

const TopoDS_Vertex& SubshapeNumbering::vertexByTag(int tag) const
{
  if(_tagVertex.IsBound(tag))
  {
      return TopoDS::Vertex(_tagVertex.Find(tag));
  }
  else
  {
      std::cout<<"vertices=";
      for (auto i=_tagVertex.begin(); i!=_tagVertex.end(); ++i)
      {
          std::cout<<" "<<i.Iterator().Key();
      }
      std::cout<<std::endl;
      throw insight::Exception("no vertex with tag %d!", tag);
  }
}

const TopoDS_Edge &SubshapeNumbering::edgeByTag(int tag) const
{
  if(_tagEdge.IsBound(tag))
  {
      return TopoDS::Edge(_tagEdge.Find(tag));
  }
  else
  {
      throw insight::Exception("no edge with tag %d!", tag);
  }
}

const TopoDS_Wire &SubshapeNumbering::wireByTag(int tag) const
{
  if(_tagWire.IsBound(tag))
  {
      return TopoDS::Wire(_tagWire.Find(tag));
  }
  else
  {
      throw insight::Exception("no vertex with tag %d!", tag);
  }
}

const TopoDS_Face &SubshapeNumbering::faceByTag(int tag) const
{
  if(_tagFace.IsBound(tag))
  {
      return TopoDS::Face(_tagFace.Find(tag));
  }
  else
  {
      throw insight::Exception("no face with tag %d!", tag);
  }
}

const TopoDS_Shell &SubshapeNumbering::shellByTag(int tag) const
{
  if(_tagShell.IsBound(tag))
  {
      return TopoDS::Shell(_tagShell.Find(tag));
  }
  else
  {
      throw insight::Exception("no shell with tag %d!", tag);
  }
}

const TopoDS_Solid &SubshapeNumbering::solidByTag(int tag) const
{
  if(_tagSolid.IsBound(tag))
  {
      return TopoDS::Solid(_tagSolid.Find(tag));
  }
  else
  {
      throw insight::Exception("no solid with tag %d!", tag);
  }
}

int SubshapeNumbering::tagOfVertex(const TopoDS_Vertex &vertex) const
{
  if(_vertexTag.IsBound(vertex))
  {
    return _vertexTag.Find(vertex);
  }
  else
  {
    throw insight::Exception("no tag for vertex!");
    return -1;
  }
}

int SubshapeNumbering::tagOfEdge(const TopoDS_Edge &edge) const
{
  if(_edgeTag.IsBound(edge))
  {
    return _edgeTag.Find(edge);
  }
  else
  {
    throw insight::Exception("no tag for edge!");
    return -1;
  }
}

int SubshapeNumbering::tagOfWire(const TopoDS_Wire &wire) const
{
  if(_wireTag.IsBound(wire))
  {
    return _wireTag.Find(wire);
  }
  else
  {
    throw insight::Exception("no tag for wire!");
    return -1;
  }
}

int SubshapeNumbering::tagOfFace(const TopoDS_Face &face) const
{
  if(_faceTag.IsBound(face))
  {
    return _faceTag.Find(face);
  }
  else
  {
    throw insight::Exception("no tag for face!");
    return -1;
  }
}

int SubshapeNumbering::tagOfShell(const TopoDS_Shell &shell) const
{
  if(_shellTag.IsBound(shell))
  {
    return _shellTag.Find(shell);
  }
  else
  {
    throw insight::Exception("no tag for shell!");
    return -1;
  }
}

int SubshapeNumbering::tagOfSolid(const TopoDS_Solid &solid) const
{
  if(_solidTag.IsBound(solid))
  {
    return _solidTag.Find(solid);
  }
  else
  {
    throw insight::Exception("no tag for solid!");
    return -1;
  }
}

void SubshapeNumbering::insertAllVertexTags(std::set<int> &set) const
{
  for (auto i = _tagVertex.begin(); i!=_tagVertex.end(); ++i)
  {
    set.insert(i.Iterator().Key());
  }
}

void SubshapeNumbering::insertAllEdgeTags(std::set<int> &set) const
{
  for (auto i = _tagEdge.begin(); i!=_tagEdge.end(); ++i)
  {
    set.insert(i.Iterator().Key());
  }
}

void SubshapeNumbering::insertAllWireTags(std::set<int> &set) const
{
  for (auto i = _tagWire.begin(); i!=_tagWire.end(); ++i)
  {
    set.insert(i.Iterator().Key());
  }
}

void SubshapeNumbering::insertAllFaceTags(std::set<int> &set) const
{
  for (auto i = _tagFace.begin(); i!=_tagFace.end(); ++i)
  {
    set.insert(i.Iterator().Key());
  }
}

void SubshapeNumbering::insertAllShellTags(std::set<int> &set) const
{
  for (auto i = _tagShell.begin(); i!=_tagShell.end(); ++i)
  {
    set.insert(i.Iterator().Key());
  }
}

void SubshapeNumbering::insertAllSolidTags(std::set<int> &set) const
{
  for (auto i = _tagSolid.begin(); i!=_tagSolid.end(); ++i)
  {
    set.insert(i.Iterator().Key());
  }
}

int SubshapeNumbering::nVertexTags() const
{
  return _tagVertex.Size();
}

int SubshapeNumbering::nEdgeTags() const
{
  return _tagEdge.Size();
}

int SubshapeNumbering::nWireTags() const
{
  return _tagWire.Size();
}

int SubshapeNumbering::nFaceTags() const
{
  return _tagFace.Size();
}

int SubshapeNumbering::nShellTags() const
{
  return _tagShell.Size();
}

int SubshapeNumbering::nSolidTags() const
{
  return _tagSolid.Size();
}

void SubshapeNumbering::bind(const TopoDS_Vertex &vertex, int tag, bool recursive)
{
  if(vertex.IsNull()) return;
  if(_vertexTag.IsBound(vertex)) {
    if(_vertexTag.Find(vertex) != tag) {
      throw insight::Exception(
                 "Cannot bind existing OpenCASCADE point %d to second tag %d",
                _vertexTag.Find(vertex), tag);
    }
  }
  else {
    if(_tagVertex.IsBound(tag)) {
      // this leaves the old vertex bound in _vertexTag, but we cannot remove it
      throw insight::Exception("Rebinding OpenCASCADE point %d", tag);
    }
    _vertexTag.Bind(vertex, tag);
    _tagVertex.Bind(tag, vertex);
    setMaxTag(0, tag);
    _changed = true;
//    _attributes->insert(new OCCAttributes(0, vertex));
  }
}

void SubshapeNumbering::bind(const TopoDS_Edge &edge, int tag, bool recursive)
{
  if(edge.IsNull()) return;
  if(_edgeTag.IsBound(edge)) {
    if(_edgeTag.Find(edge) != tag) {
      throw insight::Exception(
                "Cannot bind existing OpenCASCADE curve %d to second tag %d",
                _edgeTag.Find(edge), tag);
    }
  }
  else {
    if(_tagEdge.IsBound(tag)) {
      // this leaves the old edge bound in _edgeTag, but we cannot remove it
      throw insight::Exception("Rebinding OpenCASCADE curve %d", tag);
    }
    _edgeTag.Bind(edge, tag);
    _tagEdge.Bind(tag, edge);
    setMaxTag(1, tag);
    _changed = true;
//    _attributes->insert(new OCCAttributes(1, edge));
  }
  if(recursive) {
    TopExp_Explorer exp0;
    for(exp0.Init(edge, TopAbs_VERTEX); exp0.More(); exp0.Next()) {
      TopoDS_Vertex vertex = TopoDS::Vertex(exp0.Current());
      if(!_vertexTag.IsBound(vertex)) {
        int t = getMaxTag(0) + 1;
        bind(vertex, t, recursive);
      }
    }
  }
}

void SubshapeNumbering::bind(const TopoDS_Wire &wire, int tag, bool recursive)
{
  if(wire.IsNull()) return;
  if(_wireTag.IsBound(wire)) {
    if(_wireTag.Find(wire) != tag) {
      throw insight::Exception("Cannot bind existing OpenCASCADE wire %d to second tag %d",
                _wireTag.Find(wire), tag);
    }
  }
  else {
    if(_tagWire.IsBound(tag)) {
      // this leaves the old wire bound in _wireTag, but we cannot remove it
      throw insight::Exception("Rebinding OpenCASCADE wire %d", tag);
    }
    _wireTag.Bind(wire, tag);
    _tagWire.Bind(tag, wire);
    setMaxTag(-1, tag);
    _changed = true;
  }
  if(recursive) {
    TopExp_Explorer exp0;
    for(exp0.Init(wire, TopAbs_EDGE); exp0.More(); exp0.Next()) {
      TopoDS_Edge edge = TopoDS::Edge(exp0.Current());
      if(!_edgeTag.IsBound(edge)) {
        int t = getMaxTag(1) + 1;
        bind(edge, t, recursive);
      }
    }
  }
}

void SubshapeNumbering::bind(const TopoDS_Face &face, int tag, bool recursive)
{
  if(face.IsNull()) return;
  if(_faceTag.IsBound(face)) {
    if(_faceTag.Find(face) != tag) {
      throw insight::Exception("Cannot bind existing OpenCASCADE surface %d to second tag %d",
                _faceTag.Find(face), tag);
    }
  }
  else {
    if(_tagFace.IsBound(tag)) {
      // this leaves the old face bound in _faceTag, but we cannot remove it
      throw insight::Exception("Rebinding OpenCASCADE surface %d", tag);
    }
    _faceTag.Bind(face, tag);
    _tagFace.Bind(tag, face);
    setMaxTag(2, tag);
    _changed = true;
//    _attributes->insert(new OCCAttributes(2, face));
  }
  if(recursive) {
    TopExp_Explorer exp0;
    for(exp0.Init(face, TopAbs_WIRE); exp0.More(); exp0.Next()) {
      TopoDS_Wire wire = TopoDS::Wire(exp0.Current());
      if(!_wireTag.IsBound(wire)) {
        int t = getMaxTag(-1) + 1;
        bind(wire, t, recursive);
      }
    }
    for(exp0.Init(face, TopAbs_EDGE); exp0.More(); exp0.Next()) {
      TopoDS_Edge edge = TopoDS::Edge(exp0.Current());
      if(!_edgeTag.IsBound(edge)) {
        int t = getMaxTag(1) + 1;
        bind(edge, t, recursive);
      }
    }
  }
}

void SubshapeNumbering::bind(const TopoDS_Shell &shell, int tag, bool recursive)
{
  if(shell.IsNull()) return;
  if(_shellTag.IsBound(shell)) {
    if(_shellTag.Find(shell) != tag) {
      throw insight::Exception("Cannot bind existing OpenCASCADE shell %d to second tag %d",
                _shellTag.Find(shell), tag);
    }
  }
  else {
    if(_tagShell.IsBound(tag)) {
      // this leaves the old shell bound in _faceTag, but we cannot remove it
      throw insight::Exception("Rebinding OpenCASCADE shell %d", tag);
    }
    _shellTag.Bind(shell, tag);
    _tagShell.Bind(tag, shell);
    setMaxTag(-2, tag);
    _changed = true;
  }
  if(recursive) {
    TopExp_Explorer exp0;
    for(exp0.Init(shell, TopAbs_FACE); exp0.More(); exp0.Next()) {
      TopoDS_Face face = TopoDS::Face(exp0.Current());
      if(!_faceTag.IsBound(face)) {
        int t = getMaxTag(2) + 1;
        bind(face, t, recursive);
      }
    }
  }
}

void SubshapeNumbering::bind(const TopoDS_Solid &solid, int tag, bool recursive)
{
  if(solid.IsNull()) return;
  if(_solidTag.IsBound(solid)) {
    if(_solidTag.Find(solid) != tag) {
      throw insight::Exception("Cannot bind existing OpenCASCADE volume %d to second tag %d",
                _solidTag.Find(solid), tag);
    }
  }
  else {
    if(_tagSolid.IsBound(tag)) {
      // this leaves the old solid bound in _faceTag, but we cannot remove it
      throw insight::Exception("Rebinding OpenCASCADE volume %d", tag);
    }
    _solidTag.Bind(solid, tag);
    _tagSolid.Bind(tag, solid);
    setMaxTag(3, tag);
    _changed = true;
//    _attributes->insert(new OCCAttributes(3, solid));
  }
  if(recursive) {
    TopExp_Explorer exp0;
    for(exp0.Init(solid, TopAbs_SHELL); exp0.More(); exp0.Next()) {
      TopoDS_Shell shell = TopoDS::Shell(exp0.Current());
      if(!_shellTag.IsBound(shell)) {
        int t = getMaxTag(-2) + 1;
        bind(shell, t, recursive);
      }
    }
    for(exp0.Init(solid, TopAbs_FACE); exp0.More(); exp0.Next()) {
      TopoDS_Face face = TopoDS::Face(exp0.Current());
      if(!_faceTag.IsBound(face)) {
        int t = getMaxTag(3) + 1;
        bind(face, t, recursive);
      }
    }
  }
}

void SubshapeNumbering::bind(TopoDS_Shape shape, int dim, int tag, bool recursive)
{
  switch(dim) {
  case 0: bind(TopoDS::Vertex(shape), tag, recursive); break;
  case 1: bind(TopoDS::Edge(shape), tag, recursive); break;
  case 2: bind(TopoDS::Face(shape), tag, recursive); break;
  case 3: bind(TopoDS::Solid(shape), tag, recursive); break;
  case -1: bind(TopoDS::Wire(shape), tag, recursive); break;
  case -2: bind(TopoDS::Shell(shape), tag, recursive); break;
  default: break;
  }
}


void SubshapeNumbering::_addShapeToMaps(const TopoDS_Shape &shape)
{
  // Solids
  TopExp_Explorer exp0, exp1, exp2, exp3, exp4, exp5;
  for(exp0.Init(shape, TopAbs_SOLID); exp0.More(); exp0.Next()) {
    TopoDS_Solid solid = TopoDS::Solid(exp0.Current());
    if(_somap.FindIndex(solid) < 1) {
      _somap.Add(solid);
      for(exp1.Init(solid, TopAbs_SHELL); exp1.More(); exp1.Next()) {
        TopoDS_Shell shell = TopoDS::Shell(exp1.Current());
        if(_shmap.FindIndex(shell) < 1) {
          _shmap.Add(shell);

          for(exp2.Init(shell, TopAbs_FACE); exp2.More(); exp2.Next()) {
            TopoDS_Face face = TopoDS::Face(exp2.Current());
            if(_fmap.FindIndex(face) < 1) {
              _fmap.Add(face);

              for(exp3.Init(face.Oriented(TopAbs_FORWARD), TopAbs_WIRE);
                  exp3.More(); exp3.Next()) {
                // for(exp3.Init(face, TopAbs_WIRE); exp3.More(); exp3.Next()){
                TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
                if(_wmap.FindIndex(wire) < 1) {
                  _wmap.Add(wire);

                  for(exp4.Init(wire, TopAbs_EDGE); exp4.More(); exp4.Next()) {
                    TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
                    if(_emap.FindIndex(edge) < 1) {
                      _emap.Add(edge);

                      for(exp5.Init(edge, TopAbs_VERTEX); exp5.More();
                          exp5.Next()) {
                        TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
                        if(_vmap.FindIndex(vertex) < 1) _vmap.Add(vertex);
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // Free Shells
  for(exp1.Init(shape, TopAbs_SHELL, TopAbs_SOLID); exp1.More(); exp1.Next()) {
    const TopoDS_Shape &shell = exp1.Current();
    if(_shmap.FindIndex(shell) < 1) {
      _shmap.Add(shell);

      for(exp2.Init(shell, TopAbs_FACE); exp2.More(); exp2.Next()) {
        TopoDS_Face face = TopoDS::Face(exp2.Current());
        if(_fmap.FindIndex(face) < 1) {
          _fmap.Add(face);

          for(exp3.Init(face, TopAbs_WIRE); exp3.More(); exp3.Next()) {
            TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
            if(_wmap.FindIndex(wire) < 1) {
              _wmap.Add(wire);

              for(exp4.Init(wire, TopAbs_EDGE); exp4.More(); exp4.Next()) {
                TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
                if(_emap.FindIndex(edge) < 1) {
                  _emap.Add(edge);

                  for(exp5.Init(edge, TopAbs_VERTEX); exp5.More();
                      exp5.Next()) {
                    TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
                    if(_vmap.FindIndex(vertex) < 1) _vmap.Add(vertex);
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // Free Faces
  for(exp2.Init(shape, TopAbs_FACE, TopAbs_SHELL); exp2.More(); exp2.Next()) {
    TopoDS_Face face = TopoDS::Face(exp2.Current());
    if(_fmap.FindIndex(face) < 1) {
      _fmap.Add(face);

      for(exp3.Init(face, TopAbs_WIRE); exp3.More(); exp3.Next()) {
        TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
        if(_wmap.FindIndex(wire) < 1) {
          _wmap.Add(wire);

          for(exp4.Init(wire, TopAbs_EDGE); exp4.More(); exp4.Next()) {
            TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
            if(_emap.FindIndex(edge) < 1) {
              _emap.Add(edge);

              for(exp5.Init(edge, TopAbs_VERTEX); exp5.More(); exp5.Next()) {
                TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
                if(_vmap.FindIndex(vertex) < 1) _vmap.Add(vertex);
              }
            }
          }
        }
      }
    }
  }

  // Free Wires
  for(exp3.Init(shape, TopAbs_WIRE, TopAbs_FACE); exp3.More(); exp3.Next()) {
    TopoDS_Wire wire = TopoDS::Wire(exp3.Current());
    if(_wmap.FindIndex(wire) < 1) {
      _wmap.Add(wire);

      for(exp4.Init(wire, TopAbs_EDGE); exp4.More(); exp4.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
        if(_emap.FindIndex(edge) < 1) {
          _emap.Add(edge);

          for(exp5.Init(edge, TopAbs_VERTEX); exp5.More(); exp5.Next()) {
            TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
            if(_vmap.FindIndex(vertex) < 1) _vmap.Add(vertex);
          }
        }
      }
    }
  }

  // Free Edges
  for(exp4.Init(shape, TopAbs_EDGE, TopAbs_WIRE); exp4.More(); exp4.Next()) {
    TopoDS_Edge edge = TopoDS::Edge(exp4.Current());
    if(_emap.FindIndex(edge) < 1) {
      _emap.Add(edge);

      for(exp5.Init(edge, TopAbs_VERTEX); exp5.More(); exp5.Next()) {
        TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
        if(_vmap.FindIndex(vertex) < 1) _vmap.Add(vertex);
      }
    }
  }

  // Free Vertices
  for(exp5.Init(shape, TopAbs_VERTEX, TopAbs_EDGE); exp5.More(); exp5.Next()) {
    TopoDS_Vertex vertex = TopoDS::Vertex(exp5.Current());
    if(_vmap.FindIndex(vertex) < 1) { _vmap.Add(vertex); }
  }
}


SubshapeNumbering::SubshapeNumbering(const TopoDS_Shape& shape)
{

//  void OCC_Internals::_multiBind(const TopoDS_Shape &shape, int tag,
//                                 std::vector<std::pair<int, int> > &outDimTags,
//                                 bool highestDimOnly, bool recursive,
//                                 bool returnNewOnly)

    for(int i = 0; i < 6; i++) _maxTag[i] = 0;

    int tag=-1;
    std::vector<std::pair<int, int> > outDimTags;
    bool highestDimOnly=false;
    bool recursive=true;
    bool returnNewOnly = false;

    std::string format="brep";

    {
        TopExp_Explorer exp0;
        int count = 0;
        for(exp0.Init(shape, TopAbs_SOLID); exp0.More(); exp0.Next()) {
          TopoDS_Solid solid = TopoDS::Solid(exp0.Current());
          bool exists = false;
          int t = tag;
          if(t <= 0) {
            if(_solidTag.IsBound(solid)) {
              t = _solidTag.Find(solid);
              exists = true;
            }
            else
              t = getMaxTag(3) + 1;
          }
          else if(count) {
            throw insight::Exception("Cannot bind multiple volumes to single tag %d", t);
            return;
          }
          if(!exists) bind(solid, t, recursive);
          if(!exists || !returnNewOnly)
            outDimTags.push_back(std::pair<int, int>(3, t));
          count++;
        }
        if(highestDimOnly && count) return;
        for(exp0.Init(shape, TopAbs_FACE); exp0.More(); exp0.Next()) {
          TopoDS_Face face = TopoDS::Face(exp0.Current());
          bool exists = false;
          int t = tag;
          if(t <= 0) {
            if(_faceTag.IsBound(face)) {
              t = _faceTag.Find(face);
              exists = true;
            }
            else
              t = getMaxTag(2) + 1;
          }
          else if(count) {
            throw insight::Exception("Cannot bind multiple surfaces to single tag %d", t);
            return;
          }
          if(!exists) bind(face, t, recursive);
          if(!exists || !returnNewOnly)
            outDimTags.push_back(std::pair<int, int>(2, t));
          count++;
        }
        if(highestDimOnly && count) return;
        for(exp0.Init(shape, TopAbs_EDGE); exp0.More(); exp0.Next()) {
          TopoDS_Edge edge = TopoDS::Edge(exp0.Current());
          bool exists = false;
          int t = tag;
          if(t <= 0) {
            if(_edgeTag.IsBound(edge)) {
              t = _edgeTag.Find(edge);
              exists = true;
            }
            else
              t = getMaxTag(1) + 1;
          }
          else if(count) {
            throw insight::Exception("Cannot bind multiple curves to single tag %d", t);
            return;
          }
          if(!exists) bind(edge, t, recursive);
          if(!exists || !returnNewOnly)
            outDimTags.push_back(std::pair<int, int>(1, t));
          count++;
        }
        if(highestDimOnly && count) return;
        for(exp0.Init(shape, TopAbs_VERTEX); exp0.More(); exp0.Next()) {
          TopoDS_Vertex vertex = TopoDS::Vertex(exp0.Current());
          bool exists = false;
          int t = tag;
          if(t <= 0) {
            if(_vertexTag.IsBound(vertex)) {
              t = _vertexTag.Find(vertex);
              exists = true;
            }
            else
              t = getMaxTag(0) + 1;
          }
          else if(count) {
            throw insight::Exception("Cannot bind multiple points to single tag %d", t);
            return;
          }
          if(!exists) bind(vertex, t, recursive);
          if(!exists || !returnNewOnly)
            outDimTags.push_back(std::pair<int, int>(0, t));
          count++;
        }
    }

    {
        _somap.Clear();
        _shmap.Clear();
        _fmap.Clear();
        _wmap.Clear();
        _emap.Clear();
        _vmap.Clear();
        TopTools_DataMapIteratorOfDataMapOfIntegerShape exp0(_tagVertex);
        for(; exp0.More(); exp0.Next()) _addShapeToMaps(exp0.Value());
        TopTools_DataMapIteratorOfDataMapOfIntegerShape exp1(_tagEdge);
        for(; exp1.More(); exp1.Next()) _addShapeToMaps(exp1.Value());
        TopTools_DataMapIteratorOfDataMapOfIntegerShape exp2(_tagFace);
        for(; exp2.More(); exp2.Next()) _addShapeToMaps(exp2.Value());
        TopTools_DataMapIteratorOfDataMapOfIntegerShape exp3(_tagSolid);
        for(; exp3.More(); exp3.Next()) _addShapeToMaps(exp3.Value());
    }

}

} // namespace cad
} // namespace insight
