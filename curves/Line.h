// (c) 2023, Cary Clark cclark2@gmail.com
// new interface idea

#include "PathOpsTypes.h"

namespace PathOpsV0Lib {

inline size_t linePtCount() {
    return 2;
}

inline bool lineIsFinite(PathOpsV0Lib::Curve) {
    return true;
}

inline bool lineIsLine(PathOpsV0Lib::Curve ) {
    return true;
}

inline OpRoots lineAxisRawHit(PathOpsV0Lib::Curve c, Axis axis, float axisIntercept, MatchEnds ends) {
    const float* ptr = c.data->start.asPtr(axis);
    if (!OpMath::Between(ptr[0], axisIntercept, ptr[2]))
        return OpRoots(axisIntercept < ptr[0] ? RootFail::outsideFirstPt : RootFail::outsideLastPt);
    // strict equality fails for denomalized numbers
    // if (ptr[2] == ptr[0]) {
    if (fabsf(ptr[2] - ptr[0]) <= OpEpsilon)   // coincident line values are computed later
        return OpRoots(OpNaN, OpNaN);
    return OpRoots((axisIntercept - ptr[0]) / (ptr[2] - ptr[0]));
}

inline OpPoint linePtAtT(PathOpsV0Lib::Curve c, float t) {
    if (0 == t)
        return c.data->start;
    if (1 == t)
        return c.data->end;
    return (1 - t) * c.data->start + t * c.data->end;
    return OpPoint();
}

inline OpPair lineXYAtT(PathOpsV0Lib::Curve c, OpPair t, XyChoice xyChoice) {
    return (1 - t) * c.data->start.choice(xyChoice) + t * c.data->end.choice(xyChoice);
}

inline bool linesEqual(PathOpsV0Lib::Curve , PathOpsV0Lib::Curve ) {
    return true;
}

inline OpVector lineTangent(PathOpsV0Lib::Curve c, float ) {
    return c.data->end - c.data->start;
}

inline OpVector lineNormal(PathOpsV0Lib::Curve c, float ) {
    return { c.data->start.y - c.data->end.y, c.data->end.x - c.data->start.x };
}

inline void lineSubDivide(PathOpsV0Lib::Curve , OpPtT ptT1, OpPtT ptT2, PathOpsV0Lib::Curve result) {
    result.data->start = ptT1.pt;
    result.data->end = ptT2.pt;
}

#if 0 && OP_DEBUG_DUMP
inline size_t lineDebugDumpSize() {
    return offsetof(CurveData, optionalAdditionalData);
}
#endif


}
