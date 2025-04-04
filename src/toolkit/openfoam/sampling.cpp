#include "sampling.h"

#include "openfoam/openfoamtools.h"


namespace insight {


namespace sampleOps
{




set::set(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{}

set::~set()
{}





line::line(ParameterSetInput ip)
: set(ip.forward<Parameters>())
{
}

void line::addIntoDictionary(const OpenFOAMCase& /*ofc*/, OFDictData::dict& sampleDict) const
{
  OFDictData::list& l=sampleDict.getList("sets");

  OFDictData::dict sd;
  sd["type"]="consistentCurve";
  sd["axis"]="distance";

  OFDictData::list pl;
  for (arma::uword i=0; i<p().points.n_rows; i++)
    pl.push_back(OFDictData::vector3(p().points.row(i).t()));
  sd["points"]=pl;

  l.push_back(p().name);
  l.push_back(sd);
}

set* line::clone() const
{
  return new line(p_.clone());
}




arma::mat line::readSamples
(
  const OpenFOAMCase& ofc, const boost::filesystem::path& location,
  ColumnDescription* coldescr,
  const std::string& time
) const
{
  arma::mat data;

  boost::filesystem::path fp;
  if (ofc.OFversion()<170)
  {
    fp=absolute(location)/"sets";
  }
  else if (ofc.OFversion()>=400)
  {
    fp=absolute(location)/"postProcessing"/"sampleDict";
  }
  else
  {
    fp=absolute(location)/"postProcessing"/"sets";
  }

  TimeDirectoryList tdl=listTimeDirectories(fp);

  boost::filesystem::path timedir=tdl.rbegin()->second;
  if (!time.empty())
  {
    for (TimeDirectoryList::value_type& tde: tdl)
    {
      if (tde.second.filename().string()==time)
      {
    timedir=tde.second;
    break;
      }
    }
  }

  {
    arma::mat m;
    std::vector<std::string> files;

    boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
    for ( boost::filesystem::directory_iterator itr( timedir );
      itr != end_itr;
      ++itr )
    {
      if ( is_regular_file(itr->status()) )
      {
    std::string fn=itr->path().filename().string();
    if (boost::starts_with(fn, p().name+"_")) files.push_back(fn);
      }
    }

    sort(files.begin(), files.end());

    // join contents of all files in one time directory by column
    // provide the mapping field<>column in coldescr
    for (size_t i=0; i<files.size(); i++)
    {
      std::string fn=files[i];
      boost::filesystem::path fp=timedir/fn;

      boost::erase_tail(fn, 3); // remove file extension
      // split into field name => flnames
      std::vector<std::string> flnames;
      boost::split(flnames, fn, boost::is_any_of("_"));
      flnames.erase(flnames.begin());

      // read data of file
      arma::mat res;
      res.load(fp.string(), arma::raw_ascii);

      int start_col=1;
      if (m.n_cols==0)
      {
    // first file: init m
    m=res;
      }
      else
      {
    // another one: join cols
    start_col=m.n_cols;
    m=arma::join_rows(m, res.cols(1, res.n_cols-1)); // skip the coordinate column
      }

      int ncmpt=(res.n_cols-1)/flnames.size(); // retrieve number of cols per field
      for (size_t j=0; j<flnames.size(); j++)
      {
    if (coldescr)
    {
      (*coldescr)[flnames[j]].col=start_col+ncmpt*j;
      (*coldescr)[flnames[j]].ncmpt=ncmpt;
// 	  std::cout<<flnames[j]<<" "<<start_col<<" "<<ncmpt*j<<" "<<ncmpt<<std::endl;
    }
      }
    }

    // originally it was intended to join by row all time directories:
    if (data.n_rows==0)
      data=m;
    else
      data=join_cols(data, m);
  }


  // interpolate missing point in dataset => rdata
  arma::mat rdata;
  if (data.n_cols>0)
  {

    // compute expected length coordinates from prescribed sampling points => coords
    std::vector<double> d;
    d.push_back(0.0);
    for (arma::uword k=1; k<p().points.n_rows; k++)
    {
      d.push_back( d[k-1] + norm( p().points.row(k) - p().points.row(k-1), 2) );
    }
    arma::mat coords=arma::mat(d.data(), d.size(), 1);

//     std::cout<<"data="<<data<<" coords="<<coords<<std::endl;

    // interpolate data to expected coordinates (coords) for curve-like sampledSets or to expected indices for e.g. cloud
    arma::mat idata=Interpolator(data)
    (
//       arma::linspace(0, p().points().n_rows-1, p().points().n_rows)  // cloud
      coords				// curve-like
    );
//     std::cout<<" idata="<<idata<<endl;

    // combine expected coords with interpolated data
    rdata=join_rows( coords, idata );
  }

//   return data;
  return rdata;

}


uniformLine::uniformLine(ParameterSetInput ip)
: set(ip.forward<Parameters>()),
  l_
  (
        line::Parameters()
          .set_points( arma::linspace(0,1.,p().np)*(p().end-p().start).t() + arma::ones(p().np,1)*p().start.t() )
          .set_name(p().set::Parameters::name
    )
  )
{}

void uniformLine::addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const
{
  l_.addIntoDictionary(ofc, sampleDict);
}

set* uniformLine::clone() const
{
  return new uniformLine(p_.clone());
}

arma::mat uniformLine::readSamples(
    const OpenFOAMCase& ofc,
    const boost::filesystem::path& location,
    ColumnDescription* coldescr,
    const std::string& time ) const
{
  return l_.readSamples(ofc, location, coldescr, time);
}




circumferentialAveragedUniformLine::circumferentialAveragedUniformLine(ParameterSetInput ip)
: set(ip.forward<Parameters>())
{
  dir_=p().end-p().start;
  L_=norm(dir_,2);
  dir_/=L_;
  x_=arma::linspace(0, L_, p().np);
  for (int i=0; i<p().nc; i++)
  {
    arma::mat raddir = rotMatrix(i) * dir_;
    arma::mat pts=x_ * raddir.t() + arma::ones(p().np,1)*(rotMatrix(i)*p().start).t();

    lines_.push_back(new line( line::Parameters().set_points(pts).set_name(setname(i)) ));
  }

  if (p().name.find('_') != std::string::npos)
  {
    throw insight::Exception("circumferentialAveragedUniformLine: set name must not contains underscores (_)!");
  }
}

void circumferentialAveragedUniformLine::addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const
{
  for (const line& l: lines_)
  {
    l.addIntoDictionary(ofc, sampleDict);
  }
}

set* circumferentialAveragedUniformLine::clone() const
{
  return new circumferentialAveragedUniformLine(p_.clone());
}

arma::mat circumferentialAveragedUniformLine::rotMatrix(int i, double angularOffset) const
{
  return insight::rotMatrix( angularOffset + p().angle*(double(i)+0.5)/double(p().nc), p().axis);
}

arma::mat circumferentialAveragedUniformLine::readSamples
(
  const OpenFOAMCase& ofc, const boost::filesystem::path& location,
  ColumnDescription* coldescr,
  const std::string& time
) const
{
  arma::mat data;
  ColumnDescription cd;
  int i=0, num_valid=0;
  bool cd_set=false;
  for (const line& l: lines_)
  {
    arma::mat datai = l.readSamples(ofc, location, &cd, time);
    arma::mat Ri=rotMatrix(i++, p().angularOffset).t();

    if (datai.n_cols>1)
    {
      if (!cd_set)
      {
    std::ofstream f( (p().name+"_circularinstance_colheader.txt").c_str() );
    f<<"#";
    for (const ColumnDescription::value_type& fn: cd)
    {
      ColumnInfo ci=fn.second;
      if (ci.ncmpt==0)
      {
        f<<" "+fn.first;
      }
      else
      {
            for (size_t c=0; c<ci.ncmpt; c++)
          f<<" "+fn.first+"_"+boost::lexical_cast<std::string>(c);
      }
      //cout<<fn.first<<": c="<<ci.col<<" ncmpt="<<ci.ncmpt<<endl;
    }
    f<<endl;

    if (coldescr) *coldescr=cd;

    cd_set=true;
      }

      num_valid++;

      for (const ColumnDescription::value_type& fn: cd)
      {
    ColumnInfo ci=fn.second;
  //       cout<<fn.first<<": c="<<ci.col<<" ncmpt="<<ci.ncmpt<<endl;
    if (ci.ncmpt==1)
    {
      // scalar: no transform needed
    }
    else if (ci.ncmpt==3)
    {
      datai.cols(ci.col, ci.col+2) = (Ri * datai.cols(ci.col, ci.col+2).t()).t();
    }
    else if (ci.ncmpt==6)
    {
      // symmetric tensor
      int c0=ci.col;
          for (arma::uword j=0; j<datai.n_rows; j++)
      {
        arma::mat t;
        t << datai(j,c0)   << datai(j,c0+1) << datai(j,c0+2) << arma::endr
          << datai(j,c0+1) << datai(j,c0+3) << datai(j,c0+4) << arma::endr
          << datai(j,c0+2) << datai(j,c0+4) << datai(j,c0+5) << arma::endr;

        t = Ri * t * Ri.t();
        double symm=fabs(t(1,0)-t(0,1))+fabs(t(0,2)-t(2,0))+fabs(t(1,2)-t(2,1));
        if (symm>1e-6) cout<<"Warning: unsymmetric tensor after rotation: "<<endl<<t<<endl;

        datai(j,c0)   = t(0,0);
        datai(j,c0+1) = t(0,1);
        datai(j,c0+2) = t(0,2);
        datai(j,c0+3) = t(1,1);
        datai(j,c0+4) = t(1,2);
        datai(j,c0+5) = t(2,2);
      }
    }
    else
    {
      throw insight::Exception("circumferentialAveragedUniformLine::readSamples: encountered quantity (name "
        +fn.first+", col="+boost::lexical_cast<std::string>(ci.col)+") with "
          +boost::lexical_cast<std::string>(ci.ncmpt)+" components. Don't know how to handle.");
    }
      }

      //datai.save(p().name()+"_circularinstance_i"+lexical_cast<string>(i)+".txt", arma::raw_ascii);

      if (data.n_cols==0)
    data=datai;
      else
    data+=datai;
    }

  }

  if (num_valid==0)
  {
    insight::Exception("There were vo valid profiles for circular homogeneous averaging!");
  }

  return data / double(num_valid);

}



linearAveragedPolyLine::linearAveragedPolyLine(ParameterSetInput ip)
: set(ip.forward<Parameters>())
{

  arma::mat
    dx=p().points.col(0) - p().points(0,0),
    dy=p().points.col(1) - p().points(0,1),
    dz=p().points.col(2) - p().points(0,2);

  x_ = sqrt( pow(dx,2) + pow(dy,2) + pow(dz,2) );

  for (int i=0; i<p().nd1; i++)
    for (int j=0; j<p().nd2; j++)
    {
      arma::mat ofs = p().dir1*(double(i)/double(std::max(1,p().nd1-1))) + p().dir2*(double(j)/double(std::max(1,p().nd2-1)));
      arma::mat tp =
    join_rows(join_rows(
        p().points.col(0)+ofs(0),
        p().points.col(1)+ofs(1) ),
        p().points.col(2)+ofs(2)
    );
      lines_.push_back(new line(line::Parameters()
    .set_points( tp )
    .set_name(setname(i,j))
      ));
    }

  if (p().set::Parameters::name.find('_') != std::string::npos)
  {
    throw insight::Exception("linearAveragedPolyLine: set name must not contains underscores (_)!");
  }
}

void linearAveragedPolyLine::addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const
{
//   OFDictData::list& l=sampleDict.getList("sets");

  for (const line& l: lines_)
  {
    l.addIntoDictionary(ofc, sampleDict);
  }

}

set* linearAveragedPolyLine::clone() const
{
  return new linearAveragedPolyLine(p_.clone());
}


arma::mat linearAveragedPolyLine::readSamples
(
  const OpenFOAMCase& ofc, const boost::filesystem::path& location,
  ColumnDescription* coldescr,
  const std::string& time
) const
{
  arma::mat data; // only the data, without coordinate column!

  ColumnDescription cd;
  int valid_lines=0;
  for (const line& l: lines_)
  {
    arma::mat ds=l.readSamples(ofc, location, &cd, time);
    if (ds.n_rows>0)
    {
      arma::mat datai = Interpolator(ds)(x_);

//       ds.save( str(format("polyline_%d.dat")%valid_lines), arma::raw_ascii);

      if (data.n_cols==0)
    data=datai;
      else
    data+=datai;

      valid_lines++;

      if (coldescr) *coldescr=cd;
    }
  }

  if (valid_lines==0)
  {
    throw insight::Exception("Not a single valid dataset found!");
  }
  else
  {
    if ( valid_lines != (p().nd1*p().nd2) )
    {
      insight::Warning
      (
    str(boost::format("linearAveragedPolyLine: Only %d out of %d dataset for averaging contained valid data!")
          % valid_lines % (p().nd1*p().nd2) )
      );
    }
  }


  return arma::mat(join_rows(x_, data / double(valid_lines)));

}



linearAveragedUniformLine::linearAveragedUniformLine(ParameterSetInput ip)
: set(ip.forward<Parameters>()),
  pl_
  (
    linearAveragedPolyLine::Parameters()
    .set_points( arma::linspace(0.0, 1.0, p().np) * (p().end-p().start).t()
      + arma::ones(p().np,1)*p().start.t()
     )
    .set_dir1(p().dir1)
    .set_dir2(p().dir2)
    .set_nd1(p().nd1)
    .set_nd2(p().nd2)
    .set_name(p().name)
  )
{
  if (p().set::Parameters::name.find('_') != std::string::npos)
  {
    throw insight::Exception("linearAveragedUniformLine: set name must not contains underscores (_)!");
  }
}

void linearAveragedUniformLine::addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const
{
  pl_.addIntoDictionary(ofc, sampleDict);
}

set* linearAveragedUniformLine::clone() const
{
  return new linearAveragedUniformLine(p_.clone());
}


arma::mat linearAveragedUniformLine::readSamples
(
  const OpenFOAMCase& ofc, const boost::filesystem::path& location,
  ColumnDescription* coldescr,
  const std::string& time
) const
{
  return pl_.readSamples(ofc, location, coldescr, time);
}


}

void sample(const OpenFOAMCase& ofc,
        const boost::filesystem::path& location,
        const std::vector<std::string>& fields,
        const boost::ptr_vector<sampleOps::set>& sets,
        std::vector<std::string> addopts
        )
{
  using namespace sampleOps;

  OFDictData::dictFile sampleDict;

  sampleDict["setFormat"] = "raw";
  sampleDict["surfaceFormat"] = "vtk";
  sampleDict["interpolationScheme"] = "cellPoint";
  sampleDict["formatOptions"] = OFDictData::dict();

  OFDictData::list flds; flds.resize(fields.size());
  copy(fields.begin(), fields.end(), flds.begin());
  sampleDict["fields"] = flds;

  sampleDict["sets"] = OFDictData::list();
  sampleDict["surfaces"] = OFDictData::list();

  for ( const set& s: sets)
  {
    s.addIntoDictionary(ofc, sampleDict);
  }

  if (ofc.OFversion()>=400)
  {
   sampleDict["type"]="sets";
   OFDictData::list libs;
   libs.push_back("\"libsampling.so\"");
   sampleDict["libs"]=libs;

   addopts.insert(addopts.begin(), "sampleDict");
   addopts.insert(addopts.begin(), "-func");
  }

  // then write to file
  sampleDict.write( location / "system" / "sampleDict" );

//   std::vector<std::string> opts;
//   opts.push_back("-latestTime");
  //if (overwrite) opts.push_back("-overwrite");

  if (ofc.OFversion()>=400)
  {
   ofc.executeCommand(location, "postProcess", addopts);
  }
  else
  {
   ofc.executeCommand(location, "sample", addopts);
  }

}



} // namespace insight
