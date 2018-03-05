#ifndef DXFREADER_H
#define DXFREADER_H

#include "base/linearalgebra.h"
#include "occinclude.h"

#include "cadtypes.h"
#include "cadparameters.h"
#include "cadfeature.h"

#include "dxflib/dl_creationadapter.h"
#include "dxflib/dl_dxf.h"

#include "base/boost_include.h"

namespace insight {
namespace cad {

class DXFReader
: public DL_CreationAdapter
{
protected:
  std::string layername_;
  std::map<std::string, TopTools_ListOfShape> ls_;

  struct Polyline
  {
    DXFReader& reader;
    std::string layername_;

    Polyline(DXFReader&, const std::string&);
    ~Polyline();

    bool closed;
    std::auto_ptr<gp_Pnt> lp, p0;
    double lbulge;
  };

  mutable std::auto_ptr<Polyline> pl_;

  int spl_deg_, spl_nknot_, spl_nctrl_;
  std::vector<gp_Pnt> splp_;
  std::vector<double> splk_;

  bool notFiltered();
  std::string curLayerName();

public:
  DXFReader(const boost::filesystem::path& filename, const std::string& layername="0");

  virtual ~DXFReader();
  virtual void addArc(const DL_ArcData &);
  virtual void addLine(const DL_LineData &);
  virtual void addPolyline(const DL_PolylineData &);
  virtual void addVertex(const DL_VertexData &);

  virtual void addSpline(const DL_SplineData&);
  virtual void addKnot(const DL_KnotData&);
  virtual void addControlPoint(const DL_ControlPointData&);
  void buildSpline();

  TopoDS_Wire Wire(double tol=Precision::Confusion(), const std::string& layername="") const;
  std::vector<std::string> layers() const;
};

}
}

#endif // DXFREADER_H
