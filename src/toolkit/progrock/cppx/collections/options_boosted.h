// Copyright (c) Alf P. Steinbach, 2010.
// #include <progrock/cppx/collections/options_boosted.h>

#ifndef PROGROCK_CPPX_COLLECTIONS_OPTIONS_BOOSTED_H
#define PROGROCK_CPPX_COLLECTIONS_OPTIONS_BOOSTED_H
#include <progrock/cppx/devsupport/better_experience.h>


//----------------------------------------- Dependencies:

#include "options.h"
#include <boost/preprocessor.hpp>
#include <boost/parameter/aux_/preprocessor/for_each.hpp>


//----------------------------------------- Interface:

#define CPPXi_ADD_PARENTHESIS(r, n, elem, data) \
    (elem)

// BOOST_PARAMETER_FOR_EACH_R( r, arity, list, data, macro )
#define CPPXi_PARENTHESIZE_ELEMENTS_AUX( r, _, seq ) \
    BOOST_PARAMETER_FOR_EACH_R( \
        r, 3, seq, _, CPPXi_ADD_PARENTHESIS \
        )

#define CPPXi_PARENTHESIZE_ELEMENTS( seq ) \
    BOOST_PP_SEQ_FOR_EACH( \
        CPPXi_PARENTHESIZE_ELEMENTS_AUX, _, (seq) \
    )

#define CPPXi_OPTIONNAME_OF( optionTriple )                                 \
    BOOST_PP_TUPLE_ELEM( 3, 0, optionTriple )
    
#define CPPXi_TYPESPEC_OF( optionTriple )                                   \
    BOOST_PP_TUPLE_ELEM( 3, 1, optionTriple )

#define CPPXi_DEFAULTSPEC_OF( optionTriple )                                \
    BOOST_PP_TUPLE_ELEM( 3, 2, optionTriple )

    
#define CPPXi_ON_DEFINE_OPTIONV( r, cls, optionTriple )                     \
    CPPX_DEFINE_OPTIONV(                                                    \
        cls,                                                                \
        CPPXi_OPTIONNAME_OF( optionTriple ),                                \
        CPPXi_TYPESPEC_OF( optionTriple ),                                  \
        CPPXi_DEFAULTSPEC_OF( optionTriple )                                \
        )

#define CPPXi_ON_GENERATE_OPTION_ID( r, cls, optionTriple )                 \
    CPPXi_OPTION_ID( cls, CPPXi_OPTIONNAME_OF( optionTriple ) ),

#define CPPXi_BOOSTED_DEFINE_OPTIONCLASS( cls, base, poptions )             \
    BOOST_PP_SEQ_FOR_EACH(                                                  \
        CPPXi_ON_DEFINE_OPTIONV, cls, poptions                              \
        )                                                                   \
    typedef ::progrock::cppx::Typelist<                                     \
        BOOST_PP_SEQ_FOR_EACH(                                              \
            CPPXi_ON_GENERATE_OPTION_ID, cls, poptions                      \
            )                                                               \
        CPPX_EMPTY_TYPELIST                                                 \
        >::T CPPXi_OPTIONTYPES_ID( cls );                                   \
    CPPXi_DEFINE_VTANDO( cls, base )

#define CPPX_DEFINE_OPTIONCLASS( cls, base, options )                       \
    CPPXi_BOOSTED_DEFINE_OPTIONCLASS(                                       \
        cls,                                                                \
        base,                                                               \
        CPPXi_PARENTHESIZE_ELEMENTS( options )                              \
        )

#endif
