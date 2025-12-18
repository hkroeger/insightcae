#ifndef PARAMETERSETWITHGUICONTEXT_H
#define PARAMETERSETWITHGUICONTEXT_H

#include "cadtypes.h"

#include "base/parameterset.h"
#include "base/parameters/spatialtransformationparameter.h"
#include "base/parameters/simpleparameter.h"
#include <string>

namespace insight {


namespace ParameterGUIContext
{


struct DataBase
{
    virtual ~DataBase();
};

template <typename P>
struct Data
    : public DataBase
{ };

#define INSIGHT_DATA_DEFINE_MEMBER(r, data, M) \
BOOST_PP_TUPLE_ELEM( 2, 0, M ) BOOST_PP_TUPLE_ELEM( 2, 1, M );

#define INSIGHT_DATA_CTORARG_MEMBER(r, data, M) \
(BOOST_PP_TUPLE_ELEM( 2, 0, M ) BOOST_PP_TUPLE_ELEM( 2, 1, M ))

#define INSIGHT_DATA_CTORINIT_MEMBER(r, data, M) \
(BOOST_PP_TUPLE_ELEM( 2, 1, M )( BOOST_PP_TUPLE_ELEM( 2, 1, M )))

#define INSIGHT_DATA_MEMBERS_AND_CTOR(CLASSNAME, TYPES_MEMBERS) \
BOOST_PP_SEQ_FOR_EACH(INSIGHT_DATA_DEFINE_MEMBER, _, TYPES_MEMBERS) \
CLASSNAME ( BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(INSIGHT_DATA_CTORARG_MEMBER, _, TYPES_MEMBERS)) ) \
: BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(INSIGHT_DATA_CTORINIT_MEMBER, _, TYPES_MEMBERS)) \
{}


template <>
struct Data<SpatialTransformationParameter>
    : public DataBase
{
    INSIGHT_DATA_MEMBERS_AND_CTOR(
        Data,
        ((insight::cad::FeaturePtr, geometry))
        ((insight::CoordinateSystem, referenceCS))
        )
};

template <>
struct Data<VectorParameter>
    : public DataBase
{
    INSIGHT_DATA_MEMBERS_AND_CTOR(
        Data,
        ((arma::mat, basePoint))
        )
};

}


class ParameterSetGUIContext
{
    std::map<const Parameter*, std::unique_ptr<ParameterGUIContext::DataBase> > contextData_;

protected:
    virtual ParameterSet& theParameterSet() =0;

public:

    template<class ParameterClass, typename... Args>
    void addData(const std::string& ppath, Args... args)
    {
        auto *p=&theParameterSet().template get<ParameterClass>(ppath);
        contextData_[p]=std::make_unique<ParameterGUIContext::Data<ParameterClass> >(args...);
    }

    template<class ParameterClass, typename... Args>
    void addData(const ParameterClass* p, Args... args)
    {
        contextData_[p]=std::make_unique<ParameterGUIContext::Data<ParameterClass> >(args...);
    }

    template<class ParameterClass>
    boost::optional<ParameterGUIContext::Data<ParameterClass> >
    getData(const ParameterClass* p)
    {
        if (contextData_.count(p))
            return dynamic_cast<ParameterGUIContext::Data<ParameterClass>&>(
                *contextData_.at(p));
        else
            return boost::optional<ParameterGUIContext::Data<ParameterClass> >();
    }

    template<class ParameterClass>
    boost::optional<ParameterGUIContext::Data<ParameterClass> >
    getData(const std::string& ppath)
    {
        auto *p=&theParameterSet().template get<ParameterClass>(ppath);
        return getData(p);
    }
};


template<class P>
class ParameterSetWithGUIContext
: public P,
  public ParameterSetGUIContext
{
protected:
    ParameterSet& theParameterSet() override
    {
        return *this;
    }

public:
    using P::P;
};


} // namespace insight

#endif // PARAMETERSETWITHGUICONTEXT_H
