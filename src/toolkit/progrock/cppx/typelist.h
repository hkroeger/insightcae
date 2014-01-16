// Copyright (c) Alf P. Steinbach, 2010.
// #include <progrock/cppx/typelist.h>

#ifndef PROGROCK_CPPX_TYPELIST_H
#define PROGROCK_CPPX_TYPELIST_H
#include "devsupport/better_experience.h"


//---------------------------------------  Dependencies:



//---------------------------------------  Interface:

namespace progrock{ namespace cppx{

    struct EmptyTypelist {};

    template< class HeadType, class TailType = EmptyTypelist >
    struct Cons
    {};


    template< class ConsList >
    struct HeadOf;

    template< class HeadType, class TailType >
    struct HeadOf< Cons< HeadType, TailType > >
    {
        typedef HeadType    T;
    };


    template< class ConsList >
    struct TailOf;

    template< class HeadType, class TailType >
    struct TailOf< Cons< HeadType, TailType > >
    {
        typedef TailType    T;
    };


    template< class ConsList >
    struct InheritAllOf;

    template<>
    struct InheritAllOf< EmptyTypelist >
    {
        typedef EmptyTypelist   BaseClasses;
    };

    template< class HeadType, class TailType >
    struct InheritAllOf< Cons< HeadType, TailType > >
        : public InheritAllOf< TailType >   
        , public HeadType
    {
        typedef Cons< InheritAllOf< TailType >, Cons< HeadType > >  BaseClasses;
    };


    // Utility for specifying typelists:

    template<
        class T01 = EmptyTypelist, class T02 = EmptyTypelist, class T03 = EmptyTypelist,
        class T04 = EmptyTypelist, class T05 = EmptyTypelist, class T06 = EmptyTypelist,
        class T07 = EmptyTypelist, class T08 = EmptyTypelist, class T09 = EmptyTypelist,
        class T10 = EmptyTypelist, class T11 = EmptyTypelist, class T12 = EmptyTypelist
        >
    struct Typelist;

    template<
        class T01, class T02, class T03,
        class T04, class T05, class T06,
        class T07, class T08, class T09,
        class T10, class T11, class T12
        >
    struct HeadOf< Typelist<
        T01, T02, T03,
        T04, T05, T06,
        T07, T08, T09,
        T10, T11, T12
        > >
        : public Typelist<T01>::dont_use_Typelist_directly_use_the_nested_T_typedef
    {};

    template<
        >
    struct Typelist<
        EmptyTypelist, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist
        >
    {
        typedef EmptyTypelist   T;
    };

    template<
        class T01
        >
    struct Typelist<
        T01, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist
        >
    {
        typedef Cons< T01, EmptyTypelist >  T;
    };

    template<
        class T01, class T02
        >
    struct Typelist<
        T01, T02, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist
        >
    {
        typedef Cons< T01, typename Typelist< T02 >::T > T;
    };

    template<
        class T01, class T02, class T03
        >
    struct Typelist<
        T01, T02, T03,
        EmptyTypelist, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist
        >
    {
        typedef Cons< T01, typename Typelist< T02, T03 >::T > T;
    };

    template<
        class T01, class T02, class T03,
        class T04
        >
    struct Typelist<
        T01, T02, T03,
        T04, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist
        >
    {
        typedef Cons< T01, typename Typelist< T02, T03, T04 >::T > T;
    };

    template<
        class T01, class T02, class T03,
        class T04, class T05
        >
    struct Typelist<
        T01, T02, T03,
        T04, T05, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist
        >
    {
        typedef Cons< T01, typename Typelist< T02, T03, T04, T05 >::T > T;
    };

    template<
        class T01, class T02, class T03,
        class T04, class T05, class T06
        >
    struct Typelist<
        T01, T02, T03,
        T04, T05, T06,
        EmptyTypelist, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist
        >
    {
        typedef Cons< T01, typename Typelist< T02, T03, T04, T05, T06 >::T > T;
    };

    template<
        class T01, class T02, class T03,
        class T04, class T05, class T06,
        class T07
        >
    struct Typelist<
        T01, T02, T03,
        T04, T05, T06,
        T07, EmptyTypelist, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist
        >
    {
        typedef Cons< T01, typename Typelist< T02, T03, T04, T05, T06, T07 >::T > T;
    };

    template<
        class T01, class T02, class T03,
        class T04, class T05, class T06,
        class T07, class T08
        >
    struct Typelist<
        T01, T02, T03,
        T04, T05, T06,
        T07, T08, EmptyTypelist,
        EmptyTypelist, EmptyTypelist, EmptyTypelist
        >
    {
        typedef Cons< T01, typename Typelist< T02, T03, T04, T05, T06, T07, T08 >::T > T;
    };

    template<
        class T01, class T02, class T03,
        class T04, class T05, class T06,
        class T07, class T08, class T09
        >
    struct Typelist<
        T01, T02, T03,
        T04, T05, T06,
        T07, T08, T09,
        EmptyTypelist, EmptyTypelist, EmptyTypelist
        >
    {
        typedef Cons< T01, typename Typelist< T02, T03, T04, T05, T06, T07, T08, T09 >::T > T;
    };

    template<
        class T01, class T02, class T03,
        class T04, class T05, class T06,
        class T07, class T08, class T09,
        class T10
        >
    struct Typelist<
        T01, T02, T03,
        T04, T05, T06,
        T07, T08, T09,
        T10, EmptyTypelist, EmptyTypelist
        >
    {
        typedef Cons< T01, typename Typelist< T02, T03, T04, T05, T06, T07, T08, T09, T10 >::T > T;
    };

    template<
        class T01, class T02, class T03,
        class T04, class T05, class T06,
        class T07, class T08, class T09,
        class T10, class T11
        >
    struct Typelist<
        T01, T02, T03,
        T04, T05, T06,
        T07, T08, T09,
        T10, T11, EmptyTypelist
        >
    {
        typedef Cons< T01, typename Typelist< T02, T03, T04, T05, T06, T07, T08, T09, T10, T11 >::T > T;
    };

    template<
        class T01, class T02, class T03,
        class T04, class T05, class T06,
        class T07, class T08, class T09,
        class T10, class T11, class T12
        >
    struct Typelist
        // <
        // T01, T02, T03,
        // T04, T05, T06,
        // T07, T08, T09,
        // T10, T11, T12
        // >
    {
        typedef Cons< T01, typename Typelist< T02, T03, T04, T05, T06, T07, T08, T09, T10, T11, T12 >::T > T;
    };


    // Utility macros for avoiding namespace verbosity in macro definitions:
    #define CPPX_TYPELIST           ::progrock::cppx::Typelist
    #define CPPX_EMPTY_TYPELIST     ::progrock::cppx::EmptyTypelist

} }  // namespace progrock::cppx

#endif
