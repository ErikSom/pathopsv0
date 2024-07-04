// (c) 2023, Cary Clark cclark2@gmail.com
#include "OpContour.h"
#include "PathOps.h"

bool PathOps(OpInPath& left, OpInPath& right, OpOperator opOperator, OpOutPath& result
        OP_DEBUG_PARAMS(OpDebugData& debugData)) {
    OpContours contourList(left, right, opOperator);
#if OP_DEBUG
    contourList.debugExpect = debugData.debugExpect;
#if !OP_TEST_NEW_INTERFACE
    contourList.debugResult = &result;
#endif
    contourList.debugInPathOps = true;
    contourList.debugInClearEdges = false;
    contourList.debugTestname = debugData.debugTestname;
    OP_DEBUG_DUMP_CODE(contourList.dumpCurve1 = debugData.debugCurveCurve1);
    OP_DEBUG_DUMP_CODE(contourList.dumpCurve2 = debugData.debugCurveCurve2);
    OP_DEBUG_DUMP_CODE(contourList.debugBreakDepth = debugData.debugCurveCurveDepth);
    contourList.newInterface = false;
#endif
#if OP_DEBUG_IMAGE || OP_DEBUG_DUMP
    debugGlobalContours = &contourList;
#endif
#if OP_DEBUG_IMAGE
    OpDebugImage::init();
//    oo();
#endif
    bool success = contourList.pathOps(result);
#if OP_DEBUG
    debugData.debugWarnings = contourList.debugWarnings;
#endif
    return success;
}

// new interface

namespace PathOpsV0Lib {

Context* CreateContext(AddContext callerData) {
    OpContours* contours = new OpContours();
    contours->addCallerData(callerData);
    contours->newInterface = true;
#if OP_DEBUG_IMAGE || OP_DEBUG_DUMP
    debugGlobalContours = contours;
#endif
#if OP_DEBUG_IMAGE
    OpDebugImage::init();
    oo();
#endif
    return (Context*) contours;
}

void Add(AddCurve curve, AddWinding windings) {
    OpContour* contour = (OpContour*) windings.contour;
#if OP_DEBUG_IMAGE || OP_DEBUG_DUMP
    debugGlobalContours = contour->contours;
#endif
    contour->segments.emplace_back(curve, windings);
}

Contour* CreateContour(AddContour callerData) {
    // reuse existing contour
    OpContours* contours = (OpContours*) callerData.context;
#if OP_DEBUG_IMAGE || OP_DEBUG_DUMP
    debugGlobalContours = contours;
#endif
    OpContour* contour = contours->makeContour();
    contour->addCallerData(callerData);
    return (Contour*) contour;
}

void DeleteContext(Context* context) {
    OpContours* contours = (OpContours*) context;
#if OP_DEBUG_IMAGE || OP_DEBUG_DUMP
    debugGlobalContours = contours;
#endif
    delete contours;
#if OP_DEBUG_IMAGE || OP_DEBUG_DUMP
    debugGlobalContours = nullptr;
#endif
}

int Error(Context* context) {
#if OP_DEBUG_IMAGE || OP_DEBUG_DUMP
    OpContours* contours = (OpContours*) context;
    debugGlobalContours = contours;
#endif
    // !!! incomplete
    return 0;
}

void Resolve(Context* context, PathOutput output) {
    OpOutPath result(nullptr);  // missing external reference for now is new interface flag
    OpContours* contours = (OpContours*) context;
    contours->callerOutput = output;
#if OP_DEBUG_IMAGE || OP_DEBUG_DUMP
    debugGlobalContours = contours;
#endif
    // !!! change this to record error instead of success
    /* bool success = */ contours->pathOps(result);
}

void SetContextCallBacks(Context* context
		OP_DEBUG_IMAGE_PARAMS(DebugNativeOutColor debugNativeOutColor)
) {
    OpContours* contours = (OpContours*) context;
    contours->contextCallBacks = {
		OP_DEBUG_IMAGE_CODE(debugNativeOutColor)
    };
}

OpType SetCurveCallBacks(Context* context, AxisRawHit axisFunc, ControlNearlyEnd nearlyFunc,
        CurveHull hullFunc,
        CurveIsFinite isFiniteFunc, CurveIsLine isLineFunc, CurveIsLinear isLinearFunc, 
        SetBounds setBoundsFunc,
        CurveNormal normalFunc, CurveOutput outputFunc, CurveReverse reverseFunc, 
        CurveTangent tangentFunc,
        CurvesEqual equalFunc, PtAtT ptAtTFunc, DoublePtAtT doublePtAtTFunc, 
        PtCount ptCountFunc, Rotate rotateFunc, SubDivide subDivideFunc, XYAtT xyAtTFunc
        OP_DEBUG_DUMP_PARAMS(DebugDumpCurveSize debugDumpSizeFunc,
                DebugDumpCurveExtra debugDumpExtraFunc, DebugDumpCurveSet dumpSetFunc,
                DebugDumpCurveSetExtra dumpSetExtraFunc)
		OP_DEBUG_IMAGE_PARAMS(DebugAddToPath debugAddToPathFunc)
) {
    OpContours* contours = (OpContours*) context;
    contours->callBacks.push_back( { axisFunc, nearlyFunc, hullFunc, isFiniteFunc, isLineFunc, 
            isLinearFunc,
            setBoundsFunc, normalFunc, outputFunc, reverseFunc, tangentFunc,
            equalFunc, ptAtTFunc, doublePtAtTFunc, ptCountFunc, rotateFunc, subDivideFunc, xyAtTFunc 
		    OP_DEBUG_DUMP_PARAMS(debugDumpSizeFunc, debugDumpExtraFunc, dumpSetFunc, 
                    dumpSetExtraFunc)
		    OP_DEBUG_IMAGE_PARAMS(debugAddToPathFunc)
            } );
    return (OpType) contours->callBacks.size();
}

void SetWindingCallBacks(Contour* ctour, WindingAdd addFunc, WindingKeep keepFunc,
        WindingSubtract subtractFunc, WindingVisible visibleFunc, WindingZero zeroFunc
		OP_DEBUG_DUMP_PARAMS(DebugDumpContourIn dumpInFunc, DebugDumpContourOut dumpOutFunc, 
                DebugDumpContourExtra dumpFunc)
        OP_DEBUG_IMAGE_PARAMS(DebugImageOut dumpImageOutFunc, 
                DebugCCOverlapsColor debugCCOverlapsColorFunc,
                DebugCurveCurveColor debugCurveCurveColorFunc,
                DebugNativeFillColor debugNativeFillColorFunc, 
                DebugNativeInColor debugNativeInColorFunc,
                DebugNativePath debugNativePathFunc, 
                DebugContourDraw debugDrawFunc,
                DebugIsOpp debugIsOppFunc)
) {
    OpContour* contour = (OpContour*) ctour;
    contour->callBacks = { addFunc, keepFunc, subtractFunc, visibleFunc, zeroFunc 
            OP_DEBUG_DUMP_PARAMS(dumpInFunc, dumpOutFunc, dumpFunc)
            OP_DEBUG_IMAGE_PARAMS(dumpImageOutFunc, debugCCOverlapsColorFunc,
                    debugCurveCurveColorFunc, 
                    debugNativeFillColorFunc, debugNativeInColorFunc,
                    debugNativePathFunc, debugDrawFunc, debugIsOppFunc)
            };
}

#if OP_DEBUG
void Debug(Context* context, OpDebugData& debugData) {
    OpContours* contours = (OpContours*) context;
    contours->debugTestname = debugData.debugTestname;
    OP_DEBUG_DUMP_CODE(contours->dumpCurve1 = debugData.debugCurveCurve1);
    OP_DEBUG_DUMP_CODE(contours->dumpCurve2 = debugData.debugCurveCurve2);
    OP_DEBUG_DUMP_CODE(contours->debugBreakDepth = debugData.debugCurveCurveDepth);
}
#endif

} // namespace PathOpsV0Lib
