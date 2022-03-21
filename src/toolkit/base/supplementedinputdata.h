#ifndef INSIGHT_SUPPLEMENTEDINPUTDATA_H
#define INSIGHT_SUPPLEMENTEDINPUTDATA_H


#include <memory>

#include "base/parameterset.h"
#include "base/cppextensions.h"




namespace insight {




struct ParametersBase
{
  ParametersBase();
  ParametersBase(const insight::ParameterSet& p);
  virtual ~ParametersBase();

  virtual void set(insight::ParameterSet& p) const =0;
  virtual void get(const insight::ParameterSet& p) =0;

  static ParameterSet makeDefault();

  virtual operator ParameterSet() const =0;

  virtual std::unique_ptr<ParametersBase> clone() const =0;
};




class supplementedInputDataBase
    : public std::unique_ptr<ParametersBase>
{

public:
  typedef ParametersBase input_type;

  typedef boost::variant<double, arma::mat, std::string> ReportedSupplementQuantityValue;

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
  supplementedInputDataBase(std::unique_ptr<ParametersBase> pPtr);
  virtual ~supplementedInputDataBase();

  void reportSupplementQuantity(
      const std::string& name,
      ReportedSupplementQuantityValue value,
      const std::string& description,
      const std::string& unit = "" );

  const ReportedSupplementQuantitiesTable& reportedSupplementQuantities() const;

  inline const ParametersBase* baseParametersPtr() const
  { return this->get(); }
  inline ParametersBase* baseParametersPtrRef()
  { return this->get(); }
  inline ParameterSet parameters() const
  { return *baseParametersPtr(); }
};

typedef std::shared_ptr<supplementedInputDataBase> supplementedInputDataBasePtr;



//template<class ParametersType>
//class supplementedInputDataBase
//    : public std::unique_ptr<ParametersType>
//{
//public:
//  typedef ParametersType base_input_type;
//  typedef ParametersType input_type;

//  supplementedInputDataBase(std::unique_ptr<ParametersType> pPtr)
//    : std::unique_ptr<ParametersType>( std::move(pPtr) )
//  {}

//  inline const ParametersType* baseParametersPtr() const
//  { return this->get(); }
//  inline ParametersType* baseParametersPtrRef()
//  { return this->get(); }

//  inline const ParametersType& p() const
//  { return dynamic_cast<const ParametersType&>(*this->baseParametersPtr()); }
//  inline ParametersType& pRef()
//  { return dynamic_cast<ParametersType&>(*this->baseParametersPtrRef()); }
//};




template<class ParametersType, class SupplementedInputDataBaseType = supplementedInputDataBase>
class supplementedInputDataDerived
    : public SupplementedInputDataBaseType
{
public:
  typedef ParametersType input_type;

  template<typename... Args>
  supplementedInputDataDerived( std::unique_ptr<typename SupplementedInputDataBaseType::input_type> pPtr, Args&&... args)
    : SupplementedInputDataBaseType(std::move(pPtr), std::forward<Args>(args)... )
  {}

  template<typename... Args>
  supplementedInputDataDerived( std::unique_ptr<input_type> pPtr, Args&&... args)
    : SupplementedInputDataBaseType(
        std::dynamic_unique_ptr_cast<typename SupplementedInputDataBaseType::input_type>( std::move(pPtr) ),
        std::forward<Args>(args)...
        )
  {}

  template<typename... Args>
  supplementedInputDataDerived( const input_type& pRef, Args&&... args)
    : SupplementedInputDataBaseType(
        std::dynamic_unique_ptr_cast<typename SupplementedInputDataBaseType::input_type>( pRef.clone() ),
        std::forward<Args>(args)...
        )
  {}

  inline const ParametersType& p() const
  { return dynamic_cast<const ParametersType&>(*this->baseParametersPtr()); }
  inline ParametersType& pRef()
  { return dynamic_cast<ParametersType&>(*this->baseParametersPtrRef()); }
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
  inline ParameterSet parameters() const override { return parameters_->parameters(); }



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
