// (c) 2023, Cary Clark cclark2@gmail.com
#ifndef OpDebugRecord_DEFINED
#define OpDebugRecord_DEFINED

#include "OpTestDrive.h"

#if !OP_DEBUG_RECORD

#define OpDebugRecordStart(opp, edge)
#define OpDebugRecord(curve, t, pt)
#define OpDebugRecordEnd();

#else

#include "OpMath.h"

struct OpCurve;
struct OpEdge;

extern void OpDebugRecordStart(const OpEdge& opp, const OpEdge& edge);
extern void OpDebugRecord(const OpCurve& , float t, OpPoint , const OpRoots& );
extern void OpDebugRecordEnd();
extern void OpDebugRecordPause();
extern void OpDebugRecordResume();
extern void OpDebugRecordSuccess(unsigned );

#endif

#endif
