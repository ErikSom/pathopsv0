#ifndef OpEdges_DEFINED
#define OpEdges_DEFINED

#include "OpMath.h"

struct OpContours;
struct OpEdge;

enum class ChainFail {
	none,
	failIntercept,
	noNormal,
	normalizeOverflow,
	normalizeUnderflow
};

enum class EdgesToSort {
	byBox,		// when walking to intersect, use box only
	byCenter,	// when walking to determine winding, use box and center ray
	unsectable
};

enum class FoundIntersections {
	fail,
	yes
};

enum class FoundIntercept {
	fail,
	overflow,
	set,
	yes
};

enum class FoundWindings {
	fail,
	yes
};


enum class DistMult {
	none,
	first,
	mid,
	last,
};

// !!! is it worth wrapping a vector of these in a structure so methods can be associated?
struct EdgeDistance {
	EdgeDistance(OpEdge* e, float d, float n, float _t)
		: edge(e)
		, distance(d)
		, normal(n)
		, t(_t)
		, multiple(DistMult::none)
		, edgeMultiple(false) {
	}

	OpEdge* edge;
	float distance;
	float normal;
	float t;
	DistMult multiple;
	bool edgeMultiple;
};

struct OpEdges {
	OpEdges(OpContours& contours, EdgesToSort);
	OpEdges(OpEdge* sEdge, OpEdge* oEdge);
	bool activeUnsectable(const OpEdge* edge, EdgeMatch match, 
        std::vector<FoundEdge>& oppEdges);
	void addEdge(OpEdge* , EdgesToSort );
	static void AddLineCurveIntersection(OpEdge& opp, const OpEdge& edge);
	static void AddMix(XyChoice xyChoice, OpPtT ptTAorB, bool flipped, OpPtT cPtT, OpPtT dPtT,
			OpSegment* segment, OpSegment* oppSegment, int coinID);
	static IntersectResult AddPair(XyChoice offset, OpPtT aPtT, OpPtT bPtT, OpPtT cPtT, OpPtT dPtT,
			bool flipped, OpSegment* segment, OpSegment* oppSegment);
	static IntersectResult CoincidentCheck(OpPtT ptTa, OpPtT ptTb, OpPtT ptTc, OpPtT ptTd,
			OpSegment* segment, OpSegment* oppSegment);
	static IntersectResult CoincidentCheck(const OpEdge& edge, const OpEdge& opp);
//	FoundWindings checkForLoops(Axis axis);
//	void checkForLoopy(EdgeDistance* last, Axis axis, std::vector<EdgeDistance>& distance);
//	FoundIntersections findIntersections();
	FoundIntercept findRayIntercept(size_t inIndex, Axis , OpEdge* edge, float center, 
			float normal, float edgeCenterT, std::vector<EdgeDistance>* );
	static void MarkUnsectableGroups(std::vector<EdgeDistance>& distance);
	void markUnsortable(OpEdge* edge, Axis , ZeroReason);
	static void SetEdgeMultiple(Axis axis, EdgeDistance* edgeDist  
			OP_DEBUG_PARAMS(std::vector<EdgeDistance>& distance));
	ChainFail setSumChain(size_t inIndex, Axis );
	ResolveWinding setWindingByDistance(OpEdge* edge, Axis , std::vector<EdgeDistance>& );
	FoundWindings setWindings(OpContours* );
	void sort(EdgesToSort);

#if OP_DEBUG
	void debugValidate() const;
#endif
#if OP_DEBUG_DUMP
	void dump() const;
	void dumpAxis(Axis ) const;
	DUMP_COMMON_DECLARATIONS();
#endif
#if OP_DEBUG_IMAGE
	void debugAdd() const;
	void debugDraw() const;
#endif

	// assist to find axis-aligned ray intersection from edge center to next curve
	std::vector<OpEdge*> inX;
	std::vector<OpEdge*> inY;
};

#endif
