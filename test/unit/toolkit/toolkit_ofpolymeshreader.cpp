
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>

#include "base/exception.h"
#include "openfoam/ofpolymeshreader.h"

#include "boost/filesystem.hpp"
#include "boost/iostreams/filtering_stream.hpp"
#include "boost/iostreams/filter/gzip.hpp"

using namespace insight;
namespace fs = boost::filesystem;


// single hex cell, all faces are boundary faces
const std::vector<std::vector<double> > hexPts = {
  {0,0,0}, {1,0,0}, {1,1,0}, {0,1,0},
  {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1}
};
const std::vector<std::vector<int64_t> > hexFaces = {
  {0,3,2,1},                                            // bottom
  {0,1,5,4}, {1,2,6,5}, {2,3,7,6}, {3,0,4,7},           // sides
  {4,5,6,7}                                             // top
};


std::string header(const std::string& cls, bool binary, int labelBits)
{
  return
      "FoamFile\n{\n"
      "    version     2.0;\n"
      "    format      "+std::string(binary?"binary":"ascii")+";\n"
      "    arch        \"LSB;label="+std::to_string(labelBits)+";scalar=64\";\n"
      "    class       "+cls+";\n"
      "    object      x;\n"
      "}\n// comment\n/* block\n comment */\n\n";
}

template<class L>
void writeBinaryLabels(std::ostream& f, const std::vector<int64_t>& l)
{
  f<<l.size()<<"\n(";
  for (auto v: l) { L x=L(v); f.write(reinterpret_cast<const char*>(&x), sizeof(L)); }
  f<<")\n";
}

void writeFile(const fs::path& fn, const std::string& content, bool gz)
{
  if (gz)
  {
    std::ofstream f(fn.string()+".gz", std::ios::binary);
    boost::iostreams::filtering_ostream out;
    out.push(boost::iostreams::gzip_compressor());
    out.push(f);
    out<<content;
  }
  else
  {
    std::ofstream f(fn.string(), std::ios::binary);
    f<<content;
  }
}

enum Format { AsciiFaceList, AsciiCompact, BinaryCompact32, BinaryCompact64 };

void writeHexMesh(const fs::path& pmd, Format fmt, bool gz)
{
  fs::create_directories(pmd);

  {
    std::ofstream f((pmd/"boundary").string());
    f << header("polyBoundaryMesh", false, 32)
      << "3\n(\n"
         "bottom { type wall; inGroups 1(wall); nFaces 1; startFace 0; }\n"
         "sides { type patch; nFaces 4; startFace 1; }\n"
         "top { type wall; nFaces 1; startFace 5; }\n"
         ")\n";
  }

  bool binary = (fmt==BinaryCompact32 || fmt==BinaryCompact64);
  int lb = (fmt==BinaryCompact64) ? 64 : 32;

  {
    std::ostringstream f;
    f<<header("vectorField", binary, lb);
    f<<hexPts.size()<<"\n(";
    for (const auto& p: hexPts)
    {
      if (binary)
        f.write(reinterpret_cast<const char*>(p.data()), 3*sizeof(double));
      else
        f<<"\n("<<p[0]<<" "<<p[1]<<"e+00 "<<p[2]<<")";
    }
    f<<")\n";
    writeFile(pmd/"points", f.str(), gz);
  }

  {
    std::ostringstream f;
    if (fmt==AsciiFaceList)
    {
      f<<header("faceList", false, 32);
      f<<hexFaces.size()<<"\n(\n";
      for (const auto& fc: hexFaces)
      {
        f<<fc.size()<<"(";
        for (size_t i=0; i<fc.size(); ++i) f<<(i?" ":"")<<fc[i];
        f<<")\n";
      }
      f<<")\n";
    }
    else
    {
      std::vector<int64_t> ofs{0}, verts;
      for (const auto& fc: hexFaces)
      {
        verts.insert(verts.end(), fc.begin(), fc.end());
        ofs.push_back(verts.size());
      }
      f<<header("faceCompactList", binary, lb);
      if (fmt==AsciiCompact)
      {
        f<<ofs.size()<<"\n(\n"; for (auto o: ofs) f<<o<<"\n"; f<<")\n\n";
        f<<verts.size()<<"\n(\n"; for (auto v: verts) f<<v<<"\n"; f<<")\n";
      }
      else if (fmt==BinaryCompact32)
      {
        writeBinaryLabels<int32_t>(f, ofs);
        f<<"\n";
        writeBinaryLabels<int32_t>(f, verts);
      }
      else
      {
        writeBinaryLabels<int64_t>(f, ofs);
        f<<"\n";
        writeBinaryLabels<int64_t>(f, verts);
      }
    }
    writeFile(pmd/"faces", f.str(), gz);
  }
}


bool near(double a, double b) { return std::fabs(a-b)<1e-10; }


void checkHexMesh(const fs::path& pmd)
{
  auto bnd = readPolyMeshBoundary(pmd);
  assertion(bnd.size()==3, "expected 3 patches");
  assertion(bnd.at("sides").type=="patch" && bnd.at("sides").startFace==1 && bnd.at("sides").nFaces==4,
            "wrong patch info");

  auto bottom = readPolyMeshPatchSurface(pmd, {"bottom"});
  assertion(bottom.nFaces()==1 && bottom.nPoints()==4, "bottom: wrong size");
  for (size_t i=0; i<bottom.nPoints(); ++i)
    assertion(near(bottom.points[3*i+2], 0), "bottom: wrong z coordinate");

  auto top = readPolyMeshPatchSurface(pmd, {"top"});
  assertion(top.nFaces()==1 && top.nPoints()==4, "top: wrong size");
  for (size_t i=0; i<top.nPoints(); ++i)
    assertion(near(top.points[3*i+2], 1), "top: wrong z coordinate");
  // face connectivity preserved: original 4,5,6,7
  for (int j=0; j<4; ++j)
  {
    const double* x = &top.points[3*top.faceVerts[j]];
    for (int k=0; k<3; ++k)
      assertion(near(x[k], hexPts[4+j][k]), "top: wrong vertex coordinate");
  }

  auto sides = readPolyMeshPatchSurface(pmd, {"sides"});
  assertion(sides.nFaces()==4 && sides.nPoints()==8 && sides.faceVerts.size()==16, "sides: wrong size");

  auto topbottom = readPolyMeshPatchSurface(pmd, {"top", "bot.*"});
  assertion(topbottom.nFaces()==2 && topbottom.nPoints()==8, "top+bottom: wrong size");

  auto pd = toVTKPolyData(topbottom);
  assertion(pd->GetNumberOfCells()==2 && pd->GetNumberOfPoints()==8, "VTK: wrong size");

  auto all = readPolyMeshPatchSurface(pmd, {".*"});
  assertion(all.nFaces()==6, "all: wrong size");
  double r = maxRadius(all, vec3(0.5, 0.5, -3), vec3(0, 0, 2));
  assertion(near(r, std::sqrt(0.5)), "wrong max radius %g", r);
  r = maxRadius(all, vec3(0, 0, 0), vec3(1, 0, 0));
  assertion(near(r, std::sqrt(2.)), "wrong max radius %g", r);

  auto pdr = planeDistanceRange(all, vec3(0, 0, 0.25), vec3(0, 0, -2));
  assertion(near(pdr.first, -0.75) && near(pdr.second, 0.25), "wrong plane distance range");
  pdr = planeDistanceRange(top, vec3(0, 0, 0), vec3(1, 1, 0));
  assertion(near(pdr.first, 0) && near(pdr.second, std::sqrt(2.)), "wrong plane distance range (oblique)");

  auto fst = faceSizeStatistics(all);
  assertion(fst.nFaces==6 && near(fst.mean, 1) && near(fst.min, 1) && near(fst.max, 1) && near(fst.median, 1),
            "wrong face size statistics");
}




int main(int argc, char*argv[])
{
  try
  {
    insight::assertion(
        argc==2,
        "specify source dir with Openfoam case as argument!");

    fs::path tmp = fs::temp_directory_path()/fs::unique_path("ofpolymeshreader-%%%%-%%%%");

    for (Format fmt: {AsciiFaceList, AsciiCompact, BinaryCompact32, BinaryCompact64})
    {
      for (bool gz: {false, true})
      {
        auto pmd = tmp/("f"+std::to_string(int(fmt))+(gz?"gz":""))/"constant"/"polyMesh";
        std::cout<<"checking "<<pmd<<std::endl;
        writeHexMesh(pmd, fmt, gz);
        checkHexMesh(pmd);
      }
    }
    fs::remove_all(tmp);

    {
      // face sizes 1 (unit square), 1 (triangle with area 1), sqrt(2) (2x1 rectangle)
      OFPatchSurface s;
      s.points = { 0,0,0, 1,0,0, 1,1,0, 0,1,0, 2,0,0, 2,1,0, 0,0,5, 2,0,5, 0,1,5 };
      s.faceOffsets = { 0, 4, 7, 11 };
      s.faceVerts = { 0,1,2,3,  6,7,8,  0,4,5,3 };
      auto st = faceSizeStatistics(s);
      std::cout<<"mean="<<st.mean<<" min="<<st.min<<" max="<<st.max<<" median="<<st.median<<std::endl;
      assertion(st.nFaces==3, "wrong face count");
      assertion(near(st.min, 1) && near(st.max, std::sqrt(2.)) && near(st.median, 1)
                && near(st.mean, (2.+std::sqrt(2.))/3.), "wrong face size statistics (odd count)");

      // square and rectangle only: even count
      s.faceOffsets = { 0, 4, 8 };
      s.faceVerts = { 0,1,2,3,  0,4,5,3 };
      st = faceSizeStatistics(s);
      assertion(near(st.median, 0.5*(1.+std::sqrt(2.))), "wrong median (even count)");

      bool thrown=false;
      try { faceSizeStatistics(OFPatchSurface()); } catch (const insight::Exception&) { thrown=true; }
      assertion(thrown, "expected exception for empty surface");
    }


    {
      auto caseDir = fs::path(argv[1])/"tutorial_mixerVesselAMI2D";
      auto pmd = polyMeshDirectory(caseDir);
      size_t nTotal=0;
      for (const auto& p: readPolyMeshBoundary(pmd))
      {
        auto s = readPolyMeshPatchSurface(pmd, {p.first});
        std::cout<<p.first<<": "<<s.nFaces()<<" faces, "<<s.nPoints()<<" points"<<std::endl;
        assertion(int64_t(s.nFaces())==p.second.nFaces, "wrong number of faces in patch "+p.first);
        for (auto v: s.faceVerts)
          assertion(v>=0 && size_t(v)<s.nPoints(), "invalid vertex index");
        nTotal+=s.nFaces();
      }
      assertion(readPatchSurface(caseDir, ".*").nFaces()==nTotal, "inconsistent face number");

      auto pd = readPatchAsVTKPolyData(caseDir, "rotor");
      assertion(pd->GetNumberOfCells()==192, "VTK: wrong number of cells");

      double rs = maxPatchRadius(caseDir, "stator", vec3Zero(), vec3(0,0,1));
      double rr = maxPatchRadius(caseDir, "rotor", vec3Zero(), vec3(0,0,1));
      std::cout<<"r_stator="<<rs<<", r_rotor="<<rr<<std::endl;
      assertion(std::fabs(rs-0.1)<1e-6, "unexpected stator radius %g", rs);
      assertion(std::fabs(rr-0.05)<1e-6, "unexpected rotor radius %g", rr);
      assertion(readPatchSurface(caseDir, "rotor").nPoints()==384, "rotor: wrong number of points");

      auto zr = patchPlaneDistanceRange(caseDir, "stator", vec3(0,0,0.005), vec3(0,0,1));
      auto xr = patchPlaneDistanceRange(caseDir, "stator", vec3Zero(), vec3(-1,0,0));
      std::cout<<"stator z-range=["<<zr.first<<", "<<zr.second<<"], x-range=["<<xr.first<<", "<<xr.second<<"]"<<std::endl;
      assertion(std::fabs(zr.first+0.005)<1e-9 && std::fabs(zr.second-0.005)<1e-9, "stator: wrong z distance range");
      assertion(std::fabs(xr.first+0.1)<1e-9 && std::fabs(xr.second-0.1)<1e-9, "stator: wrong x distance range");

      // reference values computed independently
      auto st = patchFaceSizeStatistics(caseDir, "rotor");
      std::cout<<"rotor face size: mean="<<st.mean<<" min="<<st.min<<" max="<<st.max<<" median="<<st.median<<std::endl;
      assertion(st.nFaces==192, "rotor: wrong face count");
      assertion(std::fabs(st.mean-0.0043088417)<1e-6 && std::fabs(st.min-0.0036176834)<1e-6
                && std::fabs(st.max-0.005)<1e-6 && std::fabs(st.median-0.0043088417)<1e-6,
                "rotor: wrong face size statistics");
    }

    {
      // compressed mesh in region
      auto caseDir = fs::path(argv[1])/"tutorial_chtMultiRegionSimpleFoam_heatExchanger";
      auto pmd = polyMeshDirectory(caseDir, "porous");
      for (const auto& p: readPolyMeshBoundary(pmd))
      {
        auto s = readPatchSurface(caseDir, p.first, "porous");
        std::cout<<"porous/"<<p.first<<": "<<s.nFaces()<<" faces, "<<s.nPoints()<<" points"<<std::endl;
        assertion(int64_t(s.nFaces())==p.second.nFaces, "wrong number of faces in patch "+p.first);
      }
    }
  }
  catch (const std::exception& e)
  {
    std::cerr<<e.what()<<std::endl;
    return -1;
  }

  return 0;
}
