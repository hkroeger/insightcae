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
 */

#include "tools.h"
#include <fstream>
#include <cstdlib>
#include <dlfcn.h>

#include "base/boost_include.h"
#include "boost/asio.hpp"
#include "base/exception.h"
#include "base/cppextensions.h"
#include "base/externalprograms.h"

#include "vtkSTLReader.h"
#include "vtkSTLWriter.h"
#include "vtkPlane.h"
#include "vtkCutter.h"
#include "vtkSmartPointer.h"
#include "vtkClipPolyData.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkDataArray.h"

#include "boost/archive/iterators/base64_from_binary.hpp"
#include "boost/archive/iterators/binary_from_base64.hpp"
#include <boost/algorithm/string.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>

using namespace std;
using namespace boost;
using namespace boost::filesystem;
namespace fs = boost::filesystem;
using namespace boost::posix_time;


namespace insight
{


const std::string base64_padding[] = {"", "==","="};




std::string base64_encode(const std::string& s)
{
  insight::CurrentExceptionContext ex(
        boost::str(boost::format("performing base64 encode of buffer of size %d")
                   % s.size() )
        );

  namespace bai = boost::archive::iterators;

  std::stringstream os;

  // convert binary values to base64 characters
  typedef bai::base64_from_binary
  // retrieve 6 bit integers from a sequence of 8 bit bytes
  <bai::transform_width<const char *, 6, 8> > base64_enc; // compose all the above operations in to a new iterator

  std::copy(base64_enc(s.c_str()), base64_enc(s.c_str() + s.size()),
            std::ostream_iterator<char>(os));

  os << base64_padding[s.size() % 3];
  return os.str();
}




std::string
base64_encode(
    const boost::filesystem::path& f )
{
  std::string contents_raw;
  readFileIntoString(f, contents_raw);
  return base64_encode(contents_raw);
}





std::shared_ptr<std::string> base64_decode(const std::string& sourceBuffer)
{
    std::shared_ptr<std::string> targetBuffer;
    base64_decode(sourceBuffer, targetBuffer);
    return targetBuffer;
}


void
base64_decode(const char *src, size_t size,
    std::shared_ptr<std::string>& targetBuffer  )
{
//  char *src = a->value();
//  size_t size = a->value_size();

  if ((size>0) && src[size - 1] == '=')
  {
    --size;
    if ((size>0) && src[size - 1] == '=')
    {
       --size;
    }
  }

  if (size == 0)
  {
    if (targetBuffer) targetBuffer->clear();
  }
  else
  {
    using namespace boost::archive::iterators;

    typedef
      transform_width<
       binary_from_base64<
        remove_whitespace<
         const char*
        >
       >,
       8, 6
      >
      base64_dec;

      targetBuffer.reset(new std::string( base64_dec(src), base64_dec(src + size) ));
  }
}

void base64_decode(const std::string& sourceBuffer, std::shared_ptr<std::string>& targetBuffer)
{
    base64_decode(
                sourceBuffer.c_str(),
                sourceBuffer.size(),
                targetBuffer );
}




bool directoryIsWritable( const boost::filesystem::path& directoryToTest )
{
  boost::filesystem::path directory = directoryToTest;

  if (directory.empty())
    directory=".";

  insight::CurrentExceptionContext ex("checking write permissions of directory "+directory.string());

  auto testp = boost::filesystem::unique_path( directory/"%%%%%.test" );
  try
  {

    if (std::system( ("echo xx > \""+testp.string()+"\"").c_str() )!=0) return false;

//    std::ofstream f(testp.string());
//    if (!f.is_open()) return false;
//    f.close();

    if (!boost::filesystem::exists(testp)) return false;

    boost::filesystem::remove(testp);

    return true;
  }
  catch (...)
  {
    return false;
  }
}


bool isInWritableDirectory( const boost::filesystem::path& ptt )
{
    insight::CurrentExceptionContext ex("checking, if "+ptt.string()+" is in a writable location");

    namespace bf=boost::filesystem;

    if ( bf::exists( ptt ) )
    {
        if (bf::is_directory(ptt))
        {
            return directoryIsWritable(ptt);
        }
        else
        {
            return isInWritableDirectory(ptt.parent_path());
        }
    }
    else
    {
        auto pp=ptt.parent_path();
        if (!pp.empty())
        {
            return isInWritableDirectory(pp);
        }
        else
        {
            return false;
        }
    }

    throw insight::Exception("Internal error: unhandled case");
}



std::unique_ptr<GlobalTemporaryDirectory> GlobalTemporaryDirectory::td_;



std::string timeCodePrefix()
{
  ptime now = second_clock::universal_time();
  static std::locale loc(std::locale::classic(), //std::cout.getloc(),
                           new time_facet("%Y%m%d%H%M%S"));
  std::ostringstream ss;
  ss.imbue(loc);
  ss << now;

  return ss.str();
}


GlobalTemporaryDirectory::GlobalTemporaryDirectory()
  : boost::filesystem::path
    (
      absolute
      (
        unique_path
        (
          temp_directory_path() / "insightcae-%%%%%%"
        )
      )
    )
{
  create_directories(*this);
  permissions(*this, owner_all);
}

const GlobalTemporaryDirectory &GlobalTemporaryDirectory::path()
{
  if (!td_)
    td_.reset( new GlobalTemporaryDirectory );
  return *td_;
}

void GlobalTemporaryDirectory::clear()
{
  td_.reset();
}


GlobalTemporaryDirectory::~GlobalTemporaryDirectory()
{
  remove_all(*this);
}








TemporaryFile::TemporaryFile
(
    const std::string& fileNameModel,
    const boost::filesystem::path& baseDir
)
  : tempFilePath_( boost::filesystem::unique_path(
             (baseDir.empty() ? boost::filesystem::temp_directory_path() : baseDir)
             /
             fileNameModel
             ) )
{
    dbg() << tempFilePath_ << std::endl;
}


TemporaryFile::~TemporaryFile()
{
  if (stream_)
      stream_.reset();

  if (!getenv("INSIGHT_KEEPTEMPORARYFILES"))
  {
    if (fs::exists(tempFilePath_))
    {
        dbg()<<"removing file "<<tempFilePath_<<std::endl;
      fs::remove(tempFilePath_);
    }
  }
}

std::ostream& TemporaryFile::stream()
{
    if (!stream_)
        stream_=std::make_unique<std::ofstream>(path().string());
    return *stream_;
}

void TemporaryFile::closeStream()
{
    stream_.reset();
}


const boost::filesystem::path& TemporaryFile::path() const
{
  return tempFilePath_;
}




SSHCommand::SSHCommand(const std::string& hostName, const std::vector<std::string>& arguments)
  : hostName_(hostName), args_(arguments)
{
}

boost::filesystem::path SSHCommand::command() const
{
#if defined(WIN32)
    return ExternalPrograms::path("plink"); //boost::process::search_path("plink");
#else
    return ExternalPrograms::path("ssh"); //boost::process::search_path("ssh");
#endif
}

std::vector<std::string> SSHCommand::arguments() const
{
  std::vector<std::string> a(args_);
#if defined(WIN32)
  a.insert(a.begin(), { "-load", hostName_, "-no-antispoof", "-batch" });
#else
  a.insert(a.begin(), { hostName_ });
#endif
  return a;
}




RSYNCCommand::RSYNCCommand(const std::vector<std::string>& arguments)
  : args_(arguments)
{}

boost::filesystem::path RSYNCCommand::command() const
{
#if defined(WIN32)
    return boost::process::search_path("rsync.exe");
#else
    return boost::process::search_path("rsync");
#endif
}

std::vector<std::string> RSYNCCommand::arguments() const
{
  std::vector<std::string> a(args_);

#if defined(WIN32)
  auto cygnative = boost::process::search_path("cygnative.exe");
  auto plink = boost::process::search_path("plink.exe");
  a.insert(a.begin(),
           boost::str(boost::format("-e=\"%s %s -P %d\"")
                      % cygnative.string() % plink.string() % 22 )
  );
#else
  // keep args
#endif
  return a;
}




SharedPathList::SharedPathList()
{
  CurrentExceptionContext ec("building list of shared paths");

  
  if (char *var_usershareddir=getenv("INSIGHT_USERSHAREDDIR"))
  {
    push_back(var_usershareddir);
  }
  else
  {
    if (char *userdir = getenv(
#ifdef WIN32
                "USERPROFILE"
#else
                "HOME"
#endif
                ))
    {
      push_back( path(userdir)/".insight"/"share" );
    }
  }
  
  if (char *var_globalshareddir=getenv("INSIGHT_GLOBALSHAREDDIRS"))
  {
    std::vector<string> globals;
    split(globals, var_globalshareddir,
#ifdef WIN32
          is_any_of(";") // colon collides with drive letter in windows
#else
          is_any_of(":")
#endif
          );
    for (const string& s: globals) push_back(s);
  }
  else
  {
    push_back( path("/usr/share/insight") );
  }
}

SharedPathList::~SharedPathList()
{
}


path SharedPathList::getSharedFilePath(const path& file)
{
  BOOST_REVERSE_FOREACH( const path& p, *this)
  {
    if (exists(p/file)) 
      return p/file;
  }
  
  // nothing found
  throw insight::Exception(
        std::string("Requested shared file ")
         +file.string()
         +" not found either in global nor user shared directories"
        );
}

void SharedPathList::insertIfNotPresent(const path& spr)
{
  path sp = boost::filesystem::absolute(spr);
  if (std::find(begin(), end(), sp) == end())
  {
    std::cout<<"Extend search path: "<<sp.string()<<std::endl;
    push_back(sp);
  }
  else
  {
    std::cout<<"Already included in search path: "<<sp.string()<<std::endl;
  }
}

void SharedPathList::insertFileDirectoyIfNotPresent(const path& sp)
{
  if (boost::filesystem::is_directory(sp))
  {
    insertIfNotPresent(sp);
  }
  else
  {
    insertIfNotPresent(sp.parent_path());
  }
}

boost::filesystem::path SharedPathList::findFirstWritableLocation(
        const boost::filesystem::path &subPath) const
{
    insight::SharedPathList paths;
    for ( const bfs_path& p: paths )
    {
        insight::dbg()<<"checking, if "<<p.string()<<" is writable."<<std::endl;
        if ( insight::isInWritableDirectory(p) )
        {
            return p / subPath;
        }
    }
    return bfs_path();
}




SharedPathList SharedPathList::searchPathList;




ExecTimer::ExecTimer(const std::string& name)
: boost::timer::auto_cpu_timer(boost::timer::default_places, name+": END %ws wall, %us usr + %ss sys = %ts CPU (%p%)\n")
{
    std::cout<< ( name+": BEGIN\n" );
}


ExecTimer::~ExecTimer()
{}



void copyDirectoryRecursively(const path& sourceDir, const path& destinationDir)
{
    if (!exists(sourceDir) || !is_directory(sourceDir))
    {
        throw std::runtime_error("Source directory " + sourceDir.string() + " does not exist or is not a directory");
    }
    if (exists(destinationDir))
    {
        throw std::runtime_error("Destination directory " + destinationDir.string() + " already exists");
    }
    if (!create_directory(destinationDir))
    {
        throw std::runtime_error("Cannot create destination directory " + destinationDir.string());
    }

    for (const auto& dirEnt : boost::make_iterator_range(recursive_directory_iterator{sourceDir}, {}))
    {
        const auto& path = dirEnt.path();
        auto relativePathStr = path.string();
        boost::replace_first(relativePathStr, sourceDir.string(), "");
        copy(path, destinationDir / relativePathStr);
    }
}


void LineMesh_to_OrderedPointTable::calcConnectionInfo(vtkCellArray* lines)
{
    pointCells_.clear();
    cellPoints_.clear();
    endPoints_.clear();

    lines->InitTraversal();
    vtkIdType npts=-1;

#if (VTK_MAJOR_VERSION>=8) && (VTK_MINOR_VERSION>2)
    const
#endif
    vtkIdType *pt=nullptr;

    for (vtkIdType i=0; lines->GetNextCell(npts, pt); i++)
      {

        idList idl;

        for (vtkIdType j=0; j<npts; j++)
          {
            idl.push_back(pt[j]);
            pointCells_[pt[j]].push_back(i);
          }

        cellPoints_[i]=idl;
      }


    for (vtkIdType i=0; i<vtkIdType(pointCells_.size()); i++)
    {
        const idList& pc=pointCells_[i];
        if (pc.size()==1) endPoints_.insert(i);
    }

}


LineMesh_to_OrderedPointTable::LineMesh_to_OrderedPointTable(vtkPolyData* pd)
{
    vtkCellArray* lines = pd->GetLines();

    // find min element length
    double L=0.;
    int nL=0;
    {
        lines->InitTraversal();
        vtkIdType npts=-1;
#if (VTK_MAJOR_VERSION>=8) && (VTK_MINOR_VERSION>2)
        const
#endif
        vtkIdType *pt=nullptr;
        for (int i=0; lines->GetNextCell(npts, pt); i++)
          {
            if (npts==2)
            {
                double p1[3], p2[3];
                pd->GetPoint(pt[0], p1);
                pd->GetPoint(pt[1], p2);
                L+=sqrt( pow(p1[0]-p2[0],2) + pow(p1[1]-p2[1],2) + pow(p1[2]-p2[2],2) );
                nL++;
            }
          }
    }
    L/=double(nL);


    double tol=0.5*L;


    // Extract connection info
    //    std::cout<<"tol="<<tol<<std::endl;
    calcConnectionInfo(lines);
    printSummary(std::cout, pd);

    typedef std::map<vtkIdType,vtkIdType> AddLinesList;
    AddLinesList addLines;
    for (vtkIdType i: endPoints_)
    {
        double p1[3];
        pd->GetPoint(i, p1);

        double ldist=1e100;
        vtkIdType lj=-1;

        for (vtkIdType j: endPoints_)
        {
            if (i!=j)
            {
                double p2[3];
                pd->GetPoint(j, p2);

                double dist = sqrt( pow(p1[0]-p2[0],2) + pow(p1[1]-p2[1],2) + pow(p1[2]-p2[2],2) );

                if ( dist < tol )
                {
                    if (dist<ldist)
                    {
                        ldist=dist;
                        lj=j;
                    }
                }
            }
        }

        if (lj>=0)
        {
            vtkIdType li=i;
            if (li>lj) std::swap(li,lj);
            addLines[li]=lj;
//            std::cout<<"add line "<<li<<" => "<<lj<<std::endl;
        }
    }

    for (const AddLinesList::value_type& al: addLines)
    {
        vtkIdType eps[2];
        eps[0]=al.first;
        eps[1]=al.second;
        pd->InsertNextCell(VTK_LINE, 2, eps);
    }

    lines = pd->GetLines();
    calcConnectionInfo(lines);

    vtkIdType id_p0=0;
    if (endPoints_.size()>0)
        id_p0=*endPoints_.begin();

    std::set<vtkIdType> visitedCells;

    // ordered list of points (polyline)
    vtkIdType cid=id_p0;
    double xyz[3];

    pd->GetPoint(cid, xyz);
    pointIds_.push_back(cid);
    this->push_back(vec3(xyz[0], xyz[1], xyz[2]));

    do
    {
      idList pc = pointCells_[cid];
      if (visitedCells.find(pc[0]) == visitedCells.end())
        {
          const idList& pts = cellPoints_[pc[0]];
          if (pts[0]!=cid) cid=pts[0];
          else if (pts[1]!=cid) cid=pts[1];
          visitedCells.insert(pc[0]);
//          std::cout<<"visited a) "<<pc[0]<<std::endl;
        }
      else if (visitedCells.find(pc[1]) == visitedCells.end())
        {
          const idList& pts = cellPoints_[pc[1]];
          if (pts[0]!=cid) cid=pts[0];
          else if (pts[1]!=cid) cid=pts[1];
          visitedCells.insert(pc[1]);
//          std::cout<<"visited b) "<<pc[1]<<std::endl;
        }
      else
      {
//          std::cout<<"break"<<std::endl;
          break;
      }

      pd->GetPoint(cid, xyz);
      pointIds_.push_back(cid);
      this->push_back(vec3(xyz[0], xyz[1], xyz[2]));

    } while (visitedCells.size()<cellPoints_.size());
}

const std::vector<vtkIdType>& LineMesh_to_OrderedPointTable::pointIds() const
{
    return pointIds_;
}

arma::mat LineMesh_to_OrderedPointTable::extractOrderedData(vtkDataArray* data) const
{
    arma::mat result = arma::zeros( pointIds().size(), data->GetNumberOfComponents() );
    for(size_t i=0; i<pointIds().size(); ++i)
    {
        double pd[data->GetNumberOfComponents()];
        data->GetTuple( pointIds()[i], pd );
        for (int k=0;k<data->GetNumberOfComponents();++k)
            result(i, k)=pd[k];
    }
    return result;
}

void LineMesh_to_OrderedPointTable::printSummary(std::ostream& os, vtkPolyData* pd) const
{
    os<<"# points : "<<size()<<std::endl;
    os<<"# endpoints : "<<endPoints_.size()<<std::endl;
    for (vtkIdType i: endPoints_)
    {
        os<<"   "<<i;
        if (pd)
        {
            double p[3];
            pd->GetPoint(i, p);
            os<<" @ ("<<p[0]<<", "<<p[1]<<", "<<p[2]<<")";
        }
        os <<std::endl;
    }
    if (size()>=2)
    {
        os<<"first/last point : "<<std::endl;
        {
            const arma::mat& p = *begin();
            os<<" ("<<p(0)<<", "<<p(1)<<", "<<p(2)<<")";
        }
        os<<" ...\n";
        {
            const arma::mat& p = back();
            os<<" ("<<p(0)<<", "<<p(1)<<", "<<p(2)<<")";
        }
    }
    os<<"\n\n";
}

arma::mat LineMesh_to_OrderedPointTable::txyz() const
{
    arma::mat res = arma::zeros(size(), 4);
    double t=0;
    for (size_t i=0; i<size(); i++)
    {
        const arma::mat& p=(*this)[i];

        if (i>0)
        {
            t+=arma::norm( p-(*this)[i-1], 2 );
        }

        res(i,0)=t;
        res(i,1)=p(0);
        res(i,2)=p(1);
        res(i,3)=p(2);

    }
    return res;
}





arma::mat STLBndBox
(
  vtkSmartPointer<vtkPolyDataAlgorithm> in
)
{
  CurrentExceptionContext ec("Computing bounding box of VTK poly data set");

  in->Update();
  return PolyDataBndBox(in->GetOutput());
}

arma::mat PolyDataBndBox
(
  vtkSmartPointer<vtkPolyData> in
)
{
  double bb[6];
  in->GetBounds(bb);

  arma::mat bbm;
  bbm
    << bb[0] << bb[1] << arma::endr
    << bb[2] << bb[3] << arma::endr
    << bb[4] << bb[5] << arma::endr;

  return bbm;
}


arma::mat unitedBndBox(const arma::mat& bb1, const arma::mat& bb2)
{
  arma::mat bbm;
  bbm
    << std::min(bb1(0,0),bb2(0,0)) << std::max(bb1(0,1),bb2(0,1)) << arma::endr
    << std::min(bb1(1,0),bb2(1,0)) << std::max(bb1(1,1),bb2(1,1)) << arma::endr
    << std::min(bb1(2,0),bb2(2,0)) << std::max(bb1(2,1),bb2(2,1)) << arma::endr;

  std::cout<<bb1<<bb2<<bbm<<std::endl;
  return bbm;
}


void writeSTL
(
   vtkSmartPointer<vtkPolyDataAlgorithm> stl,
   const boost::filesystem::path& outfile
)
{
  CurrentExceptionContext ec("Writing STL mesh to file "+outfile.string());

  std::string file_ext = outfile.filename().extension().string();
  boost::to_lower(file_ext);

  vtkSmartPointer<vtkSTLWriter> sw = vtkSmartPointer<vtkSTLWriter>::New();
  sw->SetInputConnection(stl->GetOutputPort());
  sw->SetFileName(outfile.string().c_str());
  if (file_ext==".stlb")
  {
    sw->SetFileTypeToBinary();
  }
  else
  {
    sw->SetFileTypeToASCII();
  }
  sw->Update();
}


std::string collectIntoSingleCommand( const std::string& cmd, const std::vector<std::string>& args )
{
  std::string res = cmd;
  for (const auto& arg: args)
  {
    res += " \""+arg+"\""; // leading space!
  }
  return res;
}

string escapeShellSymbols(const string &expr)
{
  string res(expr);
  algorithm::replace_all(res, "\\", "\\\\");
  algorithm::replace_all(res, "$", "\\$");
  algorithm::replace_all(res, "\"", "\\\"");
  return res;
}


int findFreePort()
{
  using namespace boost::asio;
  using ip::tcp;

  io_service svc;
  tcp::acceptor a(svc);

  boost::system::error_code ec;
  a.open(tcp::v4(), ec) || a.bind({ tcp::v4(), 0 }, ec);

  if (ec == error::address_in_use)
  {
    throw insight::Exception("Could not find a free TCP/IP port on the local machine!");
  }
  else
  {
   return  a.local_endpoint().port(); //.address().to_string();
  }
}


int findRemoteFreePort(const std::string& SSHHostName)
{
  CurrentExceptionContext ce("determining free port on "+SSHHostName);

  boost::process::ipstream out;

  SSHCommand sc(SSHHostName, {"bash", "-lc", "isPVFindPort.sh"});
  int ret = boost::process::system(
        sc.command(), boost::process::args(sc.arguments()),
        boost::process::std_out > out,
        boost::process::std_in < boost::process::null
        );

  if (ret!=0)
  {
    throw insight::Exception(
          str( format("Failed to query host %s for free network port!") % SSHHostName)
          );
  }

  std::string outline;
  getline(out, outline);
  std::vector<std::string> parts;
  boost::split(parts, outline, is_any_of(" "));
  if (parts.size()==2)
  {
    if (parts[0]=="PORT")
      return toNumber<int>(parts[1]);
  }

  throw insight::Exception("unexpected answer: \""+outline+"\"");
}


void readStreamIntoString(istream &in, string &fileContent)
{
  in.seekg(0, std::ios::end);
  fileContent.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&fileContent[0], fileContent.size());
}

void readFileIntoString(const path &fileName, string &fileContent)
{
  std::ifstream in(fileName.string(), std::ios::in | std::ios::binary);
  if (!in)
  {
    throw insight::Exception("Could not open file "+fileName.string()+"!");
  }
  readStreamIntoString(in, fileContent);
}


void writeStringIntoFile
(
    const std::string& fileContent,
    const boost::filesystem::path& filePath
)
{
    std::ofstream file( filePath.c_str(), std::ios::out | std::ios::binary);
    if (file.good())
    {
        file.write(fileContent.c_str(), long(fileContent.size()) );
        file.close();
    }
    else
    {
      throw insight::Exception("could not write to file "+filePath.string());
    }
}


void writeStringIntoFile
(
    std::shared_ptr<std::string> fileContent,
    const boost::filesystem::path& fileName
)
{
   writeStringIntoFile(*fileContent, fileName);
}


TemplateFile::TemplateFile(const string &hardCodedTemplate)
  : std::string(hardCodedTemplate)
{}

TemplateFile::TemplateFile(std::istream &in)
{
  readStreamIntoString(in, *this);
}

TemplateFile::TemplateFile(const boost::filesystem::path& in)
{
  readFileIntoString(in, *this);
}

void TemplateFile::replace(const string &keyword, const string &content)
{
  boost::replace_all ( *this, "###"+keyword+"###", content );
}

void TemplateFile::write(ostream &os) const
{
  os.write(this->c_str(), long(this->size()) );
}

void TemplateFile::write(const path &outfile) const
{
  std::ofstream f(outfile.string(), ios::binary);
  write(f);
}

MemoryInfo::MemoryInfo()
{
  std::map<string, pair<long long, string> > entries;

  std::ifstream f("/proc/meminfo");
  string line;
  while (getline(f, line))
  {
    std::vector<string> s;
    boost::split(s, line, boost::is_any_of("\t "), boost::token_compress_on);

    entries[s[0]]=pair<int, string>( lexical_cast<long long>(s[1]), s.size()>2?s[2]:"" );
  }

  memTotal_=entries["MemTotal:"].first * 1024;
  memFree_=entries["MemFree:"].first * 1024;
}




RSyncProgressAnalyzer::RSyncProgressAnalyzer()
{}




void RSyncProgressAnalyzer::runAndParse(
    boost::process::child& rsyncProcess,
    std::function<void(int,const std::string&)> pf )
{
  if (!rsyncProcess.running())
  {
    throw insight::Exception("could not start rsync process!");
  }

  std::string line;
  boost::regex pattern(".* ([^ ]*)% *([^ ]*) *([^ ]*) \\(xfr#([0-9]+), to-chk=([0-9]+)/([0-9]+)\\)");
  while (rsyncProcess.running() && std::getline(*this, line) && !line.empty())
  {
    insight::dbg()<<line<<std::endl;
    boost::smatch match;
    if (boost::regex_search( line, match, pattern, boost::match_default ))
    {
      std::string percent=match[1];
      std::string rate=match[2];
      std::string eta=match[3];
//        int i_file=to_number<int>(match[4]);
      int i_to_chk=toNumber<int>(match[5]);
      int total_to_chk=toNumber<int>(match[6]);

      double progress = total_to_chk==0? 1.0 : double(total_to_chk-i_to_chk) / double(total_to_chk);

      if (pf) pf(int(100.*progress), rate+", "+eta+" (current file: "+percent+")");
    }
  }

  rsyncProcess.wait();
}




bool isNumber(const string &s)
{
  try {
    auto result = boost::lexical_cast<double>(s);
    return true;
  }
  catch (const boost::bad_lexical_cast&)
  {
    return false;
    }
}




path ensureFileExtension(const boost::filesystem::path &filePath, const std::string &extension)
{
    if (filePath.extension()!=extension)
        return path(filePath).replace_extension(extension);
    else
        return filePath;
}

arma::mat computeOffsetContour(const arma::mat &pl, double thickness, const arma::mat &normals)
{
    arma::mat pl2 = arma::zeros(pl.n_rows, pl.n_cols); //not existing in older armadillo: arma::reverse(pl, 0);
    for (arma::uword i=0; i<pl.n_rows; ++i)
        pl2.row(i)=pl.row(pl.n_rows-1-i);

    arma::mat lp;
    for (arma::uword i=0; i<pl2.n_rows; ++i)
    {
        auto p=pl2.row(i);
        auto n=normals.row(i);

        arma::mat t;
        if (i>0)
        {
            t = p-lp;
        }
        else
        {
            t = pl2.row(i+1)-p;
        }
        lp = p;
        t/=arma::norm(t,2);

        arma::mat th = arma::cross(n, t);
        std::cout<<p<<n<<t<<th<<std::endl;
        th /= arma::norm(th,2);
        pl2.row(i) += th * thickness;
    }
    return arma::join_vert(pl, pl2);
}




std::string getMandatoryAttribute(rapidxml::xml_node<> &node, const std::string& attributeName)
{
    if ( auto *fn = node.first_attribute(attributeName.c_str()) )
      return std::string(fn->value());
    else
      throw insight::Exception("node does not have mandatory attribute \""+attributeName+"\"!");
}


std::shared_ptr<std::string> getOptionalAttribute(rapidxml::xml_node<> &node, const std::string& attributeName)
{
    if ( auto *fn = node.first_attribute(attributeName.c_str()) )
      return std::make_shared<std::string>(fn->value());
    else
      return std::shared_ptr<std::string>();
}

}
