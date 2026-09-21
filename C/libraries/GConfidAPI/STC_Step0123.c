/*
Treatment of Aggregated Sensitive Cells
See document Treatment of Aggregated Sensitive Cells
by Jean-Louis Tambay, Ioana Schiopu-Kratina , Jean-Marc Fillion, Sylvain Poirier
*/
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EI_Message.h"
#include "MessageGConfidAPI.h"
#include "STC_Build.h"
#include "STC_Cell.h"
#include "STC_Coordinate.h"
#include "STC_SRule.h"
#include "STC_Step0123.h"
#include "STC_Write.h"

/*
set DEBUG to 1 to activate the debugging print statements.
set DEBUG to 0 to deactivate the debugging print statements.
If DEBUG is zero, most compilers will not generate any code for the debugging
statements.
*/
enum {DEBUG = 0};

enum tGoDeeperReturn {
	eGoDeeperFail,
	eGoDeeperGoDeeper,
	eGoDeeperDontGoDeeper
};
typedef enum tGoDeeperReturn tGoDeeperReturn;

static EIT_BOOLEAN AllComboNonSsContainsOneOfLargest (STCT_CELL * ComboAsCell,
	STCT_CELLSET * Combo, int MssNumberEntries, int N, int NN, char WhichNoise);
static STCT_CELLSET * CellSetMerge (STCT_CELLSET * Src1, STCT_CELLSET * Src2);
static void CellSetReverseOrder (STCT_CELLSET * CellSet);
static tGoDeeperReturn GoDeeper (STCT_CELL * SegmentAsCell, STCT_CELLSET * Mss,
	STCT_CELLSET * Combo, STCT_CELLSET * NonSs, STCT_SRULE * SRule,
	STCT_HTREEROOT * HTreeRoot, int M, double X,
	int N, int NN, int NumberNonMssNonZeroNotConsidered, int NumberDimensions,
	int * NumberCombinaisonsCalculated, int * NumberCombinaisonsSensitive);
static STCT_DATAITEM ** SearchKey (STCT_DATA * Data, size_t NumberEntries, char * Key);
static int SearchKeyCompare (const void * ppi1, const void * ppi2);
static EIT_RETURNCODE Traverse (STCT_CELL * SegmentAsCell, STCT_CELLSET * Mss,
	STCT_CELLSET * Combo, STCT_CELLSET * NonSs, STCT_SRULE * SRule,
	STCT_HTREEROOT * HTreeRoot, int M, double X,
	int N, int NN, int NumberNonMssNonZeroNotConsidered, int NumberDimensions,
	int * NumberCombinaisonsCalculated, int * NumberCombinaisonsSensitive);
static EIT_RETURNCODE WriteSensitiveAggregate (STCT_CELLSET * CellSet,
	STCT_CELL * CellSetAsCell, int NumberDimensions, STCT_SRULE * SRule);


//static int mTraverseCounter;
static int mVerbose1;
static int mVerbose2;


#define PRINTMESSAGE(...)\
	do {\
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, __VA_ARGS__);\
	} while (0)


/*------------------------------------------------------------------------------
Does treatment of Aggregated Sensitive Cells
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_Step0123 (
	STCT_CELLSET * Segment,
	STCT_SRULE * SRule,
	STCT_HTREEROOT * HTreeRoot,
	int M,
	double X,
	double Y,
	double Z,
	int N,
	int NN,
	int Verbose,
	int NumberDimensions,
	int * NumberCombinaisonsCalculated,
	int * NumberCombinaisonsSensitive)
{
	double l, c, r, d; //left, center, right, temp
	STCT_CELL * CurrentCell = NULL;
	int i;
	STCT_CELLSET * Mss = NULL;
	STCT_CELLSET * MssDup = NULL;
	STCT_CELL * MssAsCell = NULL;
	STCT_CELL * MssPlusCurrentCellAsCell = NULL;
	STCT_CELLSET * NonMss = NULL;
	STCT_CELLSET * NonMssToConsider = NULL;
	int NumberNonMssNonZero;
	int NumberSensitive;
	EIT_RETURNCODE rc;
	EIT_RETURNCODE RetCode;
	STCT_CELL * SegmentAsCell = NULL;
	char WhichNoise; /*-allowed values: 'v'=ValueX, 'p'=ProxyX, 'V'=WeightedValueX, 'P'=WeightedProxyX, 'n'=ValueN, 'N'=ProxyN */

	RetCode                  = EIE_SUCCEED;
	CurrentCell              = NULL;
	Mss                      = NULL;
	MssDup                   = NULL;
	MssAsCell                = NULL;
	MssPlusCurrentCellAsCell = NULL;
	NonMss                   = NULL;
	NonMssToConsider         = NULL;
	SegmentAsCell            = NULL;

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

	mVerbose1 = Verbose & 1;
	mVerbose2 = Verbose & 2;

	if (mVerbose1 || mVerbose2) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "**************************************************");
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "processing segment (%d): ", Segment->NumberEntries);
		STC_CellSetPrintCoordinate (Segment, HTreeRoot);
	}

	// STEP 0
	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Step 0");

	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify Segment->NumberEntries (%d) > 2", Segment->NumberEntries);
	if (!(Segment->NumberEntries > 2)) {
		if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because !Segment->NumberEntries (%d) > 2", Segment->NumberEntries);
		goto FREE;
	}

	// STEP 1
	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Step 1");

	//check if at least one cell is sensitive
	//check if at least one cell nonsensitive and nonzero
	NumberSensitive = 0;
	NumberNonMssNonZero = 0;
	for (i = 0; i < Segment->NumberEntries; i++) {
		if (STC_CellIsSensitive (Segment->Cell[i])) //sensitive
			NumberSensitive++;
		else if (SELECT_CE_VAL2(Segment->Cell[i], WhichNoise) != 0.0) //nonsensitive
			NumberNonMssNonZero++;//nonzero		
	}
	//check if at least one cell is sensitive
	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify that there is at least one sensitive cell in segment (%d)", NumberSensitive);
	if (NumberSensitive == 0 ) {
		if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because there is no sensitive cell in segment (%d)", NumberSensitive);
		goto FREE;
	}
	//check if at least one cell nonsensitive and nonzero
	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify that there is at least one non sensitive non zero cell in segment (%d)", NumberNonMssNonZero);
	if (NumberNonMssNonZero == 0) {
		if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because there is no non sensitive non zero cell in segment (%d)", NumberNonMssNonZero);
		goto FREE;
	}

	Mss = STC_CellSetAllocate (Segment->NumberEntries);
	if (Mss == NULL) {RetCode = EIE_FAIL; goto error_cleanup;}
	NonMss = STC_CellSetAllocate (Segment->NumberEntries);
	if (NonMss == NULL) {RetCode = EIE_FAIL; goto error_cleanup;}
	for (i = 0; i < Segment->NumberEntries; i++) {
		if (STC_CellIsSensitive (Segment->Cell[i])) {
			rc = STC_CellSetAddLast (Mss, Segment->Cell[i]);
			if (rc != EIE_SUCCEED) {RetCode = EIE_FAIL; goto error_cleanup;}
		}
		else {
			rc = STC_CellSetAddLast (NonMss, Segment->Cell[i]);
			if (rc != EIE_SUCCEED) {RetCode = EIE_FAIL; goto error_cleanup;}
		}
	}

	if (mVerbose1 || mVerbose2) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Mss (%d): ", Mss->NumberEntries);
		STC_CellSetPrintCoordinate (Mss, HTreeRoot);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "NonMss (%d): ", NonMss->NumberEntries);
		STC_CellSetPrintCoordinate (NonMss, HTreeRoot);
	}

	MssAsCell = STC_CellSetSensitivity (Mss, SRule, NULL);
	if (MssAsCell == NULL) {RetCode = EIE_FAIL; goto error_cleanup;}
	(*NumberCombinaisonsCalculated)++;
	if (Mss->NumberEntries > 1) {
		if (mVerbose1) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    Mss contain more than one cell");
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify Mss sensitivity");
			STC_CellPrintInfo (MssAsCell, 1);
		}
		if (STC_CellIsSensitive (MssAsCell)) {
			if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Mss is sensitive: add to aggregate file");
			(*NumberCombinaisonsSensitive)++;
			MssAsCell->FavCost = SELECT_CE_VAL2(MssAsCell, WhichNoise);
			rc = WriteSensitiveAggregate (Mss, MssAsCell, NumberDimensions, SRule);
			if (rc != EIE_SUCCEED) {RetCode = EIE_FAIL; goto error_cleanup;}
		}
		else {
			if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because Mss is not sensitive");
			goto FREE;
		}
	}
	else
		if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Mss->NumberEntries == 1");

	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify NumberNonMssNonZero (%d) > 1", NumberNonMssNonZero);
	if (!(NumberNonMssNonZero > 1)) {
		if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because !NumberNonMssNonZero (%d) > 1", NumberNonMssNonZero);
		goto FREE;
	}

	SegmentAsCell = STC_CellSetSensitivity (Segment, SRule, NULL);
	if (SegmentAsCell == NULL) {RetCode = EIE_FAIL; goto error_cleanup;}

	if (mVerbose1) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Segment");
		STC_CellPrintInfo (SegmentAsCell, 1);
	}

	// B
	//if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "check this: B");
	d = MssAsCell->Sensitivity - SegmentAsCell->Sensitivity;
	l = (1.0 - (X / 100.0)) * d;
	/* if (SRule->Parms->AdditiveNoise) {                                                                                                                                    */
	c = SELECT_CE_VAL2(SegmentAsCell, WhichNoise) - SELECT_CE_VAL2(MssAsCell, WhichNoise);
	/* }                                                                                                                                                                     */
	/* else {                                                                                                                                                                */
	/*     c = ((WhichNoise=='v')?                     fabs(SegmentAsCell->TotalValue         - MssAsCell->TotalValue        )                                               */
	/*         :(WhichNoise=='V')?                     fabs(SegmentAsCell->TotalWeightedValue - MssAsCell->TotalWeightedValue)                                               */
	/*         :(WhichNoise=='p')?((                      fabs(SegmentAsCell->TotalValue         - MssAsCell->TotalValue        ) >=                                         */
	/*                               (SRule->Parms->ProxyRatio * fabs(SegmentAsCell->TotalProxy         - MssAsCell->TotalProxy        ))                                    */
	/*                             )?(                    fabs(SegmentAsCell->TotalValue         - MssAsCell->TotalValue        ))                                           */
	/*                              :(SRule->Parms->ProxyRatio * fabs(SegmentAsCell->TotalProxy         - MssAsCell->TotalProxy        ))                                    */
	/*                            )                                                                                                                                          */
	/*         :(WhichNoise=='P')?((                      fabs(SegmentAsCell->TotalValue         - MssAsCell->TotalValue        ) >=                                         */
	/*                               (SRule->Parms->ProxyRatio * fabs(SegmentAsCell->TotalProxy         - MssAsCell->TotalProxy        ))                                    */
	/*                             )?(                    fabs(SegmentAsCell->TotalWeightedValue - MssAsCell->TotalWeightedValue))                                           */
	/*                              :(SRule->Parms->ProxyRatio * fabs(SegmentAsCell->TotalWeightedProxy - MssAsCell->TotalWeightedProxy))                                    */
	/*                            )                                                                                                                                          */
	/*         :(WhichNoise=='n')?                     fabs(SegmentAsCell->TotalWeightedValue - MssAsCell->TotalWeightedValue)                                               */
	/*         :(WhichNoise=='N')?                     fabs(SegmentAsCell->TotalWeightedProxy - MssAsCell->TotalWeightedProxy)                                               */
	/*         :report_error_in_conditional_expression_returning_double("Invalid value '%c' for WhichNoise in conditional expression in call to STC_Step0123()", WhichNoise) */
	/*         );                                                                                                                                                            */
	/* }                                                                                                                                                                     */
	r = (1.0 + (X / 100.0)) * d;
	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify %f <= %f <= %f", l, c, r);
	if (EIM_DBL_LE (l, c) && EIM_DBL_LE (c, r)) {//rene n'utilise pas EIM_DBL_LE... check partout.
		if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because %f <= %f <= %f", l, c, r);
		goto FREE;
	}

	// STEP 2
	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Step 2 -- which cells do we keep");

	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify M (%d) > 0", M);
	if (!(M > 0)) {
		if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because !M (%d) > 0", M);
		goto FREE;
	}

	NonMssToConsider = STC_CellSetAllocate (NonMss->NumberEntries);
	if (NonMssToConsider == NULL) {RetCode = EIE_FAIL; goto error_cleanup;}

	for (i = 0; i < NonMss->NumberEntries; i++) {
		CurrentCell = NonMss->Cell[i];

		if (mVerbose1 || mVerbose2) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\nChecking cell %d of NonMss: ", i+1);
			STC_CoordinatePrint (CurrentCell->Coordinate, HTreeRoot);
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");
		}

		if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify anonymous only");
		if (STC_CellHasAnonymousOnly (CurrentCell) != EIE_FALSE) {
			if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    Cell not kept because it contains anonymous only");
			continue;
		}

		l = SELECT_CE_VAL2(CurrentCell, WhichNoise);
		r = Y / 100.0 * MssAsCell->Sensitivity;
		if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify not too small: %f <= %f", l, r);
		if (EIM_DBL_LE (l, r)) {
			if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    Cell not kept because it is too small: %f <= %f", l, r);
			continue;
		}

		l = SELECT_DI_VAL2(CurrentCell->Largest[0], WhichNoise);
		r = Z / 100.0 * SELECT_CE_VAL2(CurrentCell, WhichNoise);
		if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify low dominance: %f <= %f", l, r);
		if (EIM_DBL_LE (l, r)) {
			if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    Cell not kept because it has low dominance: %f <= %f", l, r);
			continue;
		}

		//rene: test
		//rene: qu est ce qui est plus vite... meme valeur calcule 2 fois
		//UTIL_ShowTime ("init");
		MssPlusCurrentCellAsCell = STC_CellAllocate (MssAsCell->Data.NumberEntries+CurrentCell->Data.NumberEntries);
		if (MssPlusCurrentCellAsCell == NULL) {RetCode = EIE_FAIL; goto error_cleanup;}
		rc = STC_CellConcat (MssPlusCurrentCellAsCell, MssAsCell);
		if (rc != EIE_SUCCEED) {RetCode = EIE_FAIL; goto error_cleanup;}
		rc = STC_CellConcat (MssPlusCurrentCellAsCell, CurrentCell);
		if (rc != EIE_SUCCEED) {RetCode = EIE_FAIL; goto error_cleanup;}
		rc = STC_CellSensitivity (MssPlusCurrentCellAsCell, SRule, NULL);
		if (rc != EIE_SUCCEED) {RetCode = EIE_FAIL; goto error_cleanup;}
		(*NumberCombinaisonsCalculated)++;
		//UTIL_ShowTime ("Concat");
		//if (DEBUG) STC_CellPrint (MssPlusCurrentCellAsCell);

		//MssPlusCurrentCellAsCell = STC_CellMerge (MssAsCell, CurrentCell);
		//if (MssPlusCurrentCellAsCell == NULL) {RetCode = EIE_FAIL; goto error_cleanup;}
		//rc = STC_CellSensitivity (MssPlusCurrentCellAsCell, SRule);
		//if (rc != EIE_SUCCEED) {RetCode = EIE_FAIL; goto error_cleanup;}
		//UTIL_ShowTime ("Merge");
		//if (DEBUG) STC_CellPrint (MssPlusCurrentCellAsCell);

		//rene: fin test

		if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify union of Mss + current cell is sensitive: sensitivity %f", MssPlusCurrentCellAsCell->Sensitivity);
		if (!STC_CellIsSensitive (MssPlusCurrentCellAsCell)) {
			if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    Cell not kept because Mss + current cell is not sensitive: sensitivity %f", MssPlusCurrentCellAsCell->Sensitivity);
			STC_CellFree (MssPlusCurrentCellAsCell);
			MssPlusCurrentCellAsCell = NULL;
			continue;
		}

		//if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "check this: C");
		d = MssAsCell->Sensitivity - MssPlusCurrentCellAsCell->Sensitivity;
		l = (1.0 - (X / 100.0)) * d;
		c = SELECT_CE_VAL2(CurrentCell, WhichNoise);
		r = (1.0 + (X / 100.0)) * d;
		if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify that Mss + current cell between %f > %f > %f", l, c, r);
		if (EIM_DBL_GT (l, c) || EIM_DBL_GT (c, r)) {
			rc = STC_CellSetAddLast (Mss, CurrentCell);
			if (rc != EIE_SUCCEED) {RetCode = EIE_FAIL; goto error_cleanup;}
			if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Mss + current cell not between %f > %f > %f: add to aggregate file", l, c, r);
			(*NumberCombinaisonsSensitive)++;
			MssPlusCurrentCellAsCell->FavCost = SELECT_CE_VAL2(MssPlusCurrentCellAsCell, WhichNoise);
			rc = WriteSensitiveAggregate (Mss, MssPlusCurrentCellAsCell, NumberDimensions, SRule);
			if (rc != EIE_SUCCEED) {RetCode = EIE_FAIL; goto error_cleanup;}
			STC_CellSetRemoveLast (Mss);
		}

		if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify M (%d) > 1", M);
		if (!(M > 1)) {
			if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    Cell not kept because !M (%d) > 1", M);
			STC_CellFree (MssPlusCurrentCellAsCell);
			MssPlusCurrentCellAsCell = NULL;
			continue;
		}

		if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify NumberNonMssNonZero (%d) > 2", NumberNonMssNonZero);
		if (!(NumberNonMssNonZero > 2)) {
			if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    Cell not kept because !NumberNonMssNonZero (%d) > 2", NumberNonMssNonZero);
			STC_CellFree (MssPlusCurrentCellAsCell);
			MssPlusCurrentCellAsCell = NULL;
			continue;
		}

		//if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "check this: D");
		d = MssPlusCurrentCellAsCell->Sensitivity - SegmentAsCell->Sensitivity;
		l = (1.0 - (X / 100.0)) * d;
		/* if (SRule->Parms->AdditiveNoise) {                                                                                                                                    */
		c = SELECT_CE_VAL2(SegmentAsCell, WhichNoise) - SELECT_CE_VAL2(MssAsCell, WhichNoise) - SELECT_CE_VAL2(CurrentCell, WhichNoise);
		/* }                                                                                                                                                                     */
		/* else {                                                                                                                                                                */
		/*     c = ((WhichNoise=='v')?                     fabs(SegmentAsCell->TotalValue         - MssAsCell->TotalValue         - CurrentCell->TotalValue        )             */
		/*         :(WhichNoise=='V')?                     fabs(SegmentAsCell->TotalWeightedValue - MssAsCell->TotalWeightedValue - CurrentCell->TotalWeightedValue)             */
		/*         :(WhichNoise=='p')?((                      fabs(SegmentAsCell->TotalValue         - MssAsCell->TotalValue         - CurrentCell->TotalValue) >=               */
		/*                               (SRule->Parms->ProxyRatio * fabs(SegmentAsCell->TotalProxy         - MssAsCell->TotalProxy         - CurrentCell->TotalProxy))          */
		/*                             )?(                    fabs(SegmentAsCell->TotalValue         - MssAsCell->TotalValue         - CurrentCell->TotalValue))                 */
		/*                              :(SRule->Parms->ProxyRatio * fabs(SegmentAsCell->TotalProxy         - MssAsCell->TotalProxy         - CurrentCell->TotalProxy))          */
		/*                            )                                                                                                                                          */
		/*         :(WhichNoise=='P')?((                      fabs(SegmentAsCell->TotalValue         - MssAsCell->TotalValue         - CurrentCell->TotalValue        ) >=       */
		/*                               (SRule->Parms->ProxyRatio * fabs(SegmentAsCell->TotalProxy         - MssAsCell->TotalProxy         - CurrentCell->TotalProxy        ))  */
		/*                             )?(                    fabs(SegmentAsCell->TotalWeightedValue - MssAsCell->TotalWeightedValue - CurrentCell->TotalWeightedValue))         */
		/*                              :(SRule->Parms->ProxyRatio * fabs(SegmentAsCell->TotalWeightedProxy - MssAsCell->TotalWeightedProxy - CurrentCell->TotalWeightedProxy))  */
		/*                            )                                                                                                                                          */
		/*         :(WhichNoise=='n')?                     fabs(SegmentAsCell->TotalWeightedValue - MssAsCell->TotalWeightedValue - CurrentCell->TotalWeightedValue)             */
		/*         :(WhichNoise=='N')?                     fabs(SegmentAsCell->TotalWeightedProxy - MssAsCell->TotalWeightedProxy - CurrentCell->TotalWeightedProxy)             */
		/*         :report_error_in_conditional_expression_returning_double("Invalid value '%c' for WhichNoise in conditional expression in call to STC_Step0123()", WhichNoise) */
		/*         );                                                                                                                                                            */
		/* }                                                                                                                                                                     */
		r = (1.0 + (X / 100.0)) * d;
		if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify that %f <= %f <= %f", l, c, r);
		if (EIM_DBL_LE (l, c) && EIM_DBL_LE (c, r)) {
			if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    Cell not kept because %f <= %f <= %f", l, c, r);
			STC_CellFree (MssPlusCurrentCellAsCell);
			MssPlusCurrentCellAsCell = NULL;
			continue;
		}

		STC_CellFree (MssPlusCurrentCellAsCell);
		MssPlusCurrentCellAsCell = NULL;

		if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    Keeping cell %d of NonMss", i+1);
		rc = STC_CellSetAddLast (NonMssToConsider, CurrentCell);
		if (rc != EIE_SUCCEED) {RetCode = EIE_FAIL; goto error_cleanup;}
	}

	CurrentCell = NULL;

	if (mVerbose1) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "NonMssToConsider (%d): ", NonMssToConsider->NumberEntries);
		STC_CellSetPrintCoordinate (NonMssToConsider, HTreeRoot);
	}

	// STEP 3
	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Step 3");

	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify M (%d) > 1", M);
	if (!(M > 1)) {
		if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because !M (%d) > 1", M);
		goto FREE;
	}

	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify NonMssToConsider->NumberEntries (%d) > 1", NonMssToConsider->NumberEntries);
	if (!(NonMssToConsider->NumberEntries > 1)) {
		if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because !NonMssToConsider->NumberEntries (%d) > 1", NonMssToConsider->NumberEntries);
		goto FREE;
	}

	//mTraverseCounter = 0;
	if (mVerbose1 || mVerbose2) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "NonMssToConsider (%d): ", NonMssToConsider->NumberEntries);
		STC_CellSetPrintCoordinate (NonMssToConsider, HTreeRoot);
	}
	CellSetReverseOrder (NonMssToConsider); //optional. see CellSetReverseOrder() comment's.
	MssDup = STC_CellSetShallowDuplicate (Mss);
	if (MssDup == NULL) {RetCode = EIE_FAIL; goto error_cleanup;}

	rc = Traverse (SegmentAsCell, Mss, MssDup, NonMssToConsider, SRule, 
		HTreeRoot, M, X, N, NN, NumberNonMssNonZero, NumberDimensions,
		NumberCombinaisonsCalculated, NumberCombinaisonsSensitive);
	if (rc != EIE_SUCCEED) {RetCode = EIE_FAIL; goto error_cleanup;}
	STC_CellSetShallowFree (MssDup);
	MssDup = NULL;
	//if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "traverse() was called %d times", mTraverseCounter);

goto FREE;

error_cleanup:
	if (MssPlusCurrentCellAsCell != NULL) {STC_CellFree           (MssPlusCurrentCellAsCell); MssPlusCurrentCellAsCell = NULL;}
	if (MssDup                   != NULL) {STC_CellSetShallowFree (MssDup                  ); MssDup                   = NULL;}

FREE:
	// free
	if (MssAsCell        != NULL) {STC_CellFree           (MssAsCell       ); MssAsCell        = NULL;}
	if (SegmentAsCell    != NULL) {STC_CellFree           (SegmentAsCell   ); SegmentAsCell    = NULL;}
	if (Mss              != NULL) {STC_CellSetShallowFree (Mss             ); Mss              = NULL;}
	if (NonMss           != NULL) {STC_CellSetShallowFree (NonMss          ); NonMss           = NULL;}
	if (NonMssToConsider != NULL) {STC_CellSetShallowFree (NonMssToConsider); NonMssToConsider = NULL;}

	return RetCode;
}


/*------------------------------------------------------------------------------
if every non-sensitive cell in the combination has at least one of the largest
NN entreprises of the combination amongs its entreprises and the value is greater
than 0 then output the combination as a sensitive aggregate
------------------------------------------------------------------------------*/
static EIT_BOOLEAN AllComboNonSsContainsOneOfLargest (
	STCT_CELL * ComboAsCell,
	STCT_CELLSET * Combo,
	int MssNumberEntries,
	int N,
	int NN,
	char WhichNoise)
{
	EIT_BOOLEAN found;
	int i, j;
	EIT_BOOLEAN loopfound;
	int MinLargestNumberEntriesAndNN;
	size_t MinDataNumberEntriesAndN;
	STCT_DATAITEM ** pItem;

	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");
	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "ComboAsCell");
	//STC_CellPrint (ComboAsCell);
	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "ComboAsCell largest");
	//STC_CellPrintLargest (ComboAsCell);
	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Combo");
	//STC_CellSetPrint (Combo);

	MinLargestNumberEntriesAndNN = (ComboAsCell->LargestNumberEntries <= NN ? ComboAsCell->LargestNumberEntries : NN);
	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "ComboAsCell->LargestNumberEntries=%d NN=%d MinLargestNumberEntriesAndNN=%d\n",
	//	ComboAsCell->LargestNumberEntries, NN, MinLargestNumberEntriesAndNN);
	for (i = MssNumberEntries; i < Combo->NumberEntries; i++) {
		loopfound = EIE_FALSE;
		for (j = 0; j < MinLargestNumberEntriesAndNN && !loopfound; j++) {

			//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "ComboAsCell->Largest[j]->Key=%s\n", ComboAsCell->Largest[j]->Key);
			//STC_CellPrint (Combo->Cell[i]);

			MinDataNumberEntriesAndN = (Combo->Cell[i]->Data.NumberEntries <= N ? Combo->Cell[i]->Data.NumberEntries : N);
			//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Combo->Cell[i]->Data.NumberEntries=%d N=%d MinDataNumberEntriesAndN=%d\n",
			//	Combo->Cell[i]->Data.NumberEntries, N, MinDataNumberEntriesAndN);
			
			//is Key (one of the largest entreprises of the combination)
			//in this non-sensitive cell and the value is greater than 0
			pItem = SearchKey (&Combo->Cell[i]->Data, MinDataNumberEntriesAndN, ComboAsCell->Largest[j]->Key);
			/* found = ((pItem != NULL && (*pItem)->Value > 0.0) ? EIE_TRUE : EIE_FALSE); */
			found = ((pItem != NULL && SELECT_DI_VAL2((*pItem), WhichNoise) > 0.0) ? EIE_TRUE : EIE_FALSE);
			if (found)
				loopfound = EIE_TRUE;
			//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "found=%d\n", found);
		}
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "loopfound=%d\n", loopfound);
		if (!loopfound)
			return EIE_FALSE;
	}

	return EIE_TRUE;
}
/*------------------------------------------------------------------------------
Creates a CellSet by copying 2 CellSets.
Only the pointers to the cells are copied.
If you free the new CellSet with STC_CellSetFree() you will loose the original Cells.
Use STC_CellSetShallowFree() to free the CellSet without freeing the cells themselves.
------------------------------------------------------------------------------*/
static STCT_CELLSET * CellSetMerge (
	STCT_CELLSET * Src1,
	STCT_CELLSET * Src2)
{
	STCT_CELLSET * Dest;
	int i;
	EIT_RETURNCODE rc;

	Dest = STC_CellSetAllocate (Src1->NumberEntries+Src2->NumberEntries);
	if (Dest == NULL) return NULL;
	for (i = 0; i < Src1->NumberEntries; i++) {
		rc = STC_CellSetAddLast (Dest, Src1->Cell[i]);
		if (rc != EIE_SUCCEED) return NULL;
	}
	for (i = 0; i < Src2->NumberEntries; i++) {
		rc = STC_CellSetAddLast (Dest, Src2->Cell[i]);
		if (rc != EIE_SUCCEED) return NULL;
	}
	return Dest;
}
/*------------------------------------------------------------------------------
reverse the order of the cells in a cellset.
is called just before calling traverse() to change order of NonMssToConsider.
cosmetic only. the combo treated will appear to be treated in a more natural order.
------------------------------------------------------------------------------*/
static void CellSetReverseOrder (
	STCT_CELLSET * CellSet)
{
	int i;
	STCT_CELL * Cell;
	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "CellSet->NumberEntries=%d", CellSet->NumberEntries);
	for (i = 0; i < CellSet->NumberEntries/2; i++) {
		Cell = CellSet->Cell[i];
		CellSet->Cell[i] = CellSet->Cell[CellSet->NumberEntries-i-1];
		CellSet->Cell[CellSet->NumberEntries-i-1] = Cell;
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "switching i %d et CellSet->NumberEntries-i-1 %d", i, CellSet->NumberEntries-i-1);
	}
}
/*------------------------------------------------------------------------------
Check if one more set of cells should be traversed to locate sensitive aggregate
------------------------------------------------------------------------------*/
static tGoDeeperReturn GoDeeper (
	STCT_CELL * SegmentAsCell, // Segment
	STCT_CELLSET * Mss, // Minimal sensitive set
	STCT_CELLSET * Combo, // Combinaison to check
	STCT_CELLSET * NonSs, // Non sensitive set
	STCT_SRULE * SRule,
	STCT_HTREEROOT * HTreeRoot,
	int M,
	double X,
	int N,
	int NN,
	int NumberNonMssNonZeroNotConsidered,
	int NumberDimensions,
	int * NumberCombinaisonsCalculated,
	int * NumberCombinaisonsSensitive)
{
	double d, l, c, r;
	STCT_CELL * ComboAsCell;
	EIT_RETURNCODE rc;
	char WhichNoise; /*-allowed values: 'v'=ValueX, 'p'=ProxyX, 'V'=WeightedValueX, 'P'=WeightedProxyX, 'n'=ValueN, 'N'=ProxyN */

	ComboAsCell = NULL;

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

	if (mVerbose1 || mVerbose2) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\nprocessing combo (%d):", Combo->NumberEntries);
		STC_CellSetPrintCoordinate (Combo, HTreeRoot);
	}

	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify that more than 1 NonSs cell is added to combo");
	if (Combo->NumberEntries == Mss->NumberEntries+1) {
		if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because not more than 1 NonSs cell is added to combo");
		return eGoDeeperGoDeeper;
	}

	(*NumberCombinaisonsCalculated)++;
	ComboAsCell = STC_CellSetSensitivity (Combo, SRule, NULL);
	if (ComboAsCell == NULL) return eGoDeeperFail;

	if (mVerbose1) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "combo info");
		STC_CellPrintInfo (ComboAsCell, 1);
	}

	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify that combo is sensitive (%f)", ComboAsCell->Sensitivity);
	if (!STC_CellIsSensitive (ComboAsCell)) {
		if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because combo is not sensitive (%f)", ComboAsCell->Sensitivity);
		if (ComboAsCell != NULL) {
		    STC_CellFree (ComboAsCell);
		}
		return eGoDeeperDontGoDeeper; // combo not sensitive
	}

	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "ComboAsCell");
	//STC_CellPrint (ComboAsCell);
	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Combo les %d premieres cells ne comptent pas", Mss->NumberEntries);
	//STC_CellSetPrint (Combo);

	if (AllComboNonSsContainsOneOfLargest (ComboAsCell, Combo, Mss->NumberEntries, N, NN, WhichNoise)) {
#ifdef _DEBUG
		if (DEBUG) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "AllNonSsContainOneOfNNLargest(): true");
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "N=%d NN=%d", N, NN);
			//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "ComboAsCell");
			//STC_CellPrint (ComboAsCell);
			//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "NonSs");
			//STC_CellSetPrint (NonSs);
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "      toutes les cellules non sensible contiennent un des NN plus gros: on l'ajoute au aggregate file");
		}
#endif
		if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "every non-sensitive cell in combo has at least one of the NN largest entreprises: add to aggregate file");
		(*NumberCombinaisonsSensitive)++;
		ComboAsCell->FavCost = SELECT_CE_VAL2(ComboAsCell, WhichNoise);
		rc = WriteSensitiveAggregate (Combo, ComboAsCell, NumberDimensions, SRule);
		if (rc != EIE_SUCCEED) {
			if (ComboAsCell != NULL) {
			    STC_CellFree (ComboAsCell);
			}
			return eGoDeeperFail;
		}
	}
#ifdef _DEBUG
	else
		if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "AllNonSsContainOneOfNNLargest(): false");
#endif

	// starting here, decide if we consider other cells to add to this combinaison

	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify that there are still cells to treat (NonSs->NumberEntries=%d)", NonSs->NumberEntries);
	if (NonSs->NumberEntries == 0) {
		if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because no more cell to treat (NonSs->NumberEntries=%d)", NonSs->NumberEntries);
		STC_CellFree (ComboAsCell);
		return eGoDeeperDontGoDeeper; //tried all cells
	}

	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify that we treated (%d) not more than M (%d) NonSs cells", Combo->NumberEntries - Mss->NumberEntries, M);
	if (Combo->NumberEntries - Mss->NumberEntries >= M) {
		if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because we treated (%d) M (%d) NonSs cells", Combo->NumberEntries - Mss->NumberEntries, M);
		STC_CellFree (ComboAsCell);
		return eGoDeeperDontGoDeeper; //tried M cells
	}

	if (mVerbose1) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify that adding one more cell is not the whole segment (NumberNonMssNonZeroNotConsidered=%d)",
			NumberNonMssNonZeroNotConsidered);
	}
	if (NumberNonMssNonZeroNotConsidered == 1) {
		if (mVerbose2) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because adding the last cell is the whole segment (NumberNonMssNonZeroNotConsidered=%d)",
				NumberNonMssNonZeroNotConsidered);
		}
		STC_CellFree (ComboAsCell);
		return eGoDeeperDontGoDeeper; //only one nonzero nonsensitive in segment
	}

	d = ComboAsCell->Sensitivity - SegmentAsCell->Sensitivity;
	l = (1.0 - (X / 100.0)) * d;
	/* if (SRule->Parms->AdditiveNoise) {                                                                                                                                */
	c = SELECT_CE_VAL2(SegmentAsCell, WhichNoise) - SELECT_CE_VAL2(ComboAsCell, WhichNoise);
	/* }                                                                                                                                                                 */
	/* else {                                                                                                                                                            */
	/*     c = ((WhichNoise=='v')?                     fabs(SegmentAsCell->TotalValue         - ComboAsCell->TotalValue        )                                         */
	/*         :(WhichNoise=='V')?                     fabs(SegmentAsCell->TotalWeightedValue - ComboAsCell->TotalWeightedValue)                                         */
	/*         :(WhichNoise=='p')?((                      fabs(SegmentAsCell->TotalValue         - ComboAsCell->TotalValue        ) >=                                   */
	/*                               (SRule->Parms->ProxyRatio * fabs(SegmentAsCell->TotalProxy         - ComboAsCell->TotalProxy        ))                              */
	/*                             )?(                    fabs(SegmentAsCell->TotalValue         - ComboAsCell->TotalValue        ))                                     */
	/*                              :(SRule->Parms->ProxyRatio * fabs(SegmentAsCell->TotalProxy         - ComboAsCell->TotalProxy        ))                              */
	/*                            )                                                                                                                                      */
	/*         :(WhichNoise=='P')?((                      fabs(SegmentAsCell->TotalValue         - ComboAsCell->TotalValue        ) >=                                   */
	/*                               (SRule->Parms->ProxyRatio * fabs(SegmentAsCell->TotalProxy         - ComboAsCell->TotalProxy        ))                              */
	/*                             )?(                    fabs(SegmentAsCell->TotalWeightedValue - ComboAsCell->TotalWeightedValue))                                     */
	/*                              :(SRule->Parms->ProxyRatio * fabs(SegmentAsCell->TotalWeightedProxy - ComboAsCell->TotalWeightedProxy))                              */
	/*                            )                                                                                                                                      */
	/*         :(WhichNoise=='n')?                     fabs(SegmentAsCell->TotalWeightedValue - ComboAsCell->TotalWeightedValue)                                         */
	/*         :(WhichNoise=='N')?                     fabs(SegmentAsCell->TotalWeightedProxy - ComboAsCell->TotalWeightedProxy)                                         */
	/*         :report_error_in_conditional_expression_returning_double("Invalid value '%c' for WhichNoise in conditional expression in call to GoDeeper()", WhichNoise) */
	/*         );                                                                                                                                                        */
	/* }                                                                                                                                                                 */
	r = (1.0 + (X / 100.0)) * d;
	if (mVerbose1) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  verify that %f <= %f <= %f ie within X", l, c, r);
	if ((l <= c) && (c <= r)) {
		if (mVerbose2) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    quit because %f <= %f <= %f !within X", l, c, r);
		STC_CellFree (ComboAsCell);
		return eGoDeeperDontGoDeeper; //!within X%
	}

	//free
	STC_CellFree (ComboAsCell);

	return eGoDeeperGoDeeper;
}
/*------------------------------------------------------------------------------
Binary search to locate a key in a cell
------------------------------------------------------------------------------*/
static STCT_DATAITEM ** SearchKey (
	STCT_DATA * Data,
	size_t NumberEntries,
	char * Key)
{
	STCT_DATAITEM Item;
	STCT_DATAITEM * pItem = &Item;

	Item.Key = Key;

	return bsearch ((void*)&pItem, (void*)&Data->Item[0], NumberEntries, sizeof (STCT_DATAITEM **), SearchKeyCompare);
}
/*------------------------------------------------------------------------------
Comparaison function to locate a key in a cell
------------------------------------------------------------------------------*/
static int SearchKeyCompare (
	const void * ppi1, //pointer to pointer to STCT_DATAITEM
	const void * ppi2)
{
	char * k1 = (*(STCT_DATAITEM **)ppi1)->Key;
	char * k2 = (*(STCT_DATAITEM **)ppi2)->Key;
	//EI_AddMessage ("", 4, "k1=<%s>, k2=<%s>. %d", k1, k2, strcmp (k1, k2));
	return strcmp (k1, k2);
#ifdef NEVERTOBEDEFINED
	int low, high, mid;
	int rc;

	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "looking for %s", Key);

	low = 0;
	high = Data->NumberEntries-1;
	while (low <= high) {
		mid = (low + high) / 2;
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "  compare with looking for %s", Data->Item[mid]->Key);
		rc = strcmp (Key, Data->Item[mid]->Key);
		if (rc == 0)
			return EIE_TRUE;
		else if (rc > 0)
			low = mid + 1;
		else
			high = mid - 1;
	}

	return EIE_FALSE;
#endif
}
/*------------------------------------------------------------------------------
locate sensitive aggregate with one more set of cells
------------------------------------------------------------------------------*/
static EIT_RETURNCODE Traverse (
	STCT_CELL * SegmentAsCell, // Segment
	STCT_CELLSET * Mss, // Minimal sensitive set
	STCT_CELLSET * Combo, // Sensitive set
	STCT_CELLSET * NonSs, // Non sensitive set
	STCT_SRULE * SRule,
	STCT_HTREEROOT * HTreeRoot,
	int M,
	double X,
	int N,
	int NN,
	int NumberNonMssNonZeroNotConsidered,
	int NumberDimensions,
	int * NumberCombinaisonsCalculated,
	int * NumberCombinaisonsSensitive)
{
	//double d, l, c, r;
	STCT_CELL * Cell;
	//STCT_CELLSET * Combo;
	//STCT_CELL * ComboAsCell;
	int i;
	STCT_CELLSET * NonSsDup;
	tGoDeeperReturn rcGoDeeper;
	EIT_RETURNCODE rc;

	Cell     = NULL;
	NonSsDup = NULL;

	//mTraverseCounter++;
	//if (!(mTraverseCounter % 1000)) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "mTraverseCounter %d", mTraverseCounter);

	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Mss->NumberEntries %dSs->NumberEntries %dNonSs->NumberEntries %d",
		//MssNumberEntries, Combo->NumberEntries, NonSs->NumberEntries);

	NonSsDup = STC_CellSetShallowDuplicate (NonSs);
	if (NonSsDup == NULL) return EIE_FAIL;

	NumberNonMssNonZeroNotConsidered--;

	for (i = 0; i < NonSs->NumberEntries; i++) {
		Cell = STC_CellSetRemoveLast (NonSsDup);
		rc = STC_CellSetAddLast (Combo, Cell);
		if (rc != EIE_SUCCEED) {
		    if (NonSsDup != NULL) {
		        STC_CellSetShallowFree (NonSsDup);
		        NonSsDup = NULL;
		    }
		    return EIE_FAIL;
		}

		rcGoDeeper = GoDeeper (SegmentAsCell, Mss, Combo, NonSsDup, SRule, HTreeRoot,
			M, X, N, NN, NumberNonMssNonZeroNotConsidered, NumberDimensions,
			NumberCombinaisonsCalculated, NumberCombinaisonsSensitive);

		switch (rcGoDeeper) {
		case eGoDeeperGoDeeper:
			rc = Traverse (SegmentAsCell, Mss, Combo, NonSsDup, SRule, HTreeRoot,
				M, X, N, NN, NumberNonMssNonZeroNotConsidered, NumberDimensions,
				NumberCombinaisonsCalculated, NumberCombinaisonsSensitive);
			if (rc != EIE_SUCCEED) {
			    if (NonSsDup != NULL) {
			        STC_CellSetShallowFree (NonSsDup);
			        NonSsDup = NULL;
			    }
			    return EIE_FAIL;
			}
			break;

		case eGoDeeperDontGoDeeper:
			/* Nothing to do */
			break;

		case eGoDeeperFail:
			if (NonSsDup != NULL) {
			    STC_CellSetShallowFree (NonSsDup);
			    NonSsDup = NULL;
			}
			return EIE_FAIL;
		}
		STC_CellSetRemoveLast (Combo);
	}
	if (NonSsDup != NULL) {
	    STC_CellSetShallowFree (NonSsDup);
	    NonSsDup = NULL;
	}

	EI_PrintMessages (); //empty the message list. it could get quite large.

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Write a sensitive aggregate to output
------------------------------------------------------------------------------*/
static EIT_RETURNCODE WriteSensitiveAggregate (
	STCT_CELLSET * CellSet,
	STCT_CELL * CellSetAsCell,
	int NumberDimensions,
	STCT_SRULE * SRule)
{
	EIT_RETURNCODE rc;

	rc = STC_WriteConstraint (CellSet, CellSetAsCell);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	CellSetAsCell->Type = STC_CELLTYPE_AGGREGATE;
	rc = STC_WriteCell (CellSetAsCell, NumberDimensions);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	rc = STC_WriteLargest (CellSetAsCell, SRule->waiver_flags_present, SRule->Parms);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	if ((SRule->Parms->Weight != 0 && (SRule->Parms->WeightProtectionLevel != 'E'))) {
		if (SRule->Type == STCE_SRULE_TYPE_NK) {
			rc = STC_WriteTargets(CellSetAsCell, (SRule->waiver_flags_present == 0)?(CellSetAsCell->LargestFTNumberEntries):(CellSetAsCell->LargestFWNumberEntries), (SRule->waiver_flags_present == 0)?(CellSetAsCell->LargestFT):(CellSetAsCell->LargestFW), CellSetAsCell->LargestFSNumberEntries, CellSetAsCell->LargestFS, SRule->NumberEntries[0], 1, SRule->waiver_flags_present);
		}
		else if (SRule->Type == STCE_SRULE_TYPE_PQ) {
			rc = STC_WritePairs  (CellSetAsCell, (SRule->waiver_flags_present == 0)?(CellSetAsCell->LargestFTNumberEntries):(CellSetAsCell->LargestFWNumberEntries), (SRule->waiver_flags_present == 0)?(CellSetAsCell->LargestFT):(CellSetAsCell->LargestFW), CellSetAsCell->LargestFSNumberEntries, CellSetAsCell->LargestFS,                          1, SRule->waiver_flags_present);
		}
		if (SRule->Type != STCE_SRULE_TYPE_NK && SRule->Type != STCE_SRULE_TYPE_PQ && SRule->Type != STCE_SRULE_TYPE_C2 && SRule->Type != STCE_SRULE_TYPE_DUFFETT) {
			IO_PRINT_LINE(M30202);
			rc = EIE_FAIL;
		}
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}

	return EIE_SUCCEED;
}
