
#include "openfoam/fielddata.h"
#include "base/cppextensions.h"
#include "base/exception.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/openfoamcase.h"

#include "base/boost_include.h"

#include "vtkXMLMultiBlockDataReader.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCompositeDataSet.h"
#include "vtkPointData.h"
#include "base/vtktools.h"
#include <string>


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
    : p_(uniformSteady(uniformSteadyValue))
{
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
    : p_(uniformSteady(uniformSteadyValue))
{
    calcValues();
}




FieldData::FieldData(ParameterSetInput ip)
    : p_(ip.forward<Parameters>())
{
    calcValues();
}



OFDictData::data FieldData::sourceEntry(OFdicts& dictionaries) const
{
    std::ostringstream os;

    if (const auto *fd =
        boost::get<Parameters::fielddata_uniformSteady_type>(
            &p().fielddata ) ) //(type=="uniform")
    {
        os <<" uniform unsteady 0.0 " << OFDictData::to_OF(fd->value);
    }

    else if (const auto *fd =
             boost::get<Parameters::fielddata_uniform_type>(
                 &p().fielddata ) ) //(type=="uniform")
    {
        os<<" uniform unsteady";

        for (const auto& inst: fd->values)
        {
            os << " " << inst.time << " " << OFDictData::to_OF(inst.value);
        }
    }
    else if ( const auto *fd =
             boost::get<Parameters::fielddata_linearProfile_type>(
                 &p().fielddata ) )
    {
        os<<" linearProfile "
          <<OFDictData::to_OF(fd->p0)
          <<" "
          <<OFDictData::to_OF(fd->ep)
          ;

        os<<" "
          <<"unsteady";

        for (const auto& inst: fd->values)
        {
            os << " " << inst.time << " " << dictionaries.insertAdditionalInputFile(inst.profile);
        }
    }
    else if ( const auto *fd =
             boost::get<Parameters::fielddata_radialProfile_type>(
                 &p().fielddata) )
    {
        os<<" radialProfile "
          <<OFDictData::to_OF(fd->p0)
          <<" "
          <<OFDictData::to_OF(fd->ep)
          ;

        os<<" "
          <<"unsteady";

        for (const auto& inst: fd->values)
        {
            os << " " << inst.time << " " << dictionaries.insertAdditionalInputFile(inst.profile);
        }
    }
    else if ( const auto *fp =
             boost::get<Parameters::fielddata_fittedProfile_type>(
                 &p().fielddata ) )
    {
        os<<" fittedProfile "
          <<OFDictData::to_OF(fp->p0)
          <<" "
          <<OFDictData::to_OF(fp->ep)
          <<" "
          <<"unsteady";

        for (const auto& inst: fp->values)
        {
            os << " " << inst.time;

            for (const auto& coeffs: inst.component_coeffs)
            {
                os << " [";
                for (size_t cc=0; cc<coeffs.n_elem; cc++)
                    os<<" "<< str( format("%g") % coeffs[cc] );
                os<<" ]";
            }
        }
    }
    else if (const auto *fd =
               boost::get<Parameters::fielddata_fittedProfile_type>(
                   &p().fielddata ) )
    {
        os<<" fittedProfile "
          <<OFDictData::to_OF(fd->p0)
          <<" "
          <<OFDictData::to_OF(fd->ep)
          <<" "
          <<"unsteady";

        for (const auto& inst: fd->values)
        {
            os << " " << inst.time;

            for (const auto& coeffs: inst.component_coeffs)
            {
                os << " [";
                for (size_t cc=0; cc<coeffs.n_elem; cc++)
                    os<<" "<< str( format("%g") % coeffs[cc] );
                os<<" ]";
            }
        }
    }
    else if (const auto *fd =
               boost::get<Parameters::fielddata_vtkField_type>(
                   &p().fielddata ) )
    {
        os<<" vtkField"
          <<" unsteady";

        for (const auto& inst: fd->values)
        {
            os
                << " " << inst.time
                << " \"" << inst.file->filePath().string()<<"\""
                << " \"" << inst.fieldname<<"\""
                ;
        }
    }
    else throw insight::UnhandledSelection();

    return os.str();
}




void FieldData::setDirichletBC(OFDictData::dict& BC, OFdicts& dictionaries) const
{
  if (const Parameters::fielddata_uniformSteady_type *fd = boost::get<Parameters::fielddata_uniformSteady_type>(&p().fielddata) )
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


// avg, max
std::pair<arma::mat,arma::mat>
readVTKData(
    const boost::filesystem::path& fn,
    const std::string& fieldName )
{
    vtkSmartPointer<vtkDataObject> data;

    insight::assertion(
        exists(fn),
        "file %s does not exist!", fn.string().c_str());

    if (fn.extension()==".vtm")
    {
        auto r = vtkSmartPointer<vtkXMLMultiBlockDataReader>::New();
        r->SetFileName(fn.string().c_str());
        r->Update();
        data = multiBlockDataSetToUnstructuredGrid(r->GetOutput());
    }
    else
    {
        auto r = vtkSmartPointer<vtkGenericDataObjectReader>::New();
        r->SetFileName(fn.string().c_str());
        r->Update();
        data = r->GetOutput();
    }

    arma::mat magavg, maxmag;

    auto evalPointData = [&](vtkPointData* pd)
    {
        if (!pd->HasArray(fieldName.c_str()))
        {
            throw insight::Exception(
                "file %s does not contain field %s!",
                fn.c_str(), fieldName.c_str() );
        }
        else
        {
            auto *arr = pd->GetArray(fieldName.c_str());
            magavg=maxmag=arma::zeros(arr->GetNumberOfComponents());
            for (size_t c=0; c<arr->GetNumberOfComponents(); ++c)
            {
                for (size_t j=0; j<arr->GetNumberOfTuples(); ++j)
                {
                    double e=pow(pow(arr->GetTuple(j)[c], 2), 2);
                    magavg(c)+=e;
                    maxmag(c)=std::max(maxmag(c), e);
                }
            }
            magavg/=double(arr->GetNumberOfTuples());
        }
    };

    if (auto *uds = vtkUnstructuredGrid::SafeDownCast(data))
    {
        evalPointData(uds->GetPointData());
    }
    else if (auto *pd = vtkPolyData::SafeDownCast(data))
    {
        evalPointData(pd->GetPointData());
    }
    else
        throw insight::Exception(
            "not implemented: handling of VTK data set type %s",
            data->GetClassName() );

    return {magavg, maxmag};
}


double FieldData::calcRepresentativeValueMag() const
{
  if (const auto *fd =
        boost::get<Parameters::fielddata_uniformSteady_type>(
            &p().fielddata ) )
  {
    return norm(fd->value, 2);
  }
  else if (const auto *fd =
             boost::get<Parameters::fielddata_uniform_type>(
                 &p().fielddata ) )
  {
    double meanv=0.0;
    int s=0;
    for (const auto& inst: fd->values)
    {
      meanv+=pow(norm(inst.value, 2), 2);
      s++;
    }
    if (s==0)
      throw insight::Exception("Invalid data: no time instants prescribed!");
    meanv/=double(s);
    return sqrt(meanv);
  }
  else if (const auto *fd =
             boost::get<Parameters::fielddata_linearProfile_type>(
                 &p().fielddata ) )
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
        avg_inst+=pow(I(c),2);
      }
      avg+=avg_inst;
      s++;
    }
    if (s==0)
      throw insight::Exception("Invalid data: no time instants prescribed!");
    avg/=double(s);
    return sqrt(avg);
  }
  else if (const auto *fd =
             boost::get<Parameters::fielddata_radialProfile_type>(
                 &p().fielddata ) )
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
        avg_inst+=pow(I(c),2);
      }
      avg+=avg_inst;
      s++;
    }
    if (s==0)
      throw insight::Exception("Invalid data: no time instants prescribed!");
    avg/=double(s);
    return sqrt(avg);
  }
  else if (const auto *fd =
           boost::get<Parameters::fielddata_vtkField_type>(
               &p().fielddata ) )
  {
      arma::mat avg;
      for (const auto& inst: fd->values)
      {
          auto d=readVTKData(inst.file->filePath(), inst.fieldname);
          if (avg.n_elem==0)
              avg=d.first;
          else
              avg+=d.first;
      }
      avg/=double(fd->values.size());
      return sqrt(arma::as_scalar(arma::sum(avg)));
  }
  else
  {
    throw insight::Exception("not yet implemented!");
  }
}




double FieldData::calcMaxValueMag() const
{
    double maxv=-DBL_MAX;
    if (const auto *fd =
        boost::get<Parameters::fielddata_uniformSteady_type>(
            &p().fielddata) )
    {
        maxv=std::max(maxv, norm(fd->value, 2));
    }
    else if (const auto *fd =
               boost::get<Parameters::fielddata_uniform_type>(
                   &p().fielddata) )
    {
        for (const Parameters::fielddata_uniform_type::values_default_type& inst: fd->values)
        {
            maxv=std::max(maxv, norm(inst.value, 2));
        }
    }
    else if (const auto *fd =
               boost::get<Parameters::fielddata_linearProfile_type>(
                   &p().fielddata) )
    {
        for (const auto& inst: fd->values)
        {
            arma::mat xy;
            xy.load( inst.profile->stream(), arma::raw_ascii);
            arma::mat mag_inst(arma::zeros(xy.n_rows));
            arma::uword i=0;
            for (arma::uword c=0; c<mag_inst.n_cols-1; c++)
            {
                mag_inst(i) += pow(xy(i, 1+c),2);
                i++;
            }
            maxv=std::max(maxv, as_scalar(arma::max(sqrt(mag_inst))));
        }
    }
    else if (const auto *fd =
               boost::get<Parameters::fielddata_radialProfile_type>(
                   &p().fielddata) )
    {
        for (const auto& inst: fd->values)
        {
            arma::mat xy;
            xy.load( inst.profile->stream(), arma::raw_ascii);
            arma::mat mag_inst(arma::zeros(xy.n_rows));
            arma::uword i=0;
            for (arma::uword c=0; c<mag_inst.n_cols-1; c++)
            {
                mag_inst(i) += pow(xy(i, 1+c),2);
                i++;
            }
            maxv=std::max(maxv, as_scalar(arma::max(sqrt(mag_inst))));
        }
    }
    else if (const auto *fd =
             boost::get<Parameters::fielddata_vtkField_type>(
                   &p().fielddata ) )
    {
        for (const auto& inst: fd->values)
        {
            auto d=readVTKData(inst.file->filePath(), inst.fieldname);

            maxv=std::max(
                maxv,
                arma::as_scalar(arma::max(sqrt(d.second)))
                );
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
  std::unique_ptr<Parameter> p(
        Parameters::makeDefault()->get<ParameterSet>("fielddata").clone(false));
  auto opts = dynamic_cast<SelectableSubsetParameter*>(p.get());

  auto cp =
    opts->getParametersForSelection("uniformSteady").cloneParameterSet();

  cp->get<VectorParameter>("value").set( def_val );
  opts->setParametersForSelection("uniformSteady", *cp);

  return p.release();
}




void FieldData::insertGraphsToResultSet(ResultSetPtr results, const boost::filesystem::path& exepath, const std::string& name, const std::string& descr, const std::string& qtylabel) const
{
    if (const auto *fd =
        boost::get<Parameters::fielddata_linearProfile_type>(
            &p().fielddata) )
    {
        for (const auto& inst: fd->values)
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
    if (const auto *fd =
        boost::get<Parameters::fielddata_uniformSteady_type>(
            &p().fielddata ) ) //(type=="uniform")
    {
        value = fd->value;
        return true;
    }
    return false;
}




}
