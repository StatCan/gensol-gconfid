#ifndef _STC_GROUP_H_
#define _STC_GROUP_H_

#include "STC_Coordinate.h"
#include "STC_Hierarchy.h"
#include "STC_Range.h"

#ifndef STCM_CODE_MAX_LENGTH
#define STCM_CODE_MAX_LENGTH 100
#endif

struct STCT_GROUP {
	int NumberEntries;
	int NumberAllocated;
	STCT_COORDINATE ** Coordinate;
};
typedef struct STCT_GROUP STCT_GROUP;

struct STCT_GROUPROOT {
	int NumberEntries;
	int NumberAllocated;
	struct STCT_GROUP ** Group;
};
typedef struct STCT_GROUPROOT STCT_GROUPROOT;

extern EIT_RETURNCODE STC_GroupParse (char * p,
	STCT_GROUPROOT ** GroupRoot, STCT_HTREEROOT * HTreeRoot, STCT_RANGEROOT * RangeRoot,
	char * CodeFirstCharacterCharacterSet, char * CodeCharacterSet);
extern void STC_GroupRootFree (STCT_GROUPROOT * GroupRoot);
extern void STC_GroupRootPrint (STCT_GROUPROOT * GroupRoot, STCT_HTREEROOT * HTreeRoot);
extern void STC_GroupPrint (STCT_GROUP * Group, STCT_HTREEROOT * HTreeRoot);

#endif
