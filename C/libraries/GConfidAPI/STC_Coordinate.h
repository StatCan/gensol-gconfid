#ifndef _STC_COORDINATE_H_
#define _STC_COORDINATE_H_

#include "STC_Hierarchy.h"

struct STCT_COORDINATE {
	int * Tag;
    int NumberEntries;
};
typedef struct STCT_COORDINATE STCT_COORDINATE;

extern STCT_COORDINATE * STC_CoordinateAllocate (int NumberEntries);
extern void STC_CoordinateCopy (STCT_COORDINATE * d, STCT_COORDINATE * s);
extern STCT_COORDINATE * STC_CoordinateDuplicate (STCT_COORDINATE * Coordinate);
extern EIT_BOOLEAN STC_CoordinateEqual (STCT_COORDINATE * Coordinate1,
	STCT_COORDINATE * Coordinate2);
extern void STC_CoordinateFree (STCT_COORDINATE * Coordinate);
extern void STC_CoordinatePrint (STCT_COORDINATE * Coordinate,
	STCT_HTREEROOT * HTreeRoot);

#endif
