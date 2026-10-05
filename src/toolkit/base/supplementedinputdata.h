#ifndef INSIGHT_SUPPLEMENTEDINPUTDATA_H
#define INSIGHT_SUPPLEMENTEDINPUTDATA_H


#include <memory>

#include "base/parametersbase.h"
#include "base/parametersetinput.h"
#include "base/exception.h"
#include "base/parameters/selectablesubsetparameter.h"
#include "base/units.h"

#include "base/cppextensions.h"
#include "boost/filesystem/path.hpp"
#include "base/actionprogress.h"
#include "base/supplementedquantity.h"
#include "boost/variant/detail/apply_visitor_binary.hpp"
#include "boost/variant/static_visitor.hpp"



namespace insight {



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
  /**
   * @brief The ReportedError struct
   * represents a reported quantity, whose computation failed
   */
  struct ReportedError
  {
    std::string message;
  };

  typedef
      boost::variant<double, arma::mat, std::string, ReportedError>
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

#ifndef SWIG
  typedef
      std::vector<std::pair<std::string,std::exception_ptr> >
      ErrorList;

private:
  mutable boost::mutex reportedSupplementQuantitiesMutex_;
  ReportedSupplementQuantitiesTable reportedSupplementQuantities_;

  /**
   * reporters for deferred quantities,
   * evaluated, when the table is requested
   */
  std::vector<std::function<void(ReportedSupplementQuantitiesTable&)> >
      deferredReports_;

  mutable boost::mutex deferredMutex_;
  std::vector<SupplementedQuantityBasePtr> deferred_;
  std::vector<std::unique_ptr<boost::thread> > deferredWorkers_;

  /**
   * guards forking/releasing of the progress objects
   * of the deferred computations
   */
  std::shared_ptr<boost::mutex> progressMutex_;
  ActionProgressPtr deferredProgress_;

  void registerDeferred(SupplementedQuantityBasePtr q);
  void releaseDeferredProgress();
  SupplementedQuantityBase::ProgressSource progressSource();
#endif

protected:
  // only for derived types
  supplementedInputDataBase(
    const boost::filesystem::path& exePath );

public:
  supplementedInputDataBase(
        ParameterSetInput&& ip,
        const boost::filesystem::path& exePath,
        ActionProgress& ap );

  virtual ~supplementedInputDataBase();

#ifndef SWIG
  /**
   * @brief deferred
   * create a deferred quantity. To be called in the constructor
   * of derived supplementedInputData classes, e.g.
   *
   *   Lref_ = deferred("Lref", [this]() { ...; return L; } );
   *
   * or, if progress reporting is needed,
   *
   *   geometry_ = deferred("geometry", [this](ActionProgress& ap) { ...; return g; } );
   *
   * The function is executed in parallel to other deferred computations
   * (see launchAll) or on first access. It may access other deferred quantities.
   * If the function throws, all quantities depending on it fail as well,
   * but all independent quantities remain available.
   *
   * The type of the quantity is the return type of the function.
   * It has to match the type of the Supplemented<T> member exactly.
   */
  template<class F>
  auto deferred(const std::string& name, F&& f)
  {
      if constexpr (std::is_invocable<F, ActionProgress&>::value)
      {
          typedef std::decay_t<std::invoke_result_t<F, ActionProgress&> > T;
          auto q = std::make_shared<SupplementedQuantity<T> >(
              name, progressSource(),
              std::function<T(ActionProgress&)>(std::forward<F>(f)) );
          registerDeferred(q);
          return Supplemented<T>(q);
      }
      else
      {
          typedef std::decay_t<std::invoke_result_t<F> > T;
          auto q = std::make_shared<SupplementedQuantity<T> >(
              name, progressSource(),
              std::function<T(ActionProgress&)>(
                  [fn=std::forward<F>(f)](ActionProgress&) mutable { return fn(); } ) );
          registerDeferred(q);
          return Supplemented<T>(q);
      }
  }

#endif

  /**
   * @brief launchAll
   * starts the computation of all deferred quantities in background threads.
   * Returns immediately. Does nothing, if already launched.
   * The caller has to ensure, that either computeAll() or cancelDeferredComputations()
   * is called before this object is destroyed.
   * @param progress
   * parent progress object for the individual computations. May be null.
   */
  void launchAll(ActionProgressPtr progress = nullptr);

  /**
   * @brief computeAll
   * computes all deferred quantities in parallel and returns, when all are finished.
   * Does not throw on failures of individual computations.
   */
  void computeAll(ActionProgressPtr progress = nullptr);

  /**
   * @brief cancelDeferredComputations
   * interrupt all background computations and wait for the threads to end.
   * Quantities, which were not finished, are marked as failed.
   */
  void cancelDeferredComputations();

  /**
   * @brief complete
   * non-blocking.
   * @return
   * true, if all deferred quantities have been computed successfully.
   */
  bool complete() const;

#ifndef SWIG
  /**
   * @brief errors
   * non-blocking.
   * @return
   * list of all failed quantities with their errors.
   * Includes quantities, which failed because of failed dependencies.
   */
  ErrorList errors() const;

  /**
   * @brief rootErrors
   * like errors(), but includes only the quantities in which the errors originally occurred.
   */
  ErrorList rootErrors() const;

#endif

  /**
   * @brief throwIfIncomplete
   * computes all deferred quantities and throws an exception
   * listing all errors, if any computation failed.
   */
  void throwIfIncomplete(ActionProgressPtr progress = nullptr);

  void reportSupplementQuantity(
      const std::string& name,
      ReportedSupplementQuantityValue value,
      const std::string& description,
      const std::string& unit = "" );

#ifndef SWIG
  /**
   * @brief reportSupplementQuantity
   * report a deferred quantity.
   * Its value is evaluated, when the table of reported quantities is requested.
   * If the computation failed, the error is reported instead of the value.
   * Never waits for the computation.
   */
  template<class T>
  void reportSupplementQuantity(
      const std::string& name,
      const Supplemented<T>& q,
      const std::string& description,
      const std::string& unit = "" )
  {
      boost::mutex::scoped_lock lck(reportedSupplementQuantitiesMutex_);
      deferredReports_.push_back(
          [name,q,description,unit](ReportedSupplementQuantitiesTable& t)
          {
              ReportedSupplementQuantityValue v;
              switch (q.status())
              {
                  case SupplementedQuantityBase::Done:
                      v=ReportedSupplementQuantityValue(q.get());
                      break;
                  case SupplementedQuantityBase::Failed:
                      v=ReportedError{describeException(q.error())};
                      break;
                  default:
                      v=std::string("(computing...)");
              }
              t.insert({name, ReportedSupplementQuantity{v, description, unit}});
          } );
  }

#endif

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
          toString(u)
          );
  }

  /**
   * @brief insertSupplementQuantities
   * insert a set of quantities to report from e.g. subanalyses
   * @param prefix
   * a string to prepend to each quantity
   * @param rsqt
   * table of quantities to insert
   */
  void insertSupplementQuantities(
      const std::string& prefix,
      const ReportedSupplementQuantitiesTable& rsqt
      );

  /**
   * @brief reportedSupplementQuantities
   * Non-blocking. Deferred quantities, which are not yet computed, are marked as such.
   * @return
   * a copy of the current table of reported quantities.
   */
  ReportedSupplementQuantitiesTable reportedSupplementQuantities() const;

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
    supplementedInputDataFromParameters(
        ParameterSetInput ip,
        const boost::filesystem::path& exePath,
        ActionProgress& ap );


    inline const ParametersBase* baseParametersPtr() const
    { return this->std::unique_ptr<ParametersBase>::get(); }
};




namespace cad {
class FeatureVisualizationStyle;
class Feature;
typedef std::shared_ptr<Feature> FeaturePtr;
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
        ActionProgress& ap)
        : SupplementedInputDataBaseType(
              std::forward<AddArgs>(addArgs)...,
              ip.forward<Parameters>(), exePath, ap)
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
