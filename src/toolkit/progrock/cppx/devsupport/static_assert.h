// Copyright (c) Alf P. Steinbach, 2010.
// A pared-down version for this article.
// #include <progrock/cppx/devsupport/static_assert.h>

#ifndef PROGROCK_CPPX_DEVSUPPORT_STATICASSERT_H
#define PROGROCK_CPPX_DEVSUPPORT_STATICASSERT_H

#define CPPX_STATIC_ASSERT( e ) \
    typedef char CppxStaticAssertShouldBeTrue[(e)? 1 : -1]

#endif
