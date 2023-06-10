#ifndef OpEdgeIntersect_DEFINED
#define OpEdgeIntersect_DEFINED

#include "OpEdge.h"
#include "OpIntersection.h"

enum class SectFound {
	no,
	fail,
	intersects,
	overflow,
	split
};

enum class DoSplit {
	marked,
	all
};

enum class CurveRef {
	edge,
	opp
};

struct OpEdgeIntersect {
	static constexpr int maxSplits = 8;   // !!! no idea what this should be 
	static constexpr int maxDepth = 24;  // !!! no idea what this should be

	OpEdgeIntersect(const OpEdge* edge, const OpEdge* opp)
		: originalEdge(edge)
		, originalOpp(opp) {
		edgeCurves.emplace_back(*edge);
		oppCurves.emplace_back(*opp);
#if OP_DEBUG_DUMP
		debugActive = this;
#endif
	}

	SectFound addCurveCoincidence(); // if curve doesn't devolve into line segments
	SectFound curvesIntersect(CurveRef );
	SectFound divideAndConquer();
	std::vector<OpEdge> findEdgesTRanges(CurveRef );
	static void LinearIntersect(std::vector<OpEdge>& lines, std::vector<OpEdge>& linesOrCurves);
	bool split(CurveRef , DoSplit );
	bool tooFew(CurveRef );

#if OP_DEBUG_DUMP
	static const OpEdgeIntersect* debugActive;

	~OpEdgeIntersect() {
		debugActive = nullptr;
	}
	void dump(bool detail) const;
	void dump() const { dump(false); }
	void dumpDetail() const { dump(true); }
	DUMP_COMMON_DECLARATIONS();
#endif
#if OP_DEBUG_IMAGE
	void draw() const;
#endif
	const OpEdge* originalEdge;
	const OpEdge* originalOpp;
	std::vector<OpEdge> edgeCurves;
	std::vector<OpEdge> edgeLines;
	std::vector<OpEdge> oppCurves;
	std::vector<OpEdge> oppLines;
	std::vector<OpEdge> edgeResults;
	std::vector<OpEdge> oppResults;
};

#endif
