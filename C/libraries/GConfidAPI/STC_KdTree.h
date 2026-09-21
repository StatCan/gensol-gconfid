#ifndef _STC_KDTREE_H_
#define _STC_KDTREE_H_

#include "STC_Cell.h"
#include "STC_Coordinate.h"

struct STCT_KDTREE {
    STCT_CELL * Cell;
    struct STCT_KDTREE * Left;
    struct STCT_KDTREE * Right;
};
typedef struct STCT_KDTREE STCT_KDTREE;

extern void STC_KdTreeFree (STCT_KDTREE * KdTree);
extern STCT_KDTREE * STC_KdTreeInsert (STCT_KDTREE * KdTree,
	STCT_CELL * Cell);
extern void STC_KdTreePrint (STCT_KDTREE * KdTree, STCT_HTREEROOT * HTreeRoot);
extern void STC_KdTreePrintFlat (STCT_KDTREE * KdTree, STCT_HTREEROOT * HTreeRoot);
extern EIT_RETURNCODE STC_KdTreeRangeSearch (STCT_KDTREE * KdTree,
	STCT_COORDINATE * SmallestCoordinate, STCT_COORDINATE * LargestCoordinate,
	STCT_CELLSET * CellSet);
extern STCT_CELL * STC_KdTreeSearch (STCT_KDTREE * KdTree,
	STCT_COORDINATE * Coordinate);

#endif
