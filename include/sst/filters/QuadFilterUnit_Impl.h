#include "QuadFilterUnit.h"
#include "FilterConfiguration.h"

//#include "SurgeStorage.h"

#include "sst/utilities/basic_dsp.h"
#include <iostream>

//#include "filters/VintageLadders.h"
//#include "filters/OBXDFilter.h"
//#include "filters/K35Filter.h"
//#include "filters/DiodeLadder.h"
//#include "filters/NonlinearFeedback.h"
//#include "filters/NonlinearStates.h"
//#include "filters/TriPoleFilter.h"

namespace sst::filters
{

inline __m128 SVFLP12Aquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]); // F1
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]); // Q1

    __m128 L = _mm_add_ps(f->R[1], _mm_mul_ps(f->C[0], f->R[0]));
    __m128 H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], f->R[0]));
    __m128 B = _mm_add_ps(f->R[0], _mm_mul_ps(f->C[0], H));

    __m128 L2 = _mm_add_ps(L, _mm_mul_ps(f->C[0], B));
    __m128 H2 = _mm_sub_ps(_mm_sub_ps(in, L2), _mm_mul_ps(f->C[1], B));
    __m128 B2 = _mm_add_ps(B, _mm_mul_ps(f->C[0], H2));

    f->R[0] = _mm_mul_ps(B2, f->R[2]);
    f->R[1] = _mm_mul_ps(L2, f->R[2]);

    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]);
    const __m128 m01 = _mm_set1_ps(0.1f);
    const __m128 m1 = _mm_set1_ps(1.0f);
    f->R[2] = _mm_max_ps(m01, _mm_sub_ps(m1, _mm_mul_ps(f->C[2], _mm_mul_ps(B, B))));

    f->C[3] = _mm_add_ps(f->C[3], f->dC[3]); // Gain
    return _mm_mul_ps(L2, f->C[3]);
}

inline __m128 SVFLP24Aquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]); // F1
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]); // Q1

    __m128 L = _mm_add_ps(f->R[1], _mm_mul_ps(f->C[0], f->R[0]));
    __m128 H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], f->R[0]));
    __m128 B = _mm_add_ps(f->R[0], _mm_mul_ps(f->C[0], H));

    L = _mm_add_ps(L, _mm_mul_ps(f->C[0], B));
    H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], B));
    B = _mm_add_ps(B, _mm_mul_ps(f->C[0], H));

    f->R[0] = _mm_mul_ps(B, f->R[2]);
    f->R[1] = _mm_mul_ps(L, f->R[2]);

    in = L;

    L = _mm_add_ps(f->R[4], _mm_mul_ps(f->C[0], f->R[3]));
    H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], f->R[3]));
    B = _mm_add_ps(f->R[3], _mm_mul_ps(f->C[0], H));

    L = _mm_add_ps(L, _mm_mul_ps(f->C[0], B));
    H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], B));
    B = _mm_add_ps(B, _mm_mul_ps(f->C[0], H));

    f->R[3] = _mm_mul_ps(B, f->R[2]);
    f->R[4] = _mm_mul_ps(L, f->R[2]);

    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]);
    const __m128 m01 = _mm_set1_ps(0.1f);
    const __m128 m1 = _mm_set1_ps(1.0f);
    f->R[2] = _mm_max_ps(m01, _mm_sub_ps(m1, _mm_mul_ps(f->C[2], _mm_mul_ps(B, B))));

    f->C[3] = _mm_add_ps(f->C[3], f->dC[3]); // Gain
    return _mm_mul_ps(L, f->C[3]);
}

inline __m128 SVFHP24Aquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]); // F1
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]); // Q1

    __m128 L = _mm_add_ps(f->R[1], _mm_mul_ps(f->C[0], f->R[0]));
    __m128 H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], f->R[0]));
    __m128 B = _mm_add_ps(f->R[0], _mm_mul_ps(f->C[0], H));

    L = _mm_add_ps(L, _mm_mul_ps(f->C[0], B));
    H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], B));
    B = _mm_add_ps(B, _mm_mul_ps(f->C[0], H));

    f->R[0] = _mm_mul_ps(B, f->R[2]);
    f->R[1] = _mm_mul_ps(L, f->R[2]);

    in = H;

    L = _mm_add_ps(f->R[4], _mm_mul_ps(f->C[0], f->R[3]));
    H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], f->R[3]));
    B = _mm_add_ps(f->R[3], _mm_mul_ps(f->C[0], H));

    L = _mm_add_ps(L, _mm_mul_ps(f->C[0], B));
    H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], B));
    B = _mm_add_ps(B, _mm_mul_ps(f->C[0], H));

    f->R[3] = _mm_mul_ps(B, f->R[2]);
    f->R[4] = _mm_mul_ps(L, f->R[2]);

    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]);
    const __m128 m01 = _mm_set1_ps(0.1f);
    const __m128 m1 = _mm_set1_ps(1.0f);
    f->R[2] = _mm_max_ps(m01, _mm_sub_ps(m1, _mm_mul_ps(f->C[2], _mm_mul_ps(B, B))));

    f->C[3] = _mm_add_ps(f->C[3], f->dC[3]); // Gain
    return _mm_mul_ps(H, f->C[3]);
}

inline __m128 SVFBP24Aquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]); // F1
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]); // Q1

    __m128 L = _mm_add_ps(f->R[1], _mm_mul_ps(f->C[0], f->R[0]));
    __m128 H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], f->R[0]));
    __m128 B = _mm_add_ps(f->R[0], _mm_mul_ps(f->C[0], H));

    L = _mm_add_ps(L, _mm_mul_ps(f->C[0], B));
    H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], B));
    B = _mm_add_ps(B, _mm_mul_ps(f->C[0], H));

    f->R[0] = _mm_mul_ps(B, f->R[2]);
    f->R[1] = _mm_mul_ps(L, f->R[2]);

    in = B;

    L = _mm_add_ps(f->R[4], _mm_mul_ps(f->C[0], f->R[3]));
    H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], f->R[3]));
    B = _mm_add_ps(f->R[3], _mm_mul_ps(f->C[0], H));

    L = _mm_add_ps(L, _mm_mul_ps(f->C[0], B));
    H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], B));
    B = _mm_add_ps(B, _mm_mul_ps(f->C[0], H));

    f->R[3] = _mm_mul_ps(B, f->R[2]);
    f->R[4] = _mm_mul_ps(L, f->R[2]);

    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]);
    const __m128 m01 = _mm_set1_ps(0.1f);
    const __m128 m1 = _mm_set1_ps(1.0f);
    f->R[2] = _mm_max_ps(m01, _mm_sub_ps(m1, _mm_mul_ps(f->C[2], _mm_mul_ps(B, B))));

    f->C[3] = _mm_add_ps(f->C[3], f->dC[3]); // Gain
    return _mm_mul_ps(B, f->C[3]);
}

inline __m128 SVFHP12Aquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]); // F1
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]); // Q1

    __m128 L = _mm_add_ps(f->R[1], _mm_mul_ps(f->C[0], f->R[0]));
    __m128 H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], f->R[0]));
    __m128 B = _mm_add_ps(f->R[0], _mm_mul_ps(f->C[0], H));

    __m128 L2 = _mm_add_ps(L, _mm_mul_ps(f->C[0], B));
    __m128 H2 = _mm_sub_ps(_mm_sub_ps(in, L2), _mm_mul_ps(f->C[1], B));
    __m128 B2 = _mm_add_ps(B, _mm_mul_ps(f->C[0], H2));

    f->R[0] = _mm_mul_ps(B2, f->R[2]);
    f->R[1] = _mm_mul_ps(L2, f->R[2]);

    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]);
    const __m128 m01 = _mm_set1_ps(0.1f);
    const __m128 m1 = _mm_set1_ps(1.0f);
    f->R[2] = _mm_max_ps(m01, _mm_sub_ps(m1, _mm_mul_ps(f->C[2], _mm_mul_ps(B, B))));

    f->C[3] = _mm_add_ps(f->C[3], f->dC[3]); // Gain
    return _mm_mul_ps(H2, f->C[3]);
}

inline __m128 SVFBP12Aquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]); // F1
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]); // Q1

    __m128 L = _mm_add_ps(f->R[1], _mm_mul_ps(f->C[0], f->R[0]));
    __m128 H = _mm_sub_ps(_mm_sub_ps(in, L), _mm_mul_ps(f->C[1], f->R[0]));
    __m128 B = _mm_add_ps(f->R[0], _mm_mul_ps(f->C[0], H));

    __m128 L2 = _mm_add_ps(L, _mm_mul_ps(f->C[0], B));
    __m128 H2 = _mm_sub_ps(_mm_sub_ps(in, L2), _mm_mul_ps(f->C[1], B));
    __m128 B2 = _mm_add_ps(B, _mm_mul_ps(f->C[0], H2));

    f->R[0] = _mm_mul_ps(B2, f->R[2]);
    f->R[1] = _mm_mul_ps(L2, f->R[2]);

    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]);
    const __m128 m01 = _mm_set1_ps(0.1f);
    const __m128 m1 = _mm_set1_ps(1.0f);
    f->R[2] = _mm_max_ps(m01, _mm_sub_ps(m1, _mm_mul_ps(f->C[2], _mm_mul_ps(B, B))));

    f->C[3] = _mm_add_ps(f->C[3], f->dC[3]); // Gain
    return _mm_mul_ps(B2, f->C[3]);
}

inline __m128 IIR12Aquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]);                                       // K2
    f->C[3] = _mm_add_ps(f->C[3], f->dC[3]);                                       // Q2
    __m128 f2 = _mm_sub_ps(_mm_mul_ps(f->C[3], in), _mm_mul_ps(f->C[1], f->R[1])); // Q2*in - K2*R1
    __m128 g2 = _mm_add_ps(_mm_mul_ps(f->C[1], in), _mm_mul_ps(f->C[3], f->R[1])); // K2*in + Q2*R1

    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]);                                       // K1
    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]);                                       // Q1
    __m128 f1 = _mm_sub_ps(_mm_mul_ps(f->C[2], f2), _mm_mul_ps(f->C[0], f->R[0])); // Q1*f2 - K1*R0
    __m128 g1 = _mm_add_ps(_mm_mul_ps(f->C[0], f2), _mm_mul_ps(f->C[2], f->R[0])); // K1*f2 + Q1*R0

    f->C[4] = _mm_add_ps(f->C[4], f->dC[4]); // V1
    f->C[5] = _mm_add_ps(f->C[5], f->dC[5]); // V2
    f->C[6] = _mm_add_ps(f->C[6], f->dC[6]); // V3
    __m128 y = _mm_add_ps(_mm_add_ps(_mm_mul_ps(f->C[6], g2), _mm_mul_ps(f->C[5], g1)),
                          _mm_mul_ps(f->C[4], f1));

    f->R[0] = f1;
    f->R[1] = g1;

    return y;
}

inline __m128 IIR12Bquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    __m128 f2 = _mm_sub_ps(_mm_mul_ps(f->C[3], in), _mm_mul_ps(f->C[1], f->R[1])); // Q2*in - K2*R1
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]);                                       // K2
    f->C[3] = _mm_add_ps(f->C[3], f->dC[3]);                                       // Q2
    __m128 g2 = _mm_add_ps(_mm_mul_ps(f->C[1], in), _mm_mul_ps(f->C[3], f->R[1])); // K2*in + Q2*R1

    __m128 f1 = _mm_sub_ps(_mm_mul_ps(f->C[2], f2), _mm_mul_ps(f->C[0], f->R[0])); // Q1*f2 - K1*R0
    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]);                                       // K1
    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]);                                       // Q1
    __m128 g1 = _mm_add_ps(_mm_mul_ps(f->C[0], f2), _mm_mul_ps(f->C[2], f->R[0])); // K1*f2 + Q1*R0

    f->C[4] = _mm_add_ps(f->C[4], f->dC[4]); // V1
    f->C[5] = _mm_add_ps(f->C[5], f->dC[5]); // V2
    f->C[6] = _mm_add_ps(f->C[6], f->dC[6]); // V3
    __m128 y = _mm_add_ps(_mm_add_ps(_mm_mul_ps(f->C[6], g2), _mm_mul_ps(f->C[5], g1)),
                          _mm_mul_ps(f->C[4], f1));

    f->R[0] = _mm_mul_ps(f1, f->R[2]);
    f->R[1] = _mm_mul_ps(g1, f->R[2]);

    f->C[7] = _mm_add_ps(f->C[7], f->dC[7]); // Clipgain
    const __m128 m01 = _mm_set1_ps(0.1f);
    const __m128 m1 = _mm_set1_ps(1.0f);

    f->R[2] = _mm_max_ps(m01, _mm_sub_ps(m1, _mm_mul_ps(f->C[7], _mm_mul_ps(y, y))));

    return y;
}

inline __m128 IIR12WDFquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]); // E1 * sc
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]); // E2 * sc
    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]); // -E1 / sc
    f->C[3] = _mm_add_ps(f->C[3], f->dC[3]); // -E2 / sc
    f->C[4] = _mm_add_ps(f->C[4], f->dC[4]); // C1
    f->C[5] = _mm_add_ps(f->C[5], f->dC[5]); // C2
    f->C[6] = _mm_add_ps(f->C[6], f->dC[6]); // D

    __m128 y = _mm_add_ps(_mm_add_ps(_mm_mul_ps(f->C[4], f->R[0]), _mm_mul_ps(f->C[6], in)),
                          _mm_mul_ps(f->C[5], f->R[1]));
    __m128 t =
        _mm_add_ps(in, _mm_add_ps(_mm_mul_ps(f->C[2], f->R[0]), _mm_mul_ps(f->C[3], f->R[1])));

    __m128 s1 = _mm_add_ps(_mm_mul_ps(t, f->C[0]), f->R[0]);
    __m128 s2 = _mm_sub_ps(_mm_setzero_ps(), _mm_add_ps(_mm_mul_ps(t, f->C[1]), f->R[1]));

    // f->R[0] = s1;
    // f->R[1] = s2;

    f->R[0] = _mm_mul_ps(s1, f->R[2]);
    f->R[1] = _mm_mul_ps(s2, f->R[2]);

    f->C[7] = _mm_add_ps(f->C[7], f->dC[7]); // Clipgain
    const __m128 m01 = _mm_set1_ps(0.1f);
    const __m128 m1 = _mm_set1_ps(1.0f);
    f->R[2] = _mm_max_ps(m01, _mm_sub_ps(m1, _mm_mul_ps(f->C[7], _mm_mul_ps(y, y))));

    return y;
}

inline __m128 IIR12CFCquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    // State-space with clipgain (2nd order, limit within register)

    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]); // ar
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]); // ai
    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]); // b1
    f->C[4] = _mm_add_ps(f->C[4], f->dC[4]); // c1
    f->C[5] = _mm_add_ps(f->C[5], f->dC[5]); // c2
    f->C[6] = _mm_add_ps(f->C[6], f->dC[6]); // d

    // y(i) = c1.*s(1) + c2.*s(2) + d.*x(i);
    // s1 = ar.*s(1) - ai.*s(2) + x(i);
    // s2 = ai.*s(1) + ar.*s(2);

    __m128 y = _mm_add_ps(_mm_add_ps(_mm_mul_ps(f->C[4], f->R[0]), _mm_mul_ps(f->C[6], in)),
                          _mm_mul_ps(f->C[5], f->R[1]));
    __m128 s1 = _mm_add_ps(_mm_mul_ps(in, f->C[2]),
                           _mm_sub_ps(_mm_mul_ps(f->C[0], f->R[0]), _mm_mul_ps(f->C[1], f->R[1])));
    __m128 s2 = _mm_add_ps(_mm_mul_ps(f->C[1], f->R[0]), _mm_mul_ps(f->C[0], f->R[1]));

    f->R[0] = _mm_mul_ps(s1, f->R[2]);
    f->R[1] = _mm_mul_ps(s2, f->R[2]);

    f->C[7] = _mm_add_ps(f->C[7], f->dC[7]); // Clipgain
    const __m128 m01 = _mm_set1_ps(0.1f);
    const __m128 m1 = _mm_set1_ps(1.0f);
    f->R[2] = _mm_max_ps(m01, _mm_sub_ps(m1, _mm_mul_ps(f->C[7], _mm_mul_ps(y, y))));

    return y;
}

inline __m128 IIR12CFLquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    // State-space with softer limiter

    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]); // (ar)
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]); // (ai)
    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]); // b1
    f->C[4] = _mm_add_ps(f->C[4], f->dC[4]); // c1
    f->C[5] = _mm_add_ps(f->C[5], f->dC[5]); // c2
    f->C[6] = _mm_add_ps(f->C[6], f->dC[6]); // d

    // y(i) = c1.*s(1) + c2.*s(2) + d.*x(i);
    // s1 = ar.*s(1) - ai.*s(2) + x(i);
    // s2 = ai.*s(1) + ar.*s(2);

    __m128 y = _mm_add_ps(_mm_add_ps(_mm_mul_ps(f->C[4], f->R[0]), _mm_mul_ps(f->C[6], in)),
                          _mm_mul_ps(f->C[5], f->R[1]));
    __m128 ar = _mm_mul_ps(f->C[0], f->R[2]);
    __m128 ai = _mm_mul_ps(f->C[1], f->R[2]);
    __m128 s1 = _mm_add_ps(_mm_mul_ps(in, f->C[2]),
                           _mm_sub_ps(_mm_mul_ps(ar, f->R[0]), _mm_mul_ps(ai, f->R[1])));
    __m128 s2 = _mm_add_ps(_mm_mul_ps(ai, f->R[0]), _mm_mul_ps(ar, f->R[1]));

    f->R[0] = s1;
    f->R[1] = s2;

    /*m = 1 ./ max(1,abs(y(i)));
    mr = mr.*0.99 + m.*0.01;*/

    // Limiter
    const __m128 m001 = _mm_set1_ps(0.001f);
    const __m128 m099 = _mm_set1_ps(0.999f);
    const __m128 m1 = _mm_set1_ps(1.0f);
    const __m128 m2 = _mm_set1_ps(2.0f);

    __m128 m = _mm_rsqrt_ps(_mm_max_ps(m1, _mm_mul_ps(m2, _mm_and_ps(y, m128_mask_absval))));
    f->R[2] = _mm_add_ps(_mm_mul_ps(f->R[2], m099), _mm_mul_ps(m, m001));

    return y;
}

inline __m128 IIR24CFCquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    // State-space with clipgain (2nd order, limit within register)

    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]); // ar
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]); // ai
    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]); // b1

    f->C[4] = _mm_add_ps(f->C[4], f->dC[4]); // c1
    f->C[5] = _mm_add_ps(f->C[5], f->dC[5]); // c2
    f->C[6] = _mm_add_ps(f->C[6], f->dC[6]); // d

    __m128 y = _mm_add_ps(_mm_add_ps(_mm_mul_ps(f->C[4], f->R[0]), _mm_mul_ps(f->C[6], in)),
                          _mm_mul_ps(f->C[5], f->R[1]));
    __m128 s1 = _mm_add_ps(_mm_mul_ps(in, f->C[2]),
                           _mm_sub_ps(_mm_mul_ps(f->C[0], f->R[0]), _mm_mul_ps(f->C[1], f->R[1])));
    __m128 s2 = _mm_add_ps(_mm_mul_ps(f->C[1], f->R[0]), _mm_mul_ps(f->C[0], f->R[1]));

    f->R[0] = _mm_mul_ps(s1, f->R[2]);
    f->R[1] = _mm_mul_ps(s2, f->R[2]);

    __m128 y2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(f->C[4], f->R[3]), _mm_mul_ps(f->C[6], y)),
                           _mm_mul_ps(f->C[5], f->R[4]));
    __m128 s3 = _mm_add_ps(_mm_mul_ps(y, f->C[2]),
                           _mm_sub_ps(_mm_mul_ps(f->C[0], f->R[3]), _mm_mul_ps(f->C[1], f->R[4])));
    __m128 s4 = _mm_add_ps(_mm_mul_ps(f->C[1], f->R[3]), _mm_mul_ps(f->C[0], f->R[4]));

    f->R[3] = _mm_mul_ps(s3, f->R[2]);
    f->R[4] = _mm_mul_ps(s4, f->R[2]);

    f->C[7] = _mm_add_ps(f->C[7], f->dC[7]); // Clipgain
    const __m128 m01 = _mm_set1_ps(0.1f);
    const __m128 m1 = _mm_set1_ps(1.0f);
    f->R[2] = _mm_max_ps(m01, _mm_sub_ps(m1, _mm_mul_ps(f->C[7], _mm_mul_ps(y2, y2))));

    return y2;
}

inline __m128 IIR24CFLquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    // State-space with softer limiter

    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]); // (ar)
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]); // (ai)
    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]); // b1
    f->C[4] = _mm_add_ps(f->C[4], f->dC[4]); // c1
    f->C[5] = _mm_add_ps(f->C[5], f->dC[5]); // c2
    f->C[6] = _mm_add_ps(f->C[6], f->dC[6]); // d

    __m128 ar = _mm_mul_ps(f->C[0], f->R[2]);
    __m128 ai = _mm_mul_ps(f->C[1], f->R[2]);

    __m128 y = _mm_add_ps(_mm_add_ps(_mm_mul_ps(f->C[4], f->R[0]), _mm_mul_ps(f->C[6], in)),
                          _mm_mul_ps(f->C[5], f->R[1]));
    __m128 s1 = _mm_add_ps(_mm_mul_ps(in, f->C[2]),
                           _mm_sub_ps(_mm_mul_ps(ar, f->R[0]), _mm_mul_ps(ai, f->R[1])));
    __m128 s2 = _mm_add_ps(_mm_mul_ps(ai, f->R[0]), _mm_mul_ps(ar, f->R[1]));

    f->R[0] = s1;
    f->R[1] = s2;

    __m128 y2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(f->C[4], f->R[3]), _mm_mul_ps(f->C[6], y)),
                           _mm_mul_ps(f->C[5], f->R[4]));
    __m128 s3 = _mm_add_ps(_mm_mul_ps(y, f->C[2]),
                           _mm_sub_ps(_mm_mul_ps(ar, f->R[3]), _mm_mul_ps(ai, f->R[4])));
    __m128 s4 = _mm_add_ps(_mm_mul_ps(ai, f->R[3]), _mm_mul_ps(ar, f->R[4]));

    f->R[3] = s3;
    f->R[4] = s4;

    /*m = 1 ./ max(1,abs(y(i)));
    mr = mr.*0.99 + m.*0.01;*/

    // Limiter
    const __m128 m001 = _mm_set1_ps(0.001f);
    const __m128 m099 = _mm_set1_ps(0.999f);
    const __m128 m1 = _mm_set1_ps(1.0f);
    const __m128 m2 = _mm_set1_ps(2.0f);

    __m128 m = _mm_rsqrt_ps(_mm_max_ps(m1, _mm_mul_ps(m2, _mm_and_ps(y2, m128_mask_absval))));
    f->R[2] = _mm_add_ps(_mm_mul_ps(f->R[2], m099), _mm_mul_ps(m, m001));

    return y2;
}

inline __m128 IIR24Bquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]); // K2
    f->C[3] = _mm_add_ps(f->C[3], f->dC[3]); // Q2
    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]); // K1
    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]); // Q1
    f->C[4] = _mm_add_ps(f->C[4], f->dC[4]); // V1
    f->C[5] = _mm_add_ps(f->C[5], f->dC[5]); // V2
    f->C[6] = _mm_add_ps(f->C[6], f->dC[6]); // V3

    __m128 f2 = _mm_sub_ps(_mm_mul_ps(f->C[3], in), _mm_mul_ps(f->C[1], f->R[1])); // Q2*in - K2*R1
    __m128 g2 = _mm_add_ps(_mm_mul_ps(f->C[1], in), _mm_mul_ps(f->C[3], f->R[1])); // K2*in + Q2*R1
    __m128 f1 = _mm_sub_ps(_mm_mul_ps(f->C[2], f2), _mm_mul_ps(f->C[0], f->R[0])); // Q1*f2 - K1*R0
    __m128 g1 = _mm_add_ps(_mm_mul_ps(f->C[0], f2), _mm_mul_ps(f->C[2], f->R[0])); // K1*f2 + Q1*R0
    f->R[0] = _mm_mul_ps(f1, f->R[4]);
    f->R[1] = _mm_mul_ps(g1, f->R[4]);
    __m128 y1 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(f->C[6], g2), _mm_mul_ps(f->C[5], g1)),
                           _mm_mul_ps(f->C[4], f1));

    f2 = _mm_sub_ps(_mm_mul_ps(f->C[3], y1), _mm_mul_ps(f->C[1], f->R[3])); // Q2*in - K2*R1
    g2 = _mm_add_ps(_mm_mul_ps(f->C[1], y1), _mm_mul_ps(f->C[3], f->R[3])); // K2*in + Q2*R1
    f1 = _mm_sub_ps(_mm_mul_ps(f->C[2], f2), _mm_mul_ps(f->C[0], f->R[2])); // Q1*f2 - K1*R0
    g1 = _mm_add_ps(_mm_mul_ps(f->C[0], f2), _mm_mul_ps(f->C[2], f->R[2])); // K1*f2 + Q1*R0
    f->R[2] = _mm_mul_ps(f1, f->R[4]);
    f->R[3] = _mm_mul_ps(g1, f->R[4]);
    __m128 y2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(f->C[6], g2), _mm_mul_ps(f->C[5], g1)),
                           _mm_mul_ps(f->C[4], f1));

    f->C[7] = _mm_add_ps(f->C[7], f->dC[7]); // Clipgain
    const __m128 m01 = _mm_set1_ps(0.1f);
    const __m128 m1 = _mm_set1_ps(1.0f);
    f->R[4] = _mm_max_ps(m01, _mm_sub_ps(m1, _mm_mul_ps(f->C[7], _mm_mul_ps(y2, y2))));

    return y2;
}

inline __m128 LPMOOGquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]);
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]);
    f->C[2] = _mm_add_ps(f->C[2], f->dC[2]);

    f->R[0] = softclip8_ps(_mm_add_ps(
        f->R[0],
        _mm_mul_ps(f->C[1],
                   _mm_sub_ps(_mm_sub_ps(_mm_mul_ps(in, f->C[0]),
                                         _mm_mul_ps(f->C[2], _mm_add_ps(f->R[3], f->R[4]))),
                              f->R[0]))));
    f->R[1] = _mm_add_ps(f->R[1], _mm_mul_ps(f->C[1], _mm_sub_ps(f->R[0], f->R[1])));
    f->R[2] = _mm_add_ps(f->R[2], _mm_mul_ps(f->C[1], _mm_sub_ps(f->R[1], f->R[2])));
    f->R[4] = f->R[3];
    f->R[3] = _mm_add_ps(f->R[3], _mm_mul_ps(f->C[1], _mm_sub_ps(f->R[2], f->R[3])));

    return f->R[f->WP[0] & 3];
}

inline __m128 SNHquad(QuadFilterUnitState *__restrict f, __m128 in)
{
    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]);
    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]);

    f->R[0] = _mm_add_ps(f->R[0], f->C[0]);

    __m128 mask = _mm_cmpgt_ps(f->R[0], _mm_setzero_ps());

    f->R[1] =
        _mm_or_ps(_mm_andnot_ps(mask, f->R[1]),
                  _mm_and_ps(mask, softclip_ps(_mm_sub_ps(in, _mm_mul_ps(f->C[1], f->R[1])))));

    const __m128 m1 = _mm_set1_ps(-1.f);
    f->R[0] = _mm_add_ps(f->R[0], _mm_and_ps(m1, mask));

    return f->R[1];
}

// @TODO: get this to compile
//template <int COMB_SIZE> // COMB_SIZE must be a power of 2
//__m128 COMBquad_SSE2(QuadFilterUnitState *__restrict f, __m128 in)
//{
//    assert(FIRipol_M == 256); // changing the constant requires updating the code below
//    const __m128 m256 = _mm_set1_ps(256.f);
//    const __m128i m0xff = _mm_set1_epi32(0xff);
//
//    f->C[0] = _mm_add_ps(f->C[0], f->dC[0]);
//    f->C[1] = _mm_add_ps(f->C[1], f->dC[1]);
//
//    __m128 a = _mm_mul_ps(f->C[0], m256);
//    __m128i e = _mm_cvtps_epi32(a);
//    int DTi alignas(16)[4], SEi alignas(16)[4];
//    __m128i DT = _mm_srli_epi32(e, 8);
//    _mm_store_si128((__m128i *)DTi, DT);
//    __m128i SE = _mm_and_si128(e, m0xff);
//    SE = _mm_sub_epi32(m0xff, SE);
//    _mm_store_si128((__m128i *)SEi, SE);
//    __m128 DBRead = _mm_setzero_ps();
//
//    for (int i = 0; i < 4; i++)
//    {
//        if (f->active[i])
//        {
//            int RP = (f->WP[i] - DTi[i] - FIRoffset) & (COMB_SIZE - 1);
//
//            // SINC interpolation (12 samples)
//            __m128 a = _mm_loadu_ps(&f->DB[i][RP]);
//            SEi[i] *= (FIRipol_N << 1);
//            __m128 b = _mm_load_ps(&sinctable[SEi[i]]);
//            __m128 o = _mm_mul_ps(a, b);
//
//            a = _mm_loadu_ps(&f->DB[i][RP + 4]);
//            b = _mm_load_ps(&sinctable[SEi[i] + 4]);
//            o = _mm_add_ps(o, _mm_mul_ps(a, b));
//
//            a = _mm_loadu_ps(&f->DB[i][RP + 8]);
//            b = _mm_load_ps(&sinctable[SEi[i] + 8]);
//            o = _mm_add_ps(o, _mm_mul_ps(a, b));
//
//            _mm_store_ss((float *)&DBRead + i, sum_ps_to_ss(o));
//        }
//    }
//
//    __m128 d = _mm_add_ps(in, _mm_mul_ps(DBRead, f->C[1]));
//    d = softclip_ps(d);
//
//    for (int i = 0; i < 4; i++)
//    {
//        if (f->active[i])
//        {
//            // Write to delaybuffer (with "anti-wrapping")
//            __m128 t = _mm_load_ss((float *)&d + i);
//            _mm_store_ss(&f->DB[i][f->WP[i]], t);
//            if (f->WP[i] < FIRipol_N)
//                _mm_store_ss(&f->DB[i][f->WP[i] + COMB_SIZE], t);
//
//            // Increment write position
//            f->WP[i] = (f->WP[i] + 1) & (COMB_SIZE - 1);
//        }
//    }
//    return _mm_add_ps(_mm_mul_ps(f->C[3], DBRead), _mm_mul_ps(f->C[2], in));
//}

inline FilterUnitQFPtr GetQFPtrFilterUnit(FilterType type, FilterSubType subtype)
{
    switch (type)
    {
    // basic filter types
    case fut_lp12:
    {
        switch (subtype)
        {
        case st_SVF:
            return SVFLP12Aquad;
        case st_Rough:
            return IIR12CFCquad;
        case st_Smooth:
        default:
            return IIR12Bquad;
        }
    }
    case fut_lp24:
    {
        switch (subtype)
        {
        case st_SVF:
            return SVFLP24Aquad;
        case st_Rough:
            return IIR24CFCquad;
        case st_Smooth:
        default:
            return IIR24Bquad;
        }
    }
    case fut_hp12:
    {
        switch (subtype)
        {
        case st_SVF:
            return SVFHP12Aquad;
        case st_Rough:
            return IIR12CFCquad;
        case st_Smooth:
        default:
            return IIR12Bquad;
        }
    }
    case fut_hp24:
    {
        switch (subtype)
        {
        case st_SVF:
            return SVFHP24Aquad;
        case st_Rough:
            return IIR24CFCquad;
        case st_Smooth:
        default:
            return IIR24Bquad;
        }
    }
    case fut_bp12:
    {
        switch (subtype)
        {
        case st_SVF:
            return SVFBP12Aquad;
        case st_Rough:
            return IIR12CFCquad;
        case st_Smooth:
        default:
            return IIR12Bquad;
        }
    }
    case fut_bp24:
    {
        switch (subtype)
        {
        case st_SVF:
            return SVFBP24Aquad;
        case st_Rough:
            return IIR24CFCquad;
        case st_Smooth:
        default:
            return IIR24Bquad;
        }
    }
    case fut_notch12:
        return IIR12Bquad;
    case fut_notch24:
        return IIR24Bquad;
    case fut_apf:
        return IIR12Bquad;

    // next filter types...
    case fut_lpmoog:
        return LPMOOGquad;
    case fut_SNH:
        return SNHquad;
        //    case fut_comb_pos:
        //    case fut_comb_neg:
        //        if (subtype & QFUSubtypeMasks::EXTENDED_COMB)
        //        {
        //            return COMBquad_SSE2<MAX_FB_COMB_EXTENDED>;
        //        }
        //        else
        //        {
        //            return COMBquad_SSE2<MAX_FB_COMB>;
        //        }
        //    case fut_vintageladder:
        //        switch (subtype)
        //        {
        //        case 0:
        //        case 1:
        //            return VintageLadder::RK::process;
        //        case 2:
        //        case 3:
        //            return VintageLadder::Huov::process;
        //        }
        //        break;
        //    case fut_obxd_2pole_lp:
        //    case fut_obxd_2pole_hp:
        //    case fut_obxd_2pole_bp:
        //    case fut_obxd_2pole_n:
        //        // All the differences are in subtype wrangling int he coefficnent maker
        //        return OBXDFilter::process_2_pole;
        //        break;
        //    case fut_obxd_4pole:
        //        return OBXDFilter::process_4_pole;
        //        break;
        //    case fut_k35_lp:
        //        return K35Filter::process_lp;
        //        break;
        //    case fut_k35_hp:
        //        return K35Filter::process_hp;
        //        break;
        //    case fut_diode:
        //        return DiodeLadderFilter::process;
        //        break;
        //    case fut_cutoffwarp_lp:
        //    case fut_cutoffwarp_hp:
        //    case fut_cutoffwarp_n:
        //    case fut_cutoffwarp_bp:
        //    case fut_cutoffwarp_ap:
        //        return NonlinearFeedbackFilter::process;
        //        break;
        //    case fut_resonancewarp_lp:
        //    case fut_resonancewarp_hp:
        //    case fut_resonancewarp_n:
        //    case fut_resonancewarp_bp:
        //    case fut_resonancewarp_ap:
        //        return NonlinearStatesFilter::process;
        //        break;
        //    case fut_tripole:
        //        return TriPoleFilter::process;
        //        break;
    case fut_none:
    case num_filter_types:
        return nullptr;
    }
}

}
