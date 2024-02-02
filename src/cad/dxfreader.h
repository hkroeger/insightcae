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
#include "boost/regex.h"

namespace insight {
namespace cad {

class DXFReader
: public DL_CreationAdapter
{
protected:
  boost::regex layerpattern_;
  std::map<std::string, TopTools_ListOfShape> ls_;

  struct Polyline
  {
    DXFReader& reader;
    std::string layername_;

    Polyline(DXFReader&, const std::string&);
    ~Polyline();

    bool closed;
    std::unique_ptr<gp_Pnt> lp, p0;
    double lbulge;
  };

  mutable std::unique_ptr<Polyline> pl_;

  int spl_deg_, spl_nknot_, spl_nctrl_;
  std::vector<gp_Pnt> splp_;
  std::vector<double> splk_;

  std::string curLayerName();
  bool notFiltered();

public:
  /**
   * @brief DXFReader
   * @param filename
   * @param layername
   * if empty, all layers are read
   */
  DXFReader(
          const boost::filesystem::path& filename,
          const std::string& layerpattern="^0$" );

  virtual ~DXFReader();
  virtual void addArc(const DL_ArcData &);
  virtual void addLine(const DL_LineData &);
  virtual void addPolyline(const DL_PolylineData &);
  virtual void addVertex(const DL_VertexData &);
  virtual void addVertexPolyLine(const DL_VertexData &, Polyline& pl);

  virtual void addSpline(const DL_SplineData&);
  virtual void addKnot(const DL_KnotData&);
  virtual void addControlPoint(const DL_ControlPointData&);
  void buildSpline();

  Handle_TopTools_HSequenceOfShape Wires(
          double tol=Precision::Confusion(),
          const std::string& layername=std::string() ) const;

  std::vector<std::string> layers(const std::string& pattern = ".*") const;
  std::vector<std::string> layers(const boost::regex& pattern) const;
};

}
}

#endif // DXFREADER_H
