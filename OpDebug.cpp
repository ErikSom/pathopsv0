#include "OpDebug.h"

#if 0
// code pattern to find one of several id values
template <typename V, typename... T>   // replace with std::to_array in c++20
constexpr auto to_array(T&&... t)->std::array < V, sizeof...(T) > {
    return { { std::forward<T>(t)... } };
}

    auto match = to_array<int>(68);  // c++20: std::to_array<int>({... (brace)
    if (match.end() != std::find(match.begin(), match.end(), id))
        OP_ASSERT(0);
#endif

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

#include "OpContour.h"
#include "OpEdge.h"

void OpEdge::debugValidate() const {
    debugGlobalContours->debugValidateEdgeIndex += 1;
    bool loopy = debugIsLoop();
    if (loopy) {
        const OpEdge* test = this;
        do {
            OP_ASSERT(!test->priorEdge || test->priorEdge->nextEdge == test);
            OP_ASSERT(!test->nextEdge || test->nextEdge->priorEdge == test);
//            OP_ASSERT(!test->lastEdge);
            test = test->nextEdge;
        } while (test != this);
    } else if ((priorEdge || lastEdge) && debugGlobalContours->debugCheckLastEdge) {
        const OpEdge* linkStart = debugAdvanceToEnd(EdgeMatch::start);
        const OpEdge* linkEnd = debugAdvanceToEnd(EdgeMatch::end);
        OP_ASSERT(linkStart);
        OP_ASSERT(linkEnd);
        OP_ASSERT(debugGlobalContours->debugCheckLastEdge ? !!linkStart->lastEdge : !linkStart->lastEdge);
        OP_ASSERT(debugGlobalContours->debugCheckLastEdge ? linkStart->lastEdge == linkEnd : true);
        const OpEdge* test = linkStart;
        while ((test = test->nextEdge)) {
            OP_ASSERT(!test->lastEdge);
            OP_ASSERT(linkEnd == test ? !test->nextEdge : !!test->nextEdge);
        }
    }
    for (auto& edge : segment->edges) {
        if (&edge == this)
            return;
    }
    OP_ASSERT(0);
}

#include "OpJoiner.h"

// assign the same ID for all edges linked together
// also assign that ID to edges whose non-zero crossing rays attach to those edges
void::OpJoiner::debugMatchRay() {
    OpDebugOut("");
	for (auto linkup : linkups.l) {
        int nextID = 0;
        OP_ASSERT(!linkup->priorEdge);
        OP_ASSERT(linkup->lastEdge);
        do {
            if (!linkup->ray.distances.size())
                continue;
            const EdgeDistance* linkDist = nullptr;
            OpEdge* dTest = nullptr;
            OP_DEBUG_CODE(const EdgeDistance* dDist = nullptr);
            for (const EdgeDistance* dist = &linkup->ray.distances.back(); 
                    dist >= &linkup->ray.distances.front(); --dist) {
                OpEdge* test = dist->edge;
                if (test == linkup) {
                    linkDist = dist;
                    continue;
                }
                if (!linkDist)
                    continue;
                if (linkup->unsectableID && linkup->unsectableID == test->unsectableID)
                    continue;
                OP_DEBUG_CODE(dDist = dist);
                dTest = test;
                break;
            }
            // look to see if edge maps a non-zero ray to a prior edge
            WindZero linkZero = linkup->windZero;
            OP_ASSERT(WindZero::noFlip != linkZero);
	        NormalDirection NdotR = linkup->normalDirection(-linkup->ray.axis, linkDist->t);
            if (NormalDirection::downLeft == NdotR)
                WindZeroFlip(&linkZero);    // get wind zero for edge normal pointing left
            bool rayFills = WindZero::opp == linkZero;                
            OP_ASSERT(!nextID || !rayFills || !linkup->debugRayMatch 
                    || nextID == linkup->debugRayMatch);
            if (int match = linkup->debugRayMatch)
                nextID = match;
            else if (rayFills && !nextID) {
                OP_ASSERT(dTest);
                int testID = dTest->debugRayMatch;
                OP_ASSERT(!nextID || !testID || nextID == testID);
                nextID = testID;
            } 
            if (!nextID)
                nextID = linkup->segment->nextID();
            if (rayFills)
                dTest->debugRayMatch = nextID;
            linkup->debugRayMatch = nextID;        
#if OP_DEBUG
            if (!rayFills)
                continue;
            WindZero distZero = dTest->windZero;
            OP_ASSERT(!rayFills || WindZero::noFlip != distZero);
            NdotR = dTest->normalDirection(linkup->ray.axis, dDist->t);
            if (NormalDirection::downLeft == NdotR)
                WindZeroFlip(&distZero);    // get wind zero for prior normal pointing right
            // either neither zero should be opp, or both should be 
            OP_ASSERT(!rayFills || WindZero::opp == distZero);
#endif
        } while ((linkup = linkup->nextEdge));
    }
}

// !!! also debug prev/next edges (links)
void OpJoiner::debugValidate() const {
    debugGlobalContours->debugValidateJoinerIndex += 1;
    debugGlobalContours->debugCheckLastEdge = false;
    if (LinkPass::unambiguous == linkPass) {
        for (auto edge : byArea) {
            edge->debugValidate();
            OP_ASSERT(!edge->isActive() || !edge->debugIsLoop());
        }
    }
    for (auto edge : unsectByArea) {
        edge->debugValidate();
        OP_ASSERT(!edge->isActive() || !edge->debugIsLoop());
    }
    for (auto edge : disabled) {
        edge->debugValidate();
//        OP_ASSERT(!edge->debugIsLoop());
    }
    for (auto edge : unsortables) {
        edge->debugValidate();
        OP_ASSERT(!edge->isActive() || !edge->debugIsLoop());
    }
    debugGlobalContours->debugCheckLastEdge = true;
    for (auto edge : linkups.l) {
        edge->debugValidate();
        OP_ASSERT(!edge->debugIsLoop());
    }
}

void OpSegment::debugValidate() const {
    for (auto i : sects.i)
        i->debugValidate();
}

#include "OpWinder.h"

void OpWinder::debugValidate() const {
    for (auto& edge : inX)
        edge->debugValidate();
    for (auto& edge : inY)
        edge->debugValidate();
}


#endif


