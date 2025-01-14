// (c) 2023, Cary Clark cclark2@gmail.com
// new interface idea

#include "PathOpsTypes.h"

namespace PathOpsV0Lib {

inline void noBounds(Curve , OpRect& ) {
}

inline bool noNearly(Curve ) {
    return false;
}

inline OpPoint noHull(Curve , int index) {
    OP_ASSERT(0); // should never be called
    return OpPoint();
}

inline bool noLinear(Curve ) {
    OP_ASSERT(0); // should never be called
    return false;
}

inline void noPinCtrl(Curve) {
}

inline void noReverse(Curve ) {
}

inline void noRotate(Curve , const LinePts& , float , float , Curve ) {
}

#if OP_DEBUG_DUMP
inline std::string noDumpCurveExtra(Curve , DebugLevel , DebugBase ) {
    return "";
}

inline void noDumpCurveSet(Curve , const char*& ) {
}

inline void noDumpCurveSetExtra(Curve , const char*& str) {
}
#endif

}
