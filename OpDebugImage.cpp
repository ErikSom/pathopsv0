#include "OpDebug.h"
#include "OpDebugImage.h"

#if OP_DEBUG_IMAGE
#ifdef _WIN32
#pragma optimize( "", off )
#endif

#include <algorithm>
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPath.h"
#include "include/core/SkPaint.h"
#include "OpDebugDouble.h"
#include "OpDebugDump.h"
#include "OpContour.h"
#include "OpCurveCurve.h"
#include "OpEdge.h"
#include "OpJoiner.h"
#include "OpMath.h"
#include "OpSegment.h"
#include "OpTightBounds.h"
#include "OpWinder.h"
#include "PathOps.h"

SkBitmap bitmap;
SkFont labelFont(nullptr, 14, 1, 0);

std::vector<const OpIntersection*> coincidences;
std::vector<const OpEdge*> localEdges;
std::vector<const OpEdge*> foundEdges;
std::vector<OpDebugRay> lines;
std::vector<OpInPath> operands;
std::vector<const SkPath*> paths;
std::vector<const OpSegment*> localSegments;	// segments not yet added to global debug contour
int gridIntervals = 8;
int valuePrecision = -1;		// minus one means unset

#define OP_X(Thing) \
bool draw##Thing##On = false;
MASTER_LIST
#undef OP_X
bool lastDrawOperandsOn = false;

#define DRAW_IDS_ON(Thing) \
	do { \
		draw##Thing##On = true; \
		drawIDsOn = true; \
	} while (false)


struct OpDebugSegmentIter {
    OpDebugSegmentIter(bool start) {
		if (start) {
			localSegmentIndex = 0;
			return;
		}
		localSegmentIndex = localSegments.size();
		for (const auto& c : debugGlobalContours->contours) {
			localSegmentIndex += c.segments.size();
		}
	}

    bool operator!=(OpDebugSegmentIter rhs) { 
		return localSegmentIndex != rhs.localSegmentIndex; 
	}

    const OpSegment* operator*() {
		if (localSegmentIndex < localSegments.size())
			return localSegments[localSegmentIndex];
		size_t index = localSegments.size();
		for (const auto& c : debugGlobalContours->contours) {
			for (const auto& seg : c.segments) {
				if (index == localSegmentIndex)
					return &seg;
				++index;
			}
		}
		OpDebugOut("iterator out of bounds! localSegmentIndex: " + STR(localSegmentIndex) + "\n");
		return nullptr; 
	}

    void operator++() { 
		++localSegmentIndex;
	}

	size_t localSegmentIndex;
};

struct OpDebugSegmentIterator {
    OpDebugSegmentIter begin() { return OpDebugSegmentIter(true); }
    OpDebugSegmentIter end() { return OpDebugSegmentIter(false); }
	bool empty() { return !(begin() != end()); }
};

OpDebugSegmentIterator segmentIterator;

struct OpDebugEdgeIter {
    OpDebugEdgeIter(bool start)
		: isFound(false)
		, isLocal(false)
		, isTemporary(false)
		, isOpp(false)
		, isLine(false) {
		if (start) {
			localEdgeIndex = 0;
			return;
		}
		localEdgeIndex = localEdges.size();
		localEdgeIndex += foundEdges.size();
		for (const auto& c : debugGlobalContours->contours) {
			for (const auto& s : c.segments)
				localEdgeIndex += s.edges.size();
		}
		if (OpCurveCurve::debugActive) {
			localEdgeIndex += OpCurveCurve::debugActive->edgeCurves.size();
			localEdgeIndex += OpCurveCurve::debugActive->oppCurves.size();
			localEdgeIndex += OpCurveCurve::debugActive->edgeLines.size();
			localEdgeIndex += OpCurveCurve::debugActive->oppLines.size();
			localEdgeIndex += OpCurveCurve::debugActive->edgeResults.size();
			localEdgeIndex += OpCurveCurve::debugActive->oppResults.size();
		}
	}

    bool operator!=(OpDebugEdgeIter rhs) { 
		return localEdgeIndex != rhs.localEdgeIndex; 
	}

    const OpEdge* operator*() {
		if (localEdgeIndex < localEdges.size()) {
			isFound = false;
			isLocal = true;
			isTemporary = false;
			isOpp = false;   // !!! unset, at present, for local edges
			isLine = false;
			return localEdges[localEdgeIndex];
		}
		size_t index = localEdges.size();
		if (localEdgeIndex < index + foundEdges.size()) {
			isFound = true;
			isLocal = false;
			isTemporary = false;
			isOpp = false;
			isLine = false;
			return foundEdges[localEdgeIndex - index];
		}
		index += foundEdges.size();
		for (const auto& c : debugGlobalContours->contours) {
			for (const auto& s : c.segments) {
				for (const auto& edge : s.edges) {
					if (index == localEdgeIndex) {
						isFound = false;
						isLocal = false;
						isTemporary = false;
						isOpp = false; // !!! unset, at present, for permanent edges
						return &edge;
					}
					++index;
				}
			}
		}
		if (OpCurveCurve::debugActive) {
			auto advanceEdge = [&](const std::vector<OpEdge>& edges, bool opp, bool line) {	// lambda
				if (index + edges.size() > localEdgeIndex) {
					isFound = false;
					isLocal = true;
					isTemporary = true;
					isOpp = opp;
					isLine = line;
					return &edges[localEdgeIndex - index];
				}
				index += edges.size();
				return (const OpEdge* ) nullptr;
			};
			for (auto edgePtr : { &OpCurveCurve::debugActive->edgeCurves,
					&OpCurveCurve::debugActive->oppCurves,
					&OpCurveCurve::debugActive->edgeLines,
					&OpCurveCurve::debugActive->oppLines,
					&OpCurveCurve::debugActive->edgeResults,
					&OpCurveCurve::debugActive->oppResults } ) {
				const OpEdge* result = advanceEdge(*edgePtr, false, false);
				if (result)
					return result;
			}
		}
		OpDebugOut("iterator out of bounds! localEdgeIndex: " + STR(localEdgeIndex) + 
				"; max index: " + STR(index) + "\n");
		return nullptr; 
	}

    void operator++() { 
		++localEdgeIndex;
	}

	bool isFound;
	bool isLocal;
	bool isTemporary;
	bool isOpp;
	bool isLine;
	size_t localEdgeIndex;
};

struct OpDebugEdgeIterator {
    OpDebugEdgeIter begin() { return OpDebugEdgeIter(true); }
    OpDebugEdgeIter end() { return OpDebugEdgeIter(false); }
	bool empty() { return !(begin() != end()); }
};

OpDebugEdgeIterator edgeIterator;

struct OpDebugIntersectionIter {
    OpDebugIntersectionIter(bool start) {
		localIntersectionIndex = 0;
		if (start)
			return;
		for (const auto& c : debugGlobalContours->contours) {
			for (const auto& seg : c.segments) {
				localIntersectionIndex += seg.intersections.size();
			}
		}
	}

    bool operator!=(OpDebugIntersectionIter rhs) { 
		return localIntersectionIndex != rhs.localIntersectionIndex; 
	}

    const OpIntersection* operator*() {
		size_t index = 0;
		for (const auto& c : debugGlobalContours->contours) {
			for (const auto& seg : c.segments) {
				for (const auto sect : seg.intersections) {
					if (index == localIntersectionIndex)
						return sect;
					++index;
				}
			}
		}
		OpDebugOut("iterator out of bounds! localIntersectionIndex: " + STR(localIntersectionIndex) + "\n");
		return nullptr; 
	}

    void operator++() { 
		++localIntersectionIndex;
	}

	size_t localIntersectionIndex;
};

struct OpDebugIntersectionIterator {
    OpDebugIntersectionIter begin() { return OpDebugIntersectionIter(true); }
    OpDebugIntersectionIter end() { return OpDebugIntersectionIter(false); }
	bool empty() { return !(begin() != end()); }
};

OpDebugIntersectionIterator intersectionIterator;


struct OpDebugDefeatDelete {
	OpDebugDefeatDelete() {
		save = debugGlobalContours->debugInPathOps;
		debugGlobalContours->debugInPathOps = false;
	}
	~OpDebugDefeatDelete() {
		debugGlobalContours->debugInPathOps = save;
	}

	bool save;
};

void OpDebugImage::addToPath(const OpCurve& curve, SkPath& path) {
	path.moveTo(curve.pts[0].x, curve.pts[0].y);
	switch (curve.type) {
		case OpType::no:
			OP_ASSERT(0);
			break;
		case OpType::line:
			path.lineTo(curve.pts[1].x, curve.pts[1].y);
			break;
		case OpType::quad:
			path.quadTo(curve.pts[1].x, curve.pts[1].y, curve.pts[2].x, curve.pts[2].y);
			break;
		case OpType::conic:
			path.conicTo(curve.pts[1].x, curve.pts[1].y, curve.pts[2].x, curve.pts[2].y, curve.weight);
			break;
		case OpType::cubic:
			path.cubicTo(curve.pts[1].x, curve.pts[1].y, curve.pts[2].x, curve.pts[2].y,
					curve.pts[3].x, curve.pts[3].y);
			break;
		default:
			OP_ASSERT(0);
	}
}

void OpDebugImage::init(const OpInPath& left, const OpInPath& right) {
	bitmap.allocPixels(SkImageInfo::MakeN32Premul(bitmapWH, bitmapWH));
	::clear();
	operands.clear();
	paths.clear();
	operands.push_back(left);
	operands.push_back(right);
	drawLeftOn = true;
	drawRightOn = true;
	drawSegmentEdgesOn = true;
	drawTemporaryEdgesOn = true;
	SkRect opBounds = left.skPath->getBounds();
	opBounds.join(right.skPath->getBounds());
	DebugOpSetBounds(opBounds.fLeft, opBounds.fTop, opBounds.fRight, opBounds.fBottom);
}

void playback() {
	FILE* file = fopen("OpDebugImageState.txt", "r");
	if (!file)
		return;
	char str[255];
	double debugZoom;
	double debugCenter[2];
	float textSize;
	int intervals;
	int precision;
	// required
	if (fscanf(file, "debugZoom: %lg\n", &debugZoom) != 1) {
		OpDebugOut("reading debugZoom failed\n");
		goto bail;
	}
	DebugOpSetZoom(debugZoom);
	if (fscanf(file, "debugCenter: %lg, %lg\n", &debugCenter[0], &debugCenter[1]) != 2) {
		OpDebugOut("reading debugCenter failed\n");
		goto bail;
	}
	DebugOpSetCenter(debugCenter[0], debugCenter[1]);
	DebugOpResetBounds();
	if (fscanf(file, "textSize: %g\n", &textSize) != 1) {
		OpDebugOut("reading textSize failed\n");
		goto bail;
	}
	labelFont.setSize(textSize);
	if (fscanf(file, "gridIntervals: %d\n", &intervals) != 1) {
		OpDebugOut("reading gridIntervals failed\n");
		goto bail;
	}
	gridIntervals = intervals;
	if (fscanf(file, "valuePrecision: %d\n", &precision) != 1) {
		OpDebugOut("reading valuePrecision failed\n");
		goto bail;
	}
	valuePrecision = precision;
	// optional
#define OP_X(Thing) \
	draw##Thing##On = false;
	MASTER_LIST
#undef OP_X
	while (fgets(str, sizeof(str), file)) {
#define OP_X(Thing) \
		if (strlen(str) - 1 == strlen(#Thing) && 0 == strncmp(#Thing, str, strlen(#Thing))) \
			draw##Thing##On = true; \
		else
	MASTER_LIST
#undef OP_X
		{
			OpDebugOut("no match: " + std::string(str)); goto bail;
		}
	}
	redraw();
bail:
	fclose(file);
}

#undef READ_FEATURE

void OpDebugImage::clearScreen() {
	SkCanvas offscreen(bitmap);
	offscreen.clear(SK_ColorWHITE);
}

void OpDebugImage::drawPath(const SkPath& path, uint32_t color) {
	SkCanvas offscreen(bitmap);
	SkPaint paint;
	paint.setAntiAlias(true);
	paint.setStyle(SkPaint::kStroke_Style);
	paint.setColor(color);
	offscreen.drawPath(path, paint);
}

void OpDebugImage::drawDoubleFocus() {
	OpDebugDefeatDelete defeater;
	std::vector<int> ids;
	clearScreen();
	if (drawOperandsOn != lastDrawOperandsOn) {
		drawLeftOn = drawRightOn = drawOperandsOn;
		lastDrawOperandsOn = drawOperandsOn;
	}
	if (drawFillOn) {
		SkMatrix matrix;
		float scale = (float)DebugOpGetZoomScale();
		matrix.setScale(scale, scale);
		matrix.preTranslate(-DebugOpGetCenterX(), -DebugOpGetCenterY());
		matrix.postTranslate(DebugOpGetOffsetX(), DebugOpGetOffsetY());
		if (drawLeftOn) 
			drawDoubleFill(operands[0].skPath->makeTransform(matrix), SkColorSetARGB(10, 255, 0, 0));
// was:		DebugOpFill(operands[0], SkColorSetARGB(63, 255, 0, 0));
		if (drawRightOn)
			drawDoubleFill(operands[1].skPath->makeTransform(matrix), SkColorSetARGB(10, 0, 0, 255));
//	was:	DebugOpFill(operands[1], SkColorSetARGB(63, 0, 0, 255));
	}
	if (drawLeftOn || drawRightOn)
		DebugOpClearInputs();
	if (drawLeftOn)
		DebugOpAdd(operands[0]);
	if (drawRightOn)
		DebugOpAdd(operands[1]);
	if (drawLeftOn || drawRightOn)
		DebugOpDrawInputs();
	if (drawOutputsOn && debugGlobalContours->debugResult)
		DebugOpDraw(debugGlobalContours->debugResult);
	if (drawPathsOn)
		DebugOpDraw(paths);
	if (drawLinesOn)
		DebugOpDraw(lines);
	if (drawSegmentsOn) {
		DebugOpClearSegments();
		for (auto segment : segmentIterator)
			DebugOpAdd(segment);
		DebugOpDrawSegments();
	}
	if (drawEdgesOn || drawNormalsOn || drawWindingsOn) {
		DebugOpClearEdges();
		for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
			const OpEdge* edge = *edgeIter;
			if (!drawTemporaryEdgesOn && edgeIter.isLocal)
				continue;
			if (!drawSegmentEdgesOn && !edgeIter.isLocal)
				continue;
			DebugOpAdd(edge);
		}
	}
	if (drawEdgesOn)
		DebugOpDrawEdges();
	if (drawPointsOn)
		OpDebugImage::drawPoints();
	if (drawSegmentsOn && drawIDsOn) {
		for (auto segment : segmentIterator)
			DebugOpDrawSegmentID(segment, ids);
	}
	if (drawEdgesOn && (drawIDsOn || drawNormalsOn || drawTangentsOn || drawWindingsOn)) {
		for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
			const OpEdge* edge = *edgeIter;
			if (!drawTemporaryEdgesOn && edgeIter.isLocal)
				continue;
			if (!drawSegmentEdgesOn && !edgeIter.isLocal)
				continue;
			if (ids.end() != std::find(ids.begin(), ids.end(), edge->id))
				continue;
			ids.push_back(edge->id);

			if (drawIDsOn) {
				SkColor color = SK_ColorBLACK;
				if (!edge->winding.visible())
					color = SK_ColorRED;
				else if (edgeIter.isTemporary)
					color = edgeIter.isOpp ? 0xFFFFA500 : 0xFF008000;  // orange, dark green
				DebugOpDrawEdgeID(edge, color);
			}
			if (drawNormalsOn)
				DebugOpDrawEdgeNormal(edge, SK_ColorBLACK);
			if (drawTangentsOn)
				DebugOpDrawEdgeTangent(edge, SK_ColorBLACK);
			if (drawWindingsOn)
				DebugOpDrawEdgeWinding(edge, SK_ColorBLACK);
		}
	}
	if (drawIntersectionsOn && drawIDsOn) {
		for (auto sect : intersectionIterator)
			DebugOpDrawIntersectionID(sect, ids);
	}
	if (drawSegmentsOn && drawCoincidencesOn) {
		for (auto segment : segmentIterator)
			DebugOpDrawPointID(segment, ids);
	}
#if 0 // unimplmented
	if (drawSegmentsOn && drawTangentsOn) {
		for (auto segment : segmentIterator)
			DebugOpDrawTangent(segment);
	}
#endif
	if (drawGridOn)
		drawGrid();
}

void record() {
#if 0 && defined _WIN32
   char full[_MAX_PATH];
   if( _fullpath( full, ".\\", _MAX_PATH ) != NULL )
      OpDebugOut( "Full path is: %s" + std::string(full) + "\n");
   else
      OpDebugOut( "Invalid path\n" );
#endif
	FILE* recordFile = fopen("opDebugImageState.txt", "w");
	if (!recordFile) {
		OpDebugOut("failed to open opDebugImageState.txt for writing\n");
		return;
	}
	DebugOpRecord(recordFile);
	fprintf(recordFile, "textSize: %g\n", labelFont.getSize());
	fprintf(recordFile, "gridIntervals: %d\n", gridIntervals);
	fprintf(recordFile, "valuePrecision: %d\n", valuePrecision);
#define OP_X(Thing) \
	if (draw##Thing##On) \
		fprintf(recordFile, "%s\n", #Thing);
	MASTER_LIST
#undef OP_X
	fclose(recordFile);
}

#undef RECORD_FEATURE

void OpDebugImage::drawDoubleCenter(OpPoint pt, bool add) {
	add ? DebugOpAddBounds(pt.x, pt.y, pt.x, pt.y) :	// !!! should suppress zoom change
			DebugOpSetCenter(pt.x, pt.y);
	drawDoubleFocus();
}

void OpDebugImage::drawDoubleFocus(const OpPointBounds& b, bool add) {
	add ? DebugOpAddBounds(b.left, b.top, b.right, b.bottom) :
			DebugOpSetBounds(b.left, b.top, b.right, b.bottom);
	drawDoubleFocus();
}

void OpDebugImage::drawDoubleFill(const SkPath& path, uint32_t color, bool strokeAndFill) {
	SkCanvas offscreen(bitmap);
	SkPaint paint;
	paint.setAntiAlias(true);
	paint.setColor(color);
	if (strokeAndFill) {
		offscreen.drawPath(path, paint);
		paint.setColor(SK_ColorBLACK);
	}
	paint.setStyle(SkPaint::kFill_Style);
	offscreen.drawPath(path, paint);
}

void OpDebugImage::drawDoublePath(const SkPath& path, uint32_t color, bool strokeAndFill) {
	SkCanvas offscreen(bitmap);
	SkPaint paint;
	paint.setAntiAlias(true);
	paint.setColor(color);
	if (strokeAndFill) {
		offscreen.drawPath(path, paint);
		paint.setColor(SK_ColorBLACK);
	}
	paint.setStyle(SkPaint::kStroke_Style);
	offscreen.drawPath(path, paint);
}

void OpDebugImage::drawGrid() {
	SkCanvas offscreen(bitmap);
	SkPaint paint;
	paint.setAntiAlias(true);
	paint.setColor(0x3f000000);
	SkPaint textPaint = paint;
	paint.setStyle(SkPaint::kStroke_Style);
	const float wh = (float) bitmapWH;
 	const float interval = wh / gridIntervals;
	const int xOffset = 2;
	double left, top, right, bottom;
	DebugOpBounds(left, top, right, bottom);
	float xVal = left;
	float yVal = top;
	for (float x = 0; x < wh; x += interval) {
		if (x != 0) {
			offscreen.drawLine(x, 0, x, wh, paint);
			offscreen.drawLine(0, x, wh, x, paint);
		}
		if (!drawValuesOn)
			continue;
		std::string xValStr = drawHexOn ? OpDebugDumpHex(xVal) : OpDebugToString(xVal, valuePrecision);
		offscreen.drawString(SkString(xValStr), x + xOffset, bitmapWH - xOffset, labelFont, textPaint);
		xVal += (right - left) / gridIntervals;
		offscreen.save();
		offscreen.rotate(-90, 15, x - xOffset);
		std::string yValStr = drawHexOn ? OpDebugDumpHex(yVal) : OpDebugToString(yVal, valuePrecision);
		offscreen.drawString(SkString(yValStr), 15, x - xOffset, labelFont, textPaint);
		offscreen.restore();
		yVal += (bottom - top) / gridIntervals;
	}
}

// !!! should be named 'move grid center to' ? or should it keep center while zooming?
void gridCenter(int x, int y) {
	if (x < 0 || x >= gridIntervals || y < 0 || y >= gridIntervals)
		return OpDebugOut("parameters must be in grid interval range: 0 to " + STR(gridIntervals) + "\n");
	double left, top, right, bottom;
	DebugOpBounds(left, top, right, bottom);
	OpPoint center(left + (right - left) * (gridIntervals - x) / gridIntervals,
			top + (bottom - top) * (gridIntervals - y) / gridIntervals);
	OpDebugImage::drawDoubleCenter(center, false);
}

void gridLines(int intervals) {
	gridIntervals = intervals;
	OpDebugImage::drawDoubleFocus();
}

void gridStep(float dxy) {
	double left, top, right, bottom;
	DebugOpBounds(left, top, right, bottom);
	double currentStep = (right - left) / gridIntervals;
	double newZoom = DebugOpGetZoomScale() * currentStep / dxy;
	DebugOpSetZoomScale(newZoom);
	// !!! OP_ASSERT bounds is square?
	OpDebugImage::drawDoubleFocus();
}

void precision(int p) {
	valuePrecision = p;
	OpDebugImage::drawDoubleFocus();
}

void redraw() {
	OpDebugImage::drawDoubleFocus();
}

void textSize(float s) {
	labelFont.setSize(s);
	OpDebugImage::drawDoubleFocus();
}

void OpDebugImage::center(int id, bool add) {
	ConstOpPointBoundsPtr pointBoundsPtr = nullptr;
	ConstOpPointPtr pointPtr = nullptr;
	find(id, &pointBoundsPtr, &pointPtr);
	if (pointBoundsPtr)
		return OpDebugImage::drawDoubleCenter(pointBoundsPtr->center(), add);
	if (pointPtr)
		return OpDebugImage::drawDoubleCenter(*pointPtr, add);
}

typedef const OpPointBounds* ConstOpPointBoundsPtr;

void OpDebugImage::find(int id, ConstOpPointBoundsPtr* boundsPtr, ConstOpPointPtr* pointPtr) {
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		const OpEdge* edge = *edgeIter;
		if (id != edge->id)
			continue;
		if (!drawTemporaryEdgesOn && edgeIter.isLocal 
				&& foundEdges.end() == std::find(foundEdges.begin(), foundEdges.end(), edge))
			foundEdges.push_back(edge);
		if (!drawSegmentEdgesOn && !edgeIter.isLocal 
				&& foundEdges.end() == std::find(foundEdges.begin(), foundEdges.end(), edge))
			foundEdges.push_back(edge);
		DRAW_IDS_ON(Edges);
		*boundsPtr = &edge->ptBounds;
		return;
	}
	const OpSegment* segment = nullptr;
	for (auto s : segmentIterator) {
		if (id == s->id)
			segment = s;
	}
	if (segment) {
		DRAW_IDS_ON(Segments);
		*boundsPtr = &segment->ptBounds;
		return;
	}
	const OpIntersection* sect = nullptr;
	for (auto i : intersectionIterator) {
		if (id == i->id)
			sect = i;
	}
	if (sect) {
		DRAW_IDS_ON(Intersections);
		// don't change zoom
		*pointPtr = &sect->ptT.pt;
		return;
	}
	const OpIntersection* coin = nullptr;
	for (auto c : coincidences) {
		if (id == c->id)
			coin = c;
	}
	if (!coin) {
		coin = findCoincidence(id);
		if (coin)
			coincidences.push_back(coin);
	}
	if (coin) {
		DRAW_IDS_ON(Coincidences);
		// !!! wrong: add rect formed by both intersections with this id
		*boundsPtr = &coin->segment->ptBounds;
		return;
	}
	OpDebugOut("id " + STR(id) + " not found\n");
}

void OpDebugImage::focus(int id, bool add) {
	ConstOpPointBoundsPtr pointBoundsPtr = nullptr;
	ConstOpPointPtr pointPtr = nullptr;
	find(id, &pointBoundsPtr, &pointPtr);
	if (pointBoundsPtr)
		return OpDebugImage::drawDoubleFocus(*pointBoundsPtr, add);
	if (pointPtr)
		return OpDebugImage::drawDoubleCenter(*pointPtr, add);
}

void addFocus(int id){
	OpDebugImage::focus(id, true);
}

void center(int id) {
	OpDebugImage::center(id, false);
}

void center(float x, float y) {
	OpPoint c(x, y);
	OpDebugImage::drawDoubleCenter(c, false);
}

void center(const OpPoint& pt) {
	center(pt.x, pt.y);
}

void center(const OpPtT& ptT) {
	center(ptT.pt);
}

void focus(int id) {
	OpDebugImage::focus(id, false);
}

void OpDebugImage::focusEdges() {
	if (edgeIterator.empty())
		return;
	OpPointBounds focusRect;
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		const OpEdge* edge = *edgeIter;
		if (!drawTemporaryEdgesOn && edgeIter.isLocal)
			continue;
		if (!drawSegmentEdgesOn && !edgeIter.isLocal)
			continue;
		focusRect.add(edge->ptBounds);
	}
	DRAW_IDS_ON(Edges);
	OpDebugImage::drawDoubleFocus(focusRect, false);
}

void focusSegments() {
	if (segmentIterator.empty())
		return;
	OpPointBounds focusRect = (*segmentIterator.begin())->ptBounds;
	for (auto seg : segmentIterator)
		focusRect.add(seg->ptBounds);
	DRAW_IDS_ON(Segments);
	OpDebugImage::drawDoubleFocus(focusRect, false);
}

// immediate window commands

// note that operands are not cleared
void clear() {
	OpDebugImage::clearScreen();
	coincidences.clear();
	OpDebugImage::clearLines();
	paths.clear();
	OpDebugImage::clearLocalEdges();
	localSegments.clear();
	drawLeftOn = false;
	drawRightOn = false;
	drawPathsOn = false;
	DebugOpResetFocus();
}

void clearLines() {
	OpDebugImage::clearLines();
	DebugOpResetFocus();
}

bool OpDebugImage::drawValue(OpPoint pt, std::string ptStr, uint32_t color) {
	SkPaint paint;
	paint.setAntiAlias(true);
	paint.setColor(color);
	SkCanvas offscreen(bitmap);
	SkRect textBounds;
	(void) labelFont.measureText(ptStr.c_str(), ptStr.length(), SkTextEncoding::kUTF8, &textBounds);
	const int xOffset = 2;
	const int yOffset = 1;
	textBounds.inset(-xOffset, -yOffset);
	SkRect trimmed = textBounds;
	trimmed.offset(pt.x, pt.y);
	for (bool allowIntersect : { false, true } ) {
		for (float scale : { 4, 16 } ) {
			for (int toTheLeft : { -1, 0, 1 } ) {
				for (int toTheTop : { -1, 0, 1 } ) {
					OpVector offset { 0, 0 };
					if (toTheLeft)
						offset.dx = toTheLeft < 0 ? -textBounds.width() - scale : scale;
					if (toTheTop)
						offset.dy = toTheTop < 0 ? -scale : textBounds.height() + scale;
					SkRect test = trimmed;
					test.offset(offset.dx, offset.dy);
					SkRect skBounds { 0, 0, bitmapWH, bitmapWH };
					if (allowIntersect ? !skBounds.intersect(test) : !skBounds.contains(test))
						continue;
					int left = std::max(0, (int) test.fLeft);
					int top = std::max(0, (int) test.fTop);
					int right = std::min(bitmapWH, (int) test.fRight);
					int bottom = std::min(bitmapWH, (int) test.fBottom);
					for (int y = top; y < bottom; ++y) {
						for (int x = left; x < right; ++x) {
							uint32_t pixel = bitmap.getColor(x, y);
							if (SkColorGetR(pixel) == SkColorGetG(pixel)
									&& SkColorGetR(pixel) == SkColorGetB(pixel)
									&& SkColorGetR(pixel) < 0xf0)
								goto loopEnd;
							if (SkColorGetR(pixel) < 0x3F
									|| SkColorGetG(pixel) < 0x3F
									|| SkColorGetB(pixel) < 0x3F)
								goto loopEnd;
						}
					}
					offscreen.drawString(SkString(ptStr), pt.x + offset.dx,
							pt.y + offset.dy, labelFont, paint);
					if (16 == scale && drawGuidesOn) {
						// add line from edge of text box towards original point
						SkRect box = test;
						OpPoint closestSide;
						float closest = OpInfinity;
						for (int side = 0; side < 4; ++side) {
							OpPoint tSide = { 
									side & 1 ? box.centerX() : 0 == side ? box.fLeft : box.fRight,
									side & 1 ? 1 == side ? box.fTop : box.fBottom : box.centerY()
							};
							float distance = (tSide - pt).length();
							if (closest > distance) {
								closest = distance;
								closestSide = tSide;
							}
						}
						paint.setAlpha(63);
						offscreen.drawLine(closestSide.toSkPoint(), pt.toSkPoint(), paint);
					}
					return true;
				loopEnd:
					;
				}
			}
		}
	}
	offscreen.drawString(SkString("."), pt.x, pt.y, labelFont, paint);
	return false;
}

void OpDebugImage::drawPoints() {
	DebugOpClearPoints();
	auto drawPathPt = [](const SkPath* path) { // lambda
		SkPath::RawIter iter(*path);
		SkPath::Verb verb;
		do {
			SkPoint pts[4];
			verb = iter.next(pts);
			switch (verb) {
			case SkPath::kMove_Verb:
				break;
			case SkPath::kLine_Verb:
				DebugOpBuild(OpPoint(pts[0].fX, pts[0].fY));
				DebugOpBuild(OpPoint(pts[1].fX, pts[1].fY));
				break;
			case SkPath::kQuad_Verb:
			case SkPath::kConic_Verb:
				DebugOpBuild(OpPoint(pts[0].fX, pts[0].fY));
				if (drawControlsOn)
					DebugOpBuild(OpPoint(pts[1].fX, pts[1].fY));
				DebugOpBuild(OpPoint(pts[2].fX, pts[2].fY));
				break;
			case SkPath::kCubic_Verb:
				DebugOpBuild(OpPoint(pts[0].fX, pts[0].fY));
				if (drawControlsOn) {
					DebugOpBuild(OpPoint(pts[1].fX, pts[1].fY));
					DebugOpBuild(OpPoint(pts[2].fX, pts[2].fY));
				}
				DebugOpBuild(OpPoint(pts[3].fX, pts[3].fY));
				break;
			case SkPath::kClose_Verb:
			case SkPath::kDone_Verb:
				break;
			}
		} while (verb != SkPath::kDone_Verb);
	};
	if (drawLeftOn)
		drawPathPt(operands[0].skPath);
	if (drawRightOn)
		drawPathPt(operands[1].skPath);
	if (drawSegmentsOn) {
		for (auto seg : segmentIterator) {
			DebugOpBuild(seg->c.pts[0]);
			DebugOpBuild(seg->c.lastPt());
			// !!! probably need switch to say 'draw control points'
			if (drawControlsOn) {
				for (int index = 1; index < seg->c.pointCount() - 1; ++index)
					DebugOpBuild(seg->c.pts[index]);
			}
		}
	}
	if (drawEdgesOn) {
		for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
			const OpEdge* edge = *edgeIter;
			if (!drawTemporaryEdgesOn && edgeIter.isLocal)
				continue;
			if (!drawSegmentEdgesOn && !edgeIter.isLocal)
				continue;
			if (edgeIter.isTemporary) {
				DebugOpBuild(edge->start.pt, edgeIter.isOpp);
				DebugOpBuild(edge->end.pt, edgeIter.isOpp);		
			} else {
				DebugOpBuild(edge->start.pt);
				DebugOpBuild(edge->end.pt);
			}
			if (drawControlsOn && edge->curveSet) {
				for (int index = 1; index < edge->curve_impl.pointCount() - 1; ++index)
					DebugOpBuild(edge->curve_impl.pts[index]);
			}
		}
	}
	if (drawIntersectionsOn) {
		for (const auto& sect : intersectionIterator) {
			DebugOpBuild(sect->ptT.pt);
		}
	}
	if (drawLinesOn) {
		for (const auto& line : lines) {
			if (drawLeftOn)
				DebugOpBuild(*operands[0].skPath, line);
			if (drawRightOn)
				DebugOpBuild(*operands[1].skPath, line);
			if (drawSegmentsOn) {
				for (auto seg : segmentIterator) {
					DebugOpBuild(*seg, line);
				}
			}
			if (drawEdgesOn)
				for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
					const OpEdge* edge = *edgeIter;
					if (!drawTemporaryEdgesOn && edgeIter.isLocal)
						continue;
					if (!drawSegmentEdgesOn && !edgeIter.isLocal)
						continue;
					DebugOpBuild(*edge, line);
				}
		}
	}
	if (drawValuesOn) {
		if (drawTsOn)
			DebugOpDrawT(drawHexOn, valuePrecision);
		else
			DebugOpDrawValue(drawHexOn, valuePrecision);
	}
	DebugOpDrawDiamond();
}

void OpDebugImage::addDiamondToPath(OpPoint pt, SkPath& path) {
	SkPath diamond;
	diamond.moveTo(4, 0);
	diamond.lineTo(0, 4);
	diamond.lineTo(-4, 0);
	diamond.lineTo(0, -4);
	diamond.close();
	diamond.offset(pt.x, pt.y);
	path.addPath(diamond);
}

void OpDebugImage::addArrowHeadToPath(const OpLine& line, SkPath& path) {
	const SkPoint arrow[2] { { -10, -6 }, { 6, 6 } };
	float radians = atan2f(line.pts[1].y - line.pts[0].y, line.pts[1].x - line.pts[0].x);
	float degrees = (radians * 180) / 3.14159265f;
	if (degrees < 0)
		degrees += 360;
	SkMatrix matrix;
	matrix.setRotate(degrees);
	SkPoint arrowCopy[2] = { arrow[0], arrow[1] };
	matrix.mapPoints(arrowCopy, 2);
	path.rLineTo(arrowCopy[0].fX, arrowCopy[0].fY);
	path.rLineTo(arrowCopy[1].fX, arrowCopy[1].fY);
}

void OpDebugImage::add(const OpEdge* test) {
	if (!drawTemporaryEdgesOn)
		return;
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		const OpEdge* edge = *edgeIter;
		if (!drawSegmentEdgesOn && !edgeIter.isLocal)
			continue;
		if (test == edge)
			return;
	}
	localEdges.push_back(test);
}

void OpDebugImage::add(Axis axis, float value) {
	lines.emplace_back(axis, value);
}

OpDebugRay::OpDebugRay(const LinePts& pts_) 
	: pts(pts_) {
	if (pts.pts[0].x == pts.pts[1].x) {
		axis = Axis::vertical;
		value = pts.pts[0].x;
		useAxis = true;
		return;
	}
	if (pts.pts[0].y == pts.pts[1].y) {
		axis = Axis::horizontal;
		value = pts.pts[0].y;
		useAxis = true;
		return;
	}
	useAxis = false;
}

void OpDebugImage::add(const OpDebugRay& ray) {
	lines.emplace_back(ray);
}

void OpDebugImage::add(const OpSegment* segment) {
	if (localSegments.end() != std::find(localSegments.begin(), localSegments.end(), segment))
		return;
	localSegments.push_back(segment);
}

void OpDebugImage::clearLocalEdges() {
	foundEdges.clear();
	localEdges.clear();
}

void OpDebugImage::clearIntersections() {
	drawIntersectionsOn = false;
}

void OpDebugImage::clearLines() {
	lines.clear();
	drawLinesOn = false;
}

#define OP_X(Thing) \
void hide##Thing() { \
	draw##Thing##On = false; \
	OpDebugImage::drawDoubleFocus(); \
} \
 \
void show##Thing() { \
	draw##Thing##On = true; \
	OpDebugImage::drawDoubleFocus(); \
} \
 \
void toggle##Thing() { \
	draw##Thing##On ^= true; \
	OpDebugImage::drawDoubleFocus(); \
}
MASTER_LIST
#undef OP_X

void OpContour::draw() const {
	if (!drawSegmentsOn && !drawEdgesOn)
		drawEdgesOn = true;
	OpDebugImage::drawDoubleFocus();
}

void OpContours::draw() const {
	if (!drawSegmentsOn && !drawEdgesOn)
		drawEdgesOn = true;
	OpDebugImage::drawDoubleFocus();
}

OpEdge::~OpEdge() {
	if (debugGlobalContours->debugInClearEdges)
		return;
	if (!debugGlobalContours->debugInPathOps)
		return;
	OP_ASSERT(!segment->debugContains(this));
	for (auto edge = foundEdges.begin(); edge != foundEdges.end(); ++edge) {
		if (id == (*edge)->id) {
			foundEdges.erase(edge);
			return;
		}
	}
	for (auto edge = localEdges.begin(); edge != localEdges.end(); ++edge) {
		if (id == (*edge)->id) {
			localEdges.erase(edge);
			return;
		}
	}
}

void OpEdge::addLink() const {
	drawChain();
}

#if 0
void OpEdge::addSum() const {
	drawChain(EdgeLoop::sum);
}
#endif

void OpEdge::draw() const {
	OpDebugImage::add(this);
	DRAW_IDS_ON(Edges);
	OpDebugImage::drawDoubleFocus();
}

void OpEdge::drawChain() const {
	const OpEdge* chain = this;
		do {
			OpDebugImage::add(chain);
		} while ((chain = chain->nextEdge) && this != chain);
		chain = this;
	while ((chain = chain->priorEdge) && this != chain) {
		OpDebugImage::add(chain);
	}
	DRAW_IDS_ON(Edges);
	OpDebugImage::drawDoubleFocus();
}

void OpEdge::drawLink() const {
	OpDebugImage::clearLocalEdges();
	drawChain();
}

#if 0
void OpEdge::drawSum() const {
	OpDebugImage::clearLocalEdges();
	drawChain();
}
#endif

void OpJoiner::debugDraw() const {
	OpDebugImage::clearLocalEdges();
	for (auto edge : inX)
		OpDebugImage::add(edge);
	for (auto edge : unsectInX)
		OpDebugImage::add(edge);
	OpDebugImage::focusEdges();
}

void OpWinder::debugDraw() const {
	OpDebugImage::clearLocalEdges();
	for (auto edge : inX)
		OpDebugImage::add(edge);
	for (auto edge : inY)
		OpDebugImage::add(edge);
	OpDebugImage::focusEdges();
}

void OpCurveCurve::draw() const {
	if (!edgeCurves.size() && !edgeLines.size())
		return OpDebugOut("OpCurveCurve missing edgeCurves\n");
	OpPointBounds focusRect = edgeCurves.front().ptBounds;
	for (auto edgesPtrs : { &edgeCurves, &oppCurves, &edgeLines, &oppLines, &edgeResults, &oppResults }) {
		for (auto& edge : *edgesPtrs)
			focusRect.add(edge.ptBounds);
	}
	DRAW_IDS_ON(Edges);
	OpDebugImage::drawDoubleFocus(focusRect, false);
}

bool OpDebugImage::drawEdgeNormal(OpVector norm, OpPoint midTPt, int edgeID, uint32_t color) {
	OpLine normal(midTPt, midTPt + norm);
	SkPath normalPath;
	normalPath.moveTo(normal.pts[0].x, normal.pts[0].y);
	if (!normal.pts[1].isFinite()) {
		OpDebugOut("normal not finite on edge " + STR(edgeID) + "\n");
		return false;
	}
	normalPath.lineTo(normal.pts[1].x, normal.pts[1].y);
	OpDebugImage::addArrowHeadToPath(normal, normalPath);
	OpDebugImage::drawPath(normalPath, color);
	return true;
}

bool OpDebugImage::drawEdgeTangent(OpVector tan, OpPoint midTPt, int edgeID, uint32_t color) {
	OpLine tangent(midTPt, midTPt + tan);
	SkPath tangentPath;
	tangentPath.moveTo(tangent.pts[0].x, tangent.pts[0].y);
	if (!tangent.pts[1].isFinite()) {
		OpDebugOut("tangent not finite (edge id)" + STR(edgeID) + "\n");
		return false;
	}
	tangentPath.lineTo(tangent.pts[1].x, tangent.pts[1].y);
	OpDebugImage::addArrowHeadToPath(tangent, tangentPath);
	OpDebugImage::drawPath(tangentPath, color);
	return true;
}

bool OpDebugImage::drawEdgeWinding(OpVector norm, OpPoint midTPt, const OpEdge* edge, uint32_t color) {
	OpPoint sumSide = midTPt + norm;
	OpPoint oppSide = midTPt - norm;
	SkCanvas textLayer(bitmap);
	SkPaint paint;
	paint.setAntiAlias(true);
	paint.setColor(SK_ColorBLACK);
	std::string sumLeft = OpMax == edge->sum.left() ? "?" : STR(edge->sum.left());
	textLayer.drawString(SkString(sumLeft), sumSide.x, sumSide.y, labelFont, paint);
	paint.setColor(SK_ColorRED);
	std::string sumRight = OpMax == edge->sum.right() ? "?" : STR(edge->sum.right());
	textLayer.drawString(SkString(sumRight), sumSide.x + 20, sumSide.y, labelFont, paint);
	paint.setColor(SK_ColorBLACK);
	int windLeft = ZeroReason::none >= edge->winding.debugReason ? edge->winding.left() :
			edge->winding.debugLeft;
	std::string oppLeft = OpMax == edge->sum.left() ? STR(windLeft) : 
			STR(edge->sum.left() - windLeft);
	textLayer.drawString(SkString(oppLeft), oppSide.x, oppSide.y, labelFont, paint);
	paint.setColor(SK_ColorRED);
	int windRight = ZeroReason::none >= edge->winding.debugReason ?
			edge->winding.right() : edge->winding.debugRight;
	std::string oppRight = OpMax == edge->sum.left() ? STR(windRight) : 
			STR(edge->sum.right() - windRight);
	textLayer.drawString(SkString(oppRight), oppSide.x + 20, oppSide.y, labelFont, paint);
	return true;
}

#if 0
void OpDebugImage::drawLines() {
	SkCanvas offscreen(bitmap);
	SkPaint paint;
	paint.setAntiAlias(true);
	for (OpDebugRay& line : lines) {
		if (!line.useAxis)
			continue;
		if (!drawValuesOn && !drawHexOn)
			continue;
		SkString label = SkString(STR(line.value));
		OpPoint mappedPt;
		if (Axis::vertical == line.axis) {
			mappedPt = DebugOpPtToPt(OpPoint(line.value, 0));
			offscreen.save();
			offscreen.rotate(-90, mappedPt.x, bitmapWH / 2);
			offscreen.drawString(label, mappedPt.x - 10, bitmapWH / 2, labelFont, paint);
			offscreen.restore();
		} else {
			mappedPt = DebugOpPtToPt(OpPoint(0, line.value));
			offscreen.drawString(label, bitmapWH / 2, mappedPt.y - 10, labelFont, paint);
		}
	}
}
#endif

void add(const std::vector<OpEdge*>& _edges) {
	for (auto edge : _edges) {
		OpDebugImage::add(edge);
	}
	OpDebugImage::focusEdges();
}

void add(const std::vector<OpEdge>& _edges) {
	for (auto& edge : _edges) {
		OpDebugImage::add(&edge);
	}
	OpDebugImage::focusEdges();
}

void draw(const std::vector<OpEdge*>& _edges) {
	OpDebugImage::clearLocalEdges();
	add(_edges);
}

void draw(const std::vector<OpEdge>& _edges) {
	OpDebugImage::clearLocalEdges();
	add(_edges);
}

void draw(Axis axis, float value) {
	OpDebugImage::add(axis, value);
	drawLinesOn = true;
	OpDebugImage::drawDoubleFocus();
}

void draw(const LinePts& ray) {
	OpDebugImage::add(ray);
	drawLinesOn = true;
	OpDebugImage::drawDoubleFocus();
}

void draw() {
	OpDebugImage::drawDoubleFocus();
}

void OpSegment::draw() const {
	OpDebugImage::add(this);
	DRAW_IDS_ON(Segments);
	OpDebugImage::drawDoubleFocus();
}

bool OpSegment::debugContains(const OpEdge* edge) const {
	for (auto& e : edges) {
		if (edge == &e)
			return true;
	}
	return false;
}

void OpOutPath::draw() const {
	drawOutputsOn = true;
	OpDebugImage::drawDoubleFocus();
}

void help() {
	OpDebugOut("navigation: l(eft) r(ight) u(p) d(own) i(n) oo(ut)\n");
	OpDebugOut("focus(id) resetFocus() center(id) clear()\n");
	OpDebugOut("showLeft() showRight() showPoints() showIDs()\n");
	OpDebugOut("hideIDs() hideValues() hideHex()\n");
}

void resetFocus() {
	if (!operands.size())
		return OpDebugOut("missing operands\n");
	OpPointBounds focusRect;
	for (unsigned index = 0; index < operands.size(); ++index) {
		SkRect skrect = operands[index].skPath->getBounds();
		focusRect.left = std::min(skrect.fLeft, focusRect.left);
		focusRect.top = std::min(skrect.fTop, focusRect.top);
		focusRect.right = std::max(skrect.fRight, focusRect.right);
		focusRect.bottom = std::max(skrect.fBottom, focusRect.bottom);
	}
	if (focusRect.isFinite())
		OpDebugImage::drawDoubleFocus(focusRect, false);
	else
		OpDebugOut("operand bounds are not finite\n");
}

void u(float s) {
	DebugOpOffsetCenter(0, -DebugOpTranslate(s));
	OpDebugImage::drawDoubleFocus();
}

void u() {
	u(1);
}

void d(float s) {
	DebugOpOffsetCenter(0, +DebugOpTranslate(s));
	OpDebugImage::drawDoubleFocus();
}

void d() {
	d(1);
}

void l(float s) {
	DebugOpOffsetCenter(-DebugOpTranslate(s), 0);
	OpDebugImage::drawDoubleFocus();
}

void l() {
	l(1);
}

void r(float s) {
	DebugOpOffsetCenter(+DebugOpTranslate(s), 0);
	OpDebugImage::drawDoubleFocus();
}

void r() {
	r(1);
}

void i(float s) {
	DebugOpOffsetZoom(+s);
	OpDebugImage::drawDoubleFocus();
}

void i() {
	i(1);
}

void oo(float s) {
	DebugOpOffsetZoom(-s);
	OpDebugImage::drawDoubleFocus();
}

void oo() {
	oo(1);
}

#endif
