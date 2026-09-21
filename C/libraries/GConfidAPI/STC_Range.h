#ifndef _STC_RANGE_H_
#define _STC_RANGE_H_

#ifndef STCM_CODE_MAX_LENGTH
#define STCM_CODE_MAX_LENGTH 100
#endif

#define STCM_LOHI_NOT_SET INT_MAX

struct STCT_RANGEITEM {
	char * Code;
	char * Data;
	int Lo;
	int Hi;
};
typedef struct STCT_RANGEITEM STCT_RANGEITEM;

struct STCT_RANGE {
	int NumberEntries;
	int NumberAllocated;
	struct STCT_RANGEITEM ** Item;
};
typedef struct STCT_RANGE STCT_RANGE;

struct STCT_RANGEROOT {
	int NumberEntries;
	int NumberAllocated;
	STCT_RANGE ** Dimension;
	char ** Name;
};
typedef struct STCT_RANGEROOT STCT_RANGEROOT;

extern EIT_RETURNCODE STC_RangeAddItem (STCT_RANGE * Range, char * Code);
extern STCT_RANGE * STC_RangeAllocate (void);
extern void STC_RangeFree (STCT_RANGE * Range);
extern EIT_BOOLEAN STC_RangeIntersect (STCT_RANGE * R1, STCT_RANGE * R2);
extern EIT_RETURNCODE STC_RangeParse (char * p, STCT_RANGEROOT ** RangeRoot,
	char * CodeFirstCharacterCharacterSet, char * CodeCharacterSet);
extern void STC_RangePrint (STCT_RANGE * Range);
extern EIT_RETURNCODE STC_RangeSearchFromCodesToRanges (STCT_RANGE * Range,
	char * Code, STCT_RANGE * Range2);
extern STCT_RANGEROOT * STC_RangeRootAllocate (int NumberAllocated);
extern void STC_RangeRootFree (STCT_RANGEROOT * RangeRoot);
extern void STC_RangeRootPrint (STCT_RANGEROOT * RangeRoot);
extern EIT_RETURNCODE STC_RangeRootSetName (STCT_RANGEROOT * RangeRoot, char * Name, int Index);
extern void STC_RangeSimplifyAggressively (STCT_RANGE * Range);

#endif
