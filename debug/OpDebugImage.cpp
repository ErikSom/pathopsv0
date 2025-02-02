// (c) 2023, Cary Clark cclark2@gmail.com
#include "OpDebug.h"
#if OP_DEBUG_IMAGE

#ifdef _WIN32
#pragma optimize( "", off )
#endif

#include <algorithm>
#include <functional>
#if OP_TINY_SKIA
#include "TinySkia.h"
#else
#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPath.h"
#include "include/core/SkPaint.h"
#endif
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

std::vector<OpDebugRay> lines;
std::vector<OpPtT> ptTs;
int gridIntervals = 8;

#define OP_X(Thing) \
bool draw##Thing##On = false;
MASTER_LIST
#undef OP_X
bool drawFillOn = false;
#if OP_DEBUG_VERBOSE
int debugVerboseDepth = 0;
#endif

uint32_t OP_DEBUG_MULTICOLORED = 0xAbeBeBad;
uint32_t pathsOutColor = blue;

SkBitmap& bitmapRef() {
	return bitmap;
}

#define DRAW_IDS_ON(Thing) \
	do { \
		draw##Thing##On = true; \
		drawIDsOn = true; \
	} while (false)


struct OpDebugSegmentIter {
    OpDebugSegmentIter(bool start) {
		segmentIndex = 0;
		if (start)
			return;
		for (const auto c : debugGlobalContours->contours) {
			segmentIndex += c->segments.size();
		}
	}

    bool operator!=(OpDebugSegmentIter rhs) { 
		return segmentIndex != rhs.segmentIndex; 
	}

    const OpSegment* operator*() {
		size_t index = 0;
		for (const auto c : debugGlobalContours->contours) {
			for (const auto& seg : c->segments) {
				if (index == segmentIndex)
					return &seg;
				++index;
			}
		}
		OpDebugOut("iterator out of bounds! segmentIndex: " + STR(segmentIndex) + "\n");
		return nullptr; 
	}

    void operator++() { 
		++segmentIndex;
	}

	size_t segmentIndex;
};

struct OpDebugSegmentIterator {
    OpDebugSegmentIter begin() { return OpDebugSegmentIter(true); }
    OpDebugSegmentIter end() { return OpDebugSegmentIter(false); }
	bool empty() { return !(begin() != end()); }
};

OpDebugSegmentIterator segmentIterator;
int edgeIterDvLevel = -1;

struct OpDebugEdgeIter {
    OpDebugEdgeIter(bool start)
		: isCurveCurve(false)
		, isFiller(false)
#if !OP_TEST_NEW_INTERFACE
		, isOpp(false)
#endif
		, isLine(false) {
		edgeIndex = 0;
		if (start)
			return;
		for (const auto c : debugGlobalContours->contours) {
			for (const auto& s : c->segments)
				edgeIndex += s.edges.size();
		}
		if (debugGlobalContours->fillerStorage)
			edgeIndex += debugGlobalContours->fillerStorage->debugCount();
		OpCurveCurve* cc = debugGlobalContours->debugCurveCurve;
		if (!cc)
			return;
		if (edgeIterDvLevel < 0)
			edgeIndex += debugGlobalContours->ccStorage->debugCount();
#if OP_DEBUG_VERBOSE
		else if (edgeIterDvLevel) {
			int dvLevel = std::max(edgeIterDvLevel, (int) cc->dvDepthIndex.size());
			int lo = cc->dvDepthIndex[dvLevel - 1];
			int hi = (int) cc->dvDepthIndex.size() <= dvLevel 
					? (int) cc->dvAll.size() : cc->dvDepthIndex[dvLevel];
			edgeIndex += hi - lo;
		} else {
			edgeIndex += cc->edgeCurves.c.size();
			edgeIndex += cc->oppCurves.c.size();
		}
#endif
	}

    bool operator!=(OpDebugEdgeIter rhs) { 
		return edgeIndex != rhs.edgeIndex; 
	}

    const OpEdge* operator*() {
		size_t index = 0;
		for (const auto c : debugGlobalContours->contours) {
			for (const auto& s : c->segments) {
				for (const auto& edge : s.edges) {
					if (index == edgeIndex) {
						isCurveCurve = false;
						isFiller = false;
#if !OP_TEST_NEW_INTERFACE
						isOpp = OpOperand::right == c->operand;
#endif
						isLine = edge.isLine_impl;
						return &edge;
					}
					++index;
				}
			}
		}
		if (debugGlobalContours->fillerStorage) {
			const OpEdge* filler = debugGlobalContours->fillerStorage->debugIndex(edgeIndex - index);
			if (filler) {
				isCurveCurve = false;
				isFiller = true;
#if !OP_TEST_NEW_INTERFACE
				isOpp = false;
#endif
				isLine = true;
				return filler;
			}
			index += debugGlobalContours->fillerStorage->debugCount();
		}
		OpCurveCurve* cc = debugGlobalContours->debugCurveCurve;
		if (cc) {
			const OpEdge* ccEdge = nullptr;
			auto checkEdges = [&ccEdge, this, &index](std::vector<OpEdge*> edges) {
				if (ccEdge)
					return;
				if (edgeIndex - index < edges.size())
					ccEdge = edges[edgeIndex - index];
				else
					index += edges.size();
			};
			if (edgeIterDvLevel < 0)
				ccEdge = debugGlobalContours->ccStorage->debugIndex(edgeIndex - index);
#if OP_DEBUG_VERBOSE
			else if (edgeIterDvLevel) {	
				int dvLevel = std::min(edgeIterDvLevel, (int) cc->dvDepthIndex.size());
				int lo = (int) cc->dvDepthIndex[dvLevel - 1];
				int hi = (int) cc->dvDepthIndex.size() <= dvLevel 
						? (int) cc->dvAll.size() : cc->dvDepthIndex[dvLevel];
				if (index + hi - lo > edgeIndex)
					ccEdge = cc->dvAll[edgeIndex - index + lo];
			} else {
				checkEdges(cc->edgeCurves.c);
				checkEdges(cc->oppCurves.c);
			}
#endif
			if (ccEdge) {
				isCurveCurve = true;
				isFiller = false;
#if !OP_TEST_NEW_INTERFACE
				isOpp = false;
#endif
				isLine = false;
				return ccEdge;
			}
		}
		OpDebugOut("iterator out of bounds! edgeIndex: " + STR(edgeIndex) + 
				"; max index: " + STR(index) + "\n");
		return nullptr; 
	}

    void operator++() { 
		++edgeIndex;
	}

	bool isCurveCurve;
	bool isFiller;
#if !OP_TEST_NEW_INTERFACE
	bool isOpp;
#endif
	bool isLine;
	size_t edgeIndex;
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
		for (const auto c : debugGlobalContours->contours) {
			for (const auto& seg : c->segments) {
				localIntersectionIndex += seg.sects.i.size();
			}
		}
	}

    bool operator!=(OpDebugIntersectionIter rhs) { 
		return localIntersectionIndex != rhs.localIntersectionIndex; 
	}

    const OpIntersection* operator*() {
		size_t index = 0;
		for (const auto c : debugGlobalContours->contours) {
			for (const auto& seg : c->segments) {
				for (const auto sect : seg.sects.i) {
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
#if OP_DEBUG
	OpDebugDefeatDelete() {
		save = debugGlobalContours->debugInPathOps;
		debugGlobalContours->debugInPathOps = false;
	}
	~OpDebugDefeatDelete() {
		debugGlobalContours->debugInPathOps = save;
	}

	bool save;
#endif
};

void OpDebugImage::addToPath(const OpCurve& curve, SkPath& path) {
	path.moveTo(curve.firstPt().x, curve.firstPt().y);
#if OP_TEST_NEW_INTERFACE
	return curve.contours->callBack(curve.c.type).debugAddToPathFuncPtr(curve.c, path);
#else
	switch (curve.c.type) {
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
			path.conicTo(curve.pts[1].x, curve.pts[1].y, curve.pts[2].x, curve.pts[2].y, curve.weightImpl);
			break;
		case OpType::cubic:
			path.cubicTo(curve.pts[1].x, curve.pts[1].y, curve.pts[2].x, curve.pts[2].y,
					curve.pts[3].x, curve.pts[3].y);
			break;
		default:
			OP_ASSERT(0);
	}
#endif
}

#if !OP_TEST_NEW_INTERFACE
static SkPath* skPath(const OpInPath& opPath) {
	return (SkPath*) opPath.externalReference;
}
#endif

void OpDebugImage::init() {
	bitmap.allocPixels(SkImageInfo::MakeN32Premul(bitmapWH, bitmapWH));
	::clear();
#if !OP_TEST_NEW_INTERFACE
	if (!debugGlobalContours->leftIn)
		return;
	drawLeftOn = true;
	drawRightOn = true;
#endif
#if OP_TEST_NEW_INTERFACE
	focusSegments();
#else
	SkRect opBounds = skPath(*debugGlobalContours->leftIn)->getBounds();
	opBounds.join(skPath(*debugGlobalContours->rightIn)->getBounds());
	DebugOpSetBounds(opBounds.fLeft, opBounds.fTop, opBounds.fRight, opBounds.fBottom);
#endif
}

// !!! missing pathsOutColor
void  OpDebugImage::playback(FILE* file) {
//	FILE* file = fopen("OpDebugImageState.txt", "r");
	if (!file)
		return;
	char str[255];
	double debugZoom;
	double debugCenter[2];
	float textSize;
	int intervals;
	int pPrecision;
	// required
	if (fscanf(file, "debugZoom: %lg\n", &debugZoom) != 1) {
		OpDebugOut("reading debugZoom failed\n");
		fclose(file);
		return;
	}
	DebugOpSetZoom(debugZoom);
	if (fscanf(file, "debugCenter: %lg, %lg\n", &debugCenter[0], &debugCenter[1]) != 2) {
		OpDebugOut("reading debugCenter failed\n");
		fclose(file);
		return;
	}
	DebugOpSetCenter(debugCenter[0], debugCenter[1]);
	DebugOpResetBounds();
	if (fscanf(file, "textSize: %g\n", &textSize) != 1) {
		OpDebugOut("reading textSize failed\n");
		fclose(file);
		return;
	}
	labelFont.setSize(textSize);
	if (fscanf(file, "gridIntervals: %d\n", &intervals) != 1) {
		OpDebugOut("reading gridIntervals failed\n");
		fclose(file);
		return;
	}
	gridIntervals = intervals;
	if (fscanf(file, "debugPrecision: %d\n", &pPrecision) != 1) {
		OpDebugOut("reading debugPrecision failed\n");
		fclose(file);
		return;
	}
	debugPrecision = pPrecision;
	// optional
	auto noMatch = [file](const char* str) {
		OpDebugOut("no match: " + std::string(str)); 
		fclose(file);
	};
	{
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
			if (strlen(str) > strlen("colorID: ")) {
				const char* idColorStr = str + strlen("colorID: ");
				int id = strtol(idColorStr, nullptr, 0);
				OpEdge* edge = findEdge(id);
				uint32_t color = 0;
				const char* colorStr = strstr(idColorStr, "color: ");
				if (colorStr)
					color = strtoul(colorStr + strlen("color: "), nullptr, 0);
				if (edge && color)
					edge->debugColor = color;
				else
					return noMatch(str);
			} else if (0 == strcmp("brief\n", str)) {
				break;
			} else
				return noMatch(str);
		}
		redraw();
	}
//	fclose(file);
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

#if OP_TEST_NEW_INTERFACE

#else
static SkPath* sk0() {
	return (SkPath*) debugGlobalContours->leftIn->externalReference;
}

static SkPath* sk1() {
	return (SkPath*) debugGlobalContours->rightIn->externalReference;
}
#endif

void OpDebugImage::drawDoubleFocus() {
#if OP_DEBUG_VERBOSE
	edgeIterDvLevel = debugVerboseDepth;
#endif
	OP_DEBUG_CODE(OpDebugDefeatDelete defeater);
	std::vector<int> ids;
	clearScreen();
	if (drawFillOn) {
		SkMatrix matrix;
		float scale = (float)DebugOpGetZoomScale();
		matrix.setScale(scale, scale);
		matrix.preTranslate(-DebugOpGetCenterX(), -DebugOpGetCenterY());
		matrix.postTranslate(DebugOpGetOffsetX(), DebugOpGetOffsetY());
#if OP_TEST_NEW_INTERFACE
		bool first = true;
		for (auto contour : debugGlobalContours->contours) {
			if (contour->callBacks.debugGetDrawFuncPtr(contour->caller)) {
				SkPath* skPath = (SkPath*) contour->callBacks.debugNativePathFuncPtr(contour->caller);
				OP_ASSERT(skPath);
				drawDoubleFill(skPath->makeTransform(matrix), 
						first ? OpDebugAlphaColor(10, red) : OpDebugAlphaColor(10, blue));
			}
			first = false;
		}
#else
		if (drawLeftOn) 
			drawDoubleFill(sk0()->makeTransform(matrix), OpDebugAlphaColor(10, red));
		if (drawRightOn)
			drawDoubleFill(sk1()->makeTransform(matrix), OpDebugAlphaColor(10, blue));
#endif
	}
    if (drawResultOn) {
		SkMatrix matrix;
		float scale = (float)DebugOpGetZoomScale();
		matrix.setScale(scale, scale);
		matrix.preTranslate(-DebugOpGetCenterX(), -DebugOpGetCenterY());
		matrix.postTranslate(DebugOpGetOffsetX(), DebugOpGetOffsetY());
#if OP_TEST_NEW_INTERFACE
		bool first = true;
		for (auto contour : debugGlobalContours->contours) {
			SkPath* skPath = (SkPath*) contour->callBacks.debugNativePathFuncPtr(contour->caller);
			OP_ASSERT(skPath);
		    drawDoubleFill(skPath->makeTransform(matrix), 
					first ? OpDebugAlphaColor(20, red) : OpDebugAlphaColor(20, blue));
			first = false;
		}
		if (debugGlobalContours->callerOutput)
		    drawDoubleFill(((SkPath*) debugGlobalContours->callerOutput)
					->makeTransform(matrix), OpDebugAlphaColor(20, green));
#else
		drawDoubleFill(sk0()->makeTransform(matrix), OpDebugAlphaColor(20, red));
		drawDoubleFill(sk1()->makeTransform(matrix), OpDebugAlphaColor(20, blue));
        OpOutPath* result = debugGlobalContours->debugResult;
        if (result)
		    drawDoubleFill(((SkPath*) result->externalReference)
                ->makeTransform(matrix), OpDebugAlphaColor(20, green));
#endif
    }
#if !OP_TEST_NEW_INTERFACE
	if (drawLeftOn || drawRightOn)
		DebugOpClearInputs();
	if (drawLeftOn)
		DebugOpAdd(*debugGlobalContours->leftIn);
	if (drawRightOn)
		DebugOpAdd(*debugGlobalContours->rightIn);
	if (drawLeftOn || drawRightOn)
		DebugOpDrawInputs();
#endif
#if !OP_TEST_NEW_INTERFACE && OP_DEBUG
	if (drawPathsOutOn && debugGlobalContours->debugResult)
		DebugOpDraw(debugGlobalContours->debugResult, pathsOutColor);
#endif
	if (drawLinesOn)
		DebugOpDraw(lines);
	if (drawRaysOn) {
		std::vector<OpDebugRay> rays;
		for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
			const SectRay& sectRay = (*edgeIter)->ray;
			if (!sectRay.distances.size())
				continue;
			rays.emplace_back(sectRay.axis, sectRay.normal);
		}
		DebugOpDraw(rays);
	}
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
			if (!edge->debugDraw)
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
	if (drawEdgesOn && (drawIDsOn || drawNormalsOn || drawTangentsOn
			|| drawWindingsOn || drawEndToEndOn || drawControlLinesOn)) {
		for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
			const OpEdge* edge = *edgeIter;
			if (!edge->debugDraw)
				continue;
			if (ids.end() != std::find(ids.begin(), ids.end(), edge->id))
				continue;
			ids.push_back(edge->id);

			if (drawIDsOn) {
				uint32_t color = black;
				if (edge->disabled)
					color = red;
				else if (edgeIter.isCurveCurve) {
					if (edge->ccOverlaps)
#if OP_TEST_NEW_INTERFACE
						color = edge->winding.contour->callBacks
								.debugIsOppFuncPtr(edge->winding.contour->caller)
								? orange : darkGreen;
#else
						color = edgeIter.isOpp ? orange : darkGreen;
#endif
					else
						color = purple;
				}
				DebugOpDrawEdgeID(edge, color);
			}
			if (drawNormalsOn)
				DebugOpDrawEdgeNormal(edge, black);
			if (drawTangentsOn)
				DebugOpDrawEdgeTangent(edge, black);
			if (drawWindingsOn)
				DebugOpDrawEdgeWinding(edge, black);
			if (drawEndToEndOn)
				DebugOpDrawEdgeEndToEnd(edge, OpDebugAlphaColor(40, black));
			if (drawControlLinesOn)
				DebugOpDrawEdgeControlLines(edge, OpDebugAlphaColor(40, black));
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
#if 01 // unimplmented
	if (drawSegmentsOn && drawTangentsOn) {
		for (auto segment : segmentIterator)
			DebugOpDrawSegmentTangent(segment, black);
	}
#endif
	if (drawGridOn)
		drawGrid();
#if OP_DEBUG_VERBOSE
	edgeIterDvLevel = -1;
#endif
}

void OpDebugImage::record(FILE* recordFile) {
	if (!recordFile) {
		OpDebugOut("failed to open opDebugImageState.txt for writing\n");
		return;
	}
	DebugOpRecord(recordFile);
	fprintf(recordFile, "textSize: %g\n", labelFont.getSize());
	fprintf(recordFile, "gridIntervals: %d\n", gridIntervals);
	fprintf(recordFile, "debugPrecision: %d\n", debugPrecision);
#define OP_X(Thing) \
	if (draw##Thing##On) \
		fprintf(recordFile, "%s\n", #Thing);
	MASTER_LIST
#undef OP_X
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		const OpEdge* edge = *edgeIter;
		if (black != edge->debugColor)
			fprintf(recordFile, "colorID: %d color: 0x%08x\n", edge->id, edge->debugColor);
	}
//	fclose(recordFile);
}

#undef RECORD_FEATURE

void OpDebugImage::drawDoubleCenter(OpPoint pt, bool add) {
	add ? DebugOpAddBounds(pt.x, pt.y, pt.x, pt.y) :	// !!! should suppress zoom change
			DebugOpSetCenter(pt.x, pt.y);
	drawDoubleFocus();
}

void OpDebugImage::drawDoubleFocus(const OpRect& b, bool add) {
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

void OpDebugImage::drawDoublePath(const SkPath& path, uint32_t color, float strokeWidth) {
	SkCanvas offscreen(bitmap);
	SkPaint paint;
	paint.setAntiAlias(true);
	paint.setColor(color);
	if (strokeWidth < 0) {
		offscreen.drawPath(path, paint);
		paint.setColor(SK_ColorBLACK);
	}
	paint.setStyle(SkPaint::kStroke_Style);
	if (strokeWidth > 0)
		paint.setStrokeWidth(strokeWidth);
	offscreen.drawPath(path, paint);
}

void OpDebugImage::drawGrid() {
	SkCanvas offscreen(bitmap);
	SkPaint paint;
	paint.setAntiAlias(true);
	paint.setColor(0x3f000000);
	SkPaint textPaint = paint;
	paint.setStyle(SkPaint::kStroke_Style);
	int xOffset = 2;
	double left, top, right, bottom;
	DebugOpBounds(left, top, right, bottom);
	auto fixSign = [](int32_t i) {
		return i < 0 ? -(i & 0x7fffffff) : i;
	};
	auto unfixSign = [](int32_t i) {
		return i < 0 ? -i | 0x80000000 : i;
	};
	int32_t leftH = fixSign(OpDebugFloatToBits((float) left));
	int32_t topH = fixSign(OpDebugFloatToBits((float) top));
	int32_t rightH = fixSign(OpDebugFloatToBits((float) right));
	int32_t bottomH = fixSign(OpDebugFloatToBits((float) bottom));
	int xInterval = std::max(1, (rightH - leftH) / gridIntervals);
	int yInterval = std::max(1, (bottomH - topH) / gridIntervals);
	int leftS, topS, rightS, bottomS;
	DebugOpScreenBounds(leftS, topS, rightS, bottomS);
	auto screenX = [leftS, left, rightS, right](float fx) {
		return leftS + (fx - left) / (right - left) * (rightS - leftS);
	};
	auto drawXLine = [screenX, &offscreen, &paint, &textPaint, topS, bottomS, xOffset](float fx) {
		float sx = screenX(fx);
		offscreen.drawLine(sx, topS, sx, bottomS, paint);
		if (!drawValuesOn)
			return;
		std::string xValStr = drawHexOn ? OpDebugDumpHex(fx) : STR(fx);
		offscreen.drawString(SkString(xValStr), sx + xOffset, bitmapWH - xOffset - 3, 
				labelFont, textPaint);

	};
	auto walkX = [drawXLine, unfixSign, leftH, rightH, xInterval](bool preflight) {
		for (int x = leftH; x <= rightH; x += xInterval) {
			float fx = OpDebugBitsToFloat(unfixSign(x));
			if (preflight) {
				if (fabsf(fx) < 1)
					return false;
			} else
				drawXLine(fx);
		}
		return true;
	};
	bool xHexWorks = walkX(true);
	if (xHexWorks)
		walkX(false);
	else {  // if fabsf(fx) is less than 1, step by float range / gridIntervals
		for (float fx = left; fx <= right; fx += (right - left) / (gridIntervals - 1)) {
			drawXLine(fx);
		}
	}
	auto screenY = [topS, top, bottomS, bottom](float fy) {
		return topS + (fy - top) / (bottom - top) * (bottomS - topS);
	};
	auto drawYLine = [screenY, &offscreen, &paint, &textPaint, leftS, rightS, xOffset]
            (float fy, bool last) {
		float sy = screenY(fy);
		offscreen.drawLine(leftS, sy, rightS, sy, paint);
		if (!drawValuesOn)
			return;
		std::string yValStr = drawHexOn ? OpDebugDumpHex(fy) : STR(fy);
		offscreen.save();
		if (last)
			sy -= 14;
		offscreen.rotate(-90, 15, sy - xOffset);
		offscreen.drawString(SkString(yValStr), 15, sy - xOffset, labelFont, textPaint);
		offscreen.restore();
	};
	auto walkY = [drawYLine, unfixSign, topH, bottomH, yInterval](bool preflight) {
		for (int y = topH; y <= bottomH; y += yInterval) {
			float fy = OpDebugBitsToFloat(unfixSign(y));
			if (preflight) {
				if (fabsf(fy) < 1)
					return false;
			} else
				drawYLine(fy, y > bottomH - yInterval);
		}
		return true;
	};
	bool yHexWorks = walkY(true);
	if (yHexWorks)
		walkY(false);
	else {  // if fabsf(fy) is less than 1, step by float range / gridIntervals
		float fInterval = (bottom - top) / (gridIntervals - 1);
		for (float fy = top; fy <= bottom; fy += fInterval) {
			drawYLine(fy, fy > bottom - fInterval);
		}
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
	debugPrecision = p;
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
	OpPointBounds pointBounds;
	OpPoint point;
	if (!find(id, &pointBounds, &point))
		return;
	if (pointBounds.isFinite())
		return OpDebugImage::drawDoubleCenter(pointBounds.center(), add);
	if (point.isFinite())
		return OpDebugImage::drawDoubleCenter(point, add);
}

bool OpDebugImage::find(int id, OpPointBounds* boundsPtr, OpPoint* pointPtr) {
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		OpEdge* edge = const_cast<OpEdge*>(*edgeIter);
		if (id != edge->id)
			continue;
		edge->debugDraw = true;
		DRAW_IDS_ON(Edges);
		*boundsPtr = edge->ptBounds;
		return true;
	}
	const OpSegment* segment = nullptr;
	for (auto s : segmentIterator) {
		if (id == s->id)
			segment = s;
	}
	if (segment) {
		DRAW_IDS_ON(Segments);
		*boundsPtr = segment->ptBounds;
		return true;
	}
#if OP_DEBUG
	const OpIntersection* sect = nullptr;
	for (auto i : intersectionIterator) {
		if (id == i->id)
			sect = i;
	}
	if (sect) {
		DRAW_IDS_ON(Intersections);
		// don't change zoom
		*pointPtr = sect->ptT.pt;
		return true;
	}
#endif
#if OP_DEBUG
	auto coins = findCoincidence(id);
	if (coins.size()) {
		DRAW_IDS_ON(Coincidences);
		// !!! wrong: add rect formed by both intersections with this id
		for (auto coin : coins)
			boundsPtr->add(coin->ptT.pt);
		return true;
	}
#endif
	OpDebugOut("id " + STR(id) + " not found\n");
	return false;
}

// !!! not sure I need this; but it does raise the question if dump and image need their own finds
std::vector<const OpEdge*> OpDebugImage::find(int id) {
	extern OpEdge* findEdge(int id);
	extern std::vector<const OpEdge*> findEdgeOutput(int id);
	std::vector<const OpEdge*> result;
	if (OpEdge* edge = findEdge(id))
		result.push_back(edge);
	if (std::vector<const OpEdge*> oEdges = findEdgeOutput(id); oEdges.size())
		result.insert(result.end(), oEdges.begin(), oEdges.end());
	return result;
}

void OpDebugImage::focus(int id, bool add) {
	OpPointBounds pointBounds;
	OpPoint point;
	if (!find(id, &pointBounds, &point))
		return;
	if (pointBounds.isFinite())
		return OpDebugImage::drawDoubleFocus(pointBounds, add);
	if (point.isFinite())
		return OpDebugImage::drawDoubleCenter(point, add);
}

void addFocus(int id) {
	OpDebugImage::focus(id, true);
}

void addFocus(const OpContour& contour) {
	for (const OpSegment& s : contour.segments)
		addFocus(s.ptBounds);
}

void addFocus(OpContours& contours) {
	OpPointBounds bounds;
	for (auto contour : contours.contours)
		addFocus(contour);
	addFocus(bounds);
}

// !!! remove all const everywhere
void addFocus(const OpContours& contours) {
	addFocus(*const_cast<OpContours*>(&contours));
}

void addFocus(const OpEdge& edge) {
	addFocus(edge.ptBounds);
}

void addFocus(const OpIntersection& sect) {
	addFocus(sect.ptT);
}

void addFocus(const OpPoint& pt) {
	OpDebugImage::drawDoubleCenter(pt, true);
}

void addFocus(const OpPtT& ptT) {
	addFocus(ptT.pt);
}

void addFocus(const OpRect& rect) {
	OpDebugImage::drawDoubleFocus(rect, true);
}

void addFocus(const OpSegment& segment) {
	addFocus(segment.ptBounds);
}

void addFocus(const OpContour* contour) {
	addFocus(*contour);
}

void addFocus(const OpContours* contours) {
	addFocus(*contours);
}

void addFocus(const OpEdge* edge) {
	addFocus(*edge);
}

void addFocus(const OpIntersection* sect) {
	addFocus(*sect);
}

void addFocus(const OpPoint* pt) {
	addFocus(*pt);
}

void addFocus(const OpPtT* ptT) {
	addFocus(*ptT);
}

void addFocus(const OpRect* rect) {
	addFocus(*rect);
}

void addFocus(const OpSegment* segment) {
	addFocus(*segment);
}

void ctr() {
	ctr(*debugGlobalContours);
}

void ctr(int id) {
	OpDebugImage::center(id, false);
}

void ctr(float x, float y) {
	ctr(OpPoint(x, y));
}

void ctr(const OpContour& contour) {
	OpPointBounds bounds;
    for (auto& segment : contour.segments)
		bounds.add(segment.ptBounds);
	ctr(bounds);
}

void ctr(OpContours& contours) {
	OpPointBounds bounds;
	for (auto contour : contours.contours)
		for (auto& segment : contour->segments)
			bounds.add(segment.ptBounds);
	ctr(bounds);
}

void ctr(const OpContours& contours) {
	ctr(*const_cast<OpContours*>(&contours));
}

void ctr(const OpEdge& edge) {
	ctr(edge.ptBounds);
}

void ctr(const OpIntersection& sect) {
	ctr(sect.ptT);
}

void ctr(const OpPoint& pt) {
	OpDebugImage::drawDoubleCenter(pt, false);
}

void ctr(const OpPtT& ptT) {
	ctr(ptT.pt);
}

void ctr(const OpRect& rect) {
	ctr(rect.center());
}

void ctr(const OpSegment& segment) {
	ctr(segment.ptBounds);
}

void ctr(const OpContour* contour) {
	ctr(*contour);
}

void ctr(const OpContours* contours) {
	ctr(*contours);
}

void ctr(const OpEdge* edge) {
	ctr(*edge);
}

void ctr(const OpIntersection* sect) {
	ctr(*sect);
}

void ctr(const OpPoint* pt) {
	ctr(*pt);
}

void ctr(const OpPtT* ptT) {
	ctr(*ptT);
}

void ctr(const OpRect* rect) {
	ctr(*rect);
}

void ctr(const OpSegment* segment) {
	ctr(*segment);
}

void focus(int id) {
	OpDebugImage::focus(id, false);
}

void focus(const OpContour& contour) {
	drawSegmentsOn = true;
	OpPointBounds bounds;
    for (auto& segment : contour.segments)
		bounds.add(segment.ptBounds);
	focus(bounds);
}

void focus(OpContours& contours) {
	OpPointBounds bounds;
	for (auto contour : contours.contours)
		for (auto& segment : contour->segments)
			bounds.add(segment.ptBounds);
	focus(bounds);
}

// !!! remove all const everywhere
void focus(const OpContours& contours) {
	focus(*const_cast<OpContours*>(&contours));
}

void focus(const OpEdge& edge) {
	focus(edge.ptBounds);
}

void focus(const OpRect& rect) {
	OpDebugImage::drawDoubleFocus(rect, false);
}

void focus(const OpSegment& segment) {
	drawSegmentsOn = true;
	focus(segment.ptBounds);
}

void focus(const OpContour* contour) {
	focus(*contour);
}

void focus(const OpContours* contours) {
	focus(*contours);
}

void focus(const OpEdge* edge) {
	focus(*edge);
}

void focus(const OpRect* rect) {
	focus(*rect);
}

void focus(const OpSegment* segment) {
	focus(*segment);
}

void OpDebugImage::focusEdges() {
	if (edgeIterator.empty())
		return;
	OpPointBounds focusRect;
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		OpEdge* edge = const_cast<OpEdge*>(*edgeIter);
		if (edge->debugDraw)
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
	OpDebugImage::clearLines();
#if !OP_TEST_NEW_INTERFACE
	drawLeftOn = false;
	drawRightOn = false;
#endif
	DebugOpResetFocus();
}

void clearLines() {
	OpDebugImage::clearLines();
	DebugOpResetFocus();
}

bool OpDebugImage::bitsAreBlank(bool allowIntersect, SkRect& test) {
	SkRect skBounds { 0, 0, bitmapWH, bitmapWH };
	if (allowIntersect ? !skBounds.intersect(test) : !skBounds.contains(test))
		return false;
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
				return false;
			if (SkColorGetR(pixel) < 0x3F
					|| SkColorGetG(pixel) < 0x3F
					|| SkColorGetB(pixel) < 0x3F)
				return false;
		}
	}
	return true;
}

void OpDebugImage::drawGuide(const SkRect& test, OpPoint pt, uint32_t color) {
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
	SkPaint paint;
	paint.setAntiAlias(true);
	paint.setColor(color);
	paint.setAlpha(63);
	SkCanvas offscreen(bitmap);
	offscreen.drawLine(closestSide.x, closestSide.y, pt.x, pt.y, paint);
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
		for (float scale : { 4, 16, 32, 48, 64 } ) {
			for (int toTheLeft : { -1, 0, 1 } ) {
				for (int toTheTop : { -1, 0, 1 } ) {
					OpVector offset { 0, 0 };
					SkRect test = trimmed;
					if (toTheLeft)
						offset.dx = toTheLeft < 0 ? -textBounds.width() - scale : scale;
					if (toTheTop)
						offset.dy = toTheTop < 0 ? -scale : textBounds.height() + scale;
					test.offset(offset.dx, offset.dy);
					if (!bitsAreBlank(allowIntersect, test))
						continue;
					offscreen.drawString(SkString(ptStr), pt.x + offset.dx,
							pt.y + offset.dy, labelFont, paint);
					if (16 <= scale && drawGuidesOn)
						drawGuide(test, pt, color);
					return true;
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
#if OP_TEST_NEW_INTERFACE
	for (auto contour : debugGlobalContours->contours) {
		if (contour->callBacks.debugGetDrawFuncPtr(contour->caller)) {
			SkPath* skPath = (SkPath*) contour->callBacks.debugNativePathFuncPtr(contour->caller);
			drawPathPt(skPath);
		}
	}
#else
	if (drawLeftOn)
		drawPathPt(sk0());
	if (drawRightOn)
		drawPathPt(sk1());
#endif
	if (drawSegmentsOn) {
		for (auto seg : segmentIterator) {
			DebugOpBuild(seg->c.firstPt());
			DebugOpBuild(seg->c.lastPt());
			// !!! probably need switch to say 'draw control points'
			if (drawControlsOn) {
				for (int index = 1; index < seg->c.pointCount() - 1; ++index)
					DebugOpBuild(seg->c.hullPt(index));
			}
		}
	}
	if (drawEdgesOn) {
		for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
			const OpEdge* edge = *edgeIter;
			if (!edge->debugDraw)
				continue;
#if OP_TEST_NEW_INTERFACE
			bool isOpp = edge->winding.contour->callBacks
					.debugIsOppFuncPtr(edge->winding.contour->caller);
			DebugOpBuild(edge->start.pt, edge->start.t, isOpp);
			DebugOpBuild(edge->end.pt, edge->end.t, isOpp);
#else
			if (edgeIter.isCurveCurve) {
				DebugOpBuild(edge->start.pt, edge->start.t, edgeIter.isOpp);
				DebugOpBuild(edge->end.pt, edge->end.t, edgeIter.isOpp);
			} else {
				DebugOpBuild(edge->start.pt, edge->start.t);
				DebugOpBuild(edge->end.pt, edge->end.t);
			}
#endif
			if (drawControlsOn) {
				for (int index = 1; index < edge->curve.pointCount() - 1; ++index)
					DebugOpBuild(edge->curve.hullPt(index));
			}
			if (drawCentersOn)
				DebugOpBuild(edge->center.pt, edge->center.t, DebugSprite::square);
			if (drawHullsOn) {
				for (const HullSect& hull : edge->hulls.h)
					DebugOpBuild(hull.sect.pt, hull.sect.t, DebugSprite::circle);
			}
			if (drawEdgeRunsOn && debugGlobalContours->debugCurveCurve) {

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
#if OP_TEST_NEW_INTERFACE
			for (auto contour : debugGlobalContours->contours) {
				if (contour->callBacks.debugGetDrawFuncPtr(contour->caller))
					DebugOpBuild(*(SkPath*)contour->callBacks.debugNativePathFuncPtr(contour->caller), line);
			}
#else
			if (drawLeftOn)
				DebugOpBuild(*sk0(), line);
			if (drawRightOn)
				DebugOpBuild(*sk1(), line);
#endif
			if (drawSegmentsOn) {
				for (auto seg : segmentIterator) {
					DebugOpBuild(*seg, line);
				}
			}
			if (drawEdgesOn)
				for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
					if (!(*edgeIter)->debugDraw)
						continue;
					DebugOpBuild(**edgeIter, line);
				}
		}
	}
	if (drawRaysOn && drawEdgesOn) {
		for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
			const OpEdge* edge = *edgeIter;
			const SectRay& ray = edge->ray;
			for (auto dist :ray.distances)
				DebugOpBuild(ray.axis, ray.normal, dist.cept);
		}
	}
	for (OpPtT ptT : ptTs)
		DebugOpBuild(ptT.pt, ptT.t, DebugSprite::triangle);
	if (drawValuesOn) {
		if (drawTsOn)
			DebugOpDrawT(drawHexOn);
		else
			DebugOpDrawValue(drawHexOn);
	}
	DebugOpDrawSprites();
}

void OpDebugImage::add(Axis axis, float value) {
	lines.emplace_back(axis, value);
}

void OpDebugImage::add(const OpPtT& ptT) {
	ptTs.push_back(ptT);
}

void OpDebugImage::addArrowHeadToPath(const LinePts& line, SkPath& path) {
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

void OpDebugImage::addCircleToPath(OpPoint pt, SkPath& path) {
	path.addCircle(pt.x, pt.y, 4);
}

void OpDebugImage::addDiamondToPath(OpPoint pt, SkPath& path) {
	SkPath diamond;
	diamond.moveTo( 4,  0);
	diamond.lineTo( 0,  4);
	diamond.lineTo(-4,  0);
	diamond.lineTo( 0, -4);
	diamond.close();
	diamond.offset(pt.x, pt.y);
	path.addPath(diamond);
}

void OpDebugImage::addSquareToPath(OpPoint pt, SkPath& path) {
	SkPath square;
	square.moveTo(-4, -4);
	square.lineTo( 4, -4);
	square.lineTo( 4,  4);
	square.lineTo(-4,  4);
	square.close();
	square.offset(pt.x, pt.y);
	path.addPath(square);
}

void OpDebugImage::addTriangleToPath(OpPoint pt, SkPath& path) {
	SkPath triangle;
	triangle.moveTo( 0,  -4);
	triangle.lineTo( 4,  4);
	triangle.lineTo(-4,  4);
	triangle.close();
	triangle.offset(pt.x, pt.y);
	path.addPath(triangle);
}

void OpDebugRay::construct(const LinePts& pts_) {
	pts = pts_;
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

void OpDebugImage::clearIntersections() {
	drawIntersectionsOn = false;
}

void OpDebugImage::clearLines() {
	lines.clear();
	drawLinesOn = false;
}

void OpDebugImage::clearPoints() {
	ptTs.clear();
	drawPointsOn = false;
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

void hideFill() {
	drawFillOn = false;
	OpDebugImage::drawDoubleFocus();
}

void showFill() {
#if !OP_TEST_NEW_INTERFACE
	if (!drawLeftOn && !drawRightOn)
		drawLeftOn = drawRightOn = true;
#endif
	drawFillOn = true;
	OpDebugImage::drawDoubleFocus();
}

void toggleFill() {
	drawFillOn ^= true;
#if !OP_TEST_NEW_INTERFACE
	if (drawFillOn && !drawLeftOn && !drawRightOn)
		drawLeftOn = drawRightOn = true;
#endif
	OpDebugImage::drawDoubleFocus();
}

void hideIn() {
	hideOperands();
}

void showIn() {
	showOperands();
}

void toggleIn() {
	toggleOperands();
}

#if OP_TEST_NEW_INTERFACE
void hideLeft() {
	OpContour* contour = debugGlobalContours->contours.front();
	contour->callBacks.debugSetDrawFuncPtr(contour->caller, false);
	OpDebugImage::drawDoubleFocus();
}

void showLeft() {
	OpContour* contour = debugGlobalContours->contours.front();
	contour->callBacks.debugSetDrawFuncPtr(contour->caller, true);
	OpDebugImage::drawDoubleFocus();
}

void toggleLeft() {
	OpContour* contour = debugGlobalContours->contours.front();
	bool debugDraw = contour->callBacks.debugGetDrawFuncPtr(contour->caller);
	contour->callBacks.debugSetDrawFuncPtr(contour->caller, !debugDraw);
	OpDebugImage::drawDoubleFocus();
}

void hideRight() {
	OpContour* contour = debugGlobalContours->contours.back();
	contour->callBacks.debugSetDrawFuncPtr(contour->caller, false);
	OpDebugImage::drawDoubleFocus();
}

void showRight() {
	OpContour* contour = debugGlobalContours->contours.back();
	contour->callBacks.debugSetDrawFuncPtr(contour->caller, true);
	OpDebugImage::drawDoubleFocus();
}

void toggleRight() {
	OpContour* contour = debugGlobalContours->contours.back();
	bool debugDraw = contour->callBacks.debugGetDrawFuncPtr(contour->caller);
	contour->callBacks.debugSetDrawFuncPtr(contour->caller, !debugDraw);
	OpDebugImage::drawDoubleFocus();
}
#endif

void hideOperands() {
#if OP_TEST_NEW_INTERFACE
	for (auto contour : debugGlobalContours->contours) {
		contour->callBacks.debugSetDrawFuncPtr(contour->caller, false);
	}
#else
	drawLeftOn = drawRightOn = false;
#endif
	OpDebugImage::drawDoubleFocus();
}

void showOperands() {
#if OP_TEST_NEW_INTERFACE
	for (auto contour : debugGlobalContours->contours) {
		contour->callBacks.debugSetDrawFuncPtr(contour->caller, true);
	}
#else
	drawLeftOn = drawRightOn = true;
#endif
	OpDebugImage::drawDoubleFocus();
}

void toggleOperands() {
#if OP_TEST_NEW_INTERFACE
	for (auto contour : debugGlobalContours->contours) {
		bool debugDraw = contour->callBacks.debugGetDrawFuncPtr(contour->caller);
		contour->callBacks.debugSetDrawFuncPtr(contour->caller, !debugDraw);
	}
#else
	drawLeftOn ^= true;
	drawRightOn ^= true;
#endif
	OpDebugImage::drawDoubleFocus();
}

static void operateOnSegmentEdges(std::function<void (OpEdge*)> fun) {
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		OpEdge* edge = const_cast<OpEdge*>(*edgeIter);
		if (edge->debugJoin || edgeIter.isFiller || edgeIter.isCurveCurve)
			continue;
		fun(edge);
	}
	OpDebugImage::drawDoubleFocus();
}

void hideSegmentEdges() {
	operateOnSegmentEdges([](OpEdge* edge) {
		edge->debugDraw = false;
	});
}

void showSegmentEdges() {
	operateOnSegmentEdges([](OpEdge* edge) {
		edge->debugDraw = true;
	});
}

void toggleSegmentEdges() {
	operateOnSegmentEdges([](OpEdge* edge) {
		edge->debugDraw ^= true;
	});
}

static void operateOnTemporaryEdges(std::function<void (OpEdge*)> fun) {
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		OpEdge* edge = const_cast<OpEdge*>(*edgeIter);
		if (!edgeIter.isFiller && !edgeIter.isCurveCurve)
			continue;
		fun(edge);
	}
	OpDebugImage::drawDoubleFocus();
}

void hideTemporaryEdges() {
	operateOnTemporaryEdges([](OpEdge* edge) {
		edge->debugDraw = false;
	});
}

void showTemporaryEdges() {
	operateOnTemporaryEdges([](OpEdge* edge) {
		edge->debugDraw = true;
	});
}

void toggleTemporaryEdges() {
	operateOnTemporaryEdges([](OpEdge* edge) {
		edge->debugDraw ^= true;
	});
}

void colorActive(uint32_t color) {
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		OpEdge* edge = const_cast<OpEdge*>(*edgeIter);
		if (edge->active_impl) {
			edge->debugColor = color;
			edge->debugDraw = true;
		}
	}
	OpDebugImage::drawDoubleFocus();
}

void colorDisabled(uint32_t color) {
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		OpEdge* edge = const_cast<OpEdge*>(*edgeIter);
		if (edge->disabled) {
			edge->debugColor = color;
			edge->debugDraw = true;
		}
	}
	OpDebugImage::drawDoubleFocus();
}

extern std::vector<std::pair<uint32_t, std::string>> debugColorArray;

void colorOut(uint32_t color) {
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		OpEdge* edge = const_cast<OpEdge*>(*edgeIter);
		if (edge->inOutput) {
			if (OP_DEBUG_MULTICOLORED != color)
				edge->debugColor = color;
			else {
				edge->debugColor = debugColorArray[edge->debugOutPath % debugColorArray.size()].first;
			}
			edge->debugDraw = true;
		}
	}
	OpDebugImage::drawDoubleFocus();
}

void colorLinkups(uint32_t color) {
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		OpEdge* edge = const_cast<OpEdge*>(*edgeIter);
		if (edge->inLinkups) {
			edge->debugColor = color;
			edge->debugDraw = true;
		}
	}
	OpDebugImage::drawDoubleFocus();
}

void colorOpp(uint32_t color) {
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		OpEdge* edge = const_cast<OpEdge*>(*edgeIter);
#if OP_TEST_NEW_INTERFACE
		OpContour* contour = edge->winding.contour;
		if (contour->callBacks.debugIsOppFuncPtr(contour->caller)) {
			edge->debugColor = color;
			edge->debugDraw = true;
		}
#else
		if (OpOperand::right == edge->segment->contour->operand) {
			edge->debugColor = color;
			edge->debugDraw = true;
		}
#endif
	}
	OpDebugImage::drawDoubleFocus();
}

void colorPathsOut(uint32_t color) {
	pathsOutColor = color;
}

void colorUnsectables(uint32_t color) {
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		OpEdge* edge = const_cast<OpEdge*>(*edgeIter);
		if (edge->isUnsectable) {
			edge->debugColor = color;
			edge->debugDraw = true;
		}
	}
	OpDebugImage::drawDoubleFocus();
}

void colorUnsortables(uint32_t color) {
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		OpEdge* edge = const_cast<OpEdge*>(*edgeIter);
		if (edge->isUnsortable) {
			edge->debugColor = color;
			edge->debugDraw = true;
		}
	}
	OpDebugImage::drawDoubleFocus();
}

#define OP_X(Thing) \
void color##Thing() { \
	color##Thing(OP_DEBUG_MULTICOLORED); \
} \
void color##Thing(uint8_t alpha, uint32_t color) { \
	color##Thing((alpha << 24) | (color & 0x00FFFFFF)); \
} \
void uncolor##Thing() { \
	color##Thing(black); \
}
COLOR_LIST
#undef OP_X

void color(int id) {
	OpEdge* edge = findEdge(id);
	if (!edge)
		return;
	edge->debugColor = OP_DEBUG_MULTICOLORED;
	edge->debugDraw = true;
	OpDebugImage::drawDoubleFocus();
}

void color(int id, uint32_t c) {
	OpEdge* edge = findEdge(id);
	if (!edge)
		return;
	edge->debugColor = c;
	edge->debugDraw = true;
	OpDebugImage::drawDoubleFocus();
}

void uncolor(int id) {
	OpEdge* edge = findEdge(id);
	if (!edge)
		return;
	edge->debugColor = black;
	edge->debugDraw = true;
	OpDebugImage::drawDoubleFocus();
}

void colorLink(OpEdge* edge, uint32_t color) {
	auto colorChain = [edge, color](EdgeMatch which) {
		const OpEdge* looped = edge->debugIsLoop(which, LeadingLoop::in);
		bool firstLoop = false;
		int safetyCount = 0;
		OpEdge* chain = edge;
		for (;;) {
			chain->debugColor = color;
			chain->debugDraw = true;
			if (chain == looped) {
				if (firstLoop)
					return;
				firstLoop = true;
			}
			chain = EdgeMatch::start == which ? chain->priorEdge : chain->nextEdge;
			if (!chain)
				break;
			if (++safetyCount > 250) {
				OpDebugOut(std::string("!!! %s likely loops forever: ") + 
						(EdgeMatch::start == which ? "prior " : "next "));
				break;
			}
		}
	};
	colorChain(EdgeMatch::start);
	colorChain(EdgeMatch::end);
	OpDebugImage::drawDoubleFocus();
}

void colorLink(OpEdge& edge, uint32_t color) {
	colorLink(&edge, color);
}

void colorLink(int id, uint32_t color) {
	colorLink(findEdge(id), color);
}

void OpEdge::color(uint32_t c) {
	debugColor = c;
	OpDebugImage::drawDoubleFocus();
}

void OpEdge::addLink() {
	OpEdge* chain = this;
	std::vector<OpEdge*> seen;
	do {
		chain->debugDraw = true;
		seen.push_back(chain);
	} while ((chain = chain->nextEdge) && seen.end() == std::find(seen.begin(), seen.end(), chain));
	chain = this;
	while ((chain = chain->priorEdge) && seen.end() == std::find(seen.begin(), seen.end(), chain)) {
		chain->debugDraw = true;
		seen.push_back(chain);
	}
	DRAW_IDS_ON(Edges);
	OpDebugImage::drawDoubleFocus();
}

void OpEdge::drawLink() {
	hideEdges();
	addLink();
}

void OpJoiner::debugDraw() {
	for (auto e : byArea)
		e->debugDraw = true;
	for (auto e : unsectByArea)
		e->debugDraw = true;
	OpDebugImage::focusEdges();
}

void OpWinder::debugDraw() {
	for (auto e : inX)
		e->debugDraw = true;
	for (auto e : inY)
		e->debugDraw = true;
	OpDebugImage::focusEdges();
}

#if 0
void OpCurveCurve::draw() const {
	if (!edgeCurves.c.size())
		return OpDebugOut("OpCurveCurve missing edgeCurves\n");
	OpPointBounds focusRect = edgeCurves.c.front()->ptBounds;
	for (auto edgesPtrs : { &edgeCurves.c, &oppCurves.c }) {
		for (auto& edge : *edgesPtrs)
			focusRect.add(edge->ptBounds);
	}
	DRAW_IDS_ON(Edges);
	OpDebugImage::drawDoubleFocus(focusRect, false);
}
#endif

bool OpDebugImage::drawEdgeNormal(OpVector norm, OpPoint midTPt, int edgeID, uint32_t color) {
	LinePts normal { midTPt, midTPt + norm };
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

bool OpDebugImage::drawTangent(OpVector tan, OpPoint midTPt, int id, uint32_t color) {
	LinePts tangent { midTPt, midTPt + tan };
	SkPath tangentPath;
	tangentPath.moveTo(tangent.pts[0].x, tangent.pts[0].y);
	if (!tangent.pts[1].isFinite()) {
		OpDebugOut("tangent not finite (id)" + STR(id) + "\n");
		return false;
	}
	tangentPath.lineTo(tangent.pts[1].x, tangent.pts[1].y);
	OpDebugImage::addArrowHeadToPath(tangent, tangentPath);
	OpDebugImage::drawPath(tangentPath, color);
	return true;
}

bool OpDebugImage::drawWinding(const OpCurve& curve, std::string left, std::string right,
		float normSign, uint32_t color) {
	for (bool allowIntersect : { false, true } ) {
		for (float normLength : { 4, 15 } ) {
			for (float normT : { .58f, .38f, .78f, .18f, .98f } ) {
				OpVector norm = curve.normal(normT).normalize() * normLength;
				if (!norm.isFinite() || norm == OpVector{ 0, 0 })
					continue;
				OpPoint midTPt = curve.ptAtT(normT);
				SkRect bounds, rightBounds;
				std::string lefty = left + "_";
				labelFont.measureText(lefty.c_str(), lefty.length(), SkTextEncoding::kUTF8, 
						&bounds);
				labelFont.measureText(right.c_str(), right.length(), SkTextEncoding::kUTF8, 
						&rightBounds);
				float leftWidth = bounds.width();
				bounds.fRight += rightBounds.width();
				const int xOffset = 2;
				const int yOffset = 1;
				bounds.inset(-xOffset, -yOffset);
				SkRect trimmed = bounds;
				OpVector textOffset = norm * normSign;
				if (textOffset.dx < 0)
					textOffset.dx -= bounds.width();
				if (textOffset.dy > 0)
					textOffset.dy += bounds.height();
				OpPoint sumSide = midTPt + textOffset;
				trimmed.offset(sumSide.x, sumSide.y);
				if (!bitsAreBlank(allowIntersect, trimmed))
					continue;
				SkCanvas textLayer(bitmap);
				SkPaint paint;
				paint.setAntiAlias(true);
				paint.setColor(color);
				textLayer.drawString(SkString(left), sumSide.x, sumSide.y, labelFont, paint);
				paint.setColor(SK_ColorRED);
				textLayer.drawString(SkString(right), sumSide.x + leftWidth, sumSide.y,
						labelFont, paint);
				return true;
			}
		}
	}
	return false;
}

// fails may not have overflowed; they may have not found a place to draw the winding
bool OpDebugImage::drawEdgeWinding(const OpCurve& curve, const OpEdge* edge, uint32_t color) {
	bool success = true;
	const OpWinding& sum = edge->sum;
#if OP_TEST_NEW_INTERFACE
	std::string sumLeft = sum.isSet() ? sum.contour->callBacks.debugImageOutFuncPtr(sum.w, 0) : "?";
	std::string sumRight = sum.isSet() ? sum.contour->callBacks.debugImageOutFuncPtr(sum.w, 1) : "?";
#else
	std::string sumLeft = OpMax == sum.left() ? "?" : STR(sum.left());
	std::string sumRight = OpMax == sum.right() ? "?" : STR(sum.right());
#endif
	if (!drawWinding(curve, sumLeft, sumRight, 1, color)) {
//		OpDebugOut("normalize overflowed: edge " + STR(edge->id) + "\n");
		success = false;
	}
#if OP_TEST_NEW_INTERFACE
	auto sumString = [edge](const OpWinding& wind, const OpWinding& sum, int index) {
		if (!sum.isSet() && !wind.isSet())
			return std::string("?");
		if (!sum.isSet())
			return wind.contour->callBacks.debugImageOutFuncPtr(wind.w, index);
		OpWinding diffWind(edge->sum.contour, edge->sum.w);
		diffWind.w = wind.contour->callBacks.windingSubtractFuncPtr(diffWind.w, wind.w);
		return wind.contour->callBacks.debugImageOutFuncPtr(diffWind.w, index);
	};
	std::string oppLeft = sumString(edge->winding, sum, 0);
	std::string oppRight = sumString(edge->winding, sum, 1);
#else
	auto sumString = [](int w, int s) {
		return OpMax == s ? OpMax == w ? "?" : STR(w) : STR(s - w);
	};
	std::string oppLeft = sumString(edge->winding.left(), sum.left());
	std::string oppRight = sumString(edge->winding.right(), sum.right());
#endif
	if (!drawWinding(curve, oppLeft, oppRight, -1, color)) {
//		OpDebugOut("normalize overflowed: edge " + STR(edge->id) + "\n");
		success = false;
	}
	return success;
}

bool OpDebugImage::drawCurve(const OpCurve& curve, uint32_t color) {
	SkPath curvePath;
	OP_ASSERT(curve.isLine());  // !!! add more types as needed
	curvePath.moveTo(curve.firstPt().x, curve.firstPt().y);
	curvePath.lineTo(curve.lastPt().x, curve.lastPt().y);
	OpDebugImage::drawPath(curvePath, color);
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

void add(std::vector<OpEdge*>& e) {
	for (auto edge : e) {
		edge->debugDraw = true;
	}
	OpDebugImage::focusEdges();
}

void add(std::vector<OpEdge>& e) {
	for (auto& edge : e) {
		edge.debugDraw = true;
	}
	OpDebugImage::focusEdges();
}

#if OP_DEBUG_VERBOSE
void depth(int level) {
    if (!debugGlobalContours->debugCurveCurve)
        return;
    debugVerboseDepth = level;
	OpDebugImage::drawDoubleFocus();
}
#endif

void draw(std::vector<OpEdge*>& e) {
	hideEdges();
	add(e);
}

void draw(std::vector<OpEdge>& e) {
	hideEdges();
	add(e);
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

#if !OP_TEST_NEW_INTERFACE
void draw(const OpLine& line) {
	OpDebugImage::add(line);
	drawLinesOn = true;
	OpDebugImage::drawDoubleFocus();
}
#endif

void draw(const OpPoint& pt) {
	OpPtT ptT = { pt, OpNaN };
	draw(ptT);
}

void draw(const OpPtT& ptT) {
	OpDebugImage::add(ptT);
	drawPointsOn = true;
	OpDebugImage::drawDoubleFocus();
}

void draw(const OpPoint* pt) {
	draw(*pt);
}

void draw(const OpPtT* ptT) {
	draw(*ptT);
}

void draw(float x, float y) {
	draw(OpPoint(x, y));
}

bool OpSegment::debugContains(const OpEdge* edge) const {
	for (auto& e : edges) {
		if (edge == &e)
			return true;
	}
	return false;
}

#if 0
void OpInPath::draw() const {
	drawLeftOn = drawRightOn = true;
	OpDebugImage::drawDoubleFocus();
}

void OpOutPath::draw() const {
	drawPathsOutOn = true;
	OpDebugImage::drawDoubleFocus();
}
#endif

OpPoint OpDebugImage::find(int id, float t) {
	for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
		OpEdge* edge = const_cast<OpEdge*>(*edgeIter);
		if (id != edge->id)
			continue;
		edge->debugDraw = true;
		DRAW_IDS_ON(Edges);
		return edge->curve.ptAtT(t);
	}
	const OpSegment* segment = nullptr;
	for (auto s : segmentIterator) {
		if (id == s->id)
			segment = s;
	}
	if (segment) {
		DRAW_IDS_ON(Segments);
		return segment->c.ptAtT(t);
	}
#if OP_DEBUG
	const OpIntersection* sect = nullptr;
	for (auto i : intersectionIterator) {
		if (id == i->id)
			sect = i;
	}
	if (sect) {
		DRAW_IDS_ON(Intersections);
		if (t != sect->ptT.t)
			OpDebugOut("intersection id " + STR(id) + " does not match t " + STR(t) + "\n");
		return sect->ptT.pt;
	}
#endif
#if OP_DEBUG
	auto coins = findCoincidence(id);
	if (coins.size()) {
		DRAW_IDS_ON(Coincidences);
		for (auto coin : coins) {
			if (t == coin->ptT.t)
				return coin->ptT.pt;
		}
		OpDebugOut("coincidence id " + STR(id) + " does not match t " + STR(t) + "\n");
		return OpPoint();
	}
#endif
	OpDebugOut("id " + STR(id) + " not found\n");
	return OpPoint();
}

// !!! macroize?
void drawT(int id, float t) {
	OpPoint pt = OpDebugImage::find(id, t);
	if (!pt.isFinite())
		return;
	draw(pt);
}

void drawT(int id, const OpPtT& ptT) {
	drawT(id, ptT.t);
}

void drawT(int id, const OpPtT* ptT) {
	drawT(id, *ptT);
}

void drawT(const OpSegment& segment, float t) {
	draw(segment.c.ptAtT(t));
}

void drawT(const OpSegment& segment, const OpPtT& ptT) {
	drawT(segment, ptT.t);
}

void drawT(const OpSegment& segment, const OpPtT* ptT) {
	drawT(segment, *ptT);
}

void drawT(const OpSegment* segment, float t) {
	drawT(*segment, t);
}

void drawT(const OpSegment* segment, const OpPtT& ptT) {
	drawT(*segment, ptT);
}

void drawT(const OpSegment* segment, const OpPtT* ptT) {
	drawT(*segment, ptT);
}

void drawT(const OpEdge& edge, float t) {
	draw(edge.curve.ptAtT(t));
}

void drawT(const OpEdge& edge, const OpPtT& ptT) {
	drawT(edge, ptT.t);
}

void drawT(const OpEdge& edge, const OpPtT* ptT) {
	drawT(edge, *ptT);
}

void drawT(const OpEdge* edge, float t) {
	drawT(*edge, t);
}

void drawT(const OpEdge* edge, const OpPtT& ptT) {
	drawT(*edge, ptT);
}

void drawT(const OpEdge* edge, const OpPtT* ptT) {
	drawT(*edge, ptT);
}

void help() {
	OpDebugOut("navigation: l(eft) r(ight) u(p) d(own) i(n) oo(ut)\n");
	OpDebugOut("focus(id) resetFocus() center(id) clear()\n");
	OpDebugOut("showLeft() showRight() showPoints() showIDs()\n");
	OpDebugOut("hideIDs() hideValues() hideHex()\n");
}

void resetFocus() {
	OpPointBounds focusRect;
	if (!drawSegmentsOn 
#if !OP_TEST_NEW_INTERFACE
		&& !drawLeftOn && !drawRightOn 
#endif
		&& drawEdgesOn) {
		int saveLevel = edgeIterDvLevel;
		edgeIterDvLevel = std::max(0, edgeIterDvLevel);
		for (auto edgeIter = edgeIterator.begin(); edgeIter != edgeIterator.end(); ++edgeIter) {
			const OpEdge* edge = *edgeIter;
			if (!edge->debugDraw)
				continue;
			focusRect.add(edge->ptBounds);
		}
		edgeIterDvLevel = saveLevel;
	}
	if (!focusRect.isFinite()) {
#if OP_TEST_NEW_INTERFACE
		for (auto contour : debugGlobalContours->contours) {
			SkPath* path = (SkPath*) contour->callBacks.debugNativePathFuncPtr(contour->caller);
			SkRect skrect = path->getBounds();
			focusRect.left = std::min(skrect.fLeft, focusRect.left);
			focusRect.top = std::min(skrect.fTop, focusRect.top);
			focusRect.right = std::max(skrect.fRight, focusRect.right);
			focusRect.bottom = std::max(skrect.fBottom, focusRect.bottom);
		}
#else
		for (unsigned index = 0; index < 2; ++index) {
			OpInPath* inPath = index ? debugGlobalContours->rightIn : debugGlobalContours->leftIn;
			SkRect skrect = ((SkPath*) inPath->externalReference)->getBounds();
			focusRect.left = std::min(skrect.fLeft, focusRect.left);
			focusRect.top = std::min(skrect.fTop, focusRect.top);
			focusRect.right = std::max(skrect.fRight, focusRect.right);
			focusRect.bottom = std::max(skrect.fBottom, focusRect.bottom);
		}
#endif
	}
	if (focusRect.isFinite()) {
		OpDebugImage::drawDoubleFocus(focusRect, false);
		oo();
	} else
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
