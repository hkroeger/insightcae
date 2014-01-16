// Copyright (c) Alf P. Steinbach, 2010.
// #include <progrock/cppx/macro_util.h>

#ifndef PROGROCK_CPPX_MACROUTIL_H
#define PROGROCK_CPPX_MACROUTIL_H


#define CPPX_ARGS1( a )                     a
#define CPPX_ARGS2( a, b )                  a, b
#define CPPX_ARGS3( a, b, c )               a, b, c
#define CPPX_ARGS4( a, b, c, d )            a, b, c, d
#define CPPX_ARGS5( a, b, c, d, e )         a, b, c, d, e
#define CPPX_ARGS6( a, b, c, d, e, f )      a, b, c, d, e, f
#define CPPX_ARGS7( a, b, c, d, e, f, g )   a, b, c, d, e, f, g


// "C_"  Comma for macro invocations with template parameter list:
#define C_   ,

// "Tn_"  Alternative, more verbose but more readable?:
#define T1_( t, args )                  t< CPPX_ARGS1 args >
#define T2_( t, args )                  t< CPPX_ARGS2 args >
#define T3_( t, args )                  t< CPPX_ARGS3 args >
#define T4_( t, args )                  t< CPPX_ARGS4 args >
#define T5_( t, args )                  t< CPPX_ARGS5 args >
#define T6_( t, args )                  t< CPPX_ARGS6 args >
#define T7_( t, args )                  t< CPPX_ARGS7 args >

// Concatenate with expansion of arguments:
#define CPPXi_CONCAT_AUX( a, b )        a ## b
#define CPPX_CONCAT( a, b )             CPPXi_CONCAT_AUX( a, b )

#define CPPXi_CONCAT3_AUX( a, b, c )    a ## b ## c
#define CPPX_CONCAT3( a, b, c )         CPPXi_CONCAT3_AUX( a, b, c )

#endif
