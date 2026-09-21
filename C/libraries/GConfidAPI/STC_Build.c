#ifdef _DEBUG
#include <assert.h>
#endif
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EI_Message.h"
#include "MessageGConfidAPI.h"
#include "slist.h"
#include "STC_Build.h"
#include "STC_Cell.h"
#include "STC_Group.h"
#include "STC_Hierarchy.h"
#include "STC_KdTree.h"
#include "STC_Memory.h"
#include "STC_Range.h"
#include "STC_Step0123.h"
#include "STC_Write.h"
#include "util.h"

#include "internal_rules.h"


/*
set DEBUG to 1 to activate the debugging print statements.
set DEBUG to 0 to deactivate the debugging print statements.
If DEBUG is zero, most compilers will not generate any code for the debugging
statements.
*/
enum {DEBUG = 0};
enum {DEBUGCMC = 0};
enum {DEBUGFSA = 0};

//#define PRINT_NUMBER_EQ
#undef PRINT_NUMBER_EQ
#ifdef PRINT_NUMBER_EQ
static int mNumberEq;
static int mSubNumberEq;
#endif


static EIT_RETURNCODE AugmentRange (STCT_HTREE * HTree, STCT_RANGE * Range);
static EIT_RETURNCODE CalculateMarginalCell (STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet, STCT_KDTREE * KdTree, STCT_SRULE * SRule,
	STCT_COORDINATE * Coordinate, STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate, STCT_COORDINATE * WorkingCoordinate,
	STCT_CELLSET * MarginalCellSet, STCT_CELLSET * SubMarginalCellSet,
	int * NumberMarginalCellsCalculated, int * NumberMarginalCellsSensitive,
	int * NumberMarginalCellsZero);
static EIT_RETURNCODE CalculateFavCostForMarginalCell (
	STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule,
	STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate,
	STCT_CELLSET * MarginalCellSet,
	int aggregate_dimension);
static EIT_RETURNCODE CalculateMarginalCellSensitivityForOneCell (
	STCT_HTREEROOT * HTreeRoot, STCT_CELLSET * CellSet, STCT_KDTREE * KdTree,
	STCT_SRULE * SRule, STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate, STCT_COORDINATE * LargestCoordinate,
	STCT_COORDINATE * WorkingCoordinate, STCT_CELLSET * MarginalCellSet,
	STCT_CELLSET * SubMarginalCellSet, int * NumberMarginalCellsCalculated,
	int * NumberMarginalCellsSensitive, int * NumberMarginalCellsZero);
static EIT_RETURNCODE CalculateMarginalCellsSensitivityForAllCells (
	STCT_HTREEROOT * HTreeRoot, STCT_CELLSET * CellSet, STCT_KDTREE * KdTree,
	STCT_SRULE * SRule, int iDimension, STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate, STCT_COORDINATE * LargestCoordinate,
	STCT_COORDINATE * WorkingCoordinate,
	STCT_CELLSET * MarginalCellSet, STCT_CELLSET * SubMarginalCellSet,
	int * NumberMarginalCellsCalculated, int * NumberMarginalCellsSensitive,
	int * NumberMarginalCellsZero);
static EIT_RETURNCODE CalculateFavCostForInternalCells (
	STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule,
	int iDimension,
	STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate,
	STCT_CELLSET * MarginalCellSet);
static EIT_RETURNCODE STC_UpdateFavCostForInternalCells (
	STCT_CELLSET * CellSet,
	STCT_HTREEROOT * HTreeRoot,
	STCT_SRULE * SRule,
	STCT_COORDINATE * Coordinate,
	int aggregate_dimension);
static EIT_RETURNCODE CheckBranchesOverlapping (STCT_HTREE * HTree, STCT_RANGE * Range);
static EIT_RETURNCODE CheckDecompositionsEquivalence (STCT_HTREE * HTree,
	STCT_RANGE * Range);
static STCT_ADDMICRODATA_RETURNCODE CombineCodes (STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet, STCT_KDTREE ** KdTree, tSList ** Codes,
	int iDimension, STCT_COORDINATE * Coordinate, char * Key,
	double Value, double Shadow, double Proxy, double Weight, int * MessageQuota, int waiver_flags_present, int waiver_flag);
static EIT_RETURNCODE FindSensitiveAggregatesForAllMarginalCells (STCT_HTREEROOT * HTreeRoot,
	STCT_KDTREE * KdTree, STCT_SRULE * SRule,
	tIList ** Tags, tIList ** NonInternalTags,
	int M, double X, double Y, double Z, int N, int NN, int Verbose,
	int iDimension, int ExcludedDimension, STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate, STCT_COORDINATE * LargestCoordinate,
	STCT_CELLSET * MarginalCellSet, STCT_CELLSET * SubMarginalCellSet,
	int * NumberCombinaisonsCalculated, int * NumberCombinaisonsSensitive);
static EIT_RETURNCODE FindSensitiveAggregatesForOneMarginalCell (STCT_HTREEROOT * HTreeRoot,
	STCT_KDTREE * KdTree, STCT_SRULE * SRule,
	tIList ** NonInternalTags,
	int M, double X, double Y, double Z, int N, int NN, int Verbose,
	int ExcludedDimension, STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate, STCT_COORDINATE * LargestCoordinate,
	STCT_CELLSET * MarginalCellSet, STCT_CELLSET * SubMarginalCellSet,
	int * NumberCombinaisonsCalculated, int * NumberCombinaisonsSensitive);
static EIT_RETURNCODE FindSmallestCellSet (STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet, STCT_KDTREE * KdTree, STCT_SRULE * SRule,
	STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate, STCT_COORDINATE * LargestCoordinate,
	STCT_COORDINATE * WorkingCoordinate,
	STCT_CELLSET * MarginalCellSet, STCT_CELLSET * SubMarginalCellSet);
static void FindSmallestCellSetCoordinates (STCT_HTREEROOT * HTreeRoot,
	STCT_COORDINATE * Coordinate, int * SmallestNumberBranches,
	int * DecompositionWithSmallestNumberBranches,
	int * DimensionWithSmallestNumberBranches);
static int GetN (STCT_SRULE * SRule, int NN);
static int GetNN (STCT_SRULE * SRule);
static void GetSearchCoordinate (STCT_HTREEROOT * HTreeRoot,
	STCT_COORDINATE * Coordinate, STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate);
static EIT_RETURNCODE GetInternalCells (STCT_HTREEROOT * HTreeRoot,
	STCT_COORDINATE * Coordinate, STCT_KDTREE * KdTree, STCT_CELLSET * CellSet,
	STCT_COORDINATE * SmallestCoordinate, STCT_COORDINATE * LargestCoordinate);
static EIT_BOOLEAN IsCoordinateInternal (STCT_HTREEROOT * HTreeRoot,
	STCT_COORDINATE * Coordinate);
static STCT_RANGE ** RangeArrayAllocate (int n);
static void RangeArrayFree (STCT_RANGE * RangeArray[], int n);
static EIT_BOOLEAN RangeEqual (STCT_RANGE * R1, STCT_RANGE * R2);
static EIT_RETURNCODE RangeSearchFromRangesToCodes (
	STCT_RANGE * Range, char * Code, tSList *);
static EIT_RETURNCODE RemoveUnwantedCells (STCT_CELLSET * AggregateCellSet,
	STCT_HTREEROOT * HTreeRoot, STCT_COORDINATE * Coordinate);
static int SearchLargestRespondents (STCT_DATA * Data, char * Key);
static void SetLargestRespondents (STCT_CELL * d, STCT_CELL * s);
static EIT_RETURNCODE TranslateRangeCodeToHierarchyCodes (
	STCT_HTREEROOT * HtreeRoot, STCT_RANGEROOT * RangeRoot,
	char ** Code, tSList ** Codes);


/*
be carefull of the side effect on MessageQuota
same define in sensitiv.c
*/
#define ADDMESSAGEQUOTA(MessageQuota, ...)\
	do {\
		if (*MessageQuota <= 0)\
			break;\
		EI_AddMessage (M00043, EIE_MESSAGESEVERITY_WARNING, __VA_ARGS__);\
		(*MessageQuota)--;\
		if (*MessageQuota <= 0)\
			EI_AddMessage (M00043, EIE_MESSAGESEVERITY_WARNING, M00050 "\n");\
	} while (0)


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
STCT_ADDMICRODATA_RETURNCODE STC_AddMicrodata (
	STCT_HTREEROOT * HTreeRoot,
	STCT_RANGEROOT * RangeRoot,
	STCT_CELLSET * CellSet,
	STCT_KDTREE ** KdTree,
	char ** ReadCode,
	char * Key,
	double Value,
	double Shadow,
	double Proxy,
	double Weight,
	tSList ** Codes,
	tSList ** MissingCodes,
	STCT_COORDINATE * Coordinate,
	int * MessageQuota,
	int waiver_flags_present,
	int waiver_flag)
{
	int i;
	EIT_RETURNCODE rc;

	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		SList_Empty (Codes[i]);

		//find all codes in hierarchy that translate to this range code
		//rc = TranslateRangeCodeToHierarchyCodes (HTreeRoot, RangeRoot, ReadCode, Codes);
		//if (rc != EIE_SUCCEED) return STCE_ADDMICRODATA_FAIL;
		rc = RangeSearchFromRangesToCodes (RangeRoot->Dimension[i], ReadCode[i], Codes[i]);
		if (rc != EIE_SUCCEED) return STCE_ADDMICRODATA_FAIL;

		//check if a code is in the hierarchy/range
		if (Codes[i]->ne == 0) {
			/* print only once per missing code */
			if (SList_StringSearchBinary (MissingCodes[i], ReadCode[i]) == SLIST_NOTFOUND) {
				SList_Add (ReadCode[i], MissingCodes[i]);
				SList_Sort (MissingCodes[i], eSListSortAscending);
				ADDMESSAGEQUOTA (MessageQuota, M20028, HTreeRoot->Name[i], ReadCode[i]);//code not in hierarchy/range
			}
			return STCE_ADDMICRODATA_INVALID;
		}
	}

	//combine all codes from all dimensions to form all combinaison of internal cells
	return CombineCodes (HTreeRoot, CellSet, KdTree, Codes, 0, Coordinate, Key, Value, Shadow, Proxy, Weight, MessageQuota, waiver_flags_present, waiver_flag);
}

/*---------------------------------------------------------------------------------------------
Call recursive function that will calculate the value of the new "FavCost" variable
in each internal cell
(based on "STC_CalculateMarginalCellsSensitivity()"
 in "c:\tfs_global\SASTC\GeneralizedSystems\GConfid\GConfidAPI\GConfidAPI_1.01.002\STC_Build.c"
 --should be called (only when the user has specified waivers) in "sensitiv()"
 in "c:\tfs_global\SASTC\GeneralizedSystems\GConfid\GConfidProcedures\Sensitivity\sensitivity_Dev\sensitiv.c",
 right before the call to "CalculateMarginalCellsSensitivity()",
 using this code:
		rc = STC_CalculateInternalCellsFavCost (HTreeRoot, CellSet, KdTree);
)
---------------------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_CalculateInternalCellsFavCost (
	STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule)
{
	STCT_COORDINATE * Coordinate;
	STCT_COORDINATE * LargestCoordinate;
	STCT_CELLSET * MarginalCellSet;
	EIT_RETURNCODE rc;
	STCT_COORDINATE * SmallestCoordinate;

	Coordinate = STC_CoordinateAllocate (HTreeRoot->NumberEntries);
	if (Coordinate == NULL) return EIE_FAIL;
	SmallestCoordinate = STC_CoordinateAllocate (HTreeRoot->NumberEntries);
	if (SmallestCoordinate == NULL) return EIE_FAIL;
	LargestCoordinate = STC_CoordinateAllocate (HTreeRoot->NumberEntries);
	if (LargestCoordinate == NULL) return EIE_FAIL;
	MarginalCellSet = STC_CellSetAllocate (1000);
	if (LargestCoordinate == NULL) return EIE_FAIL;

	rc = CalculateFavCostForInternalCells (HTreeRoot, CellSet, KdTree, SRule,
		0, Coordinate, SmallestCoordinate, LargestCoordinate,
		MarginalCellSet);

	STC_CoordinateFree (Coordinate);
	STC_CoordinateFree (SmallestCoordinate);
	STC_CoordinateFree (LargestCoordinate);
	STC_CellSetShallowFree (MarginalCellSet);

#ifdef _DEBUG
{
	int i;
	for (i = 0; i < CellSet->NumberEntries; i++) {
		assert (CellSet->Cell[i]->TotalNumberObservations != 0);
	}
}
#endif

	return rc;
}

/*------------------------------------------------------------------------------
Calculate the value of the new "FavCost" variable in each internal cell
(based on "CalculateMarginalCellsSensitivityForAllCells()"
 in "c:\tfs_global\SASTC\GeneralizedSystems\GConfid\GConfidAPI\GConfidAPI_1.01.002\STC_Build.c"
).

SmallestCoordinate, LargestCoordinate, and MarginalCellSet
are working variables, but for performance purposes, they are allocated once.
For big problems, they could be allocated and freed millions of times.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE CalculateFavCostForInternalCells (
	STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule,
	int iDimension,
	STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate,
	STCT_CELLSET * MarginalCellSet)
{
	int i;
	int number_of_nonleaf_tags;
	int aggregate_dimension;
	EIT_RETURNCODE rc;

	if (iDimension == HTreeRoot->NumberEntries) {
		if (!IsCoordinateInternal (HTreeRoot, Coordinate)) {
			/*-the value returned by "IsCoordinateInternal()" is true,   */
			/* therefore at least one of the tags in "Coordinate" is a   */
			/* non-leaf tag; "CalculateFavCostForMarginalCell()" should  */
			/* only be called if exactly ONE of the tags in "Coordinate" */
			/* is a non-leaf tag:                                        */
			aggregate_dimension = -1;
			number_of_nonleaf_tags = 0;
			for (i = 0; i < Coordinate->NumberEntries; i++) {
				if (HTreeRoot->TagIndex[i][Coordinate->Tag[i]]->NumberDecompositions > 0) {
					number_of_nonleaf_tags = number_of_nonleaf_tags + 1;
					aggregate_dimension = i;
				}
			}
			if (number_of_nonleaf_tags == 1) {
				rc = CalculateFavCostForMarginalCell (HTreeRoot, CellSet, KdTree, SRule,
					Coordinate, SmallestCoordinate, LargestCoordinate,
					MarginalCellSet, aggregate_dimension);
				if (rc != EIE_SUCCEED) return EIE_FAIL;
			}
		}
	}
	else {
		tIList * Tags;
		IList_New (&Tags);
		if (Tags == NULL) return EIE_FAIL;
		rc = STC_HTreeGetTagsWithData (HTreeRoot->Dimension[iDimension], Tags);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
		for (i = 0; i < Tags->ne; i++) {
			Coordinate->Tag[iDimension] = IList_Entry (Tags, i);
			rc = CalculateFavCostForInternalCells (HTreeRoot, CellSet, KdTree, SRule,
				iDimension+1, Coordinate, SmallestCoordinate, LargestCoordinate,
				MarginalCellSet);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
		IList_Free (Tags);
	}
	return EIE_SUCCEED;
}

/*------------------------------------------------------------------------------
Calculate the value of the new "FavCost" variable for internal cells that
contribute to the marginal cell identified by "Coordinate"
(based on "CalculateMarginalCell()"
 in "c:\tfs_global\SASTC\GeneralizedSystems\GConfid\GConfidAPI\GConfidAPI_1.01.002\STC_Build.c"
).

SmallestCoordinate, LargestCoordinate, and MarginalCellSet
are working variables, but for performance purposes, they are allocated once.
For big problems, they could be allocated and freed millions of times.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE CalculateFavCostForMarginalCell (
	STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule,
	STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate,
	STCT_CELLSET * MarginalCellSet,
	int aggregate_dimension)
{
	EIT_RETURNCODE rc;

	MarginalCellSet->NumberEntries = 0;//empty the set
	rc = GetInternalCells (HTreeRoot, Coordinate, KdTree, MarginalCellSet, SmallestCoordinate, LargestCoordinate);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	if (MarginalCellSet->NumberEntries == 0) {
		/* the cell has no data... there's nothing to calculate */
		return EIE_SUCCEED;
	}

	rc = STC_UpdateFavCostForInternalCells(MarginalCellSet, HTreeRoot, SRule, Coordinate, aggregate_dimension);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	
	return EIE_SUCCEED;
}

/*---------------------------------------------------------------------------------------------
Update the value of the new variable "FavCost" in each internal cell that contributes to the
marginal cell
(based (very loosely) on "STC_CellSetAsCell()"
 in "c:\tfs_global\SASTC\GeneralizedSystems\GConfid\GConfidAPI\GConfidAPI_1.01.002\STC_Cell.c"
 --code must also be added to "initializeCell()"
 in "c:\tfs_global\SASTC\GeneralizedSystems\GConfid\GConfidAPI\GConfidAPI_1.01.002\STC_Cell.c"
 to initialize the value of "CellSet->Cell[i]->FavCost" to FAVCOST_NOT_CALCULATED
 --this should work because apparently every internal segment
 must contribute to at least one marginal segment (also, I THINK there must be an overall total
 segment in each dimension to which every internal segment in that dimension must contribute,
 but maybe not--that isn't necessary for this to work though))
).
---------------------------------------------------------------------------------------------*/
static EIT_RETURNCODE STC_UpdateFavCostForInternalCells (
	STCT_CELLSET * CellSet,
	STCT_HTREEROOT * HTreeRoot,
	STCT_SRULE * SRule,
	STCT_COORDINATE * Coordinate,
	int aggregate_dimension)
{
	int i;
	int j;
	int k;
	double TotalNoise;
	double TotalValue;
	double TotalProxy;
	double TotalWeightedValue;
	double TotalWeightedProxy;
	/* EIT_RETURNCODE rc; */
	int is_direct_contributor_to_aggregate;
	STCT_HTREE * HTree;
	/* char WhichProxyInternal; */ /*-allowed values: 'v'=Value ,             'V'=WeightedValue                                              */
	/* char WhichValueInternal; */ /*-allowed values:             'p'=Proxy ,                     'P'=WeightedProxy                          */
	char WhichNoise;         /*-allowed values: 'v'=ValueX, 'p'=ProxyX, 'V'=WeightedValueX, 'P'=WeightedProxyX, 'n'=ValueN, 'N'=ProxyN */

	/* if (SRule->Parms->Weight && (SRule->Parms->WeightProtectionLevel != 'E')) {          */
	/*     if (SRule->Parms->Proxy) {                                                       */
	/*         WhichNoiseInternal = 'P';                                                    */
	/*     }                                                                                */
	/*     else {                                                                           */
	/*         WhichNoiseInternal = 'V';                                                    */
	/*     }                                                                                */
	/* }                                                                                    */
	/* else { /~ (!SRule->Parms->Weight || (SRule->Parms->WeightProtectionLevel == 'E')) ~/ */
	/*     if (SRule->Parms->Weight) {                                                      */
	/*         if (SRule->Parms->Proxy) {                                                   */
	/*             WhichNoiseInternal = 'P';                                                */
	/*         }                                                                            */
	/*         else {                                                                       */
	/*             WhichNoiseInternal = 'V';                                                */
	/*         }                                                                            */
	/*     }                                                                                */
	/*     else {                                                                           */
	/*         if (SRule->Parms->Proxy) {                                                   */
	/*             WhichNoiseInternal = 'p';                                                */
	/*         }                                                                            */
	/*         else {                                                                       */
	/*             WhichNoiseInternal = 'v';                                                */
	/*         }                                                                            */
	/*     }                                                                                */
	/* }                                                                                    */

	if (SRule->Parms->Weight && (SRule->Parms->WeightProtectionLevel != 'E')) {
		if (SRule->Parms->Proxy) {
			WhichNoise = 'N';
		}
		else {
			WhichNoise = 'n';
		}
	}
	else { /* (!SRule->Parms->Weight || (SRule->Parms->WeightProtectionLevel == 'E')) */
		if (SRule->Parms->Weight) {
			if (SRule->Parms->Proxy) {
				WhichNoise = 'P';
			}
			else {
				WhichNoise = 'V';
			}
		}
		else {
			if (SRule->Parms->Proxy) {
				WhichNoise = 'p';
			}
			else {
				WhichNoise = 'v';
			}
		}
	}

	/*-calculate total noise for marginal cell that hasn't been created yet: */
	if (SRule->Parms->AdditiveNoise) {
		TotalNoise = 0.0;
	}
	else { /* (!SRule->Parms->AdditiveNoise) */
		TotalValue = 0.0;
		TotalProxy = 0.0;
		if (SRule->Parms->Weight) {
			TotalWeightedValue = 0.0;
			TotalWeightedProxy = 0.0;
		}
	}
	for (i = 0; i < CellSet->NumberEntries; i++) {
		/* TotalNoise = TotalNoise + CellSet->Cell[i]->TotalValue; */
		if (SRule->Parms->AdditiveNoise) {
			TotalNoise = TotalNoise + SELECT_CE_VAL2(CellSet->Cell[i], WhichNoise);
		}
		else {
			TotalValue = TotalValue + CellSet->Cell[i]->TotalValue;
			TotalProxy = TotalProxy + CellSet->Cell[i]->TotalProxy;
			if (SRule->Parms->Weight) {
				TotalWeightedValue = TotalWeightedValue + CellSet->Cell[i]->TotalWeightedValue;
				TotalWeightedProxy = TotalWeightedProxy + CellSet->Cell[i]->TotalWeightedProxy;
			}
		}
	}
	if (!SRule->Parms->AdditiveNoise) {
		/* TotalNoise = fabs(TotalNoise); */
		if (SRule->Parms->Proxy && (fabs(TotalValue) >= (SRule->Parms->ProxyRatio * fabs(TotalProxy)))) {
			TotalNoise = ((WhichNoise=='v')?                     fabs(TotalValue        )
			             :(WhichNoise=='V')?                     fabs(TotalWeightedValue)
			             :(WhichNoise=='p')?(SRule->Parms->ProxyRatio * fabs(TotalProxy        ))
			             :(WhichNoise=='P')?(SRule->Parms->ProxyRatio * fabs(TotalWeightedProxy))
			             :(WhichNoise=='n')?                     fabs(TotalWeightedValue)
			             :(WhichNoise=='N')?                     fabs(TotalWeightedProxy)
			             :report_error_in_conditional_expression_returning_double("Invalid value '%c' for WhichNoise in conditional expression in call to STC_UpdateFavCostForInternalCells()", WhichNoise)
			             );
			}
			else { /* ((!SRule->Parms->Proxy) || (fabs(TotalValue) < (SRule->Parms->ProxyRatio * fabs(TotalProxy)))) (*/
			TotalNoise = ((WhichNoise=='v')?                     fabs(TotalValue        )
			             :(WhichNoise=='V')?                     fabs(TotalWeightedValue)
			             :(WhichNoise=='p')?                     fabs(TotalValue        )
			             :(WhichNoise=='P')?                     fabs(TotalWeightedValue)
			             :(WhichNoise=='n')?                     fabs(TotalWeightedValue)
			             :(WhichNoise=='N')?                     fabs(TotalWeightedValue)
			             :report_error_in_conditional_expression_returning_double("Invalid value '%c' for WhichNoise in conditional expression in call to STC_UpdateFavCostForInternalCells()", WhichNoise)
			             );
			}
	}
	HTree = (HTreeRoot->TagIndex)[aggregate_dimension][Coordinate->Tag[aggregate_dimension]];
	for (i = 0; i < CellSet->NumberEntries; i++) {
		is_direct_contributor_to_aggregate = 0;
		for (j = 0; j < HTree->NumberDecompositions; j++) {
			for (k = 0; k < HTree->Decomposition[j]->NumberEntries; k++) {
				if (CellSet->Cell[i]->Coordinate->Tag[aggregate_dimension] == HTree->Decomposition[j]->Branch[k]->Tag) {
					is_direct_contributor_to_aggregate = 1;
				}
			}
		}
		if (is_direct_contributor_to_aggregate) {
			if ((CellSet->Cell[i]->Sensitivity_nowaivers > 0.0) && (CellSet->Cell[i]->Sensitivity <= 0.0)) {
				/*-ignoring waivers this cell is sensitive (Sensitivity_nowaivers > 0.0), but */
				/* taking them into account it isn't (Sensitivity <= 0.0); therefore, change  */
				/* the value of FavCost for this cell to the maximum of its current value     */
				/* and the sum of the contributions to all of the cells entering into the     */
				/* constraint:                                                                */
				if (CellSet->Cell[i]->FavCost == FAVCOST_NOT_CALCULATED) {
					CellSet->Cell[i]->FavCost = TotalNoise;
				}
				else {
					if (CellSet->Cell[i]->FavCost < TotalNoise) {
						CellSet->Cell[i]->FavCost = TotalNoise;
					}
				}
			}
			else {
				/*-it is NOT the case that ignoring waivers this cell is sensitive (Sensitivity_nowaivers > 0.0), and */
				/* taking them into account it isn't (Sensitivity <= 0.0); therefore, set the value of FavCost for    */
				/* this cell to just the total contributions to this cell:                                            */
				if (CellSet->Cell[i]->FavCost == FAVCOST_NOT_CALCULATED) {
					/* CellSet->Cell[i]->FavCost = CellSet->Cell[i]->TotalValue; */
					CellSet->Cell[i]->FavCost = SELECT_CE_VAL2(CellSet->Cell[i], WhichNoise);
				}
			}
		}
	}
	return EIE_SUCCEED;
}

/*------------------------------------------------------------------------------
Calculate sensitivity of marginal cells
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_CalculateMarginalCellsSensitivity (
	STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule,
	int * NumberMarginalCellsCalculated,
	int * NumberMarginalCellsSensitive,
	int * NumberMarginalCellsZero)
{
	STCT_COORDINATE * Coordinate;
	STCT_COORDINATE * LargestCoordinate;
	STCT_CELLSET * MarginalCellSet;
	EIT_RETURNCODE rc;
	STCT_COORDINATE * SmallestCoordinate;
	STCT_CELLSET * SubMarginalCellSet;
	STCT_COORDINATE * WorkingCoordinate;

	Coordinate = STC_CoordinateAllocate (HTreeRoot->NumberEntries);
	if (Coordinate == NULL) return EIE_FAIL;
	SmallestCoordinate = STC_CoordinateAllocate (HTreeRoot->NumberEntries);
	if (SmallestCoordinate == NULL) return EIE_FAIL;
	LargestCoordinate = STC_CoordinateAllocate (HTreeRoot->NumberEntries);
	if (LargestCoordinate == NULL) return EIE_FAIL;
	WorkingCoordinate = STC_CoordinateAllocate (HTreeRoot->NumberEntries);
	if (WorkingCoordinate == NULL) return EIE_FAIL;
	MarginalCellSet = STC_CellSetAllocate (1000);
	if (LargestCoordinate == NULL) return EIE_FAIL;
	SubMarginalCellSet = STC_CellSetAllocate (1000);
	if (LargestCoordinate == NULL) return EIE_FAIL;

	rc = CalculateMarginalCellsSensitivityForAllCells (HTreeRoot, CellSet, KdTree, SRule,
		0, Coordinate, SmallestCoordinate, LargestCoordinate, WorkingCoordinate,
		MarginalCellSet, SubMarginalCellSet,
		NumberMarginalCellsCalculated, NumberMarginalCellsSensitive, NumberMarginalCellsZero);

	STC_CoordinateFree (Coordinate);
	STC_CoordinateFree (SmallestCoordinate);
	STC_CoordinateFree (LargestCoordinate);
	STC_CoordinateFree (WorkingCoordinate);
	STC_CellSetShallowFree (MarginalCellSet);
	STC_CellSetShallowFree (SubMarginalCellSet);

#ifdef _DEBUG
{
	int i;
	for (i = 0; i < CellSet->NumberEntries; i++) {
		assert (CellSet->Cell[i]->TotalNumberObservations != 0);
	}
}
#endif

	return rc;
}
/*------------------------------------------------------------------------------
add a code in range for every internal code in hierarchy without a range
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_AugmentRange (
	STCT_RANGEROOT * RangeRoot,
	STCT_HTREEROOT * HTreeRoot)
{
	int i;
	EIT_RETURNCODE rc;

	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		rc = AugmentRange (HTreeRoot->Dimension[i], RangeRoot->Dimension[i]);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Calculate sensitivity of internal cells
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_CalculateInternalCellsSensitivity (
	STCT_CELLSET * CellSet,
	STCT_SRULE * SRule,
	int * NumberInternalCellsCalculated,
	int * NumberInternalCellsSensitive,
	int * NumberInternalCellsZero)
{
	int i;
	EIT_RETURNCODE rc;

	int waiver_flags_present;
	rc = EIE_SUCCEED;

	waiver_flags_present = SRule->waiver_flags_present;

	*NumberInternalCellsCalculated = 0;
	*NumberInternalCellsSensitive = 0;
	*NumberInternalCellsZero = 0;

	for (i = 0; i < CellSet->NumberEntries; i++) {
		if (CellSet->Cell[i]->IsInternal) {
			rc = STC_CellSensitivity (CellSet->Cell[i], SRule, NULL);
			if (rc != EIE_SUCCEED) {
			    return EIE_FAIL;
			}
			if (CellSet->Cell[i]->TotalValue == 0.0) {
				(*NumberInternalCellsZero)++;
			}
			else {
				(*NumberInternalCellsCalculated)++;
				if (CellSet->Cell[i]->Sensitivity > 0.0)
					(*NumberInternalCellsSensitive)++;
			}
			if (waiver_flags_present == 0) {
			CellSet->Cell[i]->Type = STC_CELLTYPE_CELL;
			rc = STC_WriteCell (CellSet->Cell[i], CellSet->Cell[i]->Coordinate->NumberEntries);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
			rc = STC_WriteLargest (CellSet->Cell[i], SRule->waiver_flags_present, SRule->Parms);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
			if ((SRule->Parms->Weight != 0 && (SRule->Parms->WeightProtectionLevel != 'E'))) {
				if (SRule->Type == STCE_SRULE_TYPE_NK) {
					rc = STC_WriteTargets(CellSet->Cell[i], (SRule->waiver_flags_present == 0)?(CellSet->Cell[i]->LargestFTNumberEntries):(CellSet->Cell[i]->LargestFWNumberEntries), (SRule->waiver_flags_present == 0)?(CellSet->Cell[i]->LargestFT):(CellSet->Cell[i]->LargestFW), CellSet->Cell[i]->LargestFSNumberEntries, CellSet->Cell[i]->LargestFS, SRule->NumberEntries[0], 1, SRule->waiver_flags_present);
				}
				else if (SRule->Type == STCE_SRULE_TYPE_PQ) {
					rc = STC_WritePairs  (CellSet->Cell[i], (SRule->waiver_flags_present == 0)?(CellSet->Cell[i]->LargestFTNumberEntries):(CellSet->Cell[i]->LargestFWNumberEntries), (SRule->waiver_flags_present == 0)?(CellSet->Cell[i]->LargestFT):(CellSet->Cell[i]->LargestFW), CellSet->Cell[i]->LargestFSNumberEntries, CellSet->Cell[i]->LargestFS,                          1, SRule->waiver_flags_present);
				}

				if (SRule->Type != STCE_SRULE_TYPE_NK && SRule->Type != STCE_SRULE_TYPE_PQ && SRule->Type != STCE_SRULE_TYPE_C2 && SRule->Type != STCE_SRULE_TYPE_DUFFETT) {
					IO_PRINT_LINE(M30202);
					rc = EIE_FAIL;
				}
				if (rc != EIE_SUCCEED) return EIE_FAIL;
			}
			}
		}
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Write internal cells
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_WriteInternalCells (
	STCT_CELLSET * CellSet,
	STCT_SRULE * SRule,
	int waiver_flags_present)
{
	int i;
	EIT_RETURNCODE rc;
	for (i = 0; i < CellSet->NumberEntries; i++) {
		if (CellSet->Cell[i]->IsInternal) {
			CellSet->Cell[i]->Type = STC_CELLTYPE_CELL;
			rc = STC_WriteCell (CellSet->Cell[i], CellSet->Cell[i]->Coordinate->NumberEntries);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
			rc = STC_WriteLargest (CellSet->Cell[i], waiver_flags_present, SRule->Parms);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
			if ((SRule->Parms->Weight != 0 && (SRule->Parms->WeightProtectionLevel != 'E'))) {
				if (SRule->Type == STCE_SRULE_TYPE_NK) {
					rc = STC_WriteTargets(CellSet->Cell[i], (SRule->waiver_flags_present == 0)?(CellSet->Cell[i]->LargestFTNumberEntries):(CellSet->Cell[i]->LargestFWNumberEntries), (SRule->waiver_flags_present == 0)?(CellSet->Cell[i]->LargestFT):(CellSet->Cell[i]->LargestFW), CellSet->Cell[i]->LargestFSNumberEntries, CellSet->Cell[i]->LargestFS, SRule->NumberEntries[0], 1, SRule->waiver_flags_present);
				}
				else if (SRule->Type == STCE_SRULE_TYPE_PQ) {
					rc = STC_WritePairs  (CellSet->Cell[i], (SRule->waiver_flags_present == 0)?(CellSet->Cell[i]->LargestFTNumberEntries):(CellSet->Cell[i]->LargestFWNumberEntries), (SRule->waiver_flags_present == 0)?(CellSet->Cell[i]->LargestFT):(CellSet->Cell[i]->LargestFW), CellSet->Cell[i]->LargestFSNumberEntries, CellSet->Cell[i]->LargestFS,                          1, SRule->waiver_flags_present);
				}

				if (SRule->Type != STCE_SRULE_TYPE_NK && SRule->Type != STCE_SRULE_TYPE_PQ && SRule->Type != STCE_SRULE_TYPE_C2 && SRule->Type != STCE_SRULE_TYPE_DUFFETT) {
					IO_PRINT_LINE(M30202);
					rc = EIE_FAIL;
				}
				if (rc != EIE_SUCCEED) return EIE_FAIL;
			}
		}
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Calculate sensitivity of user grouping
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_CalculateUserGroupsSensitivity (
	STCT_GROUPROOT * GroupRoot,
	STCT_HTREEROOT * HTreeRoot,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule,
	int * NumberUserGroupsCalculated,
	int * NumberUserGroupsSensitive)
{
	int i, j;
	STCT_CELL * Cell;
	STCT_GROUP * Group;
	STCT_CELLSET * GroupCellSet;
	STCT_CELL * GroupCellSetAsCell;
	STCT_COORDINATE * LargestCoordinate;
	int n;
	EIT_RETURNCODE rc;
	STCT_COORDINATE * SmallestCoordinate;
	STCT_CELL * SubGroupCell;
	STCT_CELLSET * SubGroupCellSet;

	*NumberUserGroupsCalculated = 0;
	*NumberUserGroupsSensitive = 0;

	if (GroupRoot == NULL) return EIE_SUCCEED;

	SmallestCoordinate = STC_CoordinateAllocate (HTreeRoot->NumberEntries);
	if (SmallestCoordinate == NULL) return EIE_FAIL;
	LargestCoordinate = STC_CoordinateAllocate (HTreeRoot->NumberEntries);
	if (LargestCoordinate == NULL) return EIE_FAIL;

	GroupCellSet = STC_CellSetAllocate (1000);
	if (GroupCellSet == NULL) return EIE_FAIL;
	SubGroupCellSet = STC_CellSetAllocate (1000);
	if (SubGroupCellSet == NULL) return EIE_FAIL;


	for (i = 0; i < GroupRoot->NumberEntries; i++) {

		//empty the set
		GroupCellSet->NumberEntries = 0;

		Group = GroupRoot->Group[i];

		//STC_GroupPrint (Group, HTreeRoot);

		(*NumberUserGroupsCalculated)++;

		for (j = 0; j < Group->NumberEntries; j++) {
			Cell = STC_KdTreeSearch (KdTree, Group->Coordinate[j]);
			if (Cell != NULL) {
				rc = STC_CellSetAddLast (GroupCellSet, Cell);
				if (rc != EIE_SUCCEED) return EIE_FAIL;
			}
		}

		//STC_CellSetPrintInfo (GroupCellSet);

		if (GroupCellSet->NumberEntries == 0) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_WARNING, M20036, i+1);
		}
		else if (GroupCellSet->NumberEntries == 1) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_WARNING, M20036, i+1);
		}
		else if (GroupCellSet->NumberEntries > 1) {
			/* count number of cells where total > 0.0 */
			n = 0;
			for (j = 0; j < GroupCellSet->NumberEntries; j++) {
				if (GroupCellSet->Cell[j]->TotalValue > 0.0)
					n++;
			}
			if (n < 2) {
				EI_AddMessage ("", EIE_MESSAGESEVERITY_WARNING, M20036, i+1);
			}
			else {
				for (j = 0; j < GroupCellSet->NumberEntries; j++) {
					if (!GroupCellSet->Cell[j]->IsInternal) {
						SubGroupCellSet->NumberEntries = 0;
						if (SubGroupCellSet == NULL) return EIE_FAIL;
						rc = GetInternalCells (HTreeRoot, GroupCellSet->Cell[j]->Coordinate, KdTree,
							SubGroupCellSet, SmallestCoordinate, LargestCoordinate);
						if (rc != EIE_SUCCEED) return EIE_FAIL;
						SubGroupCell = STC_CellSetSensitivity (SubGroupCellSet, SRule, NULL);
						GroupCellSet->Cell[j]->Data = SubGroupCell->Data;
						SetLargestRespondents (GroupCellSet->Cell[j], SubGroupCell);
						SubGroupCell->Data.Item = NULL;
						SubGroupCell->Data.NumberAllocated = 0;
						SubGroupCell->Data.NumberEntries = 0;
						STC_CellFree (SubGroupCell);
					}
				}
				//STC_CellSetPrint (GroupCellSet);
				GroupCellSetAsCell = STC_CellSetSensitivity (GroupCellSet, SRule, NULL);
				if (GroupCellSetAsCell->Sensitivity <= 0.0) {
				}
				else {
					(*NumberUserGroupsSensitive)++;
					rc = STC_WriteConstraint (GroupCellSet, GroupCellSetAsCell);
					if (rc != EIE_SUCCEED) return EIE_FAIL;
					GroupCellSetAsCell->Type = STC_CELLTYPE_GROUP;
					rc = STC_WriteCell (GroupCellSetAsCell, HTreeRoot->NumberEntries);
					if (rc != EIE_SUCCEED) return EIE_FAIL;
					rc = STC_WriteLargest (GroupCellSetAsCell, SRule->waiver_flags_present, SRule->Parms);
					if (rc != EIE_SUCCEED) return EIE_FAIL;
					if ((SRule->Parms->Weight != 0 && (SRule->Parms->WeightProtectionLevel != 'E'))) {
						if (SRule->Type == STCE_SRULE_TYPE_NK) {
							rc = STC_WriteTargets(GroupCellSetAsCell, (SRule->waiver_flags_present == 0)?(GroupCellSetAsCell->LargestFTNumberEntries):(GroupCellSetAsCell->LargestFWNumberEntries), (SRule->waiver_flags_present == 0)?(GroupCellSetAsCell->LargestFT):(GroupCellSetAsCell->LargestFW), GroupCellSetAsCell->LargestFSNumberEntries, GroupCellSetAsCell->LargestFS, SRule->NumberEntries[0], 1, SRule->waiver_flags_present);
						}
						else if (SRule->Type == STCE_SRULE_TYPE_PQ) {
							rc = STC_WritePairs  (GroupCellSetAsCell, (SRule->waiver_flags_present == 0)?(GroupCellSetAsCell->LargestFTNumberEntries):(GroupCellSetAsCell->LargestFWNumberEntries), (SRule->waiver_flags_present == 0)?(GroupCellSetAsCell->LargestFT):(GroupCellSetAsCell->LargestFW), GroupCellSetAsCell->LargestFSNumberEntries, GroupCellSetAsCell->LargestFS,                          1, SRule->waiver_flags_present);
						}

						if (SRule->Type != STCE_SRULE_TYPE_NK && SRule->Type != STCE_SRULE_TYPE_PQ && SRule->Type != STCE_SRULE_TYPE_C2 && SRule->Type != STCE_SRULE_TYPE_DUFFETT) {
							IO_PRINT_LINE(M30202);
							rc = EIE_FAIL;
						}
						if (rc != EIE_SUCCEED) return EIE_FAIL;
					}

				}
				//STC_CellPrintInfo (GroupCellSetAsCell, 1);
				STC_CellFree (GroupCellSetAsCell);
				for (j = 0; j < GroupCellSet->NumberEntries; j++) {
					if (!GroupCellSet->Cell[j]->IsInternal) {
						STC_CellFreeItems (GroupCellSet->Cell[j]);
					}
				}
			}
		}
	}

	STC_CoordinateFree (SmallestCoordinate);
	STC_CoordinateFree (LargestCoordinate);
	STC_CellSetShallowFree (GroupCellSet);
	STC_CellSetShallowFree (SubGroupCellSet);

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Find the sensitive aggregates
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_FindSensitiveAggregates (
	STCT_HTREEROOT * HTreeRoot,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule,
	int M,
	double X,
	double Y,
	double Z,
	int Verbose,
	int * NumberCombinaisonsCalculated,
	int * NumberCombinaisonsSensitive)
{
	STCT_COORDINATE * Coordinate;
	int i;
	STCT_COORDINATE * LargestCoordinate;
	STCT_CELLSET * MarginalCellSet;
	int N;
	int NN;
	tIList ** NonInternalTags;
	EIT_RETURNCODE rc;
	STCT_COORDINATE * SmallestCoordinate;
	STCT_CELLSET * SubMarginalCellSet;
	tIList ** Tags;

	Tags = STC_AllocateMemory (HTreeRoot->NumberEntries * sizeof *Tags);
	if (Tags == NULL) return EIE_FAIL;
	NonInternalTags = STC_AllocateMemory (HTreeRoot->NumberEntries * sizeof *NonInternalTags);
	if (NonInternalTags == NULL) return EIE_FAIL;

	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		IList_New (&Tags[i]);
		if (Tags[i] == NULL) return EIE_FAIL;
		rc = STC_HTreeGetTagsWithData (HTreeRoot->Dimension[i], Tags[i]);
		if (rc != EIE_SUCCEED) return EIE_FAIL;

		IList_New (&NonInternalTags[i]);
		if (NonInternalTags[i] == NULL) return EIE_FAIL;
		rc = STC_HTreeGetNonInternalTagsWithData (HTreeRoot->Dimension[i], NonInternalTags[i]);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}

	NN = GetNN (SRule);
	N = GetN (SRule, NN);


	Coordinate = STC_CoordinateAllocate (HTreeRoot->NumberEntries);
	if (Coordinate == NULL) return EIE_FAIL;
	SmallestCoordinate = STC_CoordinateAllocate (HTreeRoot->NumberEntries);
	if (Coordinate == NULL) return EIE_FAIL;
	LargestCoordinate = STC_CoordinateAllocate (HTreeRoot->NumberEntries);
	if (Coordinate == NULL) return EIE_FAIL;
	MarginalCellSet = STC_CellSetAllocate (1000);
	if (MarginalCellSet == NULL) return EIE_FAIL;
	SubMarginalCellSet = STC_CellSetAllocate (1000);
	if (SubMarginalCellSet == NULL) return EIE_FAIL;

#ifdef PRINT_NUMBER_EQ
	mNumberEq = 0;
#endif
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
#ifdef PRINT_NUMBER_EQ
		mSubNumberEq = 0;
#endif
		rc = FindSensitiveAggregatesForAllMarginalCells (
			HTreeRoot, KdTree, SRule, Tags, NonInternalTags,
			M, X, Y, Z, N, NN, Verbose,
			0, i, Coordinate, SmallestCoordinate, LargestCoordinate,
			MarginalCellSet, SubMarginalCellSet,
			NumberCombinaisonsCalculated, NumberCombinaisonsSensitive);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}

	STC_CoordinateFree (Coordinate);
	STC_CoordinateFree (SmallestCoordinate);
	STC_CoordinateFree (LargestCoordinate);
	STC_CellSetShallowFree (MarginalCellSet);
	STC_CellSetShallowFree (SubMarginalCellSet);

#ifdef PRINT_NUMBER_EQ
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Number of Eq=%4d\n", mNumberEq);
#endif

	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		IList_Free (Tags[i]);
		IList_Free (NonInternalTags[i]);
	}
	STC_FreeMemory (Tags);
	STC_FreeMemory (NonInternalTags);

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Find the end codes of a tree.

when no range is specified: the end codes of a tree are the internal codes.
when a range is specified: the end codes of a tree are
all the range codes specified for all the internal codes
+
all the internal codes for which no ranges are specified
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_GetEndCodes (
	STCT_HTREE * HTree,
	STCT_RANGE * Range,
	int Decomposition,
	STCT_RANGE * Range2)
{
	int i;
	EIT_RETURNCODE rc;

	if (HTree->NumberDecompositions == 0) {
		//Internal code
		rc = STC_RangeSearchFromCodesToRanges (Range, HTree->Code, Range2);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}
	else {
		//Not an internal code, go deeper
		for (i = 0; i < HTree->Decomposition[Decomposition]->NumberEntries; i++) {
			rc = STC_GetEndCodes (HTree->Decomposition[Decomposition]->Branch[i], Range,
				0, // it's really 0 I want here, and not Decomposition.
				Range2);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
remove from range all codes not in hierarchy
------------------------------------------------------------------------------*/
void STC_PruneRange (
	STCT_RANGEROOT * RangeRoot,
	STCT_HTREEROOT * HTreeRoot)
{
	int d, i, j;
	STCT_HTREE * HTree;
	STCT_RANGE * Range;
    tSList * L;

	SList_New (&L);
	for (i = 0; i < RangeRoot->NumberEntries; i++) {
		Range = RangeRoot->Dimension[i];
		for (j = 0, d = 0; j < Range->NumberEntries; j++) {
			HTree = STC_HTreeSearchCode (HTreeRoot->Dimension[i], Range->Item[j]->Code);
			if (HTree == NULL || HTree->NumberDecompositions > 0) {
				if (SList_StringSearchBinary (L, RangeRoot->Dimension[i]->Item[j]->Code) == SLIST_NOTFOUND) {
					SList_Add (RangeRoot->Dimension[i]->Item[j]->Code, L);
					SList_Sort (L, eSListSortAscending);
					if (HTree == NULL) {
						EI_AddMessage ("", EIE_MESSAGESEVERITY_INFORMATION, M20034, HTreeRoot->Name[i], Range->Item[j]->Code);
					}
					//else if (HTree->NumberDecompositions > 0) {
					//	EI_AddMessage ("", EIE_MESSAGESEVERITY_INFORMATION, M20035, HTreeRoot->Name[i], Range->Item[j]->Code);
					//}
				}
				STC_FreeMemory (Range->Item[j]->Code);
				STC_FreeMemory (Range->Item[j]->Data);
				STC_FreeMemory (Range->Item[j]);
				Range->Item[j] = NULL;
			}
			else {
				Range->Item[d++] = Range->Item[j];
			}
		}
		Range->NumberEntries = d;
		SList_Empty (L);
	}
	SList_Free (L);
}
/*------------------------------------------------------------------------------
validate the hierarchy-range combinaison
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_ValidateCodes (
	STCT_HTREEROOT * HTreeRoot,
	STCT_RANGEROOT * RangeRoot)
{
	int i, j;
	STCT_HTREE * HTree;
	EIT_RETURNCODE rc;
	tSList * L; /* list of range code not in hierarchy */

	if (HTreeRoot->NumberEntries != RangeRoot->NumberEntries) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_ERROR, M30106, //Hierarchy and Range do not have the same number of dimensions
			HTreeRoot->NumberEntries, RangeRoot->NumberEntries);
		return EIE_FAIL;
	}

	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "CheckDecompositionsEquivalence\n");
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Range %d avant CheckDecompositionsEquivalence\n", i);
		//STC_RangePrint (RangeRoot->Range[i]);
		rc = CheckDecompositionsEquivalence (HTreeRoot->Dimension[i], RangeRoot->Dimension[i]);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Range %d apres CheckDecompositionsEquivalence\n", i);
		//STC_RangePrint (RangeRoot->Range[i]);
	}

	SList_New (&L);
	if (L == NULL) return EIE_FAIL;
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		for (j = 0; j < RangeRoot->Dimension[i]->NumberEntries; j++) {
			//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "range i %d item j %d code %s\n", i+1, j+1, RangeRoot->Dimension[i]->Item[j]->Code);
			HTree = STC_HTreeSearchCode (HTreeRoot->Dimension[i], RangeRoot->Dimension[i]->Item[j]->Code);
			if (HTree == NULL) {
				if (SList_StringSearchBinary (L, RangeRoot->Dimension[i]->Item[j]->Code) == SLIST_NOTFOUND) {
					if (SList_Add (RangeRoot->Dimension[i]->Item[j]->Code, L) == eSListFail)
						return EIE_FAIL;
					SList_Sort (L, eSListSortAscending);
					EI_AddMessage ("", EIE_MESSAGESEVERITY_WARNING, M20034, //Range specifies a code which is not in the hierarchy
						RangeRoot->Name[i], RangeRoot->Dimension[i]->Item[j]->Code);
				}
			}
		}
		SList_Empty (L);
	}
	SList_Free (L);

	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "CheckBranchesOverlapping\n");
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Range %d avant CheckBranchesOverlapping\n", i);
		//STC_RangePrint (RangeRoot->Range[i]);
		rc = CheckBranchesOverlapping (HTreeRoot->Dimension[i], RangeRoot->Dimension[i]);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}

	return EIE_SUCCEED;
}


/*------------------------------------------------------------------------------
add a code in range for every internal code in hierarchy without a range
------------------------------------------------------------------------------*/
static EIT_RETURNCODE AugmentRange (
	STCT_HTREE * HTree,
	STCT_RANGE * Range)
{
	EIT_BOOLEAN Found;
	int i, j;
	EIT_RETURNCODE rc;

	if (HTree->NumberDecompositions == 0) {
		//internal code
		Found = EIE_FALSE;
		for (i = 0; i < Range->NumberEntries && !Found; i++) {
			if (strcmp (Range->Item[i]->Code, HTree->Code) == 0) {
				Found = EIE_TRUE;
			}
		}
		if (!Found) {
			rc = STC_RangeAddItem (Range, HTree->Code);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
	}

	//go deeper
	for (i = 0; i < HTree->NumberDecompositions; i++) {
		for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
			rc = AugmentRange (HTree->Decomposition[i]->Branch[j], Range);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Calculate sensitivity of marginal cell identified by Coordinate.

SmallestCoordinate, LargestCoordinate, WorkingCoordinate, MarginalCellSet and SubMarginalCellSet
are working variables, but for performance purposes, they are allocated once.
For big problems, they could be allocated and freed millions of times.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE CalculateMarginalCell (
	STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule,
	STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate,
	STCT_COORDINATE * WorkingCoordinate,
	STCT_CELLSET * MarginalCellSet,
	STCT_CELLSET * SubMarginalCellSet,
	int * NumberMarginalCellsCalculated,
	int * NumberMarginalCellsSensitive,
	int * NumberMarginalCellsZero)
{
	STCT_CELL * Cell;
	int i, j, k;
	int IsInCellSet;
	int IsInternal;
	STCT_KDTREE * kdtree;
	STCT_CELL * MarginalCell;
	STCT_CELLSET * MarginalCell_CellSet;
	STCT_HTREE * Node;
	EIT_RETURNCODE rc;
	STCT_CELL * SubMarginalCell;
	char WhichNoise; /*-allowed values: 'v'=ValueX, 'p'=ProxyX, 'V'=WeightedValueX, 'P'=WeightedProxyX, 'n'=ValueN, 'N'=ProxyN */

	if (SRule->Parms->Weight && (SRule->Parms->WeightProtectionLevel != 'E')) {
		if (SRule->Parms->Proxy) {
			WhichNoise = 'N';
		}
		else {
			WhichNoise = 'n';
		}
	}
	else { /* (!SRule->Parms->Weight || (SRule->Parms->WeightProtectionLevel == 'E')) */
		if (SRule->Parms->Weight) {
			if (SRule->Parms->Proxy) {
				WhichNoise = 'P';
			}
			else {
				WhichNoise = 'V';
			}
		}
		else {
			if (SRule->Parms->Proxy) {
				WhichNoise = 'p';
			}
			else {
				WhichNoise = 'v';
			}
		}
	}

	MarginalCell = STC_KdTreeSearch (KdTree, Coordinate);
	if (MarginalCell == NULL) {
		/* Cell at Coordinate is not in CellSet*/
		MarginalCellSet->NumberEntries = 0;//empty the set
		rc = GetInternalCells (HTreeRoot, Coordinate, KdTree, MarginalCellSet, SmallestCoordinate, LargestCoordinate);
		if (rc != EIE_SUCCEED) return EIE_FAIL;

		if (MarginalCellSet->NumberEntries == 0) {
			/* the cell has no data... there's nothing to calculate */
			return EIE_SUCCEED;
		}

		MarginalCell = STC_CellSetSensitivity (MarginalCellSet, SRule, Coordinate);
		if (MarginalCell == NULL) return EIE_FAIL;
		MarginalCell->Coordinate = STC_CoordinateDuplicate (Coordinate);
		MarginalCell->Type = STC_CELLTYPE_CELL;
		/*-this is a marginal cell and it is being added here (as the cell on the left-hand-side of a constraint) instead */
		/* of below (as one of the cells on the right-hand-side of a constraint), so the value of its FavCost member must */
		/* be assigned here:                                                                                              */
		/* MarginalCell->FavCost = MarginalCell->TotalValue; */
		MarginalCell->FavCost = SELECT_CE_VAL2(MarginalCell, WhichNoise);
		rc = STC_WriteCell (MarginalCell, HTreeRoot->NumberEntries);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
		rc = STC_WriteLargest (MarginalCell, SRule->waiver_flags_present, SRule->Parms);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
		if ((SRule->Parms->Weight != 0 && (SRule->Parms->WeightProtectionLevel != 'E'))) {
			if (SRule->Type == STCE_SRULE_TYPE_NK) {
				rc = STC_WriteTargets(MarginalCell, (SRule->waiver_flags_present == 0)?(MarginalCell->LargestFTNumberEntries):(MarginalCell->LargestFWNumberEntries), (SRule->waiver_flags_present == 0)?(MarginalCell->LargestFT):(MarginalCell->LargestFW), MarginalCell->LargestFSNumberEntries, MarginalCell->LargestFS, SRule->NumberEntries[0], 1, SRule->waiver_flags_present);
			}
			else if (SRule->Type == STCE_SRULE_TYPE_PQ) {
				rc = STC_WritePairs  (MarginalCell, (SRule->waiver_flags_present == 0)?(MarginalCell->LargestFTNumberEntries):(MarginalCell->LargestFWNumberEntries), (SRule->waiver_flags_present == 0)?(MarginalCell->LargestFT):(MarginalCell->LargestFW), MarginalCell->LargestFSNumberEntries, MarginalCell->LargestFS,                          1, SRule->waiver_flags_present);
			}

			if (SRule->Type != STCE_SRULE_TYPE_NK && SRule->Type != STCE_SRULE_TYPE_PQ && SRule->Type != STCE_SRULE_TYPE_C2 && SRule->Type != STCE_SRULE_TYPE_DUFFETT) {
				IO_PRINT_LINE(M30202);
				rc = EIE_FAIL;
			}
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
		//add to global CellSet and KdTree
		rc = STC_CellSetAddLast (CellSet, MarginalCell);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
		kdtree = STC_KdTreeInsert (KdTree, MarginalCell);
		if (kdtree == NULL) return EIE_FAIL;
	}

#ifdef _DEBUG
	if (DEBUGCMC) STC_CellPrint (MarginalCell);
#endif

	if (MarginalCell->TotalValue == 0.0) {
		(*NumberMarginalCellsZero)++;
	}
	else {
		(*NumberMarginalCellsCalculated)++;
		if (MarginalCell->Sensitivity > 0.0)
			(*NumberMarginalCellsSensitive)++;
	}

	MarginalCell_CellSet = STC_CellSetAllocate (1000);

	/* go through all dimensions of the marginal cell */
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {

		Node = HTreeRoot->TagIndex[i][Coordinate->Tag[i]];
		STC_CoordinateCopy (WorkingCoordinate, Coordinate);

		/* go through all decompositions of dimension i */
		for (j = 0; j < Node->NumberDecompositions; j++) {
#ifdef _DEBUG
			if (DEBUGCMC) {
				STC_CoordinatePrint (Coordinate, HTreeRoot);
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "=");
			}
#endif
			MarginalCell_CellSet->NumberEntries = 0;

			/* go through all branches of the decomposition j */
			for (k = 0; k < Node->Decomposition[j]->NumberEntries; k++) {
				WorkingCoordinate->Tag[i] = Node->Decomposition[j]->Branch[k]->Tag;
#ifdef _DEBUG
				if (DEBUGCMC) {
					EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "composante, ");
					STC_CoordinatePrint (WorkingCoordinate, HTreeRoot);
					EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
					EI_PrintMessages ();
				}
#endif
				Cell = STC_KdTreeSearch (KdTree, WorkingCoordinate);

				if (Cell != NULL) {
					IsInCellSet = EIE_TRUE;
					IsInternal = Cell->IsInternal;
				}
				else {
					IsInCellSet = EIE_FALSE;
					IsInternal = IsCoordinateInternal (HTreeRoot, WorkingCoordinate);
				}
#ifdef _DEBUG
				if (DEBUGCMC) {
					if (IsInternal)
						EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  internal, ");
					else
						EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " !internal, ");
					if (IsInCellSet)
						EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  in CellSet\n");
					else
						EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " !in CellSet\n");
				}
#endif		
				if (IsInternal && IsInCellSet) {
					//add to MarginalCell_CellSet
					rc = STC_CellSetAddLast (MarginalCell_CellSet, Cell);
					if (rc != EIE_SUCCEED) return EIE_FAIL;
				}
				else if (IsInternal && !IsInCellSet) {
					//nothing to do
				}
				else if (!IsInternal && IsInCellSet) {
					SubMarginalCellSet->NumberEntries = 0;
					rc = GetInternalCells (HTreeRoot, WorkingCoordinate, KdTree, SubMarginalCellSet,
						SmallestCoordinate, LargestCoordinate);
					if (rc != EIE_SUCCEED) return EIE_FAIL;

					if (SubMarginalCellSet->NumberEntries > 0) {
						SubMarginalCell = STC_CellSetSensitivity (SubMarginalCellSet, SRule, WorkingCoordinate);

						if (SubMarginalCell->TotalNumberObservations == 0) {
							STC_CellFree (SubMarginalCell);
						}
						else {
							Cell->Data.Item = SubMarginalCell->Data.Item;
							Cell->Data.NumberAllocated = SubMarginalCell->Data.NumberAllocated;
							Cell->Data.NumberEntries = SubMarginalCell->Data.NumberEntries;
							SubMarginalCell->Data.Item = NULL;
							SubMarginalCell->Data.NumberAllocated = 0;
							SubMarginalCell->Data.NumberEntries = 0;
							STC_CellFree (SubMarginalCell);
							//add to MarginalCell_CellSet
							rc = STC_CellSetAddLast (MarginalCell_CellSet, Cell);
							if (rc != EIE_SUCCEED) return EIE_FAIL;
						}
					}
					SubMarginalCellSet->NumberEntries = 0;
				}
				else {//(!IsInternal && !IsInCellSet)
					SubMarginalCellSet->NumberEntries = 0;
					rc = GetInternalCells (HTreeRoot, WorkingCoordinate, KdTree, SubMarginalCellSet,
						SmallestCoordinate, LargestCoordinate);
					if (rc != EIE_SUCCEED) return EIE_FAIL;

					if (SubMarginalCellSet->NumberEntries > 0) {
						SubMarginalCell = STC_CellSetSensitivity (SubMarginalCellSet, SRule, WorkingCoordinate);

						if (SubMarginalCell->TotalNumberObservations == 0) {
							STC_CellFree (SubMarginalCell);
						}
						else {
							/*-this is a marginal cell and it is being added here (as one of the cells on the right-hand-side of a constraint) */
							/* instead of above (as the cell on the left-hand-side of a constraint), so the value of its FavCost member must   */
							/* be assigned here:                                                                                               */
							SubMarginalCell->FavCost = SELECT_CE_VAL2(SubMarginalCell, WhichNoise);
							SubMarginalCell->Coordinate = STC_CoordinateDuplicate (WorkingCoordinate);
							SubMarginalCell->Type = STC_CELLTYPE_CELL;
							rc = STC_WriteCell (SubMarginalCell, HTreeRoot->NumberEntries);
							if (rc != EIE_SUCCEED) return EIE_FAIL;
							rc = STC_WriteLargest (SubMarginalCell, SRule->waiver_flags_present, SRule->Parms);
							if (rc != EIE_SUCCEED) return EIE_FAIL;
							if ((SRule->Parms->Weight != 0 && (SRule->Parms->WeightProtectionLevel != 'E'))) {
								if (SRule->Type == STCE_SRULE_TYPE_NK) {
									rc = STC_WriteTargets(SubMarginalCell, (SRule->waiver_flags_present == 0)?(SubMarginalCell->LargestFTNumberEntries):(SubMarginalCell->LargestFWNumberEntries), (SRule->waiver_flags_present == 0)?(SubMarginalCell->LargestFT):(SubMarginalCell->LargestFW), SubMarginalCell->LargestFSNumberEntries, SubMarginalCell->LargestFS, SRule->NumberEntries[0], 1, SRule->waiver_flags_present);
								}
								else if (SRule->Type == STCE_SRULE_TYPE_PQ) {
									rc = STC_WritePairs  (SubMarginalCell, (SRule->waiver_flags_present == 0)?(SubMarginalCell->LargestFTNumberEntries):(SubMarginalCell->LargestFWNumberEntries), (SRule->waiver_flags_present == 0)?(SubMarginalCell->LargestFT):(SubMarginalCell->LargestFW), SubMarginalCell->LargestFSNumberEntries, SubMarginalCell->LargestFS,                          1, SRule->waiver_flags_present);
								}

								if (SRule->Type != STCE_SRULE_TYPE_NK && SRule->Type != STCE_SRULE_TYPE_PQ && SRule->Type != STCE_SRULE_TYPE_C2 && SRule->Type != STCE_SRULE_TYPE_DUFFETT) {
									IO_PRINT_LINE(M30202);
									rc = EIE_FAIL;
								}
								if (rc != EIE_SUCCEED) return EIE_FAIL;
							}
							//add to MarginalCell_CellSet
							rc = STC_CellSetAddLast (MarginalCell_CellSet, SubMarginalCell);
							if (rc != EIE_SUCCEED) return EIE_FAIL;
							//add to global CellSet and KdTree
							rc = STC_CellSetAddLast (CellSet, SubMarginalCell);
							if (rc != EIE_SUCCEED) return EIE_FAIL;
							kdtree = STC_KdTreeInsert (KdTree, SubMarginalCell);
							if (kdtree == NULL) return EIE_FAIL;
						}
					}
					SubMarginalCellSet->NumberEntries = 0;
				}
			}
			rc = STC_WriteConstraint (MarginalCell_CellSet, MarginalCell);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
			for (k = 0; k < MarginalCell_CellSet->NumberEntries; k++) {
				if (!MarginalCell_CellSet->Cell[k]->IsInternal) {
					STC_CellFreeItems (MarginalCell_CellSet->Cell[k]);
				}
			}
		}
	}

	MarginalCell_CellSet->NumberEntries = 0;
	STC_CellSetFree (MarginalCell_CellSet);

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Calculate sensitivity of marginal cell identified by Coordinate.

SmallestCoordinate, LargestCoordinate, WorkingCoordinate, MarginalCellSet and SubMarginalCellSet
are working variables, but for performance purposes, they are allocated once.
For big problems, they could be allocated and freed millions of times.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE CalculateMarginalCellSensitivityForOneCell (
	STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule,
	STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate,
	STCT_COORDINATE * WorkingCoordinate,
	STCT_CELLSET * MarginalCellSet,
	STCT_CELLSET * SubMarginalCellSet,
	int * NumberMarginalCellsCalculated,
	int * NumberMarginalCellsSensitive,
	int * NumberMarginalCellsZero)
{
	int i;
	STCT_KDTREE * kdtree;//to check return code
	STCT_CELL * MarginalCell;
	STCT_CELL * PreviouslyCalculatedMarginalCell;
	EIT_RETURNCODE rc;

	//has it been calculated before?
	PreviouslyCalculatedMarginalCell = STC_KdTreeSearch (KdTree, Coordinate);

#ifdef _DEBUG
	assert (PreviouslyCalculatedMarginalCell == NULL ||
		(PreviouslyCalculatedMarginalCell != NULL && PreviouslyCalculatedMarginalCell->TotalNumberObservations != 0));
#endif

	rc = FindSmallestCellSet (HTreeRoot, CellSet, KdTree, SRule, Coordinate,
		SmallestCoordinate, LargestCoordinate, WorkingCoordinate,
		MarginalCellSet, SubMarginalCellSet);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	if (MarginalCellSet->NumberEntries != 0) {
		//STC_CellSetPrint (MarginalCellSet);
		MarginalCell = STC_CellSetSensitivity (MarginalCellSet, SRule, NULL);
		if (MarginalCell == NULL) return EIE_FAIL;
		//STC_CellPrint (MarginalCell);
		//EI_PrintMessages ();

		if (MarginalCell->TotalNumberObservations == 0) {
			STC_CellFree (MarginalCell);
			MarginalCell = NULL;
		}
		else {
			if (MarginalCell->TotalValue == 0.0) {
				(*NumberMarginalCellsZero)++;
			}
			else {
				(*NumberMarginalCellsCalculated)++;
				if (MarginalCell->Sensitivity > 0.0)
					(*NumberMarginalCellsSensitive)++;
			}

			if (PreviouslyCalculatedMarginalCell == NULL) {
				MarginalCell->Coordinate = STC_CoordinateDuplicate (Coordinate);
				if (MarginalCell->Coordinate == NULL) return EIE_FAIL;

				//add Marginal cell to CellSet, we need to keep track of the cellid of the marginal cell
				rc = STC_CellSetAddLast (CellSet, MarginalCell);
				if (rc != EIE_SUCCEED) return EIE_FAIL;
				kdtree = STC_KdTreeInsert (KdTree, MarginalCell);
				if (kdtree == NULL) return EIE_FAIL;
			}
			else {
				MarginalCell->CellId = PreviouslyCalculatedMarginalCell->CellId;
				MarginalCell->Coordinate = PreviouslyCalculatedMarginalCell->Coordinate;
			}
			rc = STC_WriteConstraint (MarginalCellSet, MarginalCell);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
			MarginalCell->Type = STC_CELLTYPE_CELL;
			rc = STC_WriteCell (MarginalCell, HTreeRoot->NumberEntries);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
			rc = STC_WriteLargest (MarginalCell, SRule->waiver_flags_present, SRule->Parms);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		if ((SRule->Parms->Weight != 0 && (SRule->Parms->WeightProtectionLevel != 'E'))) {
			if (SRule->Type == STCE_SRULE_TYPE_NK) {
				rc = STC_WriteTargets(MarginalCell, (SRule->waiver_flags_present == 0)?(MarginalCell->LargestFTNumberEntries):(MarginalCell->LargestFWNumberEntries), (SRule->waiver_flags_present == 0)?(MarginalCell->LargestFT):(MarginalCell->LargestFW), MarginalCell->LargestFSNumberEntries, MarginalCell->LargestFS, SRule->NumberEntries[0], 1, SRule->waiver_flags_present);
			}
			else if (SRule->Type == STCE_SRULE_TYPE_PQ) {
				rc = STC_WritePairs  (MarginalCell, (SRule->waiver_flags_present == 0)?(MarginalCell->LargestFTNumberEntries):(MarginalCell->LargestFWNumberEntries), (SRule->waiver_flags_present == 0)?(MarginalCell->LargestFT):(MarginalCell->LargestFW), MarginalCell->LargestFSNumberEntries, MarginalCell->LargestFS,                          1, SRule->waiver_flags_present);
			}

			if (SRule->Type != STCE_SRULE_TYPE_NK && SRule->Type != STCE_SRULE_TYPE_PQ && SRule->Type != STCE_SRULE_TYPE_C2 && SRule->Type != STCE_SRULE_TYPE_DUFFETT) {
				IO_PRINT_LINE(M30202);
				rc = EIE_FAIL;
			}
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}

			if (PreviouslyCalculatedMarginalCell == NULL) {
				//marginal cell was added to CellSet this time,
				//free the respondant data, but keep marginal cell info
				STC_CellFreeItems (MarginalCell);
			}
			else {
				//marginal cell was not added to CellSet this time,
				//free all of Marginal Cell
				MarginalCell->Coordinate = NULL;//set to NULL, so it will not be freed
				STC_CellFree (MarginalCell);
			}
		}

		for (i = 0; i < MarginalCellSet->NumberEntries; i++) {
			if (!MarginalCellSet->Cell[i]->IsInternal) {
				STC_CellFreeItems (MarginalCellSet->Cell[i]);
			}
		}
	}//MarginalCellSet->NumberEntries != 0

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Calculate sensitivity of marginal cells.

SmallestCoordinate, LargestCoordinate, WorkingCoordinate, MarginalCellSet and SubMarginalCellSet
are working variables, but for performance purposes, they are allocated once.
For big problems, they could be allocated and freed millions of times.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE CalculateMarginalCellsSensitivityForAllCells (
	STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule,
	int iDimension,
	STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate,
	STCT_COORDINATE * WorkingCoordinate,
	STCT_CELLSET * MarginalCellSet,
	STCT_CELLSET * SubMarginalCellSet,
	int * NumberMarginalCellsCalculated,
	int * NumberMarginalCellsSensitive,
	int * NumberMarginalCellsZero)
{
	int i;
	EIT_RETURNCODE rc;

	if (iDimension == HTreeRoot->NumberEntries) {
#ifdef _DEBUG
		if (DEBUGCMC) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Calculating marginal cell?\n");
			STC_CoordinatePrint (Coordinate, HTreeRoot);
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
			EI_PrintMessages ();
		}
#endif
		if (!IsCoordinateInternal (HTreeRoot, Coordinate)) {
#ifdef _DEBUG
			if (DEBUGCMC) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " not internal.\n");
#endif
			rc = CalculateMarginalCell (HTreeRoot, CellSet, KdTree, SRule,
				Coordinate, SmallestCoordinate, LargestCoordinate, WorkingCoordinate,
				MarginalCellSet, SubMarginalCellSet,
				NumberMarginalCellsCalculated, NumberMarginalCellsSensitive, NumberMarginalCellsZero);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
#ifdef _DEBUG
		else {
			if (DEBUGCMC) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "     internal.\n");
		}
#endif
	}
	else {
		tIList * Tags;
		IList_New (&Tags);
		if (Tags == NULL) return EIE_FAIL;
		rc = STC_HTreeGetTagsWithData (HTreeRoot->Dimension[iDimension], Tags);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
		for (i = 0; i < Tags->ne; i++) {
			Coordinate->Tag[iDimension] = IList_Entry (Tags, i);
			rc = CalculateMarginalCellsSensitivityForAllCells (HTreeRoot, CellSet, KdTree, SRule,
				iDimension+1, Coordinate, SmallestCoordinate, LargestCoordinate, WorkingCoordinate,
				MarginalCellSet, SubMarginalCellSet,
				NumberMarginalCellsCalculated, NumberMarginalCellsSensitive, NumberMarginalCellsZero);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
		IList_Free (Tags);
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
comparer les branches pour s'assurer qu ils ne s'overlappent pas
------------------------------------------------------------------------------*/
static EIT_RETURNCODE CheckBranchesOverlapping (
	STCT_HTREE * HTree,
	STCT_RANGE * Range)
{
	int i, j, k;
	STCT_RANGE ** RangeArray;
	EIT_RETURNCODE rc;

	for (i = 0; i < HTree->NumberDecompositions; i++) {
		RangeArray = RangeArrayAllocate (HTree->Decomposition[i]->NumberEntries);
		if (RangeArray == NULL) return EIE_FAIL;
		for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
			rc = STC_GetEndCodes (HTree->Decomposition[i]->Branch[j], Range, 0, RangeArray[j]);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}

		for (j = 0; j < HTree->Decomposition[i]->NumberEntries-1; j++) {
			for (k = j+1; k < HTree->Decomposition[i]->NumberEntries; k++) {
				if (STC_RangeIntersect (RangeArray[j], RangeArray[k])) {
					EI_AddMessage (M00051, EIE_MESSAGESEVERITY_ERROR,
						M30111,//The decomposition has 2 overlapping codes or ranges
						HTree->Decomposition[i]->Branch[j]->Code,
						HTree->Decomposition[i]->Branch[k]->Code,
						i+1, HTree->Code);
					EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, M30112 "\n");
					STC_RangePrint (RangeArray[j]);
					EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
					STC_RangePrint (RangeArray[k]);
					EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
					RangeArrayFree (RangeArray, HTree->Decomposition[i]->NumberEntries);
					return EIE_FAIL;
				}
			}
		}

		RangeArrayFree (RangeArray, HTree->Decomposition[i]->NumberEntries);

		//go deeper
		for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
			if (HTree->Decomposition[i]->Branch[j]->NumberDecompositions > 0) {
				rc = CheckBranchesOverlapping (HTree->Decomposition[i]->Branch[j], Range);
				if (rc != EIE_SUCCEED) return EIE_FAIL;
			}
		}
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
2 decompositions must have the same end codes
------------------------------------------------------------------------------*/
static EIT_RETURNCODE CheckDecompositionsEquivalence (
	STCT_HTREE * HTree,
	STCT_RANGE * Range)
{
	EIT_BOOLEAN Equal;
	int i, j;
	STCT_RANGE ** RangeArray;
	EIT_RETURNCODE rc;

	if (HTree->NumberDecompositions > 1) {
		RangeArray = RangeArrayAllocate (HTree->NumberDecompositions);
		//RangeArray = STC_AllocateMemory (HTree->NumberDecompositions * sizeof *RangeArray);
		if (RangeArray == NULL) return EIE_FAIL;
		for (i = 0; i < HTree->NumberDecompositions; i++) {
			//RangeArray[i] = STC_RangeAllocate ();
			//if (RangeArray[i] == NULL) return EIE_FAIL;
			for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
				rc = STC_GetEndCodes (HTree->Decomposition[i]->Branch[j], Range, 0, RangeArray[i]);
				if (rc != EIE_SUCCEED) return EIE_FAIL;
			}
			//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "avant Simplifying range %d\n", i);
			//STC_RangePrint (RangeArray[i]);
			STC_RangeSimplifyAggressively (RangeArray[i]);
			//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "apres Simplifying range %d\n", i);
			//STC_RangePrint (RangeArray[i]);
			//EI_PrintMessages ();
		}
		for (i = 0; i < HTree->NumberDecompositions-1; i++) {
			for (j = i+1; j < HTree->NumberDecompositions; j++) {
				Equal = RangeEqual (RangeArray[i], RangeArray[j]);
				if (!Equal) {
					EI_AddMessage ("Validation", EIE_MESSAGESEVERITY_ERROR, M30108,//2 decompositions of code are not equivalent
						i+1, j+1, HTree->Code);

					EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n" M30109 "\n", i+1);//decomposition i+1
					STC_RangePrint (RangeArray[i]);
					EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n" M30109 "\n", j+1);//decomposition j+1
					STC_RangePrint (RangeArray[j]);
					EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
					EI_PrintMessages ();

					RangeArrayFree (RangeArray, HTree->NumberDecompositions);
					return EIE_FAIL;
				}
			}
		}
		RangeArrayFree (RangeArray, HTree->NumberDecompositions);
	}

	//go deeper
	for (i = 0; i < HTree->NumberDecompositions; i++) {
		for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
			rc = CheckDecompositionsEquivalence (HTree->Decomposition[i]->Branch[j], Range);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
combine all codes from all dimensions to form all combinaison of internal cells
------------------------------------------------------------------------------*/
static STCT_ADDMICRODATA_RETURNCODE CombineCodes (
	STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet,
	STCT_KDTREE ** KdTree,
	tSList ** Codes,
	int iDimension,
	STCT_COORDINATE * Coordinate,
	char * Key,
	double Value,
	double Shadow,
	double Proxy,
	double Weight,
	int * MessageQuota,
	int waiver_flags_present,
	int waiver_flag)
{
	STCT_ADDMICRODATA_RETURNCODE amdrc;
	STCT_CELL * Cell;
	int i, j;
	STCT_KDTREE * lKdTree;
	EIT_RETURNCODE rc;
	STCT_HTREE * TempHTree;

	if (iDimension == HTreeRoot->NumberEntries) {
		Cell = STC_KdTreeSearch (*KdTree, Coordinate);
		if (Cell == NULL) {
			lKdTree = *KdTree;
			Cell = STC_CellAllocate (1000);
			if (Cell == NULL) return STCE_ADDMICRODATA_FAIL;
			Cell->IsInternal = EIE_TRUE;
			Cell->Coordinate = STC_CoordinateDuplicate (Coordinate);
			if (Cell->Coordinate == NULL) return STCE_ADDMICRODATA_FAIL;
			rc = STC_CellSetAddLast (CellSet, Cell);
			if (rc != EIE_SUCCEED) return STCE_ADDMICRODATA_FAIL;
			lKdTree = STC_KdTreeInsert (lKdTree, CellSet->Cell[CellSet->NumberEntries-1]);
			if (lKdTree == NULL) return STCE_ADDMICRODATA_FAIL;
			*KdTree = lKdTree;

			//mark the hierarchy as having data
			for (j = 0; j < HTreeRoot->NumberEntries; j++) {
				HTreeRoot->TagIndex[j][Cell->Coordinate->Tag[j]]->HasData = STCE_HASDATA_TYPE_YES;
			}
		}
		rc = STC_CellAdd (Cell, Key, Value, Shadow, Proxy, Weight, waiver_flags_present, waiver_flag);
		if (rc != EIE_SUCCEED) return STCE_ADDMICRODATA_FAIL;
	}
	else {
		for (i = 0; i < Codes[iDimension]->ne; i++) {
			TempHTree = STC_HTreeSearchCode (HTreeRoot->Dimension[iDimension],
				SList_Entry (Codes[iDimension], i));
			//no need to check this anymore because the range
			//contains an entry for every code of the hierarchy
			//if (TempHTree == NULL) {
			//	ADDMESSAGEQUOTA (MessageQuota, M20028, HTreeRoot->Name[iDimension], Key, SList_Entry (Codes[iDimension], i));//not in hierarchy
			//	return STCE_ADDMICRODATA_INVALID;
			//}

			Coordinate->Tag[iDimension] = TempHTree->Tag;
			amdrc = CombineCodes (HTreeRoot, CellSet, KdTree, Codes, iDimension+1, Coordinate, Key, Value, Shadow, Proxy, Weight, MessageQuota, waiver_flags_present, waiver_flag);
			if (amdrc != STCE_ADDMICRODATA_SUCCEED) return amdrc;
		}
	}
	return STCE_ADDMICRODATA_SUCCEED;
}
/*------------------------------------------------------------------------------
Find the sensitive aggregates.

SmallestCoordinate, LargestCoordinate, MarginalCellSet and SubMarginalCellSet
are working variables, but for performance purposes, they are allocated once.
For big problems, they could be allocated and freed millions of times.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE FindSensitiveAggregatesForAllMarginalCells (
	STCT_HTREEROOT * HTreeRoot,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule,
	tIList ** Tags,
	tIList ** NonInternalTags,
	int M,
	double X,
	double Y,
	double Z,
	int N,
	int NN,
	int Verbose,
	int iDimension,
	int ExcludedDimension,
	STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate,
	STCT_CELLSET * MarginalCellSet,
	STCT_CELLSET * SubMarginalCellSet,
	int * NumberCombinaisonsCalculated,
	int * NumberCombinaisonsSensitive)
{
	int i;
	EIT_RETURNCODE rc;

	if (iDimension == HTreeRoot->NumberEntries) {
		rc = FindSensitiveAggregatesForOneMarginalCell (HTreeRoot, KdTree, SRule, NonInternalTags,
			M, X, Y, Z, N, NN, Verbose,
			ExcludedDimension, Coordinate,
			SmallestCoordinate, LargestCoordinate,
			MarginalCellSet, SubMarginalCellSet,
			NumberCombinaisonsCalculated, NumberCombinaisonsSensitive);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}
	else if (iDimension == ExcludedDimension) {
		rc = FindSensitiveAggregatesForAllMarginalCells (HTreeRoot, KdTree, SRule, Tags, NonInternalTags,
			M, X, Y, Z, N, NN, Verbose,
			iDimension+1, ExcludedDimension, Coordinate,
			SmallestCoordinate, LargestCoordinate,
			MarginalCellSet, SubMarginalCellSet,
			NumberCombinaisonsCalculated, NumberCombinaisonsSensitive);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}
	else {
		for (i = 0; i < Tags[iDimension]->ne; i++) {
			Coordinate->Tag[iDimension] = IList_Entry (Tags[iDimension], i);
			rc = FindSensitiveAggregatesForAllMarginalCells (HTreeRoot, KdTree, SRule, Tags, NonInternalTags,
				M, X, Y, Z, N, NN, Verbose,
				iDimension+1, ExcludedDimension, Coordinate,
				SmallestCoordinate, LargestCoordinate,
				MarginalCellSet, SubMarginalCellSet,
				NumberCombinaisonsCalculated, NumberCombinaisonsSensitive);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Find the sensitive aggregates for one node.

SmallestCoordinate, LargestCoordinate, MarginalCellSet and SubMarginalCellSet
are working variables, but for performance purposes, they are allocated once.
For big problems, they could be allocated and freed millions of times.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE FindSensitiveAggregatesForOneMarginalCell (
	STCT_HTREEROOT * HTreeRoot,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule,
	tIList ** NonInternalTags,
	int M,
	double X,
	double Y,
	double Z,
	int N,
	int NN,
	int Verbose,
	int ExcludedDimension,
	STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate,
	STCT_CELLSET * MarginalCellSet,
	STCT_CELLSET * SubMarginalCellSet,
	int * NumberCombinaisonsCalculated,
	int * NumberCombinaisonsSensitive)
{
	STCT_CELL * Cell;
	int i, j, k, m;
	STCT_HTREE * Node;
	EIT_RETURNCODE rc;
	STCT_CELL * SubMarginalCell;

	MarginalCellSet->NumberEntries = 0;//empty the set

	/* iterate through every non internal codes in the exclude dimension */
	for (i = 0; i < IList_NumEntries (NonInternalTags[ExcludedDimension]); i++) {

		Coordinate->Tag[ExcludedDimension] = IList_Entry (NonInternalTags[ExcludedDimension], i);
		//STC_CoordinatePrint (Coordinate, HTreeRoot);
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
		Cell = STC_KdTreeSearch (KdTree, Coordinate);
		if (Cell == NULL) {
			//if cell does not exist, there is no need to find its sensitive aggregates
			//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " Cell is not present.\n");
			continue;//treat next NonInternalTag
		}
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");

		/* find this node in the hierarchy */
		Node = HTreeRoot->TagIndex[ExcludedDimension][IList_Entry (NonInternalTags[ExcludedDimension], i)];

		/* iterate through every decompositions of this node */
		for (j = 0; j < Node->NumberDecompositions; j++) {

			MarginalCellSet->NumberEntries = 0;//empty the set
#ifdef PRINT_NUMBER_EQ
			mNumberEq++;
			mSubNumberEq++;
			//if ((mNumberEq % 10) == 0 || (mSubNumberEq % 10) == 0 || mSubNumberEq == 1)
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%4d %4d \n", mNumberEq, mSubNumberEq);
#endif
#ifdef _DEBUG
			if (DEBUGFSA) {
				Coordinate->Tag[ExcludedDimension] = Node->Tag;
				STC_CoordinatePrint (Coordinate, HTreeRoot);
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
			}
#endif
			/* iterate through every branches of this node */
			for (k = 0; k < Node->Decomposition[j]->NumberEntries; k++) {
#ifdef _DEBUG
				if (DEBUGFSA)
					if (k == 0)
						EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " = ");
					else
						EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " + ");
#endif
				Coordinate->Tag[ExcludedDimension] = Node->Decomposition[j]->Branch[k]->Tag;
#ifdef _DEBUG
				if (DEBUGFSA) STC_CoordinatePrint (Coordinate, HTreeRoot);
#endif
				Cell = STC_KdTreeSearch (KdTree, Coordinate);
				if (Cell != NULL) {
#ifdef _DEBUG
					if (DEBUGFSA) STC_CellPrintInfo (Cell, 0);
#endif
					rc = STC_CellSetAddLast (MarginalCellSet, Cell);
					if (rc != EIE_SUCCEED) return EIE_FAIL;
				}
#ifdef _DEBUG
				else {
					if (DEBUGFSA) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " not found\n");
				}
#endif
			}
#ifdef _DEBUG
			if (DEBUGFSA) EI_PrintMessages ();
#endif
			if (MarginalCellSet->NumberEntries > 2) {//see step 0 in doc. "Treatment of Aggregated Sensitive Cells"
#ifdef _DEBUG
				if (DEBUGFSA) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
#endif
				//STC_CellSetPrintInfo (MarginalCellSet);
				//EI_PrintMessages ();

				/* iterate through every cells of the cell set*/
				for (k = 0; k < MarginalCellSet->NumberEntries; k++) {
#ifdef _DEBUG
					if (DEBUGFSA) {
						if (MarginalCellSet->Cell[k]->IsInternal) {
							EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "cell %d is internal\n", k);
						}
						else {
							EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "cell %d is not internal\n", k);
						}
						if (MarginalCellSet->Cell[k]->Data.Item == NULL) {
							EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "cell %d data item is NULL\n", k);
						}
						else {
							EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "cell %d data item is not NULL\n", k);
						}
						STC_CellPrintInfo (MarginalCellSet->Cell[k], 1);
					}
#endif
					if (!MarginalCellSet->Cell[k]->IsInternal) {
						int NumberSensitivityChanged;
						SubMarginalCellSet->NumberEntries = 0;
						rc = GetInternalCells (HTreeRoot, MarginalCellSet->Cell[k]->Coordinate, KdTree,
							SubMarginalCellSet, SmallestCoordinate, LargestCoordinate);
						if (rc != EIE_SUCCEED) return EIE_FAIL;
						/* we are NOT interested to know if the sensitivity changed when calculating the
						   sensitivity of the marginal, because we already recorded that knowledge in
						   STC_CalculateMarginalCellsSensitivity () */
						NumberSensitivityChanged = SRule->NumberSensitivityChanged;//save value
						SubMarginalCell = STC_CellSetSensitivity (SubMarginalCellSet, SRule, NULL);
                        SRule->NumberSensitivityChanged = NumberSensitivityChanged;//reset value
						MarginalCellSet->Cell[k]->Data = SubMarginalCell->Data;
						/*-duplicate in "MarginalCellSet->Cell[k]" the contents of "LargestNumberEntries"   and "Largest"   in "SubMarginalCell": */
						SetLargestRespondents (MarginalCellSet->Cell[k], SubMarginalCell);
						/*-duplicate in "MarginalCellSet->Cell[k]" the contents of "LargestPYNumberEntries" and "LargestPY" in "SubMarginalCell": */
						MarginalCellSet->Cell[k]->LargestPYNumberEntries = SubMarginalCell->LargestPYNumberEntries;
						for (m = 0; m < SubMarginalCell->LargestPYNumberEntries; m++) {
						    if      (SubMarginalCell->LargestPY[m] == NULL                                 ) {
						        MarginalCellSet->Cell[k]->LargestPY[m] = NULL;
						    }
						    else if (SubMarginalCell->LargestPY[m] == &(SubMarginalCell->AnonymousDataItem)) {
						        MarginalCellSet->Cell[k]->LargestPY[m] = &(MarginalCellSet->Cell[k]->AnonymousDataItem);
						    }
						    else {
						        MarginalCellSet->Cell[k]->LargestPY[m] =  ((STCT_DATAITEM *)((MarginalCellSet->Cell[k]->Data).Item)) +
						                                                 ((SubMarginalCell->LargestPY)[m] -
						                                                  ((STCT_DATAITEM *)((SubMarginalCell         ->Data).Item))
						                                                 );
						    }
						}
						/*-duplicate in "MarginalCellSet->Cell[k]" the contents of "LargestWVNumberEntries" and "LargestWV" in "SubMarginalCell": */
						MarginalCellSet->Cell[k]->LargestWVNumberEntries = SubMarginalCell->LargestWVNumberEntries;
						for (m = 0; m < SubMarginalCell->LargestWVNumberEntries; m++) {
						    if      (SubMarginalCell->LargestWV[m] == NULL                                 ) {
						        MarginalCellSet->Cell[k]->LargestWV[m] = NULL;
						    }
						    else if (SubMarginalCell->LargestWV[m] == &(SubMarginalCell->AnonymousDataItem)) {
						        MarginalCellSet->Cell[k]->LargestWV[m] = &(MarginalCellSet->Cell[k]->AnonymousDataItem);
						    }
						    else {
						        MarginalCellSet->Cell[k]->LargestWV[m] =  ((STCT_DATAITEM *)((MarginalCellSet->Cell[k]->Data).Item)) +
						                                                 ((SubMarginalCell->LargestWV)[m] -
						                                                  ((STCT_DATAITEM *)((SubMarginalCell         ->Data).Item))
						                                                 );
						    }
						}
						/*-duplicate in "MarginalCellSet->Cell[k]" the contents of "LargestWPNumberEntries" and "LargestWP" in "SubMarginalCell": */
						MarginalCellSet->Cell[k]->LargestWPNumberEntries = SubMarginalCell->LargestWPNumberEntries;
						for (m = 0; m < SubMarginalCell->LargestWPNumberEntries; m++) {
						    if      (SubMarginalCell->LargestWP[m] == NULL                                 ) {
						        MarginalCellSet->Cell[k]->LargestWP[m] = NULL;
						    }
						    else if (SubMarginalCell->LargestWP[m] == &(SubMarginalCell->AnonymousDataItem)) {
						        MarginalCellSet->Cell[k]->LargestWP[m] = &(MarginalCellSet->Cell[k]->AnonymousDataItem);
						    }
						    else {
						        MarginalCellSet->Cell[k]->LargestWP[m] =  ((STCT_DATAITEM *)((MarginalCellSet->Cell[k]->Data).Item)) +
						                                                 ((SubMarginalCell->LargestWP)[m] -
						                                                  ((STCT_DATAITEM *)((SubMarginalCell         ->Data).Item))
						                                                 );
						    }
						}
						/*-duplicate in "MarginalCellSet->Cell[k]" the contents of "LargestFTNumberEntries" and "LargestFT" in "SubMarginalCell": */
						MarginalCellSet->Cell[k]->LargestFTNumberEntries = SubMarginalCell->LargestFTNumberEntries;
						for (m = 0; m < SubMarginalCell->LargestFTNumberEntries; m++) {
						    if      (SubMarginalCell->LargestFT[m] == NULL                                 ) {
						        MarginalCellSet->Cell[k]->LargestFT[m] = NULL;
						    }
						    else if (SubMarginalCell->LargestFT[m] == &(SubMarginalCell->AnonymousDataItem)) {
						        MarginalCellSet->Cell[k]->LargestFT[m] = &(MarginalCellSet->Cell[k]->AnonymousDataItem);
						    }
						    else {
						        MarginalCellSet->Cell[k]->LargestFT[m] =  ((STCT_DATAITEM *)((MarginalCellSet->Cell[k]->Data).Item)) +
						                                                 ((SubMarginalCell->LargestFT)[m] -
						                                                  ((STCT_DATAITEM *)((SubMarginalCell         ->Data).Item))
						                                                 );
						    }
						}
						/*-duplicate in "MarginalCellSet->Cell[k]" the contents of "LargestFWNumberEntries" and "LargestFW" in "SubMarginalCell": */
						MarginalCellSet->Cell[k]->LargestFWNumberEntries = SubMarginalCell->LargestFWNumberEntries;
						for (m = 0; m < SubMarginalCell->LargestFWNumberEntries; m++) {
						    if      (SubMarginalCell->LargestFW[m] == NULL                                 ) {
						        MarginalCellSet->Cell[k]->LargestFW[m] = NULL;
						    }
						    else if (SubMarginalCell->LargestFW[m] == &(SubMarginalCell->AnonymousDataItem)) {
						        MarginalCellSet->Cell[k]->LargestFW[m] = &(MarginalCellSet->Cell[k]->AnonymousDataItem);
						    }
						    else {
						        MarginalCellSet->Cell[k]->LargestFW[m] =  ((STCT_DATAITEM *)((MarginalCellSet->Cell[k]->Data).Item)) +
						                                                 ((SubMarginalCell->LargestFW)[m] -
						                                                  ((STCT_DATAITEM *)((SubMarginalCell         ->Data).Item))
						                                                 );
						    }
						}
						/*-duplicate in "MarginalCellSet->Cell[k]" the contents of "LargestFSNumberEntries" and "LargestFS" in "SubMarginalCell": */
						MarginalCellSet->Cell[k]->LargestFSNumberEntries = SubMarginalCell->LargestFSNumberEntries;
						for (m = 0; m < SubMarginalCell->LargestFSNumberEntries; m++) {
						    if      (SubMarginalCell->LargestFS[m] == NULL                                 ) {
						        MarginalCellSet->Cell[k]->LargestFS[m] = NULL;
						    }
						    else if (SubMarginalCell->LargestFS[m] == &(SubMarginalCell->AnonymousDataItem)) {
						        MarginalCellSet->Cell[k]->LargestFS[m] = &(MarginalCellSet->Cell[k]->AnonymousDataItem);
						    }
						    else {
						        MarginalCellSet->Cell[k]->LargestFS[m] =  ((STCT_DATAITEM *)((MarginalCellSet->Cell[k]->Data).Item)) +
						                                                 ((SubMarginalCell->LargestFS)[m] -
						                                                  ((STCT_DATAITEM *)((SubMarginalCell         ->Data).Item))
						                                                 );
						    }
						}
						/*-duplicate in "MarginalCellSet->Cell[k]" the contents of "Largst2FTNumberEntries" and "Largst2FT" in "SubMarginalCell": */
						MarginalCellSet->Cell[k]->Largst2FTNumberEntries = SubMarginalCell->Largst2FTNumberEntries;
						for (m = 0; m < SubMarginalCell->Largst2FTNumberEntries; m++) {
						    if      (SubMarginalCell->Largst2FT[m] == NULL                                 ) {
						        MarginalCellSet->Cell[k]->Largst2FT[m] = NULL;
						    }
						    else if (SubMarginalCell->Largst2FT[m] == &(SubMarginalCell->AnonymousDataItem)) {
						        MarginalCellSet->Cell[k]->Largst2FT[m] = &(MarginalCellSet->Cell[k]->AnonymousDataItem);
						    }
						    else {
						        MarginalCellSet->Cell[k]->Largst2FT[m] =  ((STCT_DATAITEM *)((MarginalCellSet->Cell[k]->Data).Item)) +
						                                                 ((SubMarginalCell->Largst2FT)[m] -
						                                                  ((STCT_DATAITEM *)((SubMarginalCell         ->Data).Item))
						                                                 );
						    }
						}
						/*-duplicate in "MarginalCellSet->Cell[k]" the contents of "Largst2FWNumberEntries" and "Largst2FW" in "SubMarginalCell": */
						MarginalCellSet->Cell[k]->Largst2FWNumberEntries = SubMarginalCell->Largst2FWNumberEntries;
						for (m = 0; m < SubMarginalCell->Largst2FWNumberEntries; m++) {
						    if      (SubMarginalCell->Largst2FW[m] == NULL                                 ) {
						        MarginalCellSet->Cell[k]->Largst2FW[m] = NULL;
						    }
						    else if (SubMarginalCell->Largst2FW[m] == &(SubMarginalCell->AnonymousDataItem)) {
						        MarginalCellSet->Cell[k]->Largst2FW[m] = &(MarginalCellSet->Cell[k]->AnonymousDataItem);
						    }
						    else {
						        MarginalCellSet->Cell[k]->Largst2FW[m] =  ((STCT_DATAITEM *)((MarginalCellSet->Cell[k]->Data).Item)) +
						                                                 ((SubMarginalCell->Largst2FW)[m] -
						                                                  ((STCT_DATAITEM *)((SubMarginalCell         ->Data).Item))
						                                                 );
						    }
						}
						/*-duplicate in "MarginalCellSet->Cell[k]" the contents of "Largst2FSNumberEntries" and "Largst2FS" in "SubMarginalCell": */
						MarginalCellSet->Cell[k]->Largst2FSNumberEntries = SubMarginalCell->Largst2FSNumberEntries;
						for (m = 0; m < SubMarginalCell->Largst2FSNumberEntries; m++) {
						    if      (SubMarginalCell->Largst2FS[m] == NULL                                 ) {
						        MarginalCellSet->Cell[k]->Largst2FS[m] = NULL;
						    }
						    else if (SubMarginalCell->Largst2FS[m] == &(SubMarginalCell->AnonymousDataItem)) {
						        MarginalCellSet->Cell[k]->Largst2FS[m] = &(MarginalCellSet->Cell[k]->AnonymousDataItem);
						    }
						    else {
						        MarginalCellSet->Cell[k]->Largst2FS[m] =  ((STCT_DATAITEM *)((MarginalCellSet->Cell[k]->Data).Item)) +
						                                                 ((SubMarginalCell->Largst2FS)[m] -
						                                                  ((STCT_DATAITEM *)((SubMarginalCell         ->Data).Item))
						                                                 );
						    }
						}
						/*-???I think the immediately following 3 lines are causing a memory leak, and that they were probably */
						/* added here to make the program work in spite of the error in the function                           */
						/* "STC_Build.c::SetLargestRespondents()" (note that the call to "STC_Build.c::SetLargestRespondents()"*/
						/* earlier in this function is the ONLY call to that function (other than one additional call in the   */
						/* no-longer-used function "STC_Build.c::STC_CalculateUserGroupsSensitivity()")):                      */
						/* --no, this is fine, because the "STCT_DATAITEMS" in the list pointed to by                          */
						/* "MarginalCellSet->Cell[k]->Data.Item" are identical to the ones in the list pointed to by           */
						/* "SubMarginalCell->Data.Item" (because "MarginalCellSet->Cell[k]->Data.Item" was assigned a shallow  */
						/* copy of "SubMarginalCell->Data.Item")                                                               */
						SubMarginalCell->Data.Item = NULL;
						SubMarginalCell->Data.NumberAllocated = 0;
						SubMarginalCell->Data.NumberEntries = 0;
						STC_CellFree (SubMarginalCell);
					}
				}
				rc = STC_Step0123 (MarginalCellSet, SRule, HTreeRoot,
					M, X, Y, Z, N, NN, Verbose, HTreeRoot->NumberEntries,
					NumberCombinaisonsCalculated, NumberCombinaisonsSensitive);
				if (rc != EIE_SUCCEED) return EIE_FAIL;

				for (k = 0; k < MarginalCellSet->NumberEntries; k++) {
					if (!MarginalCellSet->Cell[k]->IsInternal) {
						STC_CellFreeItems (MarginalCellSet->Cell[k]);
					}
				}
#ifdef _DEBUG
				if (DEBUGFSA) {
					EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
					EI_PrintMessages ();
				}
#endif
			}
		}
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Find the smallest cell set to form the marginal cell
example: for hierarchy="00 10 20 30; 0 1 2;"
cell 00,0 = 10,0 + 20,0 + 30,0 (1)
and
cell 00,0 = 00,1 + 00,2 (2)
the second (2) one is picked.

SmallestCoordinate, LargestCoordinate, WorkingCoordinate, MarginalCellSet and SubMarginalCellSet
are working variables, but for performance purposes, they are allocated once.
For big problems, they could be allocated and freed millions of times.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE FindSmallestCellSet (
	STCT_HTREEROOT * HTreeRoot,
	STCT_CELLSET * CellSet,
	STCT_KDTREE * KdTree,
	STCT_SRULE * SRule,
	STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate,
	STCT_COORDINATE * WorkingCoordinate,
	STCT_CELLSET * MarginalCellSet,
	STCT_CELLSET * SubMarginalCellSet)
{
	STCT_CELL * Cell;
	int DecompositionWithSmallestNumberBranches;
	int DimensionWithSmallestNumberBranches;
	int i;
	EIT_BOOLEAN IsInCellSet;
	EIT_BOOLEAN IsInternal;
	STCT_KDTREE * kdtree;//to check return code
	STCT_HTREE * Node;
	EIT_RETURNCODE rc;
	int SmallestNumberBranches;
	STCT_CELL * SubMarginalCell;

#ifdef _DEBUG
	if (DEBUGCMC) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "marginal , ");
		STC_CoordinatePrint (Coordinate, HTreeRoot);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	}
#endif

	FindSmallestCellSetCoordinates (HTreeRoot, Coordinate, &SmallestNumberBranches, &DecompositionWithSmallestNumberBranches, &DimensionWithSmallestNumberBranches);

	MarginalCellSet->NumberEntries = 0;//empty the set

	Node = HTreeRoot->TagIndex[DimensionWithSmallestNumberBranches][Coordinate->Tag[DimensionWithSmallestNumberBranches]];

	STC_CoordinateCopy (WorkingCoordinate, Coordinate);

	//loop through branches of smallest dimension of current node
	for (i = 0; i < SmallestNumberBranches; i++) {
		WorkingCoordinate->Tag[DimensionWithSmallestNumberBranches] = Node->Decomposition[DecompositionWithSmallestNumberBranches]->Branch[i]->Tag;
#ifdef _DEBUG
		if (DEBUGCMC) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "composante, ");
			STC_CoordinatePrint (WorkingCoordinate, HTreeRoot);
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
			EI_PrintMessages ();
		}
#endif
		Cell = STC_KdTreeSearch (KdTree, WorkingCoordinate);

		if (Cell != NULL) {
			IsInCellSet = EIE_TRUE;
			IsInternal = Cell->IsInternal;
		}
		else {
			IsInCellSet = EIE_FALSE;
			IsInternal = IsCoordinateInternal (HTreeRoot, WorkingCoordinate);
		}
#ifdef _DEBUG
		if (DEBUGCMC) {
			if (IsInternal)
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  internal, ");
			else
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " !internal, ");
			if (IsInCellSet)
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  in CellSet\n");
			else
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " !in CellSet\n");
		}
#endif		
		if (IsInternal && IsInCellSet) {
			//add to MarginalCellSet
			rc = STC_CellSetAddLast (MarginalCellSet, Cell);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
		else if (IsInternal && !IsInCellSet) {
			//nothing to do
		}
		else if (!IsInternal && IsInCellSet) {
			SubMarginalCellSet->NumberEntries = 0;
			rc = GetInternalCells (HTreeRoot, WorkingCoordinate, KdTree, SubMarginalCellSet,
				SmallestCoordinate, LargestCoordinate);
			if (rc != EIE_SUCCEED) return EIE_FAIL;

			if (SubMarginalCellSet->NumberEntries > 0) {
				SubMarginalCell = STC_CellSetSensitivity (SubMarginalCellSet, SRule, NULL);

				if (SubMarginalCell->TotalNumberObservations == 0) {
					STC_CellFree (SubMarginalCell);
				}
				else {
					Cell->Data.Item = SubMarginalCell->Data.Item;
					Cell->Data.NumberAllocated = SubMarginalCell->Data.NumberAllocated;
					Cell->Data.NumberEntries = SubMarginalCell->Data.NumberEntries;
					SubMarginalCell->Data.Item = NULL;
					SubMarginalCell->Data.NumberAllocated = 0;
					SubMarginalCell->Data.NumberEntries = 0;
					STC_CellFree (SubMarginalCell);
					//add to MarginalCellSet
					rc = STC_CellSetAddLast (MarginalCellSet, Cell);
					if (rc != EIE_SUCCEED) return EIE_FAIL;
				}
			}
			SubMarginalCellSet->NumberEntries = 0;
		}
		else {//(!IsInternal && !IsInCellSet)
			SubMarginalCellSet->NumberEntries = 0;
			rc = GetInternalCells (HTreeRoot, WorkingCoordinate, KdTree, SubMarginalCellSet,
				SmallestCoordinate, LargestCoordinate);
			if (rc != EIE_SUCCEED) return EIE_FAIL;

			if (SubMarginalCellSet->NumberEntries > 0) {
				SubMarginalCell = STC_CellSetSensitivity (SubMarginalCellSet, SRule, NULL);

				if (SubMarginalCell->TotalNumberObservations == 0) {
					STC_CellFree (SubMarginalCell);
				}
				else {
					SubMarginalCell->Coordinate = STC_CoordinateDuplicate (WorkingCoordinate);
					//add to MarginalCellSet
					rc = STC_CellSetAddLast (MarginalCellSet, SubMarginalCell);
					if (rc != EIE_SUCCEED) return EIE_FAIL;
					//add to global CellSet and KdTree
					rc = STC_CellSetAddLast (CellSet, SubMarginalCell);
					if (rc != EIE_SUCCEED) return EIE_FAIL;
					kdtree = STC_KdTreeInsert (KdTree, SubMarginalCell);
					if (kdtree == NULL) return EIE_FAIL;
				}
			}
			SubMarginalCellSet->NumberEntries = 0;
		}
	}

	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MarginalCellSet has %d Cells\n", MarginalCellSet->NumberEntries);

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
find the decomposition and dimension with the minimum # of branches
------------------------------------------------------------------------------*/
static void FindSmallestCellSetCoordinates (
	STCT_HTREEROOT * HTreeRoot,
	STCT_COORDINATE * Coordinate,
	int * SmallestNumberBranches,
	int * DecompositionWithSmallestNumberBranches,
	int * DimensionWithSmallestNumberBranches)
{
	int i, j;
	STCT_HTREE * Node;

	*SmallestNumberBranches = INT_MAX;
	*DecompositionWithSmallestNumberBranches = 0;
	*DimensionWithSmallestNumberBranches = 0;
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		Node = HTreeRoot->TagIndex[i][Coordinate->Tag[i]];
		if (Node->NumberDecompositions > 0) {
			for (j = 0; j < Node->NumberDecompositions; j++) {
				if (Node->Decomposition[j]->NumberEntries < *SmallestNumberBranches) {
					*SmallestNumberBranches = Node->Decomposition[j]->NumberEntries;
					*DecompositionWithSmallestNumberBranches = j;
					*DimensionWithSmallestNumberBranches = i;
				}
			}
		}
	}
	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY,
	//	"DecompositionWithSmallestNumberBranches=%d DimensionWithSmallestNumberBranches=%d SmallestNumberBranches=%d\n",
	//	*DecompositionWithSmallestNumberBranches, *DimensionWithSmallestNumberBranches, *SmallestNumberBranches);
}
/*------------------------------------------------------------------------------
Get the value of N. Check spec.
------------------------------------------------------------------------------*/
static int GetN (
	STCT_SRULE * SRule,
	int NN)
{
	return 5;
}
/*------------------------------------------------------------------------------
Get the value of NN. Check spec.
------------------------------------------------------------------------------*/
static int GetNN (
	STCT_SRULE * SRule)
{
	int i;
	int NN;

	switch (SRule->Type) {
	case STCE_SRULE_TYPE_ARB:
		NN = SRule->NumberEntries[0];
		break;
#ifdef STC_C2_IS_SUPPORTED
#include "internal_code/src/STC_Build_h1.h"
#endif
#ifdef STC_DUFFETT_IS_SUPPORTED
#include "internal_code/src/STC_Build_h2.h"
#endif
	case STCE_SRULE_TYPE_NK:
		NN = 2;
		for (i = 0; i < SRule->NumberGroups; i++)
			if (SRule->NumberEntries[i] > NN)
				NN = SRule->NumberEntries[i] + 1;
		break;
	/* case STCE_SRULE_TYPE_NKREQ:                            */
	/* 	NN = 2;                                           */
	/* 	for (i = 0; i < SRule->NumberGroups; i++)         */
	/* 		if (SRule->NumberEntries[i] > NN)         */
	/* 			NN = SRule->NumberEntries[i] + 1; */
	/* 	break;                                            */
	case STCE_SRULE_TYPE_PQ:
		NN = 2;
		break;
	}

	return NN;
}
/*------------------------------------------------------------------------------
Find the coordinates necessary to locate the internal cells that contain the data
for a marginal cell
------------------------------------------------------------------------------*/
static void GetSearchCoordinate (
	STCT_HTREEROOT * HTreeRoot,
	STCT_COORDINATE * Coordinate,
	STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate)
{
	int i;
	STCT_HTREE * HTree;
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		HTree = HTreeRoot->TagIndex[i][Coordinate->Tag[i]];

		if (HTree->NumberDecompositions == 0) {
			SmallestCoordinate->Tag[i] = HTree->Tag;
			LargestCoordinate->Tag[i] = HTree->Tag;
		}
		else {
			SmallestCoordinate->Tag[i] = HTree->SmallestTag[0];//use the first decomposition
			LargestCoordinate->Tag[i] = HTree->LargestTag[0];//use the first decomposition
		}
	}
}
/*------------------------------------------------------------------------------
Get all the cells forming a marginal cell
------------------------------------------------------------------------------*/
static EIT_RETURNCODE GetInternalCells (
	STCT_HTREEROOT * HTreeRoot,
	STCT_COORDINATE * Coordinate,
	STCT_KDTREE * KdTree,
	STCT_CELLSET * CellSet,
	STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate)
{
	EIT_RETURNCODE rc;

	// find range of cells forming this agggregate cell
	GetSearchCoordinate (HTreeRoot, Coordinate, SmallestCoordinate, LargestCoordinate);
#ifdef _DEBUG
	if (DEBUG) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "CellCoordinate ");
		STC_CoordinatePrint (Coordinate, HTreeRoot);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "SmallestCoordinate");
		STC_CoordinatePrint (SmallestCoordinate, HTreeRoot);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "LargestCoordinate");
		STC_CoordinatePrint (LargestCoordinate, HTreeRoot);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	}
#endif
	// add to range of cells to the aggregate cell
	rc = STC_KdTreeRangeSearch (KdTree, SmallestCoordinate, LargestCoordinate, CellSet);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
#ifdef _DEBUG
	if (DEBUG) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\navant RemoveUnwantedCells()\n");
		STC_CellSetPrintInfo (CellSet);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "faite de %d cellules internals\n", CellSet->NumberEntries);
	}
#endif
	if (CellSet->NumberEntries != 0) {
		if (HTreeRoot->HasMultipleDecompositions) {
			rc = RemoveUnwantedCells (CellSet, HTreeRoot, Coordinate);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
#ifdef _DEBUG
			if (DEBUG) {
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "apres RemoveUnwantedCells()\n");
				STC_CellSetPrintInfo (CellSet);
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY,
					"faite de %d cellules internals\n\n\n\n", CellSet->NumberEntries);
			}
#endif
		}
#ifdef _DEBUG
		else {
			if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "RemoveUnwantedCells () not called\n");
		}
		if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "ne=%d/na=%d\n", CellSet->NumberEntries, CellSet->NumberAllocated);
#endif
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
A coordinate is internal if all its tags are leaves of the hierarchy
------------------------------------------------------------------------------*/
static EIT_BOOLEAN IsCoordinateInternal (
	STCT_HTREEROOT * HTreeRoot,
	STCT_COORDINATE * Coordinate)
{
	int i;
	for (i = 0; i < Coordinate->NumberEntries; i++)
		if (HTreeRoot->TagIndex[i][Coordinate->Tag[i]]->NumberDecompositions > 0)
			return EIE_FALSE;
	return EIE_TRUE;
}
/*------------------------------------------------------------------------------
allocate the range array
------------------------------------------------------------------------------*/
static STCT_RANGE ** RangeArrayAllocate (
	int n)
{
	int i;
	STCT_RANGE ** RangeArray;
	RangeArray = STC_AllocateMemory (n * sizeof *RangeArray);
	if (RangeArray == NULL) return NULL;
	for (i = 0; i < n; i++) {
		RangeArray[i] = STC_RangeAllocate ();
		if (RangeArray[i] == NULL) return NULL;
	}
	return RangeArray;
}
/*------------------------------------------------------------------------------
free the range array
------------------------------------------------------------------------------*/
static void RangeArrayFree (
	STCT_RANGE * RangeArray[],
	int n)
{
	int i;
	for (i = 0; i < n; i++) {
		STC_RangeFree (RangeArray[i]);
	}
	STC_FreeMemory (RangeArray);
}
/*------------------------------------------------------------------------------
2 decompositions must have the same end codes
------------------------------------------------------------------------------*/
static EIT_BOOLEAN RangeEqual (
	STCT_RANGE * R1,
	STCT_RANGE * R2)
{
	int i;

	if (R1->NumberEntries != R2->NumberEntries) return EIE_FALSE;

	for (i = 0; i < R1->NumberEntries; i++) {
		if ((R1->Item[i]->Lo == STCM_LOHI_NOT_SET && R2->Item[i]->Lo != STCM_LOHI_NOT_SET) ||
			(R1->Item[i]->Lo != STCM_LOHI_NOT_SET && R2->Item[i]->Lo == STCM_LOHI_NOT_SET))
			return EIE_FALSE;
		if (R1->Item[i]->Lo == STCM_LOHI_NOT_SET) {
			if (strcmp (R1->Item[i]->Data, R2->Item[i]->Data) != 0) {
				return EIE_FALSE;
			}
		}
		else {
			if (R1->Item[i]->Lo != R2->Item[i]->Lo || R1->Item[i]->Hi != R2->Item[i]->Hi) {
				return EIE_FALSE;
			}
		}
	}

	return EIE_TRUE;
}
/*------------------------------------------------------------------------------
Get all codes for a range item
------------------------------------------------------------------------------*/
static EIT_RETURNCODE RangeSearchFromRangesToCodes (
	STCT_RANGE * Range,
	char * Code,
	tSList * Codes)
{
	int i;
	EIT_BOOLEAN IsNumeric;
	int nCode;

	IsNumeric = UTIL_IsNumeric (Code, &nCode);

	if (IsNumeric) {
		for (i = 0; i < Range->NumberEntries; i++) {
			/* Lo <= Code <= Hi */
			if (Range->Item[i]->Lo <= nCode && Range->Item[i]->Hi >= nCode) {			
				if (SList_Add (Range->Item[i]->Code, Codes) == eSListFail)
					return EIE_FAIL;
			}
			else if (Range->Item[i]->Lo > nCode)
				return EIE_SUCCEED;
		}
	}
	else {
		for (i = 0; i < Range->NumberEntries; i++) {
			if (strcmp (Range->Item[i]->Data, Code) == 0) {
				if (SList_Add (Range->Item[i]->Code, Codes) == eSListFail)
					return EIE_FAIL;
			}
			else if (strcmp (Range->Item[i]->Data, Code) > 0)
				return EIE_SUCCEED;
		}
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Remove cells from cell set that are not part of the marginal cell.

because STC_KdTreeRangeSearch() finds all cells between SmallestCoordinate and
LargestCoordinate, we must remove the "surplus" cells
------------------------------------------------------------------------------*/
static EIT_RETURNCODE RemoveUnwantedCells (
	STCT_CELLSET * AggregateCellSet,
	STCT_HTREEROOT * HTreeRoot,
	STCT_COORDINATE * Coordinate)
{
	STCT_CELL * Cell2;
	STCT_HTREE * HTree;
	int i, j;
	tIList * List;
	EIT_RETURNCODE rc;

	IList_New (&List);
	if (List == NULL) return EIE_FAIL;
	for (i = 0; i < Coordinate->NumberEntries; i++) {
		IList_Empty (List);
		HTree = HTreeRoot->TagIndex[i][Coordinate->Tag[i]];
		rc = STC_HTreeGetInternalTags (HTree, 0, List);
		if (rc != EIE_SUCCEED) {
			IList_Free (List);
			return EIE_FAIL;
		}

		IList_Sort (List, eIListSortAscending);

		for (j = AggregateCellSet->NumberEntries-1; j >= 0; j--) {
			Cell2 = AggregateCellSet->Cell[j];
			if (IList_SearchBinary (List, Cell2->Coordinate->Tag[i]) == ILIST_NOTFOUND) {
				STC_CellSetRemoveAtIndex (AggregateCellSet, j);
			}
		}
	}
	IList_Free (List);
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
search largest respondents i.
------------------------------------------------------------------------------*/
static int SearchLargestRespondents (
	STCT_DATA * Data,
	char * Key)
{
	int low, high, mid, rc;

    if (Data == NULL) return -1;

	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "looking for %s\n", Key);
	low = 0;
	high = Data->NumberEntries-1;
	while (low <= high) {
		mid = (low + high) / 2;
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  compare with looking for %s\n", Data->Item[mid]->Key);
		rc = strcmp (Key, Data->Item[mid]->Key);
		if (rc == 0)
			return mid;
		else if (rc > 0)
			low = mid + 1;
		else
			high = mid - 1;
	}

    return -1;
}
/*------------------------------------------------------------------------------
set largest respondents.
------------------------------------------------------------------------------*/
static void SetLargestRespondents (
	STCT_CELL * d,
	STCT_CELL * s)
{
	int i;
	int Index;

	d->LargestNumberEntries = s->LargestNumberEntries;
	for (i = 0; i < s->LargestNumberEntries; i++) {
		Index = SearchLargestRespondents (&s->Data, s->Largest[i]->Key);//can't return -1, because I know it exists
		/*-???I think the immediately following line is in error, and should instead be */
		/*     d->Largest[i] = d->Data.Item[Index];                                     */
		/* --no, it's fine, because when this function is called it is always the case  */
		/* that the "STCT_DATAITEMS" in in "d->Data.Item" are identical to the ones in  */
		/* "s->Data.Item" (i.e. the pointers in "d->Data.Item" have the same values as  */
		/* the ones in "s->Data.Item", because "d->Data.Item" was assigned a shallow    */
		/* copy of "s->Data.Item")                                                      */
		/* :                                                                            */
		d->Largest[i] = s->Data.Item[Index];
	}
}
