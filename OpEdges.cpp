#include "OpContour.h"
#include "OpCurveCurve.h"
#include "OpEdges.h"
#include "OpSegment.h"
#include "PathOps.h"

OpEdges::OpEdges(OpContours& contours, EdgesToSort edgesToSort) {
	if (EdgesToSort::byBox == edgesToSort) {

	}
	for (auto& contour : contours.contours) {
		for (auto& segment : contour.segments) {
			for (auto& edge : segment.edges) {
				addEdge(&edge, edgesToSort);
			}
		}
	}
	sort(edgesToSort);
}

OpEdges::OpEdges(OpEdge* sEdge, OpEdge* oEdge) {
	addEdge(sEdge, EdgesToSort::byCenter);
	addEdge(oEdge, EdgesToSort::byCenter);
#if OP_DEBUG_IMAGE
	OpDebugImage::add(sEdge);
	OpDebugImage::add(oEdge);
#endif
}

// look for active unsectable edges that match
// We could accumulate more than one path until we know which one is best.
// Instead, we'll add one but compare it and its pals to see if the path closes.
// If a pal closes, then rewind until the pal chain and given chain match and swap
//  with the pal chain to close the path.
// returns true if more than one unsectable was found
bool OpEdges::activeUnsectable(const OpEdge* edge, EdgeMatch match, 
        std::vector<FoundEdge>& oppEdges) {

	return false;
}

void OpEdges::addEdge(OpEdge* edge, EdgesToSort edgesToSort) {
	if (!edge->winding.visible() || EdgeSum::unsortable == edge->sumType)
		return;
	if (edge->many.isSet()) {
		if (EdgesToSort::byBox == edgesToSort)
			unsectInX.push_back(edge);
		return;
	}
	if (EdgesToSort::byBox == edgesToSort || edge->ptBounds.height())
		inX.push_back(edge);
	if (EdgesToSort::byCenter == edgesToSort && edge->ptBounds.width())
		inY.push_back(edge);
}

IntersectResult OpEdges::CoincidentCheck(OpPtT aPtT, OpPtT bPtT, OpPtT cPtT, OpPtT dPtT,
		OpSegment* segment, OpSegment* oppSegment) {
	OpVector abDiff = aPtT.pt - bPtT.pt;
	XyChoice xyChoice = fabsf(abDiff.dx) < fabsf(abDiff.dy) ? XyChoice::inY : XyChoice::inX;
	float A = aPtT.pt.choice(xyChoice);
	float B = bPtT.pt.choice(xyChoice);
	OP_ASSERT(A != B);
	float C = cPtT.pt.choice(xyChoice);
	float D = dPtT.pt.choice(xyChoice);
	OP_ASSERT(C != D);
	bool flipped = A < B != C < D;
	bool AinCD = OpMath::Between(C, A, D);
	bool BinCD = OpMath::Between(C, B, D);
	if (AinCD && BinCD)
		return AddPair(xyChoice, aPtT, bPtT, cPtT, dPtT, flipped, segment, oppSegment);
	bool CinAB = OpMath::Between(A, C, B);
	bool DinAB = OpMath::Between(A, D, B);
	if (CinAB && DinAB)
		return AddPair(xyChoice, cPtT, dPtT, aPtT, bPtT, flipped, oppSegment, segment);
	if (!AinCD && !BinCD)
		return IntersectResult::no;
	OP_ASSERT(CinAB || DinAB);
	float AorB = AinCD ? A : B;
	OpPtT ptTAorB = AinCD ? aPtT : bPtT;
	float CorD = CinAB ? C : D;
	OpPtT ptTCorD = CinAB ? cPtT : dPtT;
	if (AorB == CorD) {
		if (segment->containsIntersection(ptTAorB, oppSegment))
			return IntersectResult::yes;
		OpIntersection* sect = segment->addSegSect(ptTAorB  OP_DEBUG_PARAMS(
				SECT_MAKER(addCoincidentCheck), SectReason::coinPtsMatch, oppSegment));
		OpIntersection* oSect = oppSegment->addSegSect(ptTCorD  OP_DEBUG_PARAMS(
				SECT_MAKER(addCoincidentCheckOpp), SectReason::coinPtsMatch, segment));
		sect->pair(oSect);
		return IntersectResult::yes;
	}
	// pass a mix of seg and opp; construct one t for each
	int coinID = segment->coinID(flipped);
	AddMix(xyChoice, ptTAorB, flipped, cPtT, dPtT, segment, oppSegment, coinID);
	AddMix(xyChoice, ptTCorD, flipped, aPtT, bPtT, oppSegment, segment, coinID);
	return IntersectResult::yes;
}

// this edge has points A, B; opp edge has points C, D
// adds 0: no intersection; 1: end point only; 2: partial or full coincidence
IntersectResult OpEdges::CoincidentCheck(const OpEdge& edge, const OpEdge& opp) {
	return OpEdges::CoincidentCheck(edge.start, edge.end, opp.start, opp.end,
			const_cast<OpSegment*>(edge.segment), const_cast<OpSegment*>(opp.segment));
}

void OpEdges::AddMix(XyChoice xyChoice, OpPtT ptTAorB, bool flipped,
		OpPtT cPtT, OpPtT dPtT, OpSegment* segment, OpSegment* oppSegment, int coinID) {
	float eStart = ptTAorB.pt.choice(xyChoice);
	if (flipped)
		std::swap(cPtT, dPtT);
	float oStart = cPtT.pt.choice(xyChoice);
	float oEnd = dPtT.pt.choice(xyChoice);
	float oTRange = dPtT.t - cPtT.t;
	OpPtT oCoinStart { ptTAorB.pt, cPtT.t + (eStart - oStart) / (oEnd - oStart) * oTRange };
	OP_ASSERT(OpMath::Between(cPtT.t, oCoinStart.t, dPtT.t));
	OpIntersection* sect = segment->addCoin(ptTAorB, coinID  
			OP_DEBUG_PARAMS(SECT_MAKER(addMix), SectReason::coinPtsMatch, oppSegment));
	OpIntersection* oSect = oppSegment->addCoin(oCoinStart, coinID  
			OP_DEBUG_PARAMS(SECT_MAKER(addMixOpp), SectReason::coinPtsMatch, segment));
	sect->pair(oSect);
}

// If we got here because a pair of edges are coincident, that coincidence may have already been
// recorded when the pair of segments were checked, or the intersections may have been computed.
IntersectResult OpEdges::AddPair(XyChoice xyChoice, OpPtT aPtT, OpPtT bPtT, OpPtT cPtT, OpPtT dPtT,
	bool flipped, OpSegment* segment, OpSegment* oppSegment) {
	// set range to contain intersections that match this segment and opposite segment
	std::vector<OpIntersection*> range = segment->intersectRange(oppSegment);
	// return existing intersection that matches segment coincident ends
	auto findSect = [](const std::vector<OpIntersection*>& range, OpPtT ptT) {	// lambda
		for (auto entry : range) {
			if (entry->ptT.t == ptT.t || entry->ptT.pt == ptT.pt)
				return entry;
		}
		return (OpIntersection*) nullptr;
	};
	// returns index into existing coincidence pairs. Even is outside pair, odd is inside pair.
	// !!! note the return allows us to know if pair of calls to in coin range encompasses one or
	//     more pairs, but we don't take advantage of that yet
	auto inCoinRange = [](const std::vector<OpIntersection*>& range, float t, int* coinID) {
		OpIntersection* coinStart = nullptr;
		int index = 0;
		for (auto entry : range) {
			if (!entry->coincidenceID)
				continue;
			if (!coinStart) {
				coinStart = entry;
				++index;
				continue;
			}
			OP_ASSERT(entry->coincidenceID == coinStart->coincidenceID);
			if (coinStart->ptT.t <= t && t <= entry->ptT.t) {
				if (coinID) {
					OP_ASSERT(!*coinID || *coinID == coinStart->coincidenceID);	 // !!! assert means two coin runs; is that possible?
					*coinID = coinStart->coincidenceID;
				}
				break;
			}
			coinStart = nullptr;
			++index;
		}
		return (bool) (index & 1);
	};
	// find the intersection, or make one, if it is not within an existing coincident run
	OpIntersection* sect1 = findSect(range, aPtT);
	OpIntersection* sect2 = findSect(range, bPtT);
	// remember if this t value is in a coincident range, and if so, which one
	int coinID = 0;
	bool aInCoincidence = inCoinRange(range, aPtT.t, &coinID);
	bool bInCoincidence = inCoinRange(range, bPtT.t, &coinID);
	if (!coinID)
		coinID = segment->coinID(flipped);
	else
		OP_ASSERT(flipped ? coinID < 0 : coinID > 0);	// should never assert
	// assign a new or existing coin id if sect doesn't already have one
	if (!aInCoincidence) {
		if (sect1) {
			OP_ASSERT(!sect1->coincidenceID);
			sect1->coincidenceID = coinID;
		} else	// or if it doesn't exist and isn't in a coin range, make one
			sect1 = segment->addCoin(aPtT, coinID
				OP_DEBUG_PARAMS(SECT_MAKER(addPair_aPtT), SectReason::coinPtsMatch, oppSegment));
	}
	if (!bInCoincidence) {
		if (sect2) {
			OP_ASSERT(!sect2->coincidenceID);
			sect2->coincidenceID = coinID;
		} else
			sect2 = segment->addCoin(bPtT, coinID
				OP_DEBUG_PARAMS(SECT_MAKER(addPair_bPtT), SectReason::coinPtsMatch, oppSegment));
	}
	std::vector<OpIntersection*> oRange = oppSegment->intersectRange(segment);
	OpIntersection* oSect1 = findSect(oRange, { aPtT.pt, -1 });
	OpIntersection* oSect2 = findSect(oRange, { bPtT.pt, -1 });
	// add the opposite that goes with the created segment sect
	float oStart  OP_DEBUG_INITIALIZE_TO_SILENCE_WARNING;
	float oTRange  OP_DEBUG_INITIALIZE_TO_SILENCE_WARNING;
	float oXYRange  OP_DEBUG_INITIALIZE_TO_SILENCE_WARNING;
	if (!oSect1 || !oSect2) {
		if (flipped)
			std::swap(cPtT, dPtT);
		oStart = cPtT.pt.choice(xyChoice);
		float oEnd = dPtT.pt.choice(xyChoice);
		oTRange = dPtT.t - cPtT.t;
		oXYRange = oEnd - oStart;
	}
	if (!oSect1) {
		float eStart = aPtT.pt.choice(xyChoice);
		OpPtT oCoinStart{ aPtT.pt, cPtT.t + (eStart - oStart) / oXYRange * oTRange };
		OP_ASSERT(OpMath::Between(cPtT.t, oCoinStart.t, dPtT.t));
		oSect1 = oppSegment->addCoin(oCoinStart, coinID
				OP_DEBUG_PARAMS(SECT_MAKER(addPair_oppStart), SectReason::coinPtsMatch, segment));
		sect1->pair(oSect1);
	} else if (!(inCoinRange(oRange, oSect1->ptT.t, nullptr) & 1))
		oSect1->coincidenceID = coinID;
	if (!oSect2) {
		float eEnd = bPtT.pt.choice(xyChoice);
		OpPtT oCoinEnd{ bPtT.pt, cPtT.t + (eEnd - oStart) / oXYRange * oTRange };
		OP_ASSERT(OpMath::Between(cPtT.t, oCoinEnd.t, dPtT.t));
		oSect2 = oppSegment->addCoin(oCoinEnd, coinID
				OP_DEBUG_PARAMS(SECT_MAKER(addPair_oppEnd), SectReason::coinPtsMatch, segment));
		sect2->pair(oSect2);
	} else if (!(inCoinRange(oRange, oSect2->ptT.t, nullptr) & 1))
		oSect2->coincidenceID = coinID;
	return IntersectResult::yes;
}

// !!! I'm bothered that segment / segment calls a different form of this
void OpEdges::AddLineCurveIntersection(OpEdge& opp, const OpEdge& edge) {
	OP_ASSERT(opp.segment != edge.segment);
	OpRoots septs;
	OP_ASSERT(edge.isLine_impl);
	LinePts edgePts { edge.start.pt, edge.end.pt };
	const OpCurve& oppCurve = opp.setCurve();
	septs = oppCurve.rayIntersect(edgePts);
//	OP_ASSERT(septs.count < 3);	// while it should never be 3, it's OK to accept those intersections
	if (!septs.count)
		return;
	// Note that coincident check does not receive intercepts as a parameter; in fact, the intercepts
	// were not calculated (the roots are uninitialized). This is because coincident check will 
	// compute the actual coincident start and end without the roots introducing error.
	if (2 == septs.count && opp.setLinear())
		return (void) CoincidentCheck(edge, opp);
	for (unsigned index = 0; index < septs.count; ++index) {
		float oppT = OpMath::Interp(opp.start.t, opp.end.t, septs.get(index));
		OpPtT oppPtT { oppCurve.ptAtT(septs.get(index)), oppT };
		float edgeT;
		FoundPtT foundPtT = edge.segment->findPtT(edge.start.t, edge.end.t, oppPtT.pt, &edgeT);
		if (FoundPtT::multiple == foundPtT)
			return;
		if (OpMath::Between(0, edgeT, 1)) {
            // pin point to both bounds, but only if it is on edge
			OpSegment* eSegment = const_cast<OpSegment*>(edge.segment);
			OpSegment* oSegment = const_cast<OpSegment*>(opp.segment);
        OP_DEBUG_CODE(OpPoint debugPt = oppPtT.pt);
            oSegment->ptBounds.pin(&oppPtT.pt);
//            eSegment->ptBounds.pin(&oppPtT.pt);	// !!! doubtful this is ever needed with contains test above
        OP_ASSERT(debugPt == oppPtT.pt);	// detect if pin is still needed
			OpPtT edgePtT { oppPtT.pt, OpMath::Interp(edge.start.t, edge.end.t, edgeT) };
			if (eSegment->alreadyContains(edgePtT, oSegment))
				OP_ASSERT(oSegment->alreadyContains(oppPtT, eSegment));
			else {
				OpIntersection* sect = eSegment->addEdgeSect(edgePtT  
						OP_DEBUG_PARAMS(SECT_MAKER(edgeLineCurve), SectReason::lineCurve, 
						&edge, &opp));
				OpIntersection* oSect = oSegment->addEdgeSect(oppPtT  
						OP_DEBUG_PARAMS(SECT_MAKER(edgeLineCurveOpp), SectReason::lineCurve, 
						&edge, &opp));
				sect->pair(oSect);
			}
		}
	}
}

// note: sorts from high to low
struct CompareDistance {
	bool operator()(const EdgeDistance& s1, const EdgeDistance& s2) {
		return s1.distance > s2.distance;
	}
};

// at some point, do some math or rigorous testing to figure out how extreme this can be
// for now, keep making it smaller until it breaks
#define WINDING_NORMAL_LIMIT  0.001 // !!! no idea what this should be

FoundIntercept OpEdges::findRayIntercept(size_t inIndex, Axis axis, OpEdge* edge, float center,
		float normal, float edgeCenterT, std::vector<EdgeDistance>* distance) {
	OpVector ray = Axis::horizontal == axis ? OpVector{ 1, 0 } : OpVector{ 0, 1 };
	Axis perpendicular = !axis;
	OpVector backRay = -ray;
	float mid = .5;
	float midEnd = .5;
	std::vector<OpEdge*>& inArray = Axis::horizontal == axis ? inX : inY;
	do {
		distance->emplace_back(edge, 0, normal, edgeCenterT);
		int index = inIndex;
		// start at edge with left equal to or left of center
		while (index != 0) {
			OpEdge* test = inArray[--index];
			if (test == edge)
				continue;
			if (test->ptBounds.ltChoice(axis) > normal)
				continue;
			if (test->ptBounds.rbChoice(axis) < normal)
				continue;
			if (EdgeSum::unsortable == test->sumType)
				goto tryADifferentCenter;
			if (!test->winding.visible())
				continue;
			{
				const OpCurve& testCurve = test->setCurve();
				OpRoots cepts = testCurve.axisRayHit(axis, normal);
				// get the normal at the intersect point and see if it is usable
				if (1 != cepts.count) {
					// !!! if intercepts is 2 or 3, figure out why (and what to do)
					// !!! likely need to try a different ray
					OP_ASSERT(0 == cepts.count);
					continue;
				}
				float cept = cepts.get(0);
				if (OpMath::IsNaN(cept) || 0 == cept || 1 == cept)
					goto tryADifferentCenter;
				bool overflow;
				OpVector tangent = testCurve.tangent(cept).normalize(&overflow);
				if (overflow)
					OP_DEBUG_FAIL(*test, FoundIntercept::overflow);
				float tNxR = tangent.cross(backRay);
				if (fabs(tNxR) < WINDING_NORMAL_LIMIT
					&& (!test->setLinear() || test->start.t >= cept || cept >= test->end.t)) {
					goto tryADifferentCenter;
				}
				OpPoint pt = testCurve.ptAtT(cept);
				distance->emplace_back(test, center - pt.choice(perpendicular), normal, cept);
			}
		}
		return FoundIntercept::yes;
	tryADifferentCenter:
		mid /= 2;
		midEnd = midEnd < .5 ? 1 - mid : mid;
		float middle = OpMath::Interp(edge->ptBounds.ltChoice(axis), edge->ptBounds.rbChoice(axis), 
				midEnd);
		const OpCurve& edgeCurve = edge->setCurve();  // ok to be in loop (lazy)
		edgeCenterT = edgeCurve.center(axis, middle);
		if (OpMath::IsNaN(edgeCenterT) || mid <= 1.f / 256.f) {	// give it at most eight tries
			markUnsortable(edge, axis, ZeroReason::recalcCenter);
			return FoundIntercept::fail;	// nonfatal error (!!! give it a different name!)
		}
		// if find ray intercept can't find, restart with new center, normal, distance, etc.
		center = edgeCurve.ptAtT(edgeCenterT).choice(perpendicular);
		OP_ASSERT(!OpMath::IsNaN(center));
		normal = edgeCurve.ptAtT(edgeCenterT).choice(axis);
		OP_ASSERT(!OpMath::IsNaN(normal));
		distance->clear();
	} while (true);
}

void OpEdges::linkUnambiguous(OpOutPath path) {
    // match up edges that have only a single possible prior or next link, and add them to new list
    linkPass = LinkPass::unambiguous;
    for (auto& leftMost : inX) {
        if (!leftMost->winding.visible())
            continue;   // likely marked as part of a loop below
        if (!leftMost->isActive())  // check if already saved in linkups
            continue;
		do {
			OP_ASSERT(EdgeLink::unlinked == leftMost->priorLink);
			OP_ASSERT(EdgeLink::unlinked == leftMost->nextLink);
			leftMost->whichEnd = EdgeMatch::start;
			linkMatch = EdgeMatch::start;
			leftMost = linkUp(leftMost, path);
			if (!leftMost)
				continue;
			OP_ASSERT(!leftMost->isLoop());
			linkMatch = EdgeMatch::end;
		} while ((leftMost = linkUp(leftMost, path)));
    }
}

/* relationship between prev/this/next and whichEnd: (start, end)
   a and b represent points that match; ? represents the other nonmatching end
   prev: (?, a) which:start		this: (a, b) which:start		next: (b, ?) which: start
   prev: (a, ?) which:end		this: (a, b) which:start		next: (b, ?) which: start
   prev: (?, b) which:start		this: (a, b) which:end		    next: (a, ?) which: start
   prev: (b, ?) which:end		this: (a, b) which:end  		next: (a, ?) which: start
   prev: (?, a) which:start		this: (a, b) which:start		next: (?, b) which: end		etc...
*/
// caller clears active flag
// parameter match determines whether link is looked for prior to, or next to edge
// links a single edge or (link of edges) with another edge
// first pass: only allow unambiguous connections; only one choice, matching zero side, etc.
// second pass: check for unambiuous, then allow reversing, pick smallest area, etc.
OpEdge* OpEdges::linkUp(OpEdge* edge, OpOutPath path) {
	OP_ASSERT(!edge->isLoop(WhichLoop::next, LeadingLoop::will));
	std::vector<FoundEdge> edges;
	if (LinkPass::unsectInX == linkPass)
		edge->matchUnsectable(linkMatch, &unsectInX, edges);
	else {
		const OpSegment* segment = edge->segment;
		bool hasPal = segment->activeAtT(edge, linkMatch, edges, AllowReversal::no);
		hasPal |= segment->activeNeighbor(edge, linkMatch, edges);
		if (hasPal)
			edge->skipPals(linkMatch, edges);
	}
	if (edges.size() > 1)
		(EdgeMatch::start == linkMatch ? edge->priorLink : edge->nextLink) = EdgeLink::multiple;
	if (1 != edges.size()) {
		edge->prepareForLinkup();
		linkups.push_back(edge);
		return nullptr;
	}
	FoundEdge found = edges.front();
	OP_ASSERT(!found.edge->isLoop());
	edge->linkToEdge(found, linkMatch);
	OP_ASSERT(edge->whichPtT(linkMatch).pt == found.edge->flipPtT(linkMatch).pt);
	OpEdge* firstEdge = EdgeMatch::start == linkMatch ? edge : found.edge;
	OpEdge* leftOvers = nullptr;
	if (firstEdge->detachIfLoop(path, &leftOvers)) {
		if (leftOvers)
			return linkUp(leftOvers, path);
		return nullptr;
	}
	return linkUp(firstEdge, path);	// recurse
}

// at this point all singly linked edges have been found
// every active set of links at this point must form a loop
bool OpEdges::matchLinks(OpEdge* edge) {
	OpEdge* lastEdge = edge->lastEdge;
	OP_ASSERT(lastEdge);
	OP_ASSERT(EdgeMatch::start == lastEdge->whichEnd || EdgeMatch::end == lastEdge->whichEnd);
	// count intersections equaling end
	// each intersection has zero, one, or two active edges
	std::vector<FoundEdge> found;
	const OpSegment* last = lastEdge->segment;
	// should be able to ignore result because pals have already been marked inactive
	(void) last->activeAtT(lastEdge, EdgeMatch::end, found, AllowReversal::yes);
	(void) last->activeNeighbor(edge, EdgeMatch::end, found);
	OpEdge* closest = nullptr;
	EdgeMatch closestEnd = EdgeMatch::none;
	AllowReversal closestReverse = AllowReversal::no;
	if (!found.size()) {
		OpPoint matchPt = lastEdge->whichPtT(EdgeMatch::end).pt;
		float closestDistance = OpInfinity;
		for (size_t index = 0; index < linkups.size(); ++index) {
			OpEdge* linkup = linkups[index];
			if (edge == linkup)
				continue;
			if (!linkup->priorEdge) {
				float distance = (linkup->whichPtT().pt - matchPt).length();
				if (closestDistance > distance) {
					found.clear();
					closestDistance = distance;
					closest = linkup;
					closestEnd = linkup->whichEnd;
					closestReverse = AllowReversal::no;
				} else if (closestDistance == distance)
					found.emplace_back(linkup, linkup->whichEnd, AllowReversal::no);	// keep ties
			}
			if (linkup->lastEdge && !linkup->lastEdge->nextEdge) {
				OP_ASSERT(!linkup->lastEdge->nextEdge);
				float distance = (linkup->lastEdge->whichPtT(EdgeMatch::end).pt - matchPt).length();
				if (closestDistance > distance) {
					found.clear();
					closestDistance = distance;
					closest = linkup->lastEdge;
					closestEnd = Opposite(linkup->lastEdge->whichEnd);
					closestReverse = AllowReversal::yes;
				} else if (closestDistance == distance)
					found.emplace_back(linkup->lastEdge, Opposite(linkup->lastEdge->whichEnd),
							AllowReversal::yes);	// keep ties
			}
		}
		if (!closest) {	// look for a run of unsectables that close the gap
			OP_ASSERT(!edge->priorEdge);
			OP_ASSERT(!lastEdge->nextEdge);
			if (OpEdge* result = linkUp(edge))
				return result;
		}
#if OP_DEBUG
		if (!closest) {
			focus(edge->id);
			oo(10);
			showPoints();
			showHex();
			OpDebugOut("");
		}
#endif
		OP_ASSERT(closest);
		found.emplace_back(closest, closestEnd, closestReverse);
	}
	OP_ASSERT(found.size());
	OpRect bestBounds;
	closest = nullptr;
	closestEnd = EdgeMatch::none;
	closestReverse = AllowReversal::no;
	for (int trial = 0; !closest && trial < 2; ++trial) {
		for (const auto& foundOne : found) {
			OpEdge* oppEdge = foundOne.edge;
			// skip edges which flip the fill sum total, implying there is a third edge inbetween
			OP_ASSERT(EdgeMatch::start == foundOne.whichEnd || EdgeMatch::end == foundOne.whichEnd);
			               // e.g., one if normals point to different fill sums       
			if (!trial && (((lastEdge->sum.sum() + oppEdge->sum.sum()) & 1) 
			// e.g., one if last start-to-end connects to found start-to-end (end connects to start)
					== (lastEdge->whichEnd == foundOne.whichEnd)))
				continue;
			// choose smallest closed loop -- this is an arbitrary choice
			OpPointBounds testBounds = edge->setLinkBounds();
			testBounds.add(oppEdge->setLinkBounds());
			if (!(bestBounds.area() < testBounds.area())) {	// 'not' logic since best = NaN at first
				bestBounds = testBounds;
				closest = oppEdge;
				closestEnd = foundOne.whichEnd;
				closestReverse = foundOne.reverse;
			}
		}
	}
	OP_ASSERT(closest); // !!! if found is not empty, but no edge has the right sum, choose one anyway?
	OP_ASSERT(AllowReversal::yes == closestReverse || closest->lastEdge);
	if (AllowReversal::yes == closestReverse)
		closest->setLinkDirection(closestEnd);
	OpEdge* clearClosest = closest;
	do {
		clearClosest->clearActiveAndPals();
		clearClosest = clearClosest->nextEdge;
	} while (clearClosest && clearClosest->isActive());
	closest->setPriorEdge(lastEdge);
	closest->priorLink = EdgeLink::single;
	lastEdge->setNextEdge(closest);
	lastEdge->nextLink = EdgeLink::single;
	lastEdge = closest->lastEdge;
	closest->lastEdge = nullptr;
	OP_ASSERT(lastEdge);
#if 0
	if (lastEdge->isClosed(this) 
			|| last->contour->contours->closeGap(lastEdge, this, unsectInX))
		return lastEdge->validLoop();
#else
	if (lastEdge->isLoop()) {
		OP_ASSERT(lastEdge->debugValidLoop());
		return lastEdge;
	}
#endif
	if (!lastEdge->nextEdge)
		return matchLinks(edge);
	return matchLinks(lastEdge);
}


void OpEdges::MarkUnsectableGroups(std::vector<EdgeDistance>& distance) {
	int edgeIndex = (int) distance.size();
	while (--edgeIndex >= 0) {
		float dist = distance[edgeIndex].distance;
		if (0 == dist)
			break;
		OP_ASSERT(dist < 0);
	}
	int limit = edgeIndex;
	while (limit > 0 && 0 == distance[limit - 1].distance)
		--limit;
	for (int outer = limit; outer < edgeIndex; ++outer) {	// two or more
		OpEdge* oEdge = distance[outer].edge;
		const auto& pals = oEdge->pals;
		for (int inner = outer + 1; inner <= edgeIndex; ++inner) {
			OpEdge* iEdge = distance[inner].edge;
			if (pals.end() != std::find(pals.begin(), pals.end(), iEdge))
				continue;
		   OP_ASSERT(iEdge->pals.end() == std::find(iEdge->pals.begin(), iEdge->pals.end(), oEdge));
			oEdge->sumType = EdgeSum::unsectable;
			oEdge->pals.push_back(iEdge);
			iEdge->sumType = EdgeSum::unsectable;
			iEdge->pals.push_back(oEdge);
		}
	}
	bool lastIsUnsectable = false;
	EdgeDistance* lastDistance = nullptr;
	for (auto& dist : distance) {
		bool distIsUnsectable = EdgeSum::unsectable == dist.edge->sumType;
		if (lastIsUnsectable && distIsUnsectable) {
			// check if they were generated from the same curves
			const auto& pals = lastDistance->edge->pals;
			if (pals.end() != std::find(pals.begin(), pals.end(), dist.edge)) {
				lastDistance->multiple = DistMult::none == lastDistance->multiple ?
						DistMult::first : DistMult::mid;
				dist.multiple = DistMult::last;
			}
		}
		lastIsUnsectable = distIsUnsectable;
		lastDistance = &dist;
	}
}

void OpEdges::markUnsortable(OpEdge* edge, Axis axis, ZeroReason reason) {
	if (Axis::vertical == axis || inY.end() == std::find(inY.begin(), inY.end(), edge)) {
//		edge->winding.zero(reason);
		edge->sumType = EdgeSum::unsortable;
		edge->segment->contour->contours->unsortables.push_back(edge);
	}
	edge->fail = Axis::vertical == axis ? EdgeFail::vertical : EdgeFail::horizontal;
}

void OpEdges::SetEdgeMultiple(Axis axis, EdgeDistance* edgeDist  
		OP_DEBUG_PARAMS(std::vector<EdgeDistance>& distance)) {
	OpEdge* edge = edgeDist->edge;
	NormalDirection edgeNorm = edge->normalDirection(axis, edgeDist->t);
	auto addToMany = [=](EdgeDistance& dist, OpEdge* edge  
			OP_DEBUG_PARAMS(DistMult limit)) {
		OP_ASSERT(limit == dist.multiple || DistMult::mid == dist.multiple);
		dist.edgeMultiple = true;
		NormalDirection distNorm = dist.edge->normalDirection(axis, dist.t);
		OP_ASSERT(NormalDirection::downLeft == distNorm
				|| NormalDirection::upRight == distNorm);
		if (edgeNorm == distNorm)
			edge->many += dist.edge->winding;
		else
			edge->many -= dist.edge->winding;
		OP_ASSERT(edge->pals.end() != std::find(edge->pals.begin(), edge->pals.end(), dist.edge));
	};
	OP_ASSERT(NormalDirection::downLeft == edgeNorm
			|| NormalDirection::upRight == edgeNorm);
	edgeDist->edgeMultiple = true;
	EdgeDistance* edgeFirst = edgeDist;
	OP_ASSERT(WindingType::uninitialized == edge->many.debugType);
	edge->many = edge->winding;
	while (DistMult::first != edgeFirst->multiple) {
		OP_ASSERT(edgeFirst > &distance.front() && edgeFirst <= &distance.back());
		addToMany(*--edgeFirst, edge  OP_DEBUG_PARAMS(DistMult::first));
	}
	EdgeDistance* edgeLast = edgeDist;
	while (DistMult::last != edgeLast->multiple) {
		OP_ASSERT(edgeLast >= &distance.front() && edgeLast < &distance.back());
		addToMany(*++edgeLast, edge  OP_DEBUG_PARAMS(DistMult::last));
	}
}

// if horizontal axis, look at rect top/bottom
ChainFail OpEdges::setSumChain(size_t inIndex, Axis axis) {
	// see if normal at center point is in direction of ray
	std::vector<OpEdge*>& inArray = Axis::horizontal == axis ? inX : inY;
	OpEdge* edge = inArray[inIndex];
	OP_ASSERT(edge->winding.visible());
	const OpSegment* edgeSeg = edge->segment;
	OpVector ray = Axis::horizontal == axis ? OpVector{ 1, 0 } : OpVector{ 0, 1 };
	bool overflow;
	float NxR = edgeSeg->c.tangent(edge->center.t).normalize(&overflow).cross(ray);
	if (overflow)
		OP_DEBUG_FAIL(*edge, ChainFail::normalizeOverflow);
	if (fabs(NxR) < WINDING_NORMAL_LIMIT) {
		markUnsortable(edge, axis, ZeroReason::tangentXRay);
		return ChainFail::normalizeUnderflow;  // nonfatal error
	}
	// intersect normal with every edge in the direction of ray until we run out 
	Axis perpendicular = !axis;
	float center = edge->center.pt.choice(perpendicular);
	float normal = edge->center.pt.choice(axis);
	if (normal == edge->start.pt.choice(axis)
			|| normal == edge->end.pt.choice(axis)) {
		markUnsortable(edge, axis, ZeroReason::noNormal);
		return ChainFail::noNormal;  // nonfatal error
	}
	float edgeCenterT = edge->center.t;
	// advance to furthest that could influence the sum winding of this edge
	inIndex += 1;
	for (; inIndex < inArray.size(); ++inIndex) {
		OpEdge* advance = inArray[inIndex];
		if (advance->ptBounds.ltChoice(perpendicular) > center)
			break;
	}
	std::vector<EdgeDistance> distance;
	FoundIntercept foundIntercept = findRayIntercept(inIndex, axis, edge, center, normal,
			edgeCenterT, &distance);
	OP_ASSERT(distance.size());
	if (FoundIntercept::fail == foundIntercept)
		return ChainFail::failIntercept;
	if (FoundIntercept::overflow == foundIntercept)
		return ChainFail::normalizeOverflow;
	OP_ASSERT(Axis::vertical == axis || FoundIntercept::set != foundIntercept);
	std::sort(distance.begin(), distance.end(), CompareDistance());	// sorts from high to low
	// walk edge from low to high (backwards) until simple edge with sum winding is found
	// if edge is unsectable and next to another unsectable, don't set its sum winding, 
	// but accumulate its winding for other edges and store accumulated total in each unsectable
	if (ResolveWinding::fail == setWindingByDistance(edge, axis, distance))
		return ChainFail::failIntercept;	// !!! replace with unique fail state
	return ChainFail::none;
}

// distance array may include edges to right. Remove code that collects that?
ResolveWinding OpEdges::setWindingByDistance(OpEdge* edge, Axis axis, 
		std::vector<EdgeDistance>& distance) {
	if (1 == distance.size()) {
		OP_ASSERT(edge == distance[0].edge);
		OP_ASSERT(EdgeSum::unsectable != edge->sumType);
//			return ResolveWinding::loop;	// !!! this shouldn't be possible (?)
		return CalcFail::fail == edge->calcWinding(axis, distance[0].t) ?
				ResolveWinding::fail : ResolveWinding::resolved;
	}
	// mark consecutive pairs or more of unsectable as multiples
	MarkUnsectableGroups(distance);
	// find edge; then walk backwards to first known sum 
	int sumIndex = (int) distance.size();
	int edgeIndex  OP_DEBUG_CODE(= -1);
	while (--sumIndex >= 0) {
		EdgeDistance* edgeDist = &distance[sumIndex];
		if (edgeDist->distance < 0)
			continue;
		if (edgeDist->edge == edge) {
			OP_ASSERT(0 == edgeDist->distance);
			edgeIndex = sumIndex;
			if (DistMult::none != edgeDist->multiple)
				SetEdgeMultiple(axis, edgeDist  OP_DEBUG_PARAMS(distance));
			continue;
		}
		if (0 == edgeDist->distance)
			continue;
		OP_ASSERT(edgeDist->distance > 0);
		OpEdge* prior = edgeDist->edge;
		if (EdgeSum::unsectable != prior->sumType && prior->sum.isSet())
			break;
	}
	// starting with found or zero if none, accumulate sum up to winding
	OpWinding sumWinding(WindingTemp::dummy);
	if (sumIndex >= 0) {
		EdgeDistance& sumDistance = distance[sumIndex];
		OpEdge* sumEdge = sumDistance.edge;
		sumWinding = sumEdge->sum;
		OP_DEBUG_CODE(sumWinding.debugType = WindingType::temp);
		// if pointing down/left, subtract winding
		if (CalcFail::fail == sumEdge->subIfDL(axis, sumDistance.t, &sumWinding))  
			OP_DEBUG_FAIL(*sumEdge, ResolveWinding::fail);
	}
	// walk from the known sum to (and including) the edge
	int walkIndex = sumIndex;
	OP_ASSERT(edgeIndex >= 0);
	while (++walkIndex <= edgeIndex) {
		EdgeDistance& dist = distance[walkIndex];
		if (dist.edgeMultiple)
			break;
		OpEdge* prior = dist.edge;
		NormalDirection normDir = prior->normalDirection(axis, dist.t);
		OP_ASSERT(NormalDirection::downLeft == normDir || NormalDirection::upRight == normDir);
		bool sumSet = EdgeSum::unsectable != prior->sumType || prior == edge;
		if (sumSet && NormalDirection::downLeft == normDir)
			OP_EDGE_SET_SUM(prior, sumWinding);
		if (CalcFail::fail == prior->addSub(axis, dist.t, &sumWinding)) // if d/l sub; if u/r add
			OP_DEBUG_FAIL(*prior, ResolveWinding::fail);
		if (sumSet && NormalDirection::upRight == normDir)
			OP_EDGE_SET_SUM(prior, sumWinding);
	}
	// accumate the winding for all in the group, then set all sums to that accumulation
	if (distance[edgeIndex].edgeMultiple) {
		NormalDirection normDir = edge->normalDirection(axis, distance[edgeIndex].t);
		OP_ASSERT(NormalDirection::downLeft == normDir || NormalDirection::upRight == normDir);
		if (NormalDirection::upRight == normDir)
			sumWinding += edge->many;
		OP_EDGE_SET_SUM(edge, sumWinding);
	}
	return ResolveWinding::resolved;
}

FoundWindings OpEdges::setWindings(OpContours* contours) {
	// test sum chain for correctness; recompute if prior or next are inconsistent
	for (Axis axis : { Axis::horizontal, Axis::vertical }) {
		std::vector<OpEdge*>& edges = Axis::horizontal == axis ? inX : inY;
		for (size_t index = 0; index < edges.size(); ++index) {
			OpEdge* edge = edges[index];
			if (edge->sum.isSet())
				continue;
			if (!edge->winding.visible())	// may not be visible in vertical pass
				continue;
			if (EdgeFail::horizontal == edge->fail && Axis::vertical == axis)
				edge->fail = EdgeFail::none;
			else if (EdgeSum::unsortable == edge->sumType)  // may be too small
				continue;
			ChainFail chainFail = setSumChain(index, axis);
			if (ChainFail::normalizeOverflow == chainFail)
				OP_DEBUG_FAIL(*edge, FoundWindings::fail);
		}
		for (auto& edge : edges) {
			if (!edge->winding.visible())
				continue;
			if (edge->sum.isSet())
				continue;
			if (EdgeSum::unsortable == edge->sumType)
				continue;
			if (edge->fail == EdgeFail::horizontal)
				continue;
			OP_DEBUG_FAIL(*edge, FoundWindings::fail);
		}
	}
	return FoundWindings::yes;
}

static bool compareXBox(const OpEdge* s1, const OpEdge* s2) {
	const OpRect& r1 = s1->ptBounds;
	const OpRect& r2 = s2->ptBounds;
	if (r1.left < r2.left)
		return true;
	if (r1.left > r2.left)
		return false;
	if (r1.left == r2.left && r1.right < r2.right)
		return true;
	if (r1.left == r2.left && r1.right > r2.right)
		return false;
	return s1->id < s2->id;
}

// starting at left (-x), increasing
static bool compareXCenter(const OpEdge* s1, const OpEdge* s2) {
	return s1->ptBounds.left < s2->ptBounds.left;
}

// starting at top (-y), increasing
static bool compareYCenter(const OpEdge* s1, const OpEdge* s2) {
	return s1->ptBounds.top < s2->ptBounds.top;
}

void OpEdges::sort(EdgesToSort sortBy) {
	if (EdgesToSort::byBox == sortBy || EdgesToSort::unsectable == sortBy) {
		std::sort(inX.begin(), inX.end(), compareXBox);
		return;
	}
	OP_ASSERT(EdgesToSort::byCenter == sortBy);
	std::sort(inX.begin(), inX.end(), compareXCenter);
	std::sort(inY.begin(), inY.end(), compareYCenter);
}
