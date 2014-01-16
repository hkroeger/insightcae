// Copyright (c) Alf P. Steinbach, 2010.
// #include <progrock/cppx/collections/options.h>

#ifndef PROGROCK_CPPX_COLLECTIONS_OPTIONS_H
#define PROGROCK_CPPX_COLLECTIONS_OPTIONS_H
#include <progrock/cppx/devsupport/better_experience.h>


//----------------------------------------- Dependencies:

#include <progrock/cppx/typelist.h>
#include <progrock/cppx/typechecking.h>     // CPPX_DOWNCAST_TO
#include <progrock/cppx/macro_util.h>       // CPPX_CONCAT


//----------------------------------------- Interface:

namespace progrock{ namespace cppx{ namespace options {

    template< class T, class UniqueIdType >
    class Value_
    {
    private:
        T   myValue;

    public:
        typedef CPPX_EMPTY_TYPELIST     BaseClasses;

        Value_(): myValue() {}
        Value_( T const& v ): myValue( v ) {}

        T const& value() const
        {
            return myValue;
        }

        void setValue( T const& v )
        {
            myValue = v;    // As the single value-changer supports setting breakpoint here.
        }

        Value_& operator=( T const& v )
        {
            setValue( v ); return *this;
        }
    };

    class NoBase
    {
    public:
        typedef CPPX_EMPTY_TYPELIST BaseClasses;
    };

    template< class SetterResult, class TopBase >
    class NoBase_cppx_InheritTemplatedSetters_: public TopBase
    {
    public:
        typedef typename CPPX_TYPELIST< TopBase >::T    BaseClasses;
    };

    template<
        class OptionTypes,
        class SetterResult,
        class TopBase,
        template< class, class, class > class OptionSetter_
        >
    class InheritTemplatedSetters_;

    template<
        class SetterResult,
        class TopBase,
        template< class, class, class > class OptionSetter_
        >
    class InheritTemplatedSetters_<
        cppx::EmptyTypelist, SetterResult, TopBase, OptionSetter_
        >
        : public TopBase
    {
    public:
        typedef typename cppx::Typelist< TopBase >::T    BaseClasses;
    };

    template<
        class OptionTypes,
        class SetterResult,
        class TopBase,
        template< class, class, class > class OptionSetter_
        >
    class InheritTemplatedSetters_
        : public OptionSetter_<
            typename cppx::HeadOf< OptionTypes >::T,
            SetterResult,
            InheritTemplatedSetters_<
                typename cppx::TailOf< OptionTypes >::T,
                SetterResult,
                TopBase,
                OptionSetter_
                >
            >
    {
    private:
        typedef OptionSetter_<
            typename cppx::HeadOf< OptionTypes >::T,
            SetterResult,
            InheritTemplatedSetters_<
                typename cppx::TailOf< OptionTypes >::T,
                SetterResult,
                TopBase,
                OptionSetter_
                >
            > BaseClass;
    public:
        typedef typename cppx::Typelist< BaseClass >::T  BaseClasses;
    };

}}}  // namespace progrock::progrock::cppx::options

#define CPPX_OPTIONS_NO_BASE    \
    ::progrock::cppx::options::NoBase

#define CPPXi_OPTION_ID( cls, name )                    \
    CPPX_CONCAT(                                        \
        CPPX_CONCAT( cls, _cppx_OptionValue_ ),         \
        name                                            \
        )

#define CPPXi_OPTIONTYPES_ID( name )                    \
    CPPX_CONCAT( name, _cppx_OptionTypes )

#define CPPXi_VALUES_ID( name )                         \
    CPPX_CONCAT( name, _cppx_Values )

#define CPPXi_INHERITSETTERS_ID( name )                 \
    CPPX_CONCAT( name, _cppx_InheritTemplatedSetters_ )

#define CPPXi_QUALIFIED_ID( cls, name )                 \
    CPPX_CONCAT3( cls, _, name )

#define CPPXi_IDTYPEFOR( cls, name )                    \
    struct CPPX_CONCAT( cppx_UniqueIdTypeFor_,          \
        CPPXi_QUALIFIED_ID( cls, name )                 \
        )

#define CPPXi_DEFINE_OPTIONSETTER( cls, name, type )                    \
    template< class OptionValue, class SetterResult, class Base >       \
    class cppx_OptionSetter_;                                           \
                                                                        \
    template< class SetterResult, class Base >                          \
    class cppx_OptionSetter_<                                           \
        CPPXi_OPTION_ID( cls, name ), SetterResult, Base                \
        >                                                               \
        : public Base                                                   \
    {                                                                   \
    public:                                                             \
        typedef typename CPPX_TYPELIST< Base >::T BaseClasses;          \
                                                                        \
        type const& name() const                                        \
        { return CPPXi_OPTION_ID( cls, name )::value(); }               \
                                                                        \
        SetterResult& name( type const& value )                         \
        {                                                               \
            CPPXi_OPTION_ID( cls, name )::setValue( value );            \
            return CPPX_DOWNCAST_TO( SetterResult, *this );             \
        }                                                               \
                                                                        \
        SetterResult& set_##name( type const& value )                   \
        {                                                               \
            return name( value );                                       \
        }                                                               \
    };

#define CPPX_DEFINE_OPTION_( cls, name, type )                          \
    typedef ::progrock::cppx::options::Value_<                          \
        type, CPPXi_IDTYPEFOR( cls, name )                              \
        > CPPXi_OPTION_ID( cls, name );                                 \
    CPPXi_DEFINE_OPTIONSETTER( cls, name, type )

#define CPPX_DEFINE_OPTIONV( cls, name, type, defValue )                \
    class CPPXi_OPTION_ID( cls, name )                                  \
        : public ::progrock::cppx::options::Value_<                     \
            type, CPPXi_IDTYPEFOR( cls, name )                          \
            >                                                           \
    {                                                                   \
    typedef ::progrock::cppx::options::Value_<                          \
        type, CPPXi_IDTYPEFOR( cls, name )                              \
        > BaseClass;                                                    \
    public:                                                             \
        typedef CPPX_TYPELIST< BaseClass >::T   BaseClasses;            \
        CPPXi_OPTION_ID( cls, name )(): BaseClass( defValue ) {}        \
        CPPXi_OPTION_ID( cls, name )( type const& v ): BaseClass( v ) {}\
    };                                                                  \
    CPPXi_DEFINE_OPTIONSETTER( cls, name, type )

#define CPPXi_DEFINE_VALUECLASS( name, base )                           \
    class CPPXi_VALUES_ID( name )                                       \
        : public base                                                   \
        , public ::progrock::cppx::InheritAllOf<                        \
            CPPXi_OPTIONTYPES_ID( name )                                \
            >                                                           \
    {                                                                   \
    private:                                                            \
        typedef ::progrock::cppx::InheritAllOf<                         \
            CPPXi_OPTIONTYPES_ID( name )                                \
            > MultiBaseClass;                                           \
    public:                                                             \
        typedef CPPX_TYPELIST< base, MultiBaseClass >::T BaseClasses;   \
    };

#define CPPXi_DEFINE_TEMPLATED_SETTERS_INHERITANCE( name, base )        \
    template< class SetterResult, class TopBase >                       \
    class CPPXi_INHERITSETTERS_ID( name )                               \
        : public ::progrock::cppx::options::InheritTemplatedSetters_<   \
            CPPXi_OPTIONTYPES_ID( name ),                               \
            SetterResult,                                               \
            CPPXi_INHERITSETTERS_ID( base )<                            \
                SetterResult, TopBase                                   \
                >,                                                      \
            cppx_OptionSetter_                                          \
            >                                                           \
    {};

#define CPPXi_DEFINE_OPTIONCLASS( name, base )                          \
    class name                                                          \
        : public CPPXi_INHERITSETTERS_ID( name )<                       \
            name,                                                       \
            CPPXi_VALUES_ID( name )                                     \
            >                                                           \
    {                                                                   \
        typedef CPPXi_INHERITSETTERS_ID( name )<                        \
            name, CPPXi_VALUES_ID( name )                               \
            > BaseClass;                                                \
    public:                                                             \
        typedef CPPX_TYPELIST< BaseClass >::T BaseClasses;              \
    };

#define CPPXi_DEFINE_VTANDO( name, base )                               \
    CPPXi_DEFINE_VALUECLASS( name, base )                               \
    CPPXi_DEFINE_TEMPLATED_SETTERS_INHERITANCE( name, base )            \
    CPPXi_DEFINE_OPTIONCLASS( name, base )

#define CPPXi_DEFINE_CLASSONLY_1OPTIONCLASS( cls, base, option1 )       \
    typedef CPPX_TYPELIST<                                              \
        CPPXi_OPTION_ID( cls, option1 )                                 \
        >::T CPPXi_OPTIONTYPES_ID( cls );                               \
        CPPXi_DEFINE_VTANDO( cls, base )

#define CPPX_DEFINE_1OPTIONCLASS( cls, base,                            \
    option1, type1, defValue1                                           \
    )                                                                   \
    CPPX_DEFINE_OPTIONV( cls, option1, type1, defValue1 )               \
    CPPXi_DEFINE_CLASSONLY_1OPTIONCLASS( cls, base,                     \
        option1                                                         \
        )

#define CPPXi_DEFINE_CLASSONLY_2OPTIONCLASS(                            \
    cls, base, option1, option2                                         \
    )                                                                   \
    typedef CPPX_TYPELIST<                                              \
        CPPXi_OPTION_ID( cls, option1 ),                                \
        CPPXi_OPTION_ID( cls, option2 )                                 \
        >::T CPPXi_OPTIONTYPES_ID( cls );                               \
        CPPXi_DEFINE_VTANDO( cls, base )

#define CPPX_DEFINE_2OPTIONCLASS( cls, base,                            \
    option1, type1, defValue1,                                          \
    option2, type2, defValue2                                           \
    )                                                                   \
    CPPX_DEFINE_OPTIONV( cls, option1, type1, defValue1 )               \
    CPPX_DEFINE_OPTIONV( cls, option2, type2, defValue2 )               \
    CPPXi_DEFINE_CLASSONLY_2OPTIONCLASS( cls, base,                     \
        option1,                                                        \
        option2                                                         \
        )

#define CPPXi_DEFINE_CLASSONLY_3OPTIONCLASS(                            \
    cls, base, option1, option2, option3                                \
    )                                                                   \
    typedef CPPX_TYPELIST<                                              \
        CPPXi_OPTION_ID( cls, option1 ),                                \
        CPPXi_OPTION_ID( cls, option2 ),                                \
        CPPXi_OPTION_ID( cls, option3 )                                 \
        >::T CPPXi_OPTIONTYPES_ID( cls );                               \
        CPPXi_DEFINE_VTANDO( cls, base )

#define CPPX_DEFINE_3OPTIONCLASS( cls, base,                            \
    option1, type1, defValue1,                                          \
    option2, type2, defValue2,                                          \
    option3, type3, defValue3                                           \
    )                                                                   \
    CPPX_DEFINE_OPTIONV( cls, option1, type1, defValue1 )               \
    CPPX_DEFINE_OPTIONV( cls, option2, type2, defValue2 )               \
    CPPX_DEFINE_OPTIONV( cls, option3, type3, defValue3 )               \
    CPPXi_DEFINE_CLASSONLY_3OPTIONCLASS( cls, base,                     \
        option1,                                                        \
        option2,                                                        \
        option3                                                         \
        )

#define CPPXi_DEFINE_CLASSONLY_4OPTIONCLASS(                            \
    cls, base, option1, option2, option3, option4                       \
    )                                                                   \
    typedef CPPX_TYPELIST<                                              \
        CPPXi_OPTION_ID( cls, option1 ),                                \
        CPPXi_OPTION_ID( cls, option2 ),                                \
        CPPXi_OPTION_ID( cls, option3 ),                                \
        CPPXi_OPTION_ID( cls, option4 )                                 \
        >::T CPPXi_OPTIONTYPES_ID( cls );                               \
        CPPXi_DEFINE_VTANDO( cls, base )

#define CPPX_DEFINE_4OPTIONCLASS( cls, base,                            \
    option1, type1, defValue1,                                          \
    option2, type2, defValue2,                                          \
    option3, type3, defValue3,                                          \
    option4, type4, defValue4                                           \
    )                                                                   \
    CPPX_DEFINE_OPTIONV( cls, option1, type1, defValue1 )               \
    CPPX_DEFINE_OPTIONV( cls, option2, type2, defValue2 )               \
    CPPX_DEFINE_OPTIONV( cls, option3, type3, defValue3 )               \
    CPPX_DEFINE_OPTIONV( cls, option4, type4, defValue4 )               \
    CPPXi_DEFINE_CLASSONLY_4OPTIONCLASS( cls, base,                     \
        option1,                                                        \
        option2,                                                        \
        option3,                                                        \
        option4                                                         \
        )

#define CPPXi_DEFINE_CLASSONLY_5OPTIONCLASS(                            \
    cls, base, option1, option2, option3, option4, option5              \
    )                                                                   \
    typedef CPPX_TYPELIST<                                              \
        CPPXi_OPTION_ID( cls, option1 ),                                \
        CPPXi_OPTION_ID( cls, option2 ),                                \
        CPPXi_OPTION_ID( cls, option3 ),                                \
        CPPXi_OPTION_ID( cls, option4 ),                                \
        CPPXi_OPTION_ID( cls, option5 )                                 \
        >::T CPPXi_OPTIONTYPES_ID( cls );                               \
        CPPXi_DEFINE_VTANDO( cls, base )

#define CPPX_DEFINE_5OPTIONCLASS( cls, base,                            \
    option1, type1, defValue1,                                          \
    option2, type2, defValue2,                                          \
    option3, type3, defValue3,                                          \
    option4, type4, defValue4,                                          \
    option5, type5, defValue5                                           \
    )                                                                   \
    CPPX_DEFINE_OPTIONV( cls, option1, type1, defValue1 )               \
    CPPX_DEFINE_OPTIONV( cls, option2, type2, defValue2 )               \
    CPPX_DEFINE_OPTIONV( cls, option3, type3, defValue3 )               \
    CPPX_DEFINE_OPTIONV( cls, option4, type4, defValue4 )               \
    CPPX_DEFINE_OPTIONV( cls, option5, type5, defValue5 )               \
    CPPXi_DEFINE_CLASSONLY_5OPTIONCLASS( cls, base,                     \
        option1,                                                        \
        option2,                                                        \
        option3,                                                        \
        option4,                                                        \
        option5                                                         \
        )

#define CPPXi_DEFINE_CLASSONLY_6OPTIONCLASS(                            \
    cls, base,                                                          \
    option1, option2, option3, option4, option5, option6                \
    )                                                                   \
    typedef CPPX_TYPELIST<                                              \
        CPPXi_OPTION_ID( cls, option1 ),                                \
        CPPXi_OPTION_ID( cls, option2 ),                                \
        CPPXi_OPTION_ID( cls, option3 ),                                \
        CPPXi_OPTION_ID( cls, option4 ),                                \
        CPPXi_OPTION_ID( cls, option5 ),                                \
        CPPXi_OPTION_ID( cls, option6 )                                 \
        >::T CPPXi_OPTIONTYPES_ID( cls );                               \
        CPPXi_DEFINE_VTANDO( cls, base )

#define CPPX_DEFINE_6OPTIONCLASS( cls, base,                            \
    option1, type1, defValue1,                                          \
    option2, type2, defValue2,                                          \
    option3, type3, defValue3,                                          \
    option4, type4, defValue4,                                          \
    option5, type5, defValue5,                                          \
    option6, type6, defValue6                                           \
    )                                                                   \
    CPPX_DEFINE_OPTIONV( cls, option1, type1, defValue1 )               \
    CPPX_DEFINE_OPTIONV( cls, option2, type2, defValue2 )               \
    CPPX_DEFINE_OPTIONV( cls, option3, type3, defValue3 )               \
    CPPX_DEFINE_OPTIONV( cls, option4, type4, defValue4 )               \
    CPPX_DEFINE_OPTIONV( cls, option5, type5, defValue5 )               \
    CPPX_DEFINE_OPTIONV( cls, option6, type6, defValue6 )               \
    CPPXi_DEFINE_CLASSONLY_6OPTIONCLASS( cls, base,                     \
        option1,                                                        \
        option2,                                                        \
        option3,                                                        \
        option4,                                                        \
        option5,                                                        \
        option6                                                         \
        )

#define CPPXi_DEFINE_CLASSONLY_7OPTIONCLASS(                            \
    cls, base,                                                          \
    option1, option2, option3, option4, option5, option6, option7       \
    )                                                                   \
    typedef CPPX_TYPELIST<                                              \
        CPPXi_OPTION_ID( cls, option1 ),                                \
        CPPXi_OPTION_ID( cls, option2 ),                                \
        CPPXi_OPTION_ID( cls, option3 ),                                \
        CPPXi_OPTION_ID( cls, option4 ),                                \
        CPPXi_OPTION_ID( cls, option5 ),                                \
        CPPXi_OPTION_ID( cls, option6 ),                                \
        CPPXi_OPTION_ID( cls, option7 )                                 \
        >::T CPPXi_OPTIONTYPES_ID( cls );                               \
        CPPXi_DEFINE_VTANDO( cls, base )

#define CPPX_DEFINE_7OPTIONCLASS( cls, base,                            \
    option1, type1, defValue1,                                          \
    option2, type2, defValue2,                                          \
    option3, type3, defValue3,                                          \
    option4, type4, defValue4,                                          \
    option5, type5, defValue5,                                          \
    option6, type6, defValue6,                                          \
    option7, type7, defValue7                                           \
    )                                                                   \
    CPPX_DEFINE_OPTIONV( cls, option1, type1, defValue1 )               \
    CPPX_DEFINE_OPTIONV( cls, option2, type2, defValue2 )               \
    CPPX_DEFINE_OPTIONV( cls, option3, type3, defValue3 )               \
    CPPX_DEFINE_OPTIONV( cls, option4, type4, defValue4 )               \
    CPPX_DEFINE_OPTIONV( cls, option5, type5, defValue5 )               \
    CPPX_DEFINE_OPTIONV( cls, option6, type6, defValue6 )               \
    CPPX_DEFINE_OPTIONV( cls, option7, type7, defValue7 )               \
    CPPXi_DEFINE_CLASSONLY_7OPTIONCLASS( cls, base,                     \
        option1,                                                        \
        option2,                                                        \
        option3,                                                        \
        option4,                                                        \
        option5,                                                        \
        option6,                                                        \
        option7                                                         \
        )

#define CPPXi_DEFINE_CLASSONLY_8OPTIONCLASS(                            \
    cls, base,                                                          \
    option1, option2, option3, option4, option5, option6, option7,      \
    option8                                                             \
    )                                                                   \
    typedef CPPX_TYPELIST<                                              \
        CPPXi_OPTION_ID( cls, option1 ),                                \
        CPPXi_OPTION_ID( cls, option2 ),                                \
        CPPXi_OPTION_ID( cls, option3 ),                                \
        CPPXi_OPTION_ID( cls, option4 ),                                \
        CPPXi_OPTION_ID( cls, option5 ),                                \
        CPPXi_OPTION_ID( cls, option6 ),                                \
        CPPXi_OPTION_ID( cls, option7 ),                                \
        CPPXi_OPTION_ID( cls, option8 )                                 \
        >::T CPPXi_OPTIONTYPES_ID( cls );                               \
        CPPXi_DEFINE_VTANDO( cls, base )

#define CPPX_DEFINE_8OPTIONCLASS( cls, base,                            \
    option1, type1, defValue1,                                          \
    option2, type2, defValue2,                                          \
    option3, type3, defValue3,                                          \
    option4, type4, defValue4,                                          \
    option5, type5, defValue5,                                          \
    option6, type6, defValue6,                                          \
    option7, type7, defValue7,                                          \
    option8, type8, defValue8                                           \
    )                                                                   \
    CPPX_DEFINE_OPTIONV( cls, option1, type1, defValue1 )               \
    CPPX_DEFINE_OPTIONV( cls, option2, type2, defValue2 )               \
    CPPX_DEFINE_OPTIONV( cls, option3, type3, defValue3 )               \
    CPPX_DEFINE_OPTIONV( cls, option4, type4, defValue4 )               \
    CPPX_DEFINE_OPTIONV( cls, option5, type5, defValue5 )               \
    CPPX_DEFINE_OPTIONV( cls, option6, type6, defValue6 )               \
    CPPX_DEFINE_OPTIONV( cls, option7, type7, defValue7 )               \
    CPPX_DEFINE_OPTIONV( cls, option8, type8, defValue8 )               \
    CPPXi_DEFINE_CLASSONLY_8OPTIONCLASS( cls, base,                     \
        option1,                                                        \
        option2,                                                        \
        option3,                                                        \
        option4,                                                        \
        option5,                                                        \
        option6,                                                        \
        option7,                                                        \
        option8                                                         \
        )

#define CPPXi_DEFINE_CLASSONLY_9OPTIONCLASS(                            \
    cls, base,                                                          \
    option1, option2, option3, option4, option5, option6, option7,      \
    option8, option9                                                    \
    )                                                                   \
    typedef CPPX_TYPELIST<                                              \
        CPPXi_OPTION_ID( cls, option1 ),                                \
        CPPXi_OPTION_ID( cls, option2 ),                                \
        CPPXi_OPTION_ID( cls, option3 ),                                \
        CPPXi_OPTION_ID( cls, option4 ),                                \
        CPPXi_OPTION_ID( cls, option5 ),                                \
        CPPXi_OPTION_ID( cls, option6 ),                                \
        CPPXi_OPTION_ID( cls, option7 ),                                \
        CPPXi_OPTION_ID( cls, option8 ),                                \
        CPPXi_OPTION_ID( cls, option9 )                                 \
        >::T CPPXi_OPTIONTYPES_ID( cls );                               \
        CPPXi_DEFINE_VTANDO( cls, base )

#define CPPX_DEFINE_9OPTIONCLASS( cls, base,                            \
    option1, type1, defValue1,                                          \
    option2, type2, defValue2,                                          \
    option3, type3, defValue3,                                          \
    option4, type4, defValue4,                                          \
    option5, type5, defValue5,                                          \
    option6, type6, defValue6,                                          \
    option7, type7, defValue7,                                          \
    option8, type8, defValue8,                                          \
    option9, type9, defValue9                                           \
    )                                                                   \
    CPPX_DEFINE_OPTIONV( cls, option1, type1, defValue1 )               \
    CPPX_DEFINE_OPTIONV( cls, option2, type2, defValue2 )               \
    CPPX_DEFINE_OPTIONV( cls, option3, type3, defValue3 )               \
    CPPX_DEFINE_OPTIONV( cls, option4, type4, defValue4 )               \
    CPPX_DEFINE_OPTIONV( cls, option5, type5, defValue5 )               \
    CPPX_DEFINE_OPTIONV( cls, option6, type6, defValue6 )               \
    CPPX_DEFINE_OPTIONV( cls, option7, type7, defValue7 )               \
    CPPX_DEFINE_OPTIONV( cls, option8, type8, defValue8 )               \
    CPPX_DEFINE_OPTIONV( cls, option9, type9, defValue9 )               \
    CPPXi_DEFINE_CLASSONLY_9OPTIONCLASS( cls, base,                     \
        option1,                                                        \
        option2,                                                        \
        option3,                                                        \
        option4,                                                        \
        option5,                                                        \
        option6,                                                        \
        option7,                                                        \
        option8,                                                        \
        option9                                                         \
        )

#endif
