#include "OpDebug.h"

#if OP_DEBUG || OP_DEBUG_IMAGE || OP_DEBUG_DUMP
OpContours* debugGlobalContours;
#endif

#if OP_DEBUG || OP_RELEASE_TEST
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

void OpPrintOut(const std::string& s) {
#ifdef _WIN32
    OutputDebugStringA(s.c_str());
    FILE* out = fopen("out.txt", "a+");
    fprintf(out, "%s", s.c_str());
    fclose(out);
#else
    fprintf(stderr, "%s", s.c_str());
#endif
}

uint64_t OpInitTimer() {
#ifdef _WIN32
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
#else
// #error "unimplmented"
    return 0;
#endif
}

uint64_t OpReadTimer() {
#ifdef _WIN32
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return time.QuadPart;
#else
// #error "unimplmented"
    return 0;
#endif
}

float OpTicksToSeconds(uint64_t diff, uint64_t frequency) {
#ifdef _WIN32
    return (float) (diff * 1000000 / frequency) / 1000000;
#else
// #error "unimplmented"
    return 0;
#endif
}

#endif

#if !defined(NDEBUG) || OP_RELEASE_TEST
void OpDebugOut(const std::string& s) {
#ifdef _WIN32
    OutputDebugStringA(s.c_str());
#else
    fprintf(stderr, "%s", s.c_str());
#endif
}
#endif

#if OP_DEBUG || OP_DEBUG_DUMP || OP_DEBUG_IMAGE

#include "OpCurve.h"

union FloatIntUnion {
    float   f;
    int32_t i;
};

float OpDebugBitsToFloat(int32_t i) {
    FloatIntUnion d;
    d.i = i;
    return d.f;
}

std::string OpDebugDump(float f) {
    if (OpMath::IsNaN(f))
        return "NaN";
    if (!OpMath::IsFinite(f))
        return f > 0 ? "Inf" : "-Inf";
    char buffer[20];
    int precision = 8;
    float copyF = f;
    while (copyF >= 10) {
        if (0 == --precision)
            break;
        copyF /= 10;
    }
    int size = snprintf(buffer, sizeof(buffer), "%.*g", precision, f);
    std::string s(buffer, size);
//    while ('0' == s.back())
//        s.pop_back();
//    if ('.' == s.back())
//        s.pop_back();
    if ("1" == s && 1 != f)
        s = "~" + s;
    else if ("0" == s && 0 != f)
        s = "~" + s;
    return s;
}

std::string OpDebugDumpHex(float f) {
    std::string s = "0x";
    int32_t hex = OpDebugFloatToBits(f);
    for (int index = 28; index >= 0; index -= 4) {
        int nybble = (hex >> index) & 0xF;
        if (nybble <= 9)
            s += '0' + nybble;
        else 
            s += 'a' + nybble - 10;
    }
    return s;
}

void OpDumpHex(float f) {
    OpDebugOut(OpDebugDumpHex(f));
}

std::string OpDebugDumpHexToFloat(float f) {
    return "OpDebugBitsToFloat(" + OpDebugDumpHex(f) + ")";    
}

int32_t OpDebugFloatToBits(float f) {
    FloatIntUnion d;
    d.f = f;
    return d.i;
}

float OpDebugHexToFloat(const char*& str) {
    FloatIntUnion d;
    d.i = OpDebugHexToInt(str);
    return d.f;
}

int32_t OpDebugHexToInt(const char*& str) {
    int32_t result = 0;
    OpDebugSkip(str, "0x");
    for (int index = 0; index < 8; ++index) {
        char c = *str++;
        int nybble = c - '0';
        if (nybble > 9)
            nybble = c - 'a' + 10;
        result = (result << 4) | nybble;
}
    return result;
}

void OpDebugSkip(const char*& str, const char* match) {
    size_t matchLen = strlen(match);
    size_t strLen = strlen(str);
    while (strLen) {
        if (!strncmp(match, str, matchLen)) {
            break;
        }
        str += 1;
        --strLen;
    }
    OP_ASSERT(strlen(str));
}

std::string OpDebugToString(float value, int precision) {
    if (precision < 0)
        return STR(value);
    std::string s(16, '\0');
    auto written = std::snprintf(&s[0], s.size(), "%.*f", precision, value);
    s.resize(written);
    return s;
}

#endif

#if OP_DEBUG

void OpMath::DebugCompare(float a, float b) {
    float diff = fabsf(a - b);
    float max = std::max(fabsf(a), fabsf(b));
    float precision = diff / max;
    OP_ASSERT(precision < OpEpsilon);
}

OpVector OpCurve::debugTangent(float t) const {
    switch (type) {
    case OpType::line: return asLine().tangent();
    case OpType::quad: return asQuad().debugTangent(t);
    case OpType::conic: return asConic().debugTangent(t);
    case OpType::cubic: return asCubic().debugTangent(t);
    default:
        OP_ASSERT(0);
    }
    return OpVector();
}

OpVector OpQuad::debugTangent(float t) const {
    OpVector result = tangent(t);
    if ((0 == t && pts[0] == pts[1]) || (1 == t && pts[2] == pts[1]))
        return pts[2] - pts[0];
    return result;
}

OpVector OpConic::debugTangent(float t) const {
    return ((OpQuad*) this)->debugTangent(t);
}

OpVector OpCubic::debugTangent(float t) const {
    OpVector result = tangent(t);
    if (0 == t && pts[0] == pts[1]) {
        if (pts[1] == pts[2])
            return pts[3] - pts[0];
        else
            return pts[2] - pts[0];
    }
    if (1 == t && pts[3] == pts[2]) {
        if (pts[2] == pts[1])
            return pts[3] - pts[0];
        else
            return pts[3] - pts[1];
    }
    return result;
}

#endif


