/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "ofpolymeshreader.h"

#include "base/exception.h"
#include "openfoam/openfoamdict.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <locale.h>
#include <sstream>
#include <stdlib.h>

#include "boost/filesystem.hpp"
#include "boost/regex.hpp"
#include "boost/iostreams/filtering_stream.hpp"
#include "boost/iostreams/filter/gzip.hpp"

#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkIdList.h"

namespace fs = boost::filesystem;

namespace insight
{

namespace
{


/**
 * read file content into memory. Use <file>.gz, if the uncompressed file does not exist.
 */
std::string loadFile(const fs::path& file)
{
  fs::path gzFile(file.string()+".gz");

  if (fs::exists(file))
  {
    std::ifstream f(file.string(), std::ios::binary);
    f.seekg(0, std::ios::end);
    std::string content(size_t(f.tellg()), '\0');
    f.seekg(0, std::ios::beg);
    f.read(&content[0], content.size());
    return content;
  }
  else if (fs::exists(gzFile))
  {
    std::ifstream compressed(gzFile.string(), std::ios::binary);
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(boost::iostreams::gzip_decompressor());
    in.push(compressed);
    std::istream f(&in);
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
  }
  else
  {
    throw insight::Exception(
        "Neither file %s nor %s exist!",
        file.string().c_str(), gzFile.string().c_str() );
  }
}




/**
 * minimal parser state for OpenFOAM mesh files
 */
struct FoamFileParser
{
  std::string fileName;
  const char *p, *end;

  bool binary = false;
  std::string className;
  int labelSize = 4;
  int scalarSize = 8;

  FoamFileParser(const std::string& content, const std::string& fn)
    : fileName(fn),
      p(content.data()),
      end(content.data()+content.size())
  {
    parseHeader();
  }

  [[noreturn]] void error(const std::string& msg) const
  {
    throw insight::Exception(
        "Error while reading %s: %s",
        fileName.c_str(), msg.c_str() );
  }

  void skipWS()
  {
    while (p<end)
    {
      if (std::isspace(static_cast<unsigned char>(*p)))
      {
        ++p;
      }
      else if (*p=='/' && p+1<end && p[1]=='/')
      {
        const char* nl = static_cast<const char*>(std::memchr(p, '\n', end-p));
        p = nl ? nl+1 : end;
      }
      else if (*p=='/' && p+1<end && p[1]=='*')
      {
        const char* q=p+2;
        while (q+1<end && !(q[0]=='*' && q[1]=='/')) ++q;
        p = (q+1<end) ? q+2 : end;
      }
      else
        break;
    }
  }

  void expect(char c)
  {
    skipWS();
    if (p>=end || *p!=c)
      error(std::string("expected '")+c+"'");
    ++p;
  }

  std::string readWord()
  {
    skipWS();
    std::string w;
    if (p<end && *p=='"')
    {
      ++p;
      const char* q = static_cast<const char*>(std::memchr(p, '"', end-p));
      if (!q) error("unterminated string");
      w.assign(p, q);
      p=q+1;
    }
    else
    {
      const char* s=p;
      while (p<end
             && !std::isspace(static_cast<unsigned char>(*p))
             && *p!=';' && *p!='{' && *p!='}' && *p!='(' && *p!=')')
        ++p;
      w.assign(s, p);
    }
    return w;
  }

  void parseHeader()
  {
    skipWS();
    if (readWord()!="FoamFile")
      error("FoamFile header expected");
    expect('{');
    for (;;)
    {
      skipWS();
      if (p>=end) error("unterminated FoamFile header");
      if (*p=='}') { ++p; break; }

      std::string key=readWord();
      std::string value;
      for (;;) // value may consist of multiple words
      {
        skipWS();
        if (p>=end) error("unterminated header entry "+key);
        if (*p==';') { ++p; break; }
        if (!value.empty()) value+=" ";
        value+=readWord();
      }

      if (key=="format")
        binary = (value=="binary");
      else if (key=="class")
        className=value;
      else if (key=="arch")
      {
        boost::smatch m;
        if (boost::regex_search(value, m, boost::regex("label=([0-9]+)")))
          labelSize=std::stoi(m[1])/8;
        if (boost::regex_search(value, m, boost::regex("scalar=([0-9]+)")))
          scalarSize=std::stoi(m[1])/8;
        if (value.find("MSB")!=std::string::npos)
          error("big endian binary files are not supported");
      }
    }
    if ( (labelSize!=4 && labelSize!=8) || (scalarSize!=4 && scalarSize!=8) )
      error("unsupported label or scalar size");
  }

  // ascii number parsing

  int64_t readLabel()
  {
    skipWS();
    bool neg=false;
    if (p<end && (*p=='-' || *p=='+')) { neg=(*p=='-'); ++p; }
    if (p>=end || !std::isdigit(static_cast<unsigned char>(*p)))
      error("integer number expected");
    int64_t v=0;
    while (p<end && std::isdigit(static_cast<unsigned char>(*p)))
    {
      v=v*10+(*p-'0');
      ++p;
    }
    return neg ? -v : v;
  }

  double readScalar()
  {
    skipWS();
    if (p<end && *p=='+') ++p;
    double v;
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
    auto r = std::from_chars(p, end, v);
    if (r.ec != std::errc())
      error("floating point number expected");
    p=r.ptr;
#else
    // locale independent fallback (the GUI may have set a locale with decimal comma)
    // buffer is null-terminated (std::string), so strtod can not run past the end
    char* e;
#  ifdef _WIN32
    static _locale_t cloc = _create_locale(LC_NUMERIC, "C");
    v = _strtod_l(p, &e, cloc);
#  else
    static locale_t cloc = newlocale(LC_NUMERIC_MASK, "C", locale_t(0));
    v = strtod_l(p, &e, cloc);
#  endif
    if (e==p)
      error("floating point number expected");
    p=e;
#endif
    return v;
  }

  // skip one whitespace-separated ascii token
  void skipToken()
  {
    skipWS();
    while (p<end && !std::isspace(static_cast<unsigned char>(*p)) && *p!=')') ++p;
  }

  // skip ascii list entries of the form "...(...)"
  void skipParenthesizedEntries(int64_t n)
  {
    for (int64_t i=0; i<n; ++i)
    {
      const char* q = static_cast<const char*>(std::memchr(p, ')', end-p));
      if (!q) error("unexpected end of file");
      p=q+1;
    }
  }

  /**
   * reads list size and opening bracket.
   * Leaves p at first list element (or first byte of binary block).
   */
  int64_t beginList()
  {
    int64_t n=readLabel();
    skipWS();
    if (p<end && *p=='{')
      error("uniform lists are not supported");
    if (binary)
    {
      if (p>=end || *p!='(') error("expected '('");
      ++p;
    }
    else
      expect('(');
    return n;
  }

  /**
   * binary list block, p is advanced behind closing bracket
   */
  const char* binaryBlock(int64_t n, int elemSize)
  {
    const char* data = p;
    if (end-p < n*elemSize+1)
      error("binary block truncated");
    p += n*elemSize;
    expect(')');
    return data;
  }

  int64_t binaryLabel(const char* block, int64_t i) const
  {
    if (labelSize==4)
    {
      int32_t v; std::memcpy(&v, block+4*i, 4); return v;
    }
    int64_t v; std::memcpy(&v, block+8*i, 8); return v;
  }

  double binaryScalar(const char* block, int64_t i) const
  {
    if (scalarSize==8)
    {
      double v; std::memcpy(&v, block+8*i, 8); return v;
    }
    float v; std::memcpy(&v, block+4*i, 4); return v;
  }
};




typedef std::vector<std::pair<int64_t,int64_t> > FaceRanges; // [start, end)


/**
 * reads the faces in the given (sorted, non-overlapping) ranges
 * @param faceOffsets
 * output: offsets
 * @param faceVerts
 * output: global point labels
 */
void readFaces(
    const fs::path& polyMeshDir,
    const FaceRanges& ranges,
    std::vector<int64_t>& faceOffsets,
    std::vector<int64_t>& faceVerts )
{
  faceOffsets.assign(1, 0);
  faceVerts.clear();
  if (ranges.empty()) return;

  auto content = loadFile(polyMeshDir/"faces");
  FoamFileParser fp(content, (polyMeshDir/"faces").string());

  int64_t maxFace = ranges.back().second;

  if (fp.className=="faceCompactList")
  {
    int64_t nOfs = fp.beginList();
    if (nOfs < maxFace+1)
      fp.error("face index out of range");

    if (fp.binary)
    {
      const char* ofs = fp.binaryBlock(nOfs, fp.labelSize);
      int64_t nVerts = fp.beginList();
      const char* verts = fp.binaryBlock(nVerts, fp.labelSize);

      for (const auto& r: ranges)
      {
        for (int64_t fi=r.first; fi<r.second; ++fi)
        {
          int64_t s=fp.binaryLabel(ofs, fi), e=fp.binaryLabel(ofs, fi+1);
          if (s<0 || e<s || e>nVerts) fp.error("invalid face offsets");
          for (int64_t j=s; j<e; ++j)
            faceVerts.push_back(fp.binaryLabel(verts, j));
          faceOffsets.push_back(faceVerts.size());
        }
      }
    }
    else
    {
      std::vector<int64_t> ofs(maxFace+1);
      for (auto& o: ofs) o=fp.readLabel();
      // skip remaining offsets
      const char* q = static_cast<const char*>(std::memchr(fp.p, ')', fp.end-fp.p));
      if (!q) fp.error("unexpected end of file");
      fp.p=q+1;

      int64_t nVerts = fp.beginList();
      int64_t cur=0;
      for (const auto& r: ranges)
      {
        int64_t s=ofs[r.first], e=ofs[r.second];
        if (s<cur || e<s || e>nVerts) fp.error("invalid face offsets");
        for (; cur<s; ++cur) fp.skipToken();
        for (int64_t fi=r.first; fi<r.second; ++fi)
        {
          for (int64_t j=ofs[fi]; j<ofs[fi+1]; ++j)
            faceVerts.push_back(fp.readLabel());
          faceOffsets.push_back(faceVerts.size());
        }
        cur=e;
      }
    }
  }
  else if (fp.className=="faceList")
  {
    if (fp.binary)
      fp.error("binary faceList format is not supported");

    int64_t nFaces = fp.beginList();
    if (nFaces < maxFace)
      fp.error("face index out of range");

    int64_t cur=0;
    for (const auto& r: ranges)
    {
      fp.skipParenthesizedEntries(r.first-cur);
      for (int64_t fi=r.first; fi<r.second; ++fi)
      {
        int64_t n=fp.readLabel();
        fp.expect('(');
        for (int64_t j=0; j<n; ++j)
          faceVerts.push_back(fp.readLabel());
        fp.expect(')');
        faceOffsets.push_back(faceVerts.size());
      }
      cur=r.second;
    }
  }
  else
  {
    fp.error("unexpected class "+fp.className);
  }
}




/**
 * reads the coordinates of the given (sorted, unique) point labels
 */
std::vector<double> readPoints(
    const fs::path& polyMeshDir,
    const std::vector<int64_t>& ids )
{
  std::vector<double> pts;
  pts.reserve(3*ids.size());
  if (ids.empty()) return pts;

  auto content = loadFile(polyMeshDir/"points");
  FoamFileParser fp(content, (polyMeshDir/"points").string());

  int64_t nPts = fp.beginList();
  if (ids.front()<0 || ids.back()>=nPts)
    fp.error("point index out of range");

  if (fp.binary)
  {
    const char* block = fp.binaryBlock(3*nPts, fp.scalarSize);
    for (int64_t i: ids)
    {
      for (int k=0; k<3; ++k)
        pts.push_back(fp.binaryScalar(block, 3*i+k));
    }
  }
  else
  {
    int64_t cur=0;
    for (int64_t i: ids)
    {
      fp.skipParenthesizedEntries(i-cur);
      fp.expect('(');
      for (int k=0; k<3; ++k)
        pts.push_back(fp.readScalar());
      fp.expect(')');
      cur=i+1;
    }
  }

  return pts;
}


} // anonymous namespace




boost::filesystem::path polyMeshDirectory(
    const boost::filesystem::path& caseDir,
    const std::string& regionName,
    const std::string& time )
{
  if (regionName.empty())
    return caseDir / time / "polyMesh";
  else
    return caseDir / time / regionName / "polyMesh";
}




std::map<std::string, OFPolyMeshPatchInfo> readPolyMeshBoundary(
    const boost::filesystem::path& polyMeshDir )
{
  auto boundaryFile = polyMeshDir/"boundary";
  std::istringstream is(loadFile(boundaryFile));

  OFDictData::dict bd;
  if (!readOpenFOAMBoundaryDict(is, bd))
    throw insight::Exception(
        "Failed to parse boundary dict %s",
        boundaryFile.string().c_str() );

  std::map<std::string, OFPolyMeshPatchInfo> patches;
  for (const auto& e: bd)
  {
    if (const auto* pd = boost::get<OFDictData::dict>(&e.second))
    {
      OFPolyMeshPatchInfo pi;
      pi.type = pd->getString("type");
      pi.startFace = pd->getInt("startFace");
      pi.nFaces = pd->getInt("nFaces");
      patches[e.first]=pi;
    }
  }
  return patches;
}




OFPatchSurface readPolyMeshPatchSurface(
    const boost::filesystem::path& polyMeshDir,
    const std::vector<std::string>& patchNamePatterns )
{
  CurrentExceptionContext ex(
      "reading boundary patch surface from mesh in "+polyMeshDir.string() );

  auto patches = readPolyMeshBoundary(polyMeshDir);

  std::vector<boost::regex> res;
  for (const auto& pat: patchNamePatterns)
    res.push_back(boost::regex(pat));

  FaceRanges ranges;
  for (const auto& p: patches)
  {
    if (std::any_of(res.begin(), res.end(),
                    [&](const boost::regex& re) { return boost::regex_match(p.first, re); } ))
    {
      if (p.second.nFaces>0)
        ranges.push_back({p.second.startFace, p.second.startFace+p.second.nFaces});
    }
  }
  std::sort(ranges.begin(), ranges.end());

  OFPatchSurface surf;
  readFaces(polyMeshDir, ranges, surf.faceOffsets, surf.faceVerts);

  // collect used points and renumber
  std::vector<int64_t> ids(surf.faceVerts);
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

  for (auto& v: surf.faceVerts)
    v = std::lower_bound(ids.begin(), ids.end(), v) - ids.begin();

  surf.points = readPoints(polyMeshDir, ids);

  return surf;
}




OFPatchSurface readPatchSurface(
    const boost::filesystem::path& caseDir,
    const std::string& patchNamePattern,
    const std::string& regionName,
    const std::string& time )
{
  return readPolyMeshPatchSurface(
      polyMeshDirectory(caseDir, regionName, time),
      std::vector<std::string>{patchNamePattern} );
}




vtkSmartPointer<vtkPolyData> toVTKPolyData(const OFPatchSurface& surf)
{
  auto pts = vtkSmartPointer<vtkPoints>::New();
  pts->SetDataTypeToDouble();
  pts->SetNumberOfPoints(surf.nPoints());
  for (size_t i=0; i<surf.nPoints(); ++i)
    pts->SetPoint(i, &surf.points[3*i]);

  auto polys = vtkSmartPointer<vtkCellArray>::New();
  std::vector<vtkIdType> ids;
  for (size_t i=0; i<surf.nFaces(); ++i)
  {
    ids.assign(
        surf.faceVerts.begin()+surf.faceOffsets[i],
        surf.faceVerts.begin()+surf.faceOffsets[i+1] );
    polys->InsertNextCell(ids.size(), ids.data());
  }

  auto pd = vtkSmartPointer<vtkPolyData>::New();
  pd->SetPoints(pts);
  pd->SetPolys(polys);
  return pd;
}




vtkSmartPointer<vtkPolyData> readPatchAsVTKPolyData(
    const boost::filesystem::path& caseDir,
    const std::string& patchNamePattern,
    const std::string& regionName,
    const std::string& time )
{
  return toVTKPolyData(
      readPatchSurface(caseDir, patchNamePattern, regionName, time) );
}




double maxRadius(
    const OFPatchSurface& surf,
    const arma::mat& p0,
    const arma::mat& axis )
{
  double l = arma::norm(axis, 2);
  insight::assertion(l>0, "axis direction must not be zero");
  const double ex=axis(0)/l, ey=axis(1)/l, ez=axis(2)/l;
  const double x0=p0(0), y0=p0(1), z0=p0(2);

  double r2max=0;
  for (size_t i=0; i<surf.nPoints(); ++i)
  {
    const double* x = &surf.points[3*i];
    double dx=x[0]-x0, dy=x[1]-y0, dz=x[2]-z0;
    double da=dx*ex+dy*ey+dz*ez;
    r2max = std::max(r2max, dx*dx+dy*dy+dz*dz - da*da);
  }
  return std::sqrt(r2max);
}




double maxPatchRadius(
    const boost::filesystem::path& caseDir,
    const std::string& patchNamePattern,
    const arma::mat& p0,
    const arma::mat& axis,
    const std::string& regionName,
    const std::string& time )
{
  return maxRadius(
      readPatchSurface(caseDir, patchNamePattern, regionName, time),
      p0, axis );
}




std::pair<double,double> planeDistanceRange(
    const OFPatchSurface& surf,
    const arma::mat& p0,
    const arma::mat& normal )
{
  insight::assertion(surf.nPoints()>0, "plane distance range: surface contains no points");
  double l = arma::norm(normal, 2);
  insight::assertion(l>0, "plane normal must not be zero");
  const double nx=normal(0)/l, ny=normal(1)/l, nz=normal(2)/l;
  const double x0=p0(0), y0=p0(1), z0=p0(2);

  double dmin=std::numeric_limits<double>::max(), dmax=-dmin;
  for (size_t i=0; i<surf.nPoints(); ++i)
  {
    const double* x = &surf.points[3*i];
    double d = (x[0]-x0)*nx + (x[1]-y0)*ny + (x[2]-z0)*nz;
    dmin = std::min(dmin, d);
    dmax = std::max(dmax, d);
  }
  return {dmin, dmax};
}




std::pair<double,double> patchPlaneDistanceRange(
    const boost::filesystem::path& caseDir,
    const std::string& patchNamePattern,
    const arma::mat& p0,
    const arma::mat& normal,
    const std::string& regionName,
    const std::string& time )
{
  return planeDistanceRange(
      readPatchSurface(caseDir, patchNamePattern, regionName, time),
      p0, normal );
}




OFPatchFaceSizeStatistics faceSizeStatistics(const OFPatchSurface& surf)
{
  OFPatchFaceSizeStatistics st;
  st.nFaces = surf.nFaces();
  if (st.nFaces==0)
    throw insight::Exception("face size statistics: surface contains no faces!");

  std::vector<double> sizes(st.nFaces);
  for (size_t i=0; i<st.nFaces; ++i)
  {
    // area vector by fan triangulation from first vertex
    const double* x0 = &surf.points[3*surf.faceVerts[surf.faceOffsets[i]]];
    double ax=0, ay=0, az=0;
    for (int64_t j=surf.faceOffsets[i]+1; j+1<surf.faceOffsets[i+1]; ++j)
    {
      const double* x1 = &surf.points[3*surf.faceVerts[j]];
      const double* x2 = &surf.points[3*surf.faceVerts[j+1]];
      double ux=x1[0]-x0[0], uy=x1[1]-x0[1], uz=x1[2]-x0[2];
      double vx=x2[0]-x0[0], vy=x2[1]-x0[1], vz=x2[2]-x0[2];
      ax += uy*vz-uz*vy;
      ay += uz*vx-ux*vz;
      az += ux*vy-uy*vx;
    }
    sizes[i] = std::sqrt( 0.5*std::sqrt(ax*ax+ay*ay+az*az) );
  }

  auto mm = std::minmax_element(sizes.begin(), sizes.end());
  st.min = *mm.first;
  st.max = *mm.second;

  double sum=0;
  for (double s: sizes) sum+=s;
  st.mean = sum/double(st.nFaces);

  size_t m = st.nFaces/2;
  std::nth_element(sizes.begin(), sizes.begin()+m, sizes.end());
  st.median = sizes[m];
  if (st.nFaces%2==0)
  {
    // lower middle value is the largest in the first half
    st.median = 0.5*( st.median + *std::max_element(sizes.begin(), sizes.begin()+m) );
  }

  return st;
}




OFPatchFaceSizeStatistics patchFaceSizeStatistics(
    const boost::filesystem::path& caseDir,
    const std::string& patchNamePattern,
    const std::string& regionName,
    const std::string& time )
{
  return faceSizeStatistics(
      readPatchSurface(caseDir, patchNamePattern, regionName, time) );
}


}
