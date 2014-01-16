// Copyright (c) Alf P. Steinbach, 2010.
// #include <progrock/cppx/typechecking.h>

#ifndef PROGROCK_CPPX_TYPECHECKING_H
#define PROGROCK_CPPX_TYPECHECKING_H
#include <progrock/cppx/devsupport/better_experience.h>


//------------------------------------- Dependencies:

#include "macro_util.h"     // C_ as comma for macro invocations with template params.



//------------------------------------- Interface:

namespace progrock{ namespace cppx{

    //--------------------------------------- Template metaprogramming support:

    typedef char                        TrueType;
    typedef struct { char dummy[2]; }   FalseType;

    CPPX_STATIC_ASSERT( sizeof( TrueType ) == 1 );
    CPPX_STATIC_ASSERT( sizeof( FalseType ) > 1 );

    #define CPPX_SIZE_EQ( a, b )            (sizeof( a ) == sizeof( b ))
    #define CPPX_SIZE_TRUE( a )             CPPX_SIZE_EQ( a, ::progrock::cppx::TrueType )
    #define CPPX_SIZE_FALSE( a )            CPPX_SIZE_EQ( a, ::progrock::cppx::FalseType )
    #define CPPX_IS( Relation, A, B )       Relation< A, B >::yes
    #define CPPX_IS_( Relation, A, B )      CPPX_IS( ::progrock::cppx::##Relation, A, B )
    #define CPPX_IS_A( Kind, T )            Kind< T >::yes
    #define CPPX_IS_A_( Kind, T )           CPPX_IS_A( ::progrock::cppx::##Kind, T )


    //--------------------------------------- SameType

    template< typename TypeA, typename TypeB >
    struct SameType                 { enum { yes = false }; };

    template< typename Type >
    struct SameType< Type, Type >   { enum { yes = true }; };


    //--------------------------------------- Peeling off outer types:

    template< typename Type > struct ReferentType;

    template< typename Type >
    struct ReferentType< Type& >        { typedef Type T; };

    template< typename Type >
    struct ReferentType< Type* >        { typedef Type T; };

    template< typename Type >
    struct NonConstType                 { typedef Type T; };

    template< typename Type >
    struct NonConstType< Type const >   { typedef Type T; };


    //--------------------------------------- Transferring const qualifier:

    template< typename Qualified, typename Unqualified >
    struct WithConstLike
    {
        typedef typename NonConstType< Unqualified >::T     T;
    };

    template< typename Qualified, typename Unqualified >
    struct WithConstLike< Qualified const, Unqualified >
    {
        typedef Unqualified const   T;
    };


    //--------------------------------------- PtrConvertible

    template< class Specific, class General >
    class PtrConvertible
    {
    private:
        static Specific* specificPtr();
        static TrueType convertsToGeneral( General* );
        static FalseType convertsToGeneral( ... );
    public:
        enum{ yes = CPPX_SIZE_EQ( convertsToGeneral( specificPtr() ), TrueType ) };
    };


    //--------------------------------------- DerivedAndBase

    enum Inclusive {};
    enum Exclusive {};

    template< class Derived, class Base, typename Strictness = Exclusive >
    class DerivedAndBase
    {
    private:
        typedef typename NonConstType< Base >::T    UnqualifiedBase;
    public:
        enum{ yes =
            CPPX_IS( PtrConvertible, Derived, Base ) &&
            !CPPX_IS( SameType, UnqualifiedBase, void )
            };
    };

    template< class Derived, typename Strictness >
    class DerivedAndBase< Derived, Derived, Strictness >
    { public: enum{ yes = CPPX_IS( SameType, Strictness, Inclusive ) }; };


    //--------------------------------------- DerivedAndBaseOrSame

    template< class Derived, class Base >
    struct DerivedAndBaseOrSame
    { enum{ yes = DerivedAndBase< Derived, Base, Inclusive >::yes }; };


    //--------------------------------------- downcast

    template< class Derived, class Base >
    inline Derived* downcast( Base* p )
    {
        CPPX_STATIC_ASSERT( CPPX_IS( DerivedAndBase, Derived, Base ) );
        return static_cast< Derived* >( p );
    }

    template< class Derived, class Base >
    inline Derived& downcast( Base& r )
    {
        CPPX_STATIC_ASSERT( CPPX_IS( DerivedAndBase, Derived, Base ) );
        return static_cast< Derived& >( r );
    }

    #define CPPX_DOWNCAST_TO( Derived, v ) \
        ::progrock::cppx::downcast< Derived >( v )

}}  // namespace progrock::cppx


#endif
