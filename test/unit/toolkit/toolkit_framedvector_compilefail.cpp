// Each CASE_* define selects one misuse of the frame-tagged types,
// which has to be rejected by the compiler. CASE_NONE has to compile.

#include "base/framedvector.h"

using namespace insight;

INSIGHT_DECLARE_FRAME(LocalA);
INSIGHT_DECLARE_FRAME(LocalB);

void use()
{
    PointIn<GlobalFrame> pg(1, 2, 3);
    PointIn<LocalA> pa(1, 2, 3);
    VectorIn<GlobalFrame> vg(1, 0, 0);
    VectorIn<LocalA> va(1, 0, 0);
    FrameTransformation<LocalA,GlobalFrame> tAG;
    FrameTransformation<LocalB,LocalA> tBA;

#if defined(CASE_NONE)
    VectorIn<GlobalFrame> d = pg - tAG(pa);
    PointIn<GlobalFrame> p = pg + 2.*vg;
    FrameTransformation<LocalB,GlobalFrame> tBG = tAG*tBA;
    (void)d; (void)p; (void)tBG; (void)va;
#elif defined(CASE_POINT_MINUS_OTHERFRAME)
    auto r = pg - pa;
#elif defined(CASE_VECTOR_PLUS_OTHERFRAME)
    auto r = vg + va;
#elif defined(CASE_POINT_PLUS_POINT)
    auto r = pg + pg;
#elif defined(CASE_SCALAR_TIMES_POINT)
    auto r = 2.*pg;
#elif defined(CASE_TRSF_WRONG_FRAME)
    auto r = tAG(pg);
#elif defined(CASE_COMPOSE_MISMATCH)
    auto r = tBA*tAG;
#else
#error "no test case selected"
#endif
}
