#ifndef INSIGHT_SUPPLEMENTEDINPUTDATA_H
#define INSIGHT_SUPPLEMENTEDINPUTDATA_H


#include <memory>

#include "base/exception.h"
#include "base/parameters/subsetparameter.h"
#include "base/units.h"
#include "base/parameterset.h"
#include "base/cppextensions.h"
#include "boost/filesystem/path.hpp"
#include "boost/variant/detail/apply_visitor_binary.hpp"
#include "boost/variant/static_visitor.hpp"



namespace insight {




struct ParametersBase
{
  ParametersBase();
  ParametersBase(const insight::ParameterSet& p);
  virtual ~ParametersBase();

  virtual void set(insight::ParameterSet& p) const;
  virtual void get(const insight::ParameterSet& p);

  static std::unique_ptr<ParameterSet> makeDefault();

  virtual std::unique_ptr<ParameterSet> cloneParameterSet() const =0;

  virtual std::unique_ptr<ParametersBase> clone() const =0;
};




// typedef boost::variant<
//     const SubsetParameter*,
//     std::unique_ptr<insight::ParametersBase>,
//     std::unique_ptr<insight::ParameterSet>
//     >
//     ParameterSetInputBase;


// struct ParameterSet_visitor
//     : public boost::static_visitor<std::unique_ptr<insight::ParameterSet> >
// {
//     std::unique_ptr<insight::ParameterSet>
//     operator()( const std::unique_ptr<insight::ParametersBase>& pb )
//     {
//         throw insight::Exception("conversion not allowed");
//         return nullptr;
//     }

//     std::unique_ptr<insight::ParameterSet>
//     operator()( const SubsetParameter* ps )
//     {
//         auto r=std::make_unique<ParameterSet>();
//         r->insight::SubsetParameter::operator=(*ps);
//         return r;
//     }

//     std::unique_ptr<insight::ParameterSet>
//     operator()(std::unique_ptr<insight::ParameterSet>& ps)
//     {
//         return std::move(ps);
//     }
// };


// struct ParametersBase_visitor
//     : public boost::static_visitor<std::unique_ptr<insight::ParametersBase> >
// {
//     std::unique_ptr<insight::ParametersBase>
//     operator()( std::unique_ptr<insight::ParametersBase>& pb )
//     {
//         return std::move(pb);
//     }

//     std::unique_ptr<insight::ParametersBase>
//     operator()( const SubsetParameter* ps )
//     {
//         return nullptr;
//     }

//     std::unique_ptr<insight::ParametersBase>
//     operator()(std::unique_ptr<insight::ParameterSet>& ps)
//     {
//         return nullptr;
//     }
// };

// template<class P>
// struct forward_visitor
//     : public boost::static_visitor<std::unique_ptr<insight::ParametersBase> >
// {
//     std::unique_ptr<insight::ParametersBase>
//     operator()( std::unique_ptr<insight::ParametersBase>& pb )
//     {
//         return std::move(pb);
//     }


//     std::unique_ptr<insight::ParametersBase>
//     operator()( const SubsetParameter* ps )
//     {
//         if (ps==nullptr)
//             return std::make_unique<P>(); // default parameters
//         else
//             return std::make_unique<P>(*ps);
//     }

//     std::unique_ptr<insight::ParametersBase>
//     operator()(const std::unique_ptr<insight::ParameterSet>& ps)
//     {
//         return std::make_unique<P>(*ps);
//     }
// };


struct ParameterSetInput
    // : public ParameterSetInputBase
{
private:
    std::observer_ptr<const ParameterSet> ps_;
    std::unique_ptr<insight::ParametersBase> p_;

public:
    // using ParameterSetInputBase::ParameterSetInputBase;

    ParameterSetInput();
    ParameterSetInput(ParameterSetInput&& o);
    ParameterSetInput(const ParameterSet& subs);
    ParameterSetInput(const ParameterSet* ps, std::unique_ptr<insight::ParametersBase>&& p );

    // store a copy of given parameters without source parameter set
    ParameterSetInput( const insight::ParametersBase& p );

    template<class P>
    // std::unique_ptr<insight::ParametersBase>
    std::unique_ptr<insight::ParametersBase>
    create()
    {
        if (p_)
            return std::move(p_);
        else
            return std::make_unique<P>(parameterSet());
    }

    template<class P>
    // std::unique_ptr<insight::ParametersBase>
    ParameterSetInput
    forward()
    {
        // forward_visitor<P> v;
        // return boost::apply_visitor(v, *this);
        return ParameterSetInput(ps_.valid()?ps_.get():nullptr, create<P>() );
    }

    // void operator=(ParameterSetInput& other);
    ParameterSetInput& operator=(ParameterSetInput&& o);

    bool hasParameters() const;
    const ParametersBase& parameters() const;
    /**
     * @brief tweakParameters
     * returns a writable reference to the parameters.
     * Invalidates the parameter set pointer, since it is out of sync after modification
     * @return
     * a reference to the parameters
     */
    ParametersBase& tweakParameters();

    std::unique_ptr<insight::ParametersBase> moveParameters();

    bool hasParameterSet() const;
    const ParameterSet& parameterSet() const;
    std::unique_ptr<ParameterSet> parameterSetCopy() const;

    ParameterSetInput clone() const;

private:
    ParameterSetInput( const ParameterSetInput& o ) = delete;
    ParameterSetInput& operator=( const ParameterSetInput& o ) = delete;
};



class supplementedInputDataBase
{

    boost::filesystem::path executionPath_;

protected:
    /**
     * @brief ps_
     * maintain a reference to the source ParameterSet
     */
    std::observer_ptr<const ParameterSet> parameters_;

public:
  typedef
      boost::variant<double, arma::mat, std::string>
      ReportedSupplementQuantityValue;

  struct ReportedSupplementQuantity
  {
    ReportedSupplementQuantityValue value;
    std::string description;
    std::string unit;
  };

  typedef
      std::map<std::string, ReportedSupplementQuantity>
      ReportedSupplementQuantitiesTable;

private:
  ReportedSupplementQuantitiesTable reportedSupplementQuantities_;

protected:
  // only for derived types
  supplementedInputDataBase(
    const boost::filesystem::path& exePath );

public:
  supplementedInputDataBase(
        ParameterSetInput ip,
        const boost::filesystem::path& exePath,
        ProgressDisplayer& pd );

  virtual ~supplementedInputDataBase();

  void reportSupplementQuantity(
      const std::string& name,
      ReportedSupplementQuantityValue value,
      const std::string& description,
      const std::string& unit = "" );

  template<class Dimension, class Type, class Unit>
  void reportSupplementQuantity(
      const std::string& name,
      const boost::units::quantity<Dimension,Type>& q,
      const std::string& description,
      const Unit& u )
  {
      reportSupplementQuantity(
          name,
          toValue(q, u),
          description,
          boost::lexical_cast<std::string>(u)
          );
  }

  const ReportedSupplementQuantitiesTable& reportedSupplementQuantities() const;

  virtual const ParameterSet& parameters() const;

  const boost::filesystem::path& executionPath() const;
};




typedef
    std::shared_ptr<supplementedInputDataBase>
    supplementedInputDataBasePtr;



class supplementedInputDataFromParameters
    : public std::unique_ptr<ParametersBase>, // this first! order here determines order of initialization
      public supplementedInputDataBase
{

public:
    typedef
        boost::variant<double, arma::mat, std::string>
            ReportedSupplementQuantityValue;

    struct ReportedSupplementQuantity
    {
        ReportedSupplementQuantityValue value;
        std::string description;
        std::string unit;
    };

    typedef std::map<std::string, ReportedSupplementQuantity> ReportedSupplementQuantitiesTable;

private:
    ReportedSupplementQuantitiesTable reportedSupplementQuantities_;

public:
    supplementedInputDataFromParameters(
        ParameterSetInput ip,
        const boost::filesystem::path& exePath,
        ProgressDisplayer& pd );


    inline const ParametersBase* baseParametersPtr() const
    { return this->std::unique_ptr<ParametersBase>::get(); }
};




namespace cad {
class FeatureVisualizationStyle;
}

typedef std::function<void(
    const std::string& name,
    insight::cad::FeaturePtr feat,
    const insight::cad::FeatureVisualizationStyle& fvs)> FeatureDisplayCallback;


template<class ParametersType,
         class SupplementedInputDataBaseType = supplementedInputDataFromParameters,
         typename... AddArgs>
class supplementedInputDataDerived
    : public SupplementedInputDataBaseType
{
public:
    typedef ParametersType Parameters;

    supplementedInputDataDerived(
        AddArgs&&... addArgs,
        ParameterSetInput ip,
        const boost::filesystem::path& exePath,
        ProgressDisplayer& pd)
        : SupplementedInputDataBaseType(
              std::forward<AddArgs>(addArgs)...,
              std::move(ip), exePath, pd)
    {}


  inline const ParametersType& p() const
  { return dynamic_cast<const ParametersType&>(*this->baseParametersPtr()); }
};




#define defineBaseClassWithSupplementedInputData_WithoutParametersFunction(ParameterClass, SupplementedInputDataBaseType) \
protected:\
  std::unique_ptr<SupplementedInputDataBaseType> parameters_;\
  const ParameterClass& p() const\
  { return dynamic_cast<const ParameterClass&>(parameters_->p()); }\
  ParameterClass& pRef()\
  { return dynamic_cast<ParameterClass&>(parameters_->pRef()); }\
  const SupplementedInputDataBaseType& sp() const\
  { return dynamic_cast<const SupplementedInputDataBaseType&>(*parameters_); }\
  SupplementedInputDataBaseType& spRef()\
  { return dynamic_cast<SupplementedInputDataBaseType&>(*parameters_); }




#define defineBaseClassWithSupplementedInputData(ParameterClass, SupplementedInputDataBaseType) \
  defineBaseClassWithSupplementedInputData_WithoutParametersFunction(ParameterClass, SupplementedInputDataBaseType) \
public:\
  inline const ParameterSet& parameters() const override { return parameters_->parameters(); }


#define addParameterMembers_ParameterClass(ParameterClass) \
public:\
    const ParameterClass& p() const \
    { return dynamic_cast<const ParameterClass&>(\
        *this->spPBase().baseParametersPtr() ); }


#define addParameterMembers_SupplementedInputData(ParameterClass) \
public:\
    const ParameterClass& p() const \
    { return dynamic_cast<const ParameterClass&>(\
        *this->spPBase().baseParametersPtr() ); } \
    const supplementedInputData& sp() const \
    { return dynamic_cast<const supplementedInputData&>(\
        this->spPBase() ); }


#define defineDerivedClassWithSupplementedInputData(ParametersType, SupplementedInputDataBaseType) \
protected:\
  inline const ParametersType& p() const\
  { return dynamic_cast<const ParametersType&>(this->parameters_->p()); }\
  inline ParametersType& pRef()\
  { return dynamic_cast<ParametersType&>(this->parameters_->pRef()); }\
  inline const SupplementedInputDataBaseType& sp() const\
  { return dynamic_cast<const SupplementedInputDataBaseType&>(*this->parameters_); }\
  inline SupplementedInputDataBaseType& spRef()\
  { return dynamic_cast<SupplementedInputDataBaseType&>(*this->parameters_); }\





} // namespace insight

#endif // INSIGHT_SUPPLEMENTEDINPUTDATA_H
