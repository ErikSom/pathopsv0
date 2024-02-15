// (c) 2023, Cary Clark cclark2@gmail.com
#ifndef OpEdge_DEFINED
#define OpEdge_DEFINED

#include "OpTightBounds.h"
#include "OpOperators.h"
#include <vector>

struct FoundEdge;
struct OpContours;
struct OpIntersection;
struct OpOutPath;
struct OpSegment;

enum class OpOperand {
	left,
	right
};

enum class NewEdge {
	isLeft,
	isRight
};

inline NewEdge operator!(const NewEdge& a) {
	return (NewEdge) !static_cast<int>(a);
}

enum RotateVertical {
	rotateVertical
};

enum class IntersectResult {
	fail,
	no,
	yes,
	coincident,
	maybe
};

enum EdgeRay {
	xRay,
	yRay,
};

enum class EdgeMatch : uint8_t {
	none,
	start,
	end,
	both	// used by flip
};

enum class FoundPtT;

inline EdgeMatch Opposite(EdgeMatch match) {
	OP_ASSERT(EdgeMatch::start == match || EdgeMatch::end == match);
	return EdgeMatch::start == match ? EdgeMatch::end : EdgeMatch::start;
}

#if 0  // not needed for now
inline EdgeMatch operator| (const EdgeMatch& a, const EdgeMatch& b) {
	return (EdgeMatch) (static_cast<int>(a) | static_cast<int>(b));
}

inline EdgeMatch& operator|= (EdgeMatch& a, const EdgeMatch& b) {
	return a = a | b;
}
#endif

enum class EdgeFail : uint8_t {
	none,
	center,
	horizontal,
	vertical
};

enum class WindingEdge {
	dummy
};

enum class WindingUninitialized {
	dummy
};

enum class WindingSum {
	dummy
};

enum class WindingTemp {	// used to accumulate winding sum before it is applied
	dummy
};

enum class ZeroReason : uint8_t {
	uninitialized,
	addedPalToOutput,
	addIntersection,
	applyOp,
	centerNaN,
	findCoincidences,
	hvCoincidence1,
	hvCoincidence2,
	hvCoincidence3,
	hvCoincidence4,
	isPoint,
	noFlip,
	none,
	palWinding,
};


enum class WindingType  {
	uninitialized = -1,
	temp,
	winding,
	sum
};

struct OpWinding {
private:
	// used by operator-
	OpWinding(int l, int r)
		: left_impl(l)
		, right_impl(r)
#if OP_DEBUG
		, debugType(WindingType::winding)
#endif
	{
	}

public:
	OpWinding(WindingTemp)  // used for winding accumulators before sum is set
		: left_impl(0)
		, right_impl(0)
#if OP_DEBUG
		, debugType(WindingType::temp)
#endif
	{
	}


	OpWinding(WindingUninitialized)	 // used by edge and segment winding before they are set
		: left_impl(OpMax)
#if OP_DEBUG
		, right_impl(OpMax)
		, debugType(WindingType::uninitialized)
#endif	
	{
	}

	OpWinding(OpOperand operand)	// used to set initial segment winding
		: left_impl(OpOperand::left == operand ? 1 : 0)
		, right_impl(OpOperand::right == operand ? 1 : 0)
#if OP_DEBUG
		, debugType(WindingType::winding)
#endif	
	{
	}

	bool operator==(OpWinding w) {
		return left_impl == w.left_impl && right_impl == w.right_impl;
	}

	OpWinding operator-() const {
		OP_ASSERT(WindingType::winding == debugType);
		return { -left_impl, -right_impl };
	}

	OpWinding& operator+=(const OpWinding& w) {
		OP_ASSERT(WindingType::temp == debugType || WindingType::winding == debugType);
		left_impl += w.left_impl;
		right_impl += w.right_impl;
		return *this;
	}

	OpWinding& operator-=(const OpWinding& w) {
		OP_ASSERT(WindingType::temp == debugType || WindingType::winding == debugType);
		left_impl -= w.left_impl;
		right_impl -= w.right_impl;
		return *this;
	}

	bool isSet() const {
		return OpMax != left_impl;
	}

	int left() const {
		return left_impl;
	}

	void move(OpWinding opp, const OpContours* , bool backwards);

	int oppSide(OpOperand operand) const {
		return OpOperand::left == operand ? right_impl : left_impl;
	}

	int right() const {
		return right_impl;
	}

	void setSum(OpWinding winding, const OpContours* segment);

	void setWind(int left, int right) {	// shouldn't be 0, 0 (call zero() for that)
		OP_ASSERT(WindingType::uninitialized == debugType);
		OP_ASSERT(left || right);
		left_impl = left;
		right_impl = right;
	#if OP_DEBUG
		debugType = WindingType::winding;
	#endif
	}

	int sum() const {
		return left_impl + right_impl;
	}

	bool visible() const {
		return left_impl || right_impl;
	}

	void zero() {
		left_impl = right_impl = 0;	// only used by coincident lines
	}
#if OP_DEBUG_DUMP
	std::string debugDump() const;
	DUMP_DECLARATIONS
#endif

	int left_impl;	// indirection to make set debugging breakpoints easier 
	int right_impl;

#if OP_DEBUG
	WindingType debugType;
#endif
};

// An edge that can contribute to the answer has a zero winding on one side
// For a pair of edges to connect, they have to have zero windings on the same side
// If they have zero windings that do not match, there should be a third (and fourth)
// edge at the same point that is a better match.
// A pair of edges that are nearly coincident may be mis-sorted so that the zero
// winding is wrong.
enum class WindZero : uint8_t {
	noFlip,
	normal,
	opp,
};

inline void OpDebugCheckSingleZero(WindZero left, WindZero right) {
	OP_ASSERT(WindZero::noFlip != left || WindZero::noFlip != right);
	OP_ASSERT((int) left + (int) right != 3);	// not normal and opp at same time
}

inline void WindZeroFlip(WindZero* windZero) {
    if (WindZero::noFlip == *windZero)
        return;
    *windZero = WindZero::normal == *windZero ? WindZero::opp : WindZero::normal;
}

struct EdgeDistance {
	EdgeDistance(OpEdge* e, float c, float tIn, bool r)
		: edge(e)
		, cept(c)
		, t(tIn)
		, reversed(r)
		, skipPal(false)
		, skipSum(false) {
	}

#if OP_DEBUG_DUMP
	DUMP_DECLARATIONS
#endif

	OpEdge* edge;
	float cept;		// where normal intersects edge (for home edge, equals center)
	float t;
	bool reversed;
	bool skipPal;  // true if another edge has this edge's winding through its pal list
	bool skipSum;  // true if edge contributes to sum, but sum should not be stored in edge
};

enum class FindCept {
	ok,
	okNew,	// intercept was found, and new distance was pushed
	retry,
	unsectable,
	unsortable
};

// captures ray info from edge that intersects other edges, horizontally or vertically
struct SectRay {
	SectRay()
		: normal(OpNaN)
		, homeCept(OpNaN)
		, homeT(OpNaN)
        , axis(Axis::neither)
	{
	}
	void addPals(OpEdge* );
	bool checkOrder(const OpEdge* ) const;
	FindCept findIntercept(OpEdge*  OP_DEBUG_PARAMS(OpEdge* ));
	EdgeDistance* find(OpEdge* );
	void sort();
#if OP_DEBUG_DUMP
	DUMP_DECLARATIONS
#endif

	std::vector<EdgeDistance> distances;
	OpVector homeTangent;  // used to determine if unsectable edge is reversed
	float normal;  // ray used to find windings on home edge
	float homeCept;  // intersection of normal on home edge
	float homeT;
	Axis axis;
};

enum class SectType {
	none,
	endHull,  // intersection is close to or equal to curve end point
	center,   // break curve at geometric center
	controlHull,  // intersection is on hull between end and control, or pair of control points
	midHull,  // hull intersects, but not near end point
	snipLo,   // snip at t lower than intersection
	snipHi    // snip at t higher than intersection
};

// intersections of opposite curve's hull and this edge's segment
struct HullSect {
	HullSect(const OpPtT& ptT, SectType st, const OpEdge* o = nullptr)
		: opp(o)
		, sect(ptT)
		, type(st) {
	}
#if OP_DEBUG_DUMP
	DUMP_DECLARATIONS
#endif
	const OpEdge* opp;
	OpPtT sect;			// point and t of intersection with hull on this edge
	SectType type;
};

enum class AllowPals {
	no,
	yes
};

enum class AllowClose {
	no,
	yes
};

enum class CalcFail {
	none,
	fail,
};

// !!! worrisome: normal direction below is downLeft (correct); why is this downRight?
enum class EdgeDirection {
	downRight = -1,
	upLeft = 1,
};

#if 0
enum class EdgeLoop {
	link,
	sum,
};
#endif

enum class EdgeSplit : uint8_t {
	no,
	keep,  // used only by curve experiment
	defer,  // used only by curve experiment : set split like partner
	yes = 3, // allows |= keep to set no->keep, keep->keep, yes->yes
};

inline EdgeSplit& operator|=(EdgeSplit& a, EdgeSplit b) {
    return a = (EdgeSplit)((int) a | (int) b);
}

enum class LeadingLoop {
	in,
	will,
};

enum class ResolveWinding {
	resolved,
	loop,
	retry,
	fail,
};

enum class Unsectable {
	none,
	single,
	multiple,
};

enum class WhichLoop {
	prior,
	next,
	undetermined,
};

enum class SplitBias : uint8_t {
	none,
	low,
	high
};

#if OP_DEBUG
enum class EdgeMaker {
	empty,
	filler,
	hull,
	makeEdges,
	oppSect,
	segSect,
	snip,
	split,
	splitLeft,
	splitRight,
	// tests only
	addTest,
	opTest,
};

#define EDGE_MAKER(maker) EdgeMaker::maker, __LINE__, std::string(__FILE__)

#endif

constexpr float OP_CURVACIOUS_LIMIT = 1.f / 16;  // !!! tune to guess line/line split ratio

struct OpEdge {
private:
	OpEdge()	// note : all release values are zero
		: priorEdge(nullptr)
		, nextEdge(nullptr)
		, lastEdge(nullptr)
		, winding(WindingUninitialized::dummy)
		, sum(WindingUninitialized::dummy)
		, many(WindingUninitialized::dummy)
		, unsectableID(0)
		, whichEnd(EdgeMatch::none)
		, rayFail(EdgeFail::none)
		, windZero(WindZero::noFlip)
		, doSplit(EdgeSplit::no)
		, bias(SplitBias::none)
		, curvySet(false)
		, lineSet(false)
		, verticalSet(false)
		, isLine_impl(false)
		, exactLine(false)
		, active_impl(false)
		, inLinkups(false)
		, inOutput(false)
		, disabled(false)
		, unsortable(false)
		, between(false)
	{
#if OP_DEBUG // a few debug values are nonzero. Boo!
        id = 0;
        segment = nullptr;
		curvy = OpNaN;
		debugSplitStart = SectType::none;
		debugSplitEnd = SectType::none;
		debugStart = nullptr;
		debugEnd = nullptr;
		debugMatch = nullptr;
		debugZeroErr = nullptr;
		debugMaker = EdgeMaker::empty;
		debugZero = ZeroReason::uninitialized;
		debugOutPath = 0;
		debugParentID = 0;
		debugRayMatch = 0;
		debugFiller = false;
#endif
#if OP_DEBUG_IMAGE
		debugColor = debugBlack;
		debugDraw = true;
		debugJoin = false;
#endif
	}
public:
	OpEdge(OpSegment* s, const OpPtT& t1, const OpPtT& t2
			OP_DEBUG_PARAMS(EdgeMaker maker, int line, std::string file, const OpIntersection* i1, 
			const OpIntersection* i2))
		: OpEdge() {
		segment = s;
		start = t1;
		end = t2;
#if OP_DEBUG
		debugMaker = maker;
		debugSetMaker = { file, line };
		debugStart = i1;
		debugEnd = i2;
#endif
		complete();
	}

	OpEdge(const OpEdge* e, const OpPtT& newPtT, NewEdge isLeftRight  
			OP_DEBUG_PARAMS(EdgeMaker , int line, std::string file));
	OpEdge(const OpEdge* e, const OpPtT& start, const OpPtT& end  
			OP_DEBUG_PARAMS(EdgeMaker , int line, std::string file));
	OpEdge(const OpEdge* e, float t1, float t2  
			OP_DEBUG_PARAMS(EdgeMaker , int line, std::string file));

#if OP_DEBUG_IMAGE
	OpEdge(const OpEdge&) = default;
	OpEdge(OpEdge&&) = default;
	OpEdge& operator=(const OpEdge&) = default;
	OpEdge& operator=(OpEdge&&) = default;
	struct DebugOpCurve debugSetCurve() const;
#endif
	CalcFail addIfUR(Axis xis, float t, OpWinding* );
	void addPal(EdgeDistance& );
	CalcFail addSub(Axis axis, float t, OpWinding* );
	OpEdge* advanceToEnd(EdgeMatch );
	void apply();
	bool calcCenterT();
	void clearActiveAndPals(ZeroReason );
	void clearLastEdge();
	void clearNextEdge();
	void clearPriorEdge();
	void complete();
	bool containsLink(const OpEdge* edge) const;
	OpContours* contours() const;
	size_t countUnsortable() const;
	float curviness();
	OpIntersection* findSect(EdgeMatch );
	OpPtT findT(Axis , float oppXY) const;
	OpPtT flipPtT(EdgeMatch match) const { 
		return match == whichEnd ? end : start; }
	void flipWhich() { 
		whichEnd = (EdgeMatch)((int)whichEnd ^ (int)EdgeMatch::both); }
	bool hasLinkTo(EdgeMatch match) const { 
		return EdgeMatch::start == match ? priorEdge : nextEdge; }  // !!! was reversed!
	bool isActive() const { 
		return active_impl; }
	bool isLinear();
	bool isPal(const OpEdge* opp) const {
		return pals.end() != std::find_if(pals.begin(), pals.end(), 
				[opp](const auto& test) { return opp == test.edge; }); }
	void linkToEdge(FoundEdge& , EdgeMatch );
//	void linkNextPrior(OpEdge* first, OpEdge* last);
	bool linksTo(OpEdge* match) const;
	void markPals();
	void matchUnsectable(EdgeMatch , const std::vector<OpEdge*>& unsectInX, 
			std::vector<FoundEdge>& , AllowPals , AllowClose );
	OpEdge* nextOut();
	NormalDirection normalDirection(Axis axis, float t);
	void output(OpOutPath path);  // provided by the graphics implmentation
	OpPtT ptT(EdgeMatch match) const { 
		return EdgeMatch::start == match ? start : end; }
	void reenable() {
		disabled = false; OP_DEBUG_CODE(debugZero = ZeroReason::uninitialized); }
	void setActive(bool state);  // setter exists so debug breakpoints can be set
	void setBetween();  // setter exists so debug breakpoints can be set
//	const OpCurve& setCurve();  // copies start, end to points 0, last
	void setCurveCenter();  // adds center point after curve points
	void setDisabled(OP_DEBUG_CODE(ZeroReason reason));
	void setDisabledZero(OP_DEBUG_CODE(ZeroReason reason)) {
		winding.zero(); setDisabled(OP_DEBUG_CODE(reason)); }
//	void setFromPoints(const OpPoint pts[]);
	OpEdge* setLastEdge(OpEdge* old = nullptr);
	bool setLastLink(EdgeMatch );  // returns true if link order was changed
	OpPointBounds setLinkBounds();
	bool setLinkDirection(EdgeMatch );  // reverse links if handed link end instead of link start
	void setNextEdge(OpEdge*);  // setter exists so debug breakpoints can be set
	void setPointBounds();
	void setPriorEdge(OpEdge* );  // setter exists so debug breakpoints can be set
	void setSumImpl(OpWinding w) {	// use debug macro instead to record line/file
		sum.setSum(w, contours()); }
	void setUnsortable();
	const OpCurve& setVertical();
	void skipPals(EdgeMatch match, std::vector<FoundEdge>& edges); 
	void subDivide();
	CalcFail subIfDL(Axis axis, float t, OpWinding* );
	OpType type();
	OpPtT whichPtT(EdgeMatch match = EdgeMatch::start) const { 
		return match == whichEnd ? start : end; }

	bool debugFail() const;
    bool debugSuccess() const;
#if OP_DEBUG_DUMP
	OpEdge(std::string );
	OpEdge(OpPtT data[2]);
	OpEdge(OpHexPtT data[2]);
	void debugCompare(std::string ) const;
	std::string debugDumpBrief() const;
	std::string debugDumpChain(WhichLoop , bool detail) const;
	void dumpCenter(bool asHex) const;
	void dumpChain(bool detail = false) const;
	void dumpEnd() const;
	void dumpLink() const;
	void dumpLinkDetail() const;
	void dumpStart() const;
	#include "OpDebugDeclarations.h"
#endif
#if OP_DEBUG
	const OpEdge* debugAdvanceToEnd(EdgeMatch match) const;
	bool debugIsLoop() const {
		return debugIsLoop(WhichLoop::prior) || debugIsLoop(WhichLoop::next); }
	const OpEdge* debugIsLoop(WhichLoop , LeadingLoop = LeadingLoop::will) const;
	int debugLinkCount() const;
	void debugValidate() const;  // make sure pointer to edge is valid
//	bool debugValidLoop() const;  // currently unused
#endif
#if OP_DEBUG_IMAGE
	void addLink();
	void color(uint32_t );
	void drawLink();
	void draw();
#endif

	const OpSegment* segment;
	SectRay ray;
	OpEdge* priorEdge;	// edges that link to form completed contour
	OpEdge* nextEdge;
	OpEdge* lastEdge;
	// !!! Can start, end be shared with intersection?
	// what about id/ptr struct (union?) with intersect id and ptr
	// ptr is set up once intersects is sorted
	// should there be a different edge structure after intersections are known?
	OpPtT start;
	OpPtT center;  // curve location used to find winding contribution
	OpPtT end;
	OpCurve curve;
	OpCurve vertical_impl;	// only access through set vertical function
	OpPointBounds ptBounds;
	OpPointBounds linkBounds;
	OpWinding winding;	// contribution: always starts as 1, 0 (or 0, 1)
	OpWinding sum;  // total incl. normal side of edge for operands (fill count in normal direction)
	OpWinding many;  // temporary used by unsectables to contain all pal windings combined
	std::vector<EdgeDistance> pals;	 // list of unsectable adjacent edges !!! should be pointers?
	std::vector<OpEdge*> lessRay;  // edges found placed with smaller edge distance cept values
	std::vector<OpEdge*> moreRay;  // edges found placed with larger edge distance cept values
	std::vector<HullSect> hulls;  // curve-curve intersections
	float curvy;  // rough ratio of midpoint line point line to length of end point line
	int id;
	int unsectableID;	// used to pair unsectable intersections and find edges between
	EdgeMatch whichEnd;	// if 'start', prior link end equals start; if 'end' prior end matches end
	EdgeFail rayFail;	// how computation (e.g., center) failed (on fail, windings are set to zero)
	WindZero windZero; // normal means edge normal points to zero side; opposite, normal is non-zero
	EdgeSplit doSplit; // used by curve/curve intersection to track subdivision
	SplitBias bias;  // in curve/curve, which end to favor for next intersect guess
	bool curvySet;
	bool lineSet;
	bool verticalSet;
	bool isLine_impl;	// ptBounds 0=h/0=w catches horz/vert lines; if true, line is diagonal(?)
	bool exactLine;   // set if quad/conic/cubic is degenerate; clear if control points are linear
	bool active_impl;  // used by ray casting to mark edges that may be to the left of casting edge
	bool inLinkups; // like inOutput, to marks unsectable edges; all edges in linkups l vector
	bool inOutput;	// likely only used to find inactive unsectables that are not on output path
	bool disabled;	// winding is zero, or apply disqualified edge from appearing in output
	bool unsortable;
	bool between;  // between unsectables (also unsortable); !!! begs for an enum class, instead...
	bool ccOverlaps;  // set if curve/curve edges have bounds that overlap
#if OP_DEBUG
	SectType debugSplitStart;
	SectType debugSplitEnd;
	const OpIntersection* debugStart;
	const OpIntersection* debugEnd;
	OpEdge* debugMatch;  // left side of nonzero ray from this edge
	OpEdge* debugZeroErr;  // debug match ray found edge that does not match -- diagnostic for now
	EdgeMaker debugMaker;
	ZeroReason debugZero;	// why edge was disabled
	OpDebugMaker debugSetMaker;
	OpDebugMaker debugSetSum;
	int debugOutPath;	// id to color output contours
	int debugParentID;
	int debugRayMatch;	// id that denotes edges in common output contour determined from ray
	bool debugFiller;  // edge created to span short gaps
#endif
#if OP_DEBUG_IMAGE
	uint32_t debugColor;
	bool debugDraw;
	bool debugJoin;	 // true if included by joiner
#endif
};

// allocating storage separately allows filler edges to be immobile and have reliable pointers
struct OpEdgeStorage {
	OpEdgeStorage()
		: next(nullptr)
		, used(0) {
        OP_DEBUG_CODE(storage[0] = 0);
	}
	bool contains(OpIntersection* start, OpIntersection* end) const;
#if OP_DEBUG_DUMP
	size_t debugCount() const;
	const OpEdge* debugFind(int id) const;
	const OpEdge* debugIndex(int index) const;
	DUMP_DECLARATIONS
#endif

	OpEdgeStorage* next;
	uint8_t storage[sizeof(OpEdge) * 256];
	int used;
};

#if OP_DEBUG == 0
#define OP_EDGE_SET_SUM(edge, winding) edge->setSumImpl(winding)
#else
#define OP_EDGE_SET_SUM(edge, winding) \
	do {	\
		OP_ASSERT(!edge->sum.isSet());  \
		edge->setSumImpl(winding); \
		edge->debugSetSum = { __FILE__, __LINE__ }; \
	} while (false)
#endif

#endif
