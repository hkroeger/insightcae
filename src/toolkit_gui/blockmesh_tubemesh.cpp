#include "blockmesh_tubemesh.h"
#include "base/boost_include.h"

#include "base/units.h"

#include "occinclude.h"
#include "BRepTools.hxx"

using namespace boost;
using namespace boost::assign;

namespace insight
{

namespace bmd
{



defineType ( blockMeshDict_TubeMesh );
addToOpenFOAMCaseElementFactoryTable( blockMeshDict_TubeMesh );


blockMeshDict_TubeMesh::blockMeshDict_TubeMesh ( OpenFOAMCase& c, const ParameterSet& ps )
    : BlockMeshTemplate ( c ), p_ ( ps )
{}



void blockMeshDict_TubeMesh::create_bmd()
{

  TopoDS_Shape s;
  BRep_Builder sb;
  BRepTools::Read(s, p_.geometry.wire.c_str(), sb);
  TopoDS_Wire w = TopoDS::Wire(s);

  GProp_GProps p;
  BRepGProp::LinearProperties(w, p);
  double L=p.Mass();
  double dx=L/double(p_.mesh.nx);

  arma::mat last_er;
  auto calc_or_get_er = [&] (const arma::mat& ex) -> arma::mat
  {
      if (last_er.n_elem==0)
        {
          arma::mat im=arma::cross(ex, vec3(1,0,0));
          if (arma::norm(im,2)<1e-10)
            {
              im=arma::cross(ex, vec3(0,1,0));
            }
          last_er=arma::cross(im, ex);
        }

      return last_er;
  };

  std::vector<TopoDS_Edge> edgs;
  for ( BRepTools_WireExplorer wex(w); wex.More(); wex.Next() )
    {
      edgs.push_back(wex.Current());
    }

  for ( decltype(edgs)::const_iterator i=edgs.begin(); i!=edgs.end(); i++ )
    {
      bool is_start = (i==edgs.begin());
      bool is_end = ((i+1)==edgs.end());
      TopoDS_Edge e = *i;

      BRepAdaptor_Curve ac(e);
      gp_Pnt ps = BRep_Tool::Pnt(TopExp::FirstVertex(e, true));
      gp_Pnt pe = BRep_Tool::Pnt(TopExp::LastVertex(e, true));

      switch (ac.GetType())
        {
        case GeomAbs_Line: {
            blockMeshDict_Cylinder::Parameters cp;

            arma::mat L = vec3( pe.XYZ() - ps.XYZ() );
            double l = arma::norm(L, 2);

            cp.geometry.ex = L/l;
            cp.geometry.er = calc_or_get_er(cp.geometry.ex);

            cp.geometry.D = p_.geometry.D;
            cp.geometry.L = l;
            cp.geometry.p0 = vec3(ps);

            cp.mesh.nr = p_.mesh.nr;
            cp.mesh.nu = p_.mesh.nu;
            cp.mesh.nx = std::max(1, int(l/dx));
            cp.mesh.gradr = p_.mesh.gradr;
            cp.mesh.core_fraction = p_.mesh.core_fraction;

            cp.mesh.defaultPatchName = p_.mesh.defaultPatchName;
            cp.mesh.circumPatchName = p_.mesh.circumPatchName;
            if (is_start) cp.mesh.basePatchName = p_.mesh.basePatchName;
            if (is_end) cp.mesh.topPatchName = p_.mesh.topPatchName;

            OpenFOAMCase dummy(OFcase().ofe());
            blockMeshDict_Cylinder c(dummy, cp);
            c.create_bmd();
            copy(c);
          }
          break;

        case GeomAbs_Circle: {
            blockMeshDict_CurvedCylinder::Parameters ccp;

            gp_Vec v; gp_Pnt p;
            ac.D1(ac.FirstParameter(), p, v);

            GProp_GProps pp;
            BRepGProp::LinearProperties(e, pp);
            double l=pp.Mass();

            ccp.geometry.ex = vec3(v.Normalized());
            ccp.geometry.er = calc_or_get_er(ccp.geometry.ex);

            ccp.geometry.D = p_.geometry.D;
            ccp.geometry.p0 = vec3(ps);
            ccp.geometry.p1 = vec3(pe);

            ccp.mesh.nr = p_.mesh.nr;
            ccp.mesh.nu = p_.mesh.nu;
            ccp.mesh.nx = std::max(1, int(l/dx));
            ccp.mesh.gradr = p_.mesh.gradr;
            ccp.mesh.core_fraction = p_.mesh.core_fraction;

            ccp.mesh.defaultPatchName = p_.mesh.defaultPatchName;
            ccp.mesh.circumPatchName = p_.mesh.circumPatchName;
            if (is_start) ccp.mesh.basePatchName = p_.mesh.basePatchName;
            if (is_end) ccp.mesh.topPatchName = p_.mesh.topPatchName;

            OpenFOAMCase dummy(OFcase().ofe());
            blockMeshDict_CurvedCylinder cc(dummy, ccp);
            last_er=cc.calc_end_CS().ez;
            cc.create_bmd();
            copy(cc);
          }
          break;

        default:
          throw insight::Exception(
                boost::str(boost::format("encountered unhandled curve type: %d") % ac.GetType())
                );
        }

    }
}


}
}
