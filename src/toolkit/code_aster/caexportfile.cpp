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

#include "caexportfile.h"

#include "base/tools.h"
#include "boost/foreach.hpp"
#include <boost/iterator/iterator_concepts.hpp>

#include <fstream>


#if BOOST_VERSION>016500
#else
#include <boost/fusion/container/vector/limits.hpp>
#endif
#include "boost/spirit/include/qi.hpp"
#include "boost/variant/recursive_variant.hpp"
#include "boost/spirit/repository/include/qi_confix.hpp"
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/function/adapt_callable.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/spirit/home/classic/utility/distinct.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/repository/include/qi_iter_pos.hpp>
#include <boost/fusion/adapted.hpp>


namespace fs=boost::filesystem;

namespace insight
{

CAExportFile::FileInfo::FileInfo()
{}


CAExportFile::FileInfo::FileInfo(
  std::string ft,
  boost::filesystem::path fp )
  :fileType(ft), filePath(fp)
{}

CAExportFile::CAExportFile(const boost::filesystem::path& exportFile)
: boost::filesystem::path(exportFile),
  mem_max_(-1)
{
  std::ifstream in(exportFile.string());

  in >> std::noskipws; // ! keep newline,spaces etc !

  std::istream_iterator<char> inbeg(in);
  std::istream_iterator<char> inend;
  std::string contents_raw(inbeg, inend);

  std::string::iterator
      first=contents_raw.begin(),
      last=contents_raw.end();

  namespace qi = boost::spirit::qi;
  using namespace qi;
  namespace repo = boost::spirit::repository;
  namespace phx  = boost::phoenix;
  namespace bf = boost::fusion;


  qi::rule<std::string::iterator, std::string(), decltype(boost::spirit::ascii::space)>
      r_word = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ],
      r_string =  lexeme[ *(~char_(" ") - qi::eol) >> omit[-qi::eol] ]
      ;

  bool ok = phrase_parse
  (
     first, last,

     *(
          ('P' >> lit("mpi_nbcpu") >> int_[ phx::bind(&CAExportFile::np_mpi_, this)= qi::_1 ])
        | ('P' >> lit("ncpus") >> int_[ phx::bind(&CAExportFile::np_omp_, this)= qi::_1 ])
        | ('P' >> lit("version") >> r_word[ phx::bind(&CAExportFile::version_, this)= qi::_1 ])
        | ('P' >> lit("rep_trav") >> r_string[ phx::bind(&CAExportFile::workDir_, this)= qi::_1 ])
        | ('A' >> lit("tpmax") >> int_[ phx::bind(&CAExportFile::t_max_, this)= qi::_1 ])

        | ('F' >> lit("comm") >> r_string >> char_ >> int_ )
          [ phx::bind(&CAExportFile::commFile_, this) = qi::_1 ]

        | ('F' >> lit("mmed") >> r_string >> char_ >> int_ )
            [  phx::insert( phx::ref(inputFiles_),
                            phx::construct<FileList::value_type>( qi::_3, phx::construct<FileInfo>("mmed", qi::_1) ) ) ]

        | ('F' >> lit("rmed") >> r_string >> char_ >> int_ )
            [  phx::insert( phx::ref(outputFiles_),
                            phx::construct<FileList::value_type>( qi::_3, phx::construct<FileInfo>("rmed", qi::_1) ) ) ]

        | ('F' >> lit("mess") >> r_string >> char_ >> int_ )
            [  phx::insert( phx::ref(outputFiles_),
                            phx::construct<FileList::value_type>( qi::_3, phx::construct<FileInfo>("mess", qi::_1) ) ) ]

        | ( no_skip[ qi::as_string[ *(qi::char_- qi::eol) ] >> qi::eol ] )   // ignore else

      ),

      boost::spirit::ascii::space //skipper
  );

  if ( (!ok) || (first!=last) )
  {
    throw insight::Exception(
          "Could not parse the export file correctly!\n"
          "This part remained unparsed:\n"
          + std::string(first, last) );
  }
}




CAExportFile::CAExportFile(const boost::filesystem::path& commFile, std::string version, int t_max, int mem_max)
  : boost::filesystem::path(commFile.parent_path()/(commFile.filename().stem().string()+".export")),
    t_max_(t_max),
    mem_max_(mem_max),
    version_(version),
    commFile_(commFile)
{
  setMessFile(commFile.parent_path()/(commFile.filename().stem().string()+".mess.txt"));
  setRMedFile(commFile.parent_path()/(commFile.filename().stem().string()+".rmed"));
}




CAExportFile::~CAExportFile()
{
}




void CAExportFile::setNP(int np_omp, int np_mpi)
{
  np_omp_=np_omp;
  np_mpi_=np_mpi;
}




void CAExportFile::setMessFile(const boost::filesystem::path& fn)
{
  //messFile_.reset(new boost::filesystem::path(fn));
  outputFiles_[6] = FileInfo{MESS, fn};
}




void CAExportFile::setRMedFile(const boost::filesystem::path& fn)
{
  //rmedFile_.reset(new boost::filesystem::path(fn));
  outputFiles_[80] = FileInfo{RMED, fn};
}




boost::filesystem::path CAExportFile::RMedFile() const
{
  auto of = outputFiles_.find(80);
  if (of==outputFiles_.end())
    return boost::filesystem::path();
  else
    return of->second.filePath;
}




void CAExportFile::addMeshMedFile(const boost::filesystem::path& fn, int unit)
{
  if (unit<0)
  {
    std::set<int> mmedUnits;
    for (const auto& of: inputFiles_)
      if (of.second.fileType==MMED)
        mmedUnits.insert(of.first);

    if (mmedUnits.size()==0)
    {
      unit = 20;
    }
    else
    {
      unit = *mmedUnits.rbegin() +1;
    }
  }
  inputFiles_[unit] = FileInfo{MMED, fn};
}

std::set<boost::filesystem::path> CAExportFile::inputFiles(const std::string &fileType) const
{
  std::set<boost::filesystem::path> result;

  for (const auto& of: inputFiles_)
    if (of.second.fileType==fileType)
      result.insert(of.second.filePath);

  return result;
}


std::set<boost::filesystem::path> CAExportFile::outputFiles(const std::string &fileType) const
{
  std::set<boost::filesystem::path> result;

  for (const auto& of: outputFiles_)
    if (of.second.fileType==fileType)
      result.insert(of.second.filePath);

  return result;
}




const boost::filesystem::path &CAExportFile::commFilePath() const
{
  return commFile_;
}

const boost::filesystem::path &CAExportFile::workDir() const
{
  return workDir_;
}




void CAExportFile::writeFile(const fs::path& exportFileName) const
{
  fs::path fn=*this;
  if (!exportFileName.empty())
    fn=exportFileName;

  long int mem_max = mem_max_;
  if (mem_max<1)
  {
    MemoryInfo mi;
    mem_max = mi.memTotal_/1024/1024; // MB
  }

  std::ofstream f(fn.string());
  f
  <<"P actions make_etude\n"
  <<"P consbtc oui\n"
  <<"P corefilesize unlimited\n"
  <<"P cpresok RESNOOK\n"
  <<"P debug nodebug\n"
  <<"P soumbtc oui\n"
  <<"P facmtps 1\n"

  <<"P mode interactif\n"
  <<"P version " << version_ << "\n"
  <<"P mpi_nbcpu "<<np_mpi_<<"\n"
  <<"P mpi_nbnoeud 1\n"
  <<"P ncpus "<<np_omp_<<"\n"
  <<"A args\n"
  <<"A memjeveux " << (mem_max/8/np_mpi_) << "\n"
  <<"A tpmax " << t_max_ << "\n"
  <<"F comm " << commFile_.c_str() << " D  1\n";
  
  for (const FileList::value_type& item: inputFiles_)
  {
    f << "F " << item.second.fileType << " " << item.second.filePath.string() << " D  " << item.first << "\n";
  }

  for (const FileList::value_type& item: outputFiles_)
  {
    f << "F " << item.second.fileType << " " << item.second.filePath.string() << " R  " << item.first << "\n";
  }

}


}
