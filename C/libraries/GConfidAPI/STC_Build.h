#ifndef _STC_BUILD_H_
#define _STC_BUILD_H_

#include "STC_Cell.h"
#include "STC_Coordinate.h"
#include "STC_Group.h"
#include "STC_Hierarchy.h"
#include "STC_KdTree.h"
#include "STC_Range.h"
#include "STC_SRule.h"

#include "slist.h"

/*! Return codes for STC_AddMicrodata() function. */
enum STCT_ADDMICRODATA_RETURNCODE {
         /*! Function failed. Caller should quit. It's a out of memory error. */
    STCE_ADDMICRODATA_FAIL = EIE_FAIL,
         /*! Function succeed. */
    STCE_ADDMICRODATA_SUCCEED = EIE_SUCCEED,
         /*! Function succeed, but obs read was invalid. */
    STCE_ADDMICRODATA_INVALID
};
typedef enum STCT_ADDMICRODATA_RETURNCODE STCT_ADDMICRODATA_RETURNCODE;


extern STCT_ADDMICRODATA_RETURNCODE STC_AddMicrodata (STCT_HTREEROOT * HTreeRoot,
	STCT_RANGEROOT * RangeRoot, STCT_CELLSET * CellSet, STCT_KDTREE ** KdTree,
	char ** ReadCode, char * Key, double Value, double Shadow, double Proxy, double Weight,
	tSList ** Codes, tSList ** MissingCodes, STCT_COORDINATE * Coordinate, int * MessageQuota,
	int waiver_flags_present, int waiver_flag);
extern EIT_RETURNCODE STC_AugmentRange (STCT_RANGEROOT * RangeRoot,
	STCT_HTREEROOT * HTreeRoot);
extern EIT_RETURNCODE STC_CalculateMarginalCellsSensitivity (
	STCT_HTREEROOT * HTreeRoot, STCT_CELLSET * CellSet, STCT_KDTREE * KDTree,
	STCT_SRULE * SRule, int * NumberMarginalCellsCalculated, int * NumberMarginalCellsSensitive,
	int * NumberMarginalCellsZero);
extern EIT_RETURNCODE STC_CalculateInternalCellsFavCost (
	STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule);
extern EIT_RETURNCODE STC_CalculateInternalCellsSensitivity (
	STCT_CELLSET * CellSet, STCT_SRULE * SRule,
	int * NumberInternalCellsCalculated, int * NumberInternalCellsSensitive, int * NumberInternalCellsZero);
extern EIT_RETURNCODE STC_WriteInternalCells (
	STCT_CELLSET * CellSet,
	STCT_SRULE * SRule,
	int waiver_flags_present);
extern EIT_RETURNCODE STC_CalculateUserGroupsSensitivity (
	STCT_GROUPROOT * GroupRoot, STCT_HTREEROOT * HTreeRoot,
	STCT_KDTREE * KdTree, STCT_SRULE * SRule,
	int * NumberUserGroupsCalculated, int * NumberUserGroupsSensitive);
extern EIT_RETURNCODE STC_FindSensitiveAggregates (
	STCT_HTREEROOT * HTreeRoot, STCT_KDTREE * KdTree, STCT_SRULE * SRule,
	int M, double X, double Y, double Z, int Verbose,
	int * NumberCombinaisonsCalculated, int * NumberCombinaisonsSensitive);
extern EIT_RETURNCODE STC_GetEndCodes (STCT_HTREE * HTree, STCT_RANGE * Range,
	int Decomposition, STCT_RANGE * Range2);
extern void STC_PruneRange (STCT_RANGEROOT * RangeRoot, STCT_HTREEROOT * HTreeRoot);
extern EIT_RETURNCODE STC_ValidateCodes (STCT_HTREEROOT * HTreeRoot,
	STCT_RANGEROOT * RangeRoot);

#endif
