#ifndef SkiaEnumSkPathOp_DEFINED
#define SkiaEnumSkPathOp_DEFINED

// because enum cannot be forward declared, duplicate Skia source here
enum SkPathOp {
    kDifference_SkPathOp,         //!< subtract the op path from the first path
    kIntersect_SkPathOp,          //!< intersect the two paths
    kUnion_SkPathOp,              //!< union (inclusive-or) the two paths
    kXOR_SkPathOp,                //!< exclusive-or the two paths
    kReverseDifference_SkPathOp,  //!< subtract the first path from the op path
};

#endif
