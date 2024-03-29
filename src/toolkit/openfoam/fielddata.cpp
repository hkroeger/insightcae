
#include "openfoam/fielddata.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/openfoamcase.h"

#include "base/boost_include.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;

namespace insight
{




void FieldData::calcValues()
{
    representativeValueMag_ = calcRepresentativeValueMag();
    maxValueMag_ = calcMaxValueMag();
}




FieldData::Parameters FieldData::uniformSteady(double uniformSteadyValue)
{
  Parameters::fielddata_uniformSteady_type data;
  data.value=arma::ones(1)*uniformSteadyValue;
  Parameters p;
  p.fielddata=data;
  return p;
}




FieldData::FieldData(double uniformSteadyValue)
: p_(Parameters::makeDefault())
{
//   Parameters::fielddata_uniform_type data;
//   data.values.resize(1);
//   data.values[0].time=0;
//   data.values[0].value=arma::ones(1)*uniformSteadyValue;
//   p_.fielddata=data;
    p_=uniformSteady(uniformSteadyValue);
    calcValues();
}




FieldData::Parameters FieldData::uniformSteady(double uniformSteadyX, double uniformSteadyY, double uniformSteadyZ)
{
    return FieldData::uniformSteady(vec3(uniformSteadyX, uniformSteadyY, uniformSteadyZ));
}




FieldData::Parameters FieldData::uniformSteady(const arma::mat& uniformSteadyValue)
{
  Parameters::fielddata_uniformSteady_type data;
  data.value=uniformSteadyValue;
  Parameters p;
  p.fielddata=data;
  return p;
}




FieldData::FieldData(const arma::mat& uniformSteadyValue)
: p_(Parameters::makeDefault())
{
//   Parameters::fielddata_uniform_type data;
//   data.values.resize(1);
//   data.values[0].time=0;
//   data.values[0].value=uniformSteadyValue;
//   p_.fielddata=data;
    p_=uniformSteady(uniformSteadyValue);
    calcValues();
}




FieldData::FieldData(const ParameterSet& p)
: p_(p)
{
    calcValues();
}




OFDictData::data FieldData::sourceEntry(OFdicts& dictionaries) const
{
    std::ostringstream os;

    if (const Parameters::fielddata_uniformSteady_type *fd =
        boost::get<Parameters::fielddata_uniformSteady_type>(&p_.fielddata) ) //(type=="uniform")
    {
        os <<" uniform unsteady 0.0 " << OFDictData::to_OF(fd->value);
    }

    else if (const Parameters::fielddata_uniform_type *fd = boost::get<Parameters::fielddata_uniform_type>(&p_.fielddata) ) //(type=="uniform")
    {
        os<<" uniform unsteady";

        for (const Parameters::fielddata_uniform_type::values_default_type& inst: fd->values)
        {
            os << " " << inst.time << " " << OFDictData::to_OF(inst.value);
        }
    }

    else if
    (const Parameters::fielddata_linearProfile_type *fd = boost::get<Parameters::fielddata_linearProfile_type>(&p_.fielddata) )
    {
        os<<" linearProfile "
          <<OFDictData::to_OF(fd->p0)
          <<" "
          <<OFDictData::to_OF(fd->ep)
          ;

        os<<" "
          <<"unsteady";

        for (const Parameters::fielddata_linearProfile_type::values_default_type& inst: fd->values)
        {
            os << " " << inst.time << " " << dictionaries.insertAdditionalInputFile(inst.profile);
        }
    }

    else if
    (const Parameters::fielddata_radialProfile_type *fd = boost::get<Parameters::fielddata_radialProfile_type>(&p_.fielddata) )
    {
        os<<" radialProfile "
          <<OFDictData::to_OF(fd->p0)
          <<" "
          <<OFDictData::to_OF(fd->ep)
          ;

        os<<" "
          <<"unsteady";

        for (const Parameters::fielddata_radialProfile_type::values_default_type& inst: fd->values)
        {
            os << " " << inst.time << " " << dictionaries.insertAdditionalInputFile(inst.profile);
        }
    }

    else if
    (const Parameters::fielddata_fittedProfile_type *fd = boost::get<Parameters::fielddata_fittedProfile_type>(&p_.fielddata) )
    {
        os<<" fittedProfile "
          <<OFDictData::to_OF(fd->p0)
          <<" "
          <<OFDictData::to_OF(fd->ep)
          <<" "
          <<"unsteady";

        for (const Parameters::fielddata_fittedProfile_type::values_default_type& inst: fd->values)
        {
            os << " " << inst.time;

            for
            (
                const Parameters::fielddata_fittedProfile_type::values_default_type::component_coeffs_default_type& coeffs:
                inst.component_coeffs
            )
            {
                os << " [";
                for (size_t cc=0; cc<coeffs.n_elem; cc++)
                    os<<" "<< str( format("%g") % coeffs[cc] );
                os<<" ]";
            }
        }
    }
    else
    {
        throw insight::Exception("Unknown field data description type!");
    }

    return os.str();
}




void FieldData::setDirichletBC(OFDictData::dict& BC, OFdicts& dictionaries) const
{
  if (const Parameters::fielddata_uniformSteady_type *fd = boost::get<Parameters::fielddata_uniformSteady_type>(&p_.fielddata) )
  {
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]="uniform " + OFDictData::to_OF(fd->value);
  }
  else
  {
    BC["type"]=OFDictData::data("extendedFixedValue");
    BC["source"]=sourceEntry(dictionaries);

    auto& controlDict = dictionaries.lookupDict("system/controlDict");
    controlDict.getList("libs").insertNoDuplicate("\"libextendedFixedValueBC.so\"");
  }
}


//boost::filesystem::path completed_path(const boost::filesystem::path& basepath, const insight::PathParameter* filename)
//{
//    if (filename.is_absolute())
//        return filename->filePath(basepath);
//    else
//        return basepath/filename->filePath(basepath);
//}

double FieldData::calcRepresentativeValueMag() const
{
  if (const auto *fd = boost::get<Parameters::fielddata_uniformSteady_type>(&p_.fielddata) )
  {
    return norm(fd->value, 2);
  }
  else if (const auto *fd = boost::get<Parameters::fielddata_uniform_type>(&p_.fielddata) )
  {
    double meanv=0.0;
    int s=0;
    for (const Parameters::fielddata_uniform_type::values_default_type& inst: fd->values)
    {
      meanv+=pow(norm(inst.value, 2), 2);
      s++;
    }
    if (s==0)
      throw insight::Exception("Invalid data: no time instants prescribed!");
    meanv/=double(s);
    return sqrt(meanv);
  }
  else if (const auto *fd = boost::get<Parameters::fielddata_linearProfile_type>(&p_.fielddata) )
  {
    double avg=0.0;
    int s=0;
    for (const Parameters::fielddata_linearProfile_type::values_default_type& inst: fd->values)
    {
      arma::mat xy;
      xy.load( inst.profile->stream(), arma::raw_ascii);
      arma::mat I=integrate(xy);
      double avg_inst=0.0;
      for (arma::uword c=0; c<I.n_cols-1; c++)
      {
        avg_inst+=pow(I(/*cm.column*/c),2);
      }
      avg+=avg_inst;
      s++;
    }
    if (s==0)
      throw insight::Exception("Invalid data: no time instants prescribed!");
    avg/=double(s);
    return sqrt(avg);
  }
  else if (const auto *fd = boost::get<Parameters::fielddata_radialProfile_type>(&p_.fielddata) )
  {
    double avg=0.0;
    int s=0;
    for (const auto& inst: fd->values)
    {
      arma::mat xy;
      xy.load( inst.profile->stream(), arma::raw_ascii);
      arma::mat I=integrate(xy);
      double avg_inst=0.0;
      for (arma::uword c=0; c<I.n_cols-1; c++)
      {
        avg_inst+=pow(I(/*cm.column*/c),2);
      }
      avg+=avg_inst;
      s++;
    }
    if (s==0)
      throw insight::Exception("Invalid data: no time instants prescribed!");
    avg/=double(s);
    return sqrt(avg);
  }
  else
  {
    throw insight::Exception("not yet implemented!");
  }
}




double FieldData::calcMaxValueMag() const
{
    double maxv=-DBL_MAX;
    if (const auto *fd = boost::get<Parameters::fielddata_uniformSteady_type>(&p_.fielddata) )
    {
        maxv=std::max(maxv, norm(fd->value, 2));
    }
    else if (const auto *fd = boost::get<Parameters::fielddata_uniform_type>(&p_.fielddata) )
    {
        for (const Parameters::fielddata_uniform_type::values_default_type& inst: fd->values)
        {
            maxv=std::max(maxv, norm(inst.value, 2));
        }
    }
    else if (const auto *fd = boost::get<Parameters::fielddata_linearProfile_type>(&p_.fielddata) )
    {
        for (const auto& inst: fd->values)
        {
            arma::mat xy;
            xy.load( inst.profile->stream(), arma::raw_ascii);
            arma::mat mag_inst(arma::zeros(xy.n_rows));
            arma::uword i=0;
            for (arma::uword c=0; c<mag_inst.n_cols-1; c++)
            {
                mag_inst(i) += pow(xy(i, 1+c/*cm.column*/),2);
                i++;
            }
            maxv=std::max(maxv, as_scalar(arma::max(sqrt(mag_inst))));
        }
    }
    else if (const auto *fd = boost::get<Parameters::fielddata_radialProfile_type>(&p_.fielddata) )
    {
        for (const auto& inst: fd->values)
        {
            arma::mat xy;
            xy.load( inst.profile->stream(), arma::raw_ascii);
            arma::mat mag_inst(arma::zeros(xy.n_rows));
            arma::uword i=0;
            for (arma::uword c=0; c<mag_inst.n_cols-1; c++)
            {
                mag_inst(i) += pow(xy(i, 1+c/*cm.column*/),2);
                i++;
            }
            maxv=std::max(maxv, as_scalar(arma::max(sqrt(mag_inst))));
        }
    }
    else
    {
        throw insight::Exception("not yet implemented!");
    }
    return maxv;
}




Parameter* FieldData::defaultParameter(const arma::mat& def_val, const std::string& )
{
  std::unique_ptr<Parameter> p(Parameters::makeDefault().get<SubsetParameter>("fielddata").clone());
  auto opts = dynamic_cast<SelectableSubsetParameter*>(p.get());

  ParameterSet cp( opts->getParametersForSelection("uniformSteady") );
  cp.get<VectorParameter>("value").set( def_val );
  opts->setParametersForSelection("uniformSteady", cp);

  return p.release();
}




void FieldData::insertGraphsToResultSet(ResultSetPtr results, const boost::filesystem::path& exepath, const std::string& name, const std::string& descr, const std::string& qtylabel) const
{
    if (const Parameters::fielddata_linearProfile_type *fd = boost::get<Parameters::fielddata_linearProfile_type>(&p_.fielddata) )
    {
        for (const Parameters::fielddata_linearProfile_type::values_default_type& inst: fd->values)
        {
            arma::mat xy;
            xy.load( inst.profile->stream(), arma::raw_ascii);

            arma::uword ncmpt=xy.n_cols-1;
            PlotCurveList crvs(ncmpt);
            for (arma::uword i=0; i<ncmpt; i++)
            {
                std::string cmptlabel=getOpenFOAMComponentLabel(int(i), int(ncmpt));
                crvs.push_back( PlotCurve(xy.col(0), xy.col(i+1), cmptlabel, boost::str(boost::format("w lp t '${%s}_{%s}$'")%qtylabel%cmptlabel)) );
            }

            // Resistance convergence
            addPlot
            (
                results, exepath, boost::str(boost::format("%s_t%g")%name%inst.time),
                "ordinate", "$"+qtylabel+"$",
                crvs,
                descr+str(format(" Used from time instance $t=%g$")%inst.time)
            );

        }
    }

}

bool FieldData::isAConstantValue(arma::mat &value) const
{
    if (const Parameters::fielddata_uniformSteady_type *fd =
        boost::get<Parameters::fielddata_uniformSteady_type>(&p_.fielddata) ) //(type=="uniform")
    {
        value = fd->value;
        return true;
    }
    return false;
}

}
