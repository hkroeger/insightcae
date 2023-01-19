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
    bool highestDimOnly=false;
    std::vector<std::pair<int, int> > outDimTags;
    std::string format="brep";
    bool recursive=true;
    bool returnNewOnly = false;

    int tag=-1;
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
