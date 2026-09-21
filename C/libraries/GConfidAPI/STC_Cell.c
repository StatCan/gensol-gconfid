#ifdef _DEBUG
#include <assert.h>
#endif
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "EI_Message.h"
#include "STC_Cell.h"
#include "STC_Memory.h"
#include "STC_Write.h"
#include "MessageGConfidAPI.h"
#include "util.h"

#include "ilist.h"
#include "slist.h"

#include "internal_rules.h"

#define FMAX(x,y)  (double)y > (double)x ? (double)y : (double)x

/*
set DEBUG to 1 to activate the debugging print statements.
set DEBUG to 0 to deactivate the debugging print statements.
If DEBUG is zero, most compilers will not generate any code for the debugging
statements.
*/
enum {DEBUG = 0};

#define DEBUGALLOCATION
#undef DEBUGALLOCATION

#define SENSITIVITY_NOT_CALCULATED -2.987e-111

static int CalculateSensitivityVariables(void * StructPtr,
                                         int C,
                                         STCT_SRULE * SRule,
                                         double alphaplus1,
                                         double alphaplus2,
                                         double ProxyRatio,
                                         char WeightProtectionLevel);
static int CalculateFXAndLargestFX (int               ProxySpecifiedParm,
                                    STCT_SRULE *      SRule,
                                    STCT_CELL *       Cell);
static double AddLargest (STCT_DATAITEM ** Largest, int LargestNumberEntries, int n, STCT_DATAITEM * Item,
                          char ValueType,
                          int * SmallestOfTheLargest_waiver_flag_p, int waiver_flags_present);
static EIT_RETURNCODE CellAddItem (STCT_CELL * Cell, STCT_DATAITEM * Item);
static void CountNonAnonymousPositiveNumberEntries (STCT_CELL * Cell);
#ifdef _DEBUG
static void CheckSortKeys (STCT_CELL * Cell);
#endif
static void FindLargest (STCT_CELL * Cell, int n,
                         STCT_DATAITEM ** Largest, int * LargestNumberEntriesPtr, char ValueType,
                         int waiver_flags_present);
static void InitializeCell (STCT_CELL * Cell);
static void InitializeCellSet (STCT_CELLSET * CellSet);
static STCT_CELL * ReallocateCell (STCT_CELL * Cell);
static STCT_CELLSET * ReallocateCellSet (STCT_CELLSET * CellSet);
/* static void RemoveDuplicateKeys (STCT_CELL * Cell); */
/* static void SortKeys (STCT_CELL * Cell); */
static int SortKeysCompare (const void * ppi1, const void * ppi2);

static int mCellId = -123;
static int mConstraintId = -456;

/*------------------------------------------------------------------------------
Add an item to a cell
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_CellAdd (
	STCT_CELL * Cell,
	const char * Key,
	const double Value,
	const double Shadow,
	const double Proxy,
	const double Weight,
	int waiver_flags_present,
	int waiver_flag)
{
	STCT_DATAITEM Item;

	Item.Key = (char *) Key;//set pointer only, CellAddItem() will allocate or not the memory
	Item.NumberObservations = 1;
	Item.Value          = Value                    ;
	Item.Shadow         = Shadow                   ;
	Item.Proxy          = Proxy                    ;
	Item.ValuePt        = 0.0                      ;
	Item.ValuePw        = 0.0                      ;
	Item.ValueN         = 0.0                      ;
	Item.ValueSn        = 0.0                      ;
	Item.ValueX         = 0.0                      ;
	Item.ProxyPt        = 0.0                      ;
	Item.ProxyPw        = 0.0                      ;
	Item.ProxyN         = 0.0                      ;
	Item.ProxySn        = 0.0                      ;
	Item.ProxyX         = 0.0                      ;
	Item.FT             = 0.0                      ;
	Item.FW             = 0.0                      ;
	Item.FS             = 0.0                      ;
	Item.Weight         = Weight                   ;
	Item.WeightedValue  = Item.Weight * Item.Value ;
	Item.WeightedShadow = Item.Weight * Item.Shadow;
	Item.WeightedProxy  = Item.Weight * Item.Proxy ;
	Item.SecondryValuePt= 0.0                      ;
	Item.SecondryValuePw= 0.0                      ;
	Item.SecondryProxyPt= 0.0                      ;
	Item.SecondryProxyPw= 0.0                      ;
	Item.SecondryFT     = 0.0                      ;
	Item.SecondryFW     = 0.0                      ;
	Item.SecondryFS     = 0.0                      ;
	Item.WeightedValueX = 0.0                      ;
	Item.WeightedProxyX = 0.0                      ;

	/* set components for WeightedNbResp calculations */
	Item.AbsVar         = fabs(Item.Value)         ;
	Item.WAbsVar        = Item.Weight * Item.AbsVar;

	/*-Item.MixedSignStatus equal to:                    */
	/*     -1 if all contributing records were <=0.0,    */
	/*      0 if all contributing records were ==0.0,    */
	/*      1 if all contributing records were >=0.0,    */
	/*      2 if some contributing records were <0.0 and */
	/*           some were >0.0                          */
	if      (Item.Value < 0.0) {
		Item.MixedSignStatus = -1;
	}
	else if (Item.Value == 0.0) {
		Item.MixedSignStatus = 0;
	}
	else { /* (Item.Value > 0.0) */
		Item.MixedSignStatus = 1;
	}
	if (waiver_flags_present == 2 || waiver_flags_present == 1) {
		Item.waiver_flag = waiver_flag;
	} else {
		Item.waiver_flag = 0;
	}
	return CellAddItem (Cell, &Item);
}
/*------------------------------------------------------------------------------
Reallocates Cell member of Cellset to the exact number of Cells.
Used mostly to reduce the memory footprint of the cell
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_CellAdjustAllocation (
	STCT_CELL * Cell)
{
	void * Ptr;

	if (Cell->Data.NumberAllocated == Cell->Data.NumberEntries) return EIE_SUCCEED;

#ifdef DEBUGALLOCATION
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "STC_CellAdjustAllocation ne=%d/na=%d\n", Cell->Data.NumberEntries, Cell->Data.NumberAllocated);
#endif
	Ptr = STC_ReallocateMemory (
		Cell->Data.NumberAllocated * sizeof *Cell->Data.Item,
		Cell->Data.NumberEntries * sizeof *Cell->Data.Item,
		Cell->Data.Item);
	if (Ptr == NULL && Cell->Data.NumberEntries != 0)
		return EIE_FAIL;
	Cell->Data.Item = Ptr;
	Cell->Data.NumberAllocated = Cell->Data.NumberEntries;
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Allocate the STCT_CELL structure
------------------------------------------------------------------------------*/
STCT_CELL * STC_CellAllocate (
	const int AllocationIncrement)
{
	STCT_CELL * Cell;
	/* allocate the structure */
	Cell = STC_AllocateMemory (sizeof *Cell);
	if (Cell == NULL) return NULL;
	Cell->Data.AllocationIncrement = (AllocationIncrement < 1 ? 10 : AllocationIncrement);
	InitializeCell (Cell);
	return Cell;
}
/*------------------------------------------------------------------------------
Concatenate the content of 2 cells. Add src to dest.
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_CellConcat (
	STCT_CELL * Dest,
	STCT_CELL * Src)
{
	int i;
	EIT_RETURNCODE rc;

	for (i = 0; i < Src->Data.NumberEntries; i++) {
		rc = CellAddItem (Dest, Src->Data.Item[i]);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}
	Dest->AnonymousDataItem.NumberObservations += Src->AnonymousDataItem.NumberObservations;
	Dest->AnonymousDataItem.Value              += Src->AnonymousDataItem.Value             ;
	Dest->AnonymousDataItem.AbsVar             += Src->AnonymousDataItem.AbsVar            ;
	Dest->AnonymousDataItem.WAbsVar            += Src->AnonymousDataItem.WAbsVar           ;
	Dest->AnonymousDataItem.Shadow             += Src->AnonymousDataItem.Shadow            ;
	Dest->AnonymousDataItem.Proxy              += Src->AnonymousDataItem.Proxy             ;
	Dest->AnonymousDataItem.ValuePt            += Src->AnonymousDataItem.ValuePt           ;
	Dest->AnonymousDataItem.ValuePw            += Src->AnonymousDataItem.ValuePw           ;
	Dest->AnonymousDataItem.ValueN             += Src->AnonymousDataItem.ValueN            ;
	Dest->AnonymousDataItem.ValueSn            += Src->AnonymousDataItem.ValueSn           ;
	Dest->AnonymousDataItem.ValueX             += Src->AnonymousDataItem.ValueX            ;
	Dest->AnonymousDataItem.ProxyPt            += Src->AnonymousDataItem.ProxyPt           ;
	Dest->AnonymousDataItem.ProxyPw            += Src->AnonymousDataItem.ProxyPw           ;
	Dest->AnonymousDataItem.ProxyN             += Src->AnonymousDataItem.ProxyN            ;
	Dest->AnonymousDataItem.ProxySn            += Src->AnonymousDataItem.ProxySn           ;
	Dest->AnonymousDataItem.ProxyX             += Src->AnonymousDataItem.ProxyX            ;
	Dest->AnonymousDataItem.FT                 += Src->AnonymousDataItem.FT                ;
	Dest->AnonymousDataItem.FW                 += Src->AnonymousDataItem.FW                ;
	Dest->AnonymousDataItem.FS                 += Src->AnonymousDataItem.FS                ;
	Dest->AnonymousDataItem.Weight             += Src->AnonymousDataItem.Weight            ;
	Dest->AnonymousDataItem.WeightedValue      += Src->AnonymousDataItem.WeightedValue     ;
	Dest->AnonymousDataItem.WeightedShadow     += Src->AnonymousDataItem.WeightedShadow    ;
	Dest->AnonymousDataItem.WeightedProxy      += Src->AnonymousDataItem.WeightedProxy     ;
	Dest->AnonymousDataItem.SecondryValuePt    += Src->AnonymousDataItem.SecondryValuePt   ;
	Dest->AnonymousDataItem.SecondryValuePw    += Src->AnonymousDataItem.SecondryValuePw   ;
	Dest->AnonymousDataItem.SecondryProxyPt    += Src->AnonymousDataItem.SecondryProxyPt   ;
	Dest->AnonymousDataItem.SecondryProxyPw    += Src->AnonymousDataItem.SecondryProxyPw   ;
	Dest->AnonymousDataItem.SecondryFT         += Src->AnonymousDataItem.SecondryFT        ;
	Dest->AnonymousDataItem.SecondryFW         += Src->AnonymousDataItem.SecondryFW        ;
	Dest->AnonymousDataItem.SecondryFS         += Src->AnonymousDataItem.SecondryFS        ;
	Dest->AnonymousDataItem.WeightedValueX     += Src->AnonymousDataItem.WeightedValueX    ;
	Dest->AnonymousDataItem.WeightedProxyX     += Src->AnonymousDataItem.WeightedProxyX    ;
	Dest->TotalNumberObservations     += Src->AnonymousDataItem.NumberObservations;
	Dest->TotalValue                  += Src->AnonymousDataItem.Value             ;
	Dest->TotalShadow                 += Src->AnonymousDataItem.Shadow            ;
	Dest->TotalProxy                  += Src->AnonymousDataItem.Proxy             ;
	Dest->TotalValuePt                += Src->AnonymousDataItem.ValuePt           ;
	Dest->TotalValuePw                += Src->AnonymousDataItem.ValuePw           ;
	Dest->TotalValueN                 += Src->AnonymousDataItem.ValueN            ;
	Dest->TotalValueSn                += Src->AnonymousDataItem.ValueSn           ;
	Dest->TotalValueX                 += Src->AnonymousDataItem.ValueX            ;
	Dest->TotalProxyPt                += Src->AnonymousDataItem.ProxyPt           ;
	Dest->TotalProxyPw                += Src->AnonymousDataItem.ProxyPw           ;
	Dest->TotalProxyN                 += Src->AnonymousDataItem.ProxyN            ;
	Dest->TotalProxySn                += Src->AnonymousDataItem.ProxySn           ;
	Dest->TotalProxyX                 += Src->AnonymousDataItem.ProxyX            ;
	Dest->TotalFT                     += Src->AnonymousDataItem.FT                ;
	Dest->TotalFW                     += Src->AnonymousDataItem.FW                ;
	Dest->TotalFS                     += Src->AnonymousDataItem.FS                ;
	Dest->TotalWeight                 += Src->AnonymousDataItem.Weight            ;
	Dest->TotalWeightedValue          += Src->AnonymousDataItem.WeightedValue     ;
	Dest->TotalWeightedShadow         += Src->AnonymousDataItem.WeightedShadow    ;
	Dest->TotalWeightedProxy          += Src->AnonymousDataItem.WeightedProxy     ;
	Dest->TotalSecondryValuePt        += Src->AnonymousDataItem.SecondryValuePt   ;
	Dest->TotalSecondryValuePw        += Src->AnonymousDataItem.SecondryValuePw   ;
	Dest->TotalSecondryProxyPt        += Src->AnonymousDataItem.SecondryProxyPt   ;
	Dest->TotalSecondryProxyPw        += Src->AnonymousDataItem.SecondryProxyPw   ;
	Dest->TotalSecondryFT             += Src->AnonymousDataItem.SecondryFT        ;
	Dest->TotalSecondryFW             += Src->AnonymousDataItem.SecondryFW        ;
	Dest->TotalSecondryFS             += Src->AnonymousDataItem.SecondryFS        ;
	Dest->TotalWeightedValueX         += Src->AnonymousDataItem.WeightedValueX    ;
	Dest->TotalWeightedProxyX         += Src->AnonymousDataItem.WeightedProxyX    ;
	Dest->Sensitivity                  = SENSITIVITY_NOT_CALCULATED      ;
	Dest->Sensitivity_nowaivers        = SENSITIVITY_NOT_CALCULATED      ;
	Dest->Sensitivity_noproxy          = SENSITIVITY_NOT_CALCULATED      ;
	Dest->Sensitivity_noweights        = SENSITIVITY_NOT_CALCULATED      ;
	Dest->WhichNoise                   = ' '                             ;
	/* Dest->FavCost = Dest->FavCost + Src->FavCost; */
	Dest->FavCost               = FAVCOST_NOT_CALCULATED;
	Dest->LargestNumberEntries = 0;
	Dest->LargestPYNumberEntries = 0;
	Dest->LargestWVNumberEntries = 0;
	Dest->LargestWPNumberEntries = 0;
	Dest->LargestFTNumberEntries = 0;
	Dest->LargestFWNumberEntries = 0;
	Dest->LargestFSNumberEntries = 0;
	Dest->Largst2FTNumberEntries = 0;
	Dest->Largst2FWNumberEntries = 0;
	Dest->Largst2FSNumberEntries = 0;
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Create an exact copy of a cell
------------------------------------------------------------------------------*/
STCT_CELL * STC_CellDuplicate (
	STCT_CELL * Cell)
{
	STCT_CELL * Dup;
	int i;
	EIT_RETURNCODE rc;

	Dup = STC_CellAllocate (Cell->Data.NumberEntries);
	if (Dup == NULL) return NULL;

	//it is important to set this flag before calling STC_CellAdd ()
	//since STC_CellAdd () allocate memory for keys of internal cells
	//but copy the pointer for keys of marginal cells
	Dup->IsInternal = Cell->IsInternal;

	for (i = 0; i < Cell->Data.NumberEntries; i++) {
		/*-I don't think that the handling of the "waiver_flags_present" argument (just passing the value 1) in the following call to "STC_Cell.c::STC_CellAdd()" is correct; */
		/* as of now this function isn't called anywhere, so it isn't a problem, but if it is ever used this may have to be fixed (the value passed should be the value of    */
		/* the variable "waiver_flags_present" that is defined in the main function "sensitiv.c::sensitiv()", but that isn't accessible from here)--actually, I see now that  */
		/* just passing the value "2" is correct--the way that "STC_Cell.c::STC_CellAdd()" processes its "waiver_flags_present" argument is to assign the value of its        */
		/* "waiver_flag" argument to the "waiver_flag" field of the new STCT_DATAITEM that it appends to the cell's array of "STCT_DATAITEMS", and that will be correct even  */
		/* if the user hasn't specified waiver flags, because in either case the value of the "waiver_flag" field in the STCT_DATAITEMs in the cell being duplicated will be  */
		/* what you want them to be in the copy:                                                                                                                              */
		rc = STC_CellAdd (Dup, Cell->Data.Item[i]->Key, Cell->Data.Item[i]->Value, Cell->Data.Item[i]->Shadow, Cell->Data.Item[i]->Proxy, Cell->Data.Item[i]->Weight, 2, Cell->Data.Item[i]->waiver_flag);
		if (rc != EIE_SUCCEED) return NULL;
	}
	Dup->Coordinate = STC_CoordinateDuplicate (Cell->Coordinate);
	if (Dup->Coordinate == NULL) return NULL;

	Dup->CellId                      = Cell->CellId                     ;
	Dup->Type                        = Cell->Type                       ;
	Dup->AnonymousDataItem.NumberObservations = Cell->AnonymousDataItem.NumberObservations;
	Dup->AnonymousDataItem.Value              = Cell->AnonymousDataItem.Value             ;
	Dup->AnonymousDataItem.Shadow             = Cell->AnonymousDataItem.Shadow            ;
	Dup->AnonymousDataItem.Proxy              = Cell->AnonymousDataItem.Proxy             ;
	Dup->AnonymousDataItem.ValuePt            = Cell->AnonymousDataItem.ValuePt           ;
	Dup->AnonymousDataItem.ValuePw            = Cell->AnonymousDataItem.ValuePw           ;
	Dup->AnonymousDataItem.ValueN             = Cell->AnonymousDataItem.ValueN            ;
	Dup->AnonymousDataItem.ValueSn            = Cell->AnonymousDataItem.ValueSn           ;
	Dup->AnonymousDataItem.ValueX             = Cell->AnonymousDataItem.ValueX            ;
	Dup->AnonymousDataItem.ProxyPt            = Cell->AnonymousDataItem.ProxyPt           ;
	Dup->AnonymousDataItem.ProxyPw            = Cell->AnonymousDataItem.ProxyPw           ;
	Dup->AnonymousDataItem.ProxyN             = Cell->AnonymousDataItem.ProxyN            ;
	Dup->AnonymousDataItem.ProxySn            = Cell->AnonymousDataItem.ProxySn           ;
	Dup->AnonymousDataItem.ProxyX             = Cell->AnonymousDataItem.ProxyX            ;
	Dup->AnonymousDataItem.FT                 = Cell->AnonymousDataItem.FT                ;
	Dup->AnonymousDataItem.FW                 = Cell->AnonymousDataItem.FW                ;
	Dup->AnonymousDataItem.FS                 = Cell->AnonymousDataItem.FS                ;
	Dup->AnonymousDataItem.Weight             = Cell->AnonymousDataItem.Weight            ;
	Dup->AnonymousDataItem.WeightedValue      = Cell->AnonymousDataItem.WeightedValue     ;
	Dup->AnonymousDataItem.WeightedShadow     = Cell->AnonymousDataItem.WeightedShadow    ;
	Dup->AnonymousDataItem.WeightedProxy      = Cell->AnonymousDataItem.WeightedProxy     ;
	Dup->AnonymousDataItem.SecondryValuePt    = Cell->AnonymousDataItem.SecondryValuePt   ;
	Dup->AnonymousDataItem.SecondryValuePw    = Cell->AnonymousDataItem.SecondryValuePw   ;
	Dup->AnonymousDataItem.SecondryProxyPt    = Cell->AnonymousDataItem.SecondryProxyPt   ;
	Dup->AnonymousDataItem.SecondryProxyPw    = Cell->AnonymousDataItem.SecondryProxyPw   ;
	Dup->AnonymousDataItem.SecondryFT         = Cell->AnonymousDataItem.SecondryFT        ;
	Dup->AnonymousDataItem.SecondryFW         = Cell->AnonymousDataItem.SecondryFW        ;
	Dup->AnonymousDataItem.SecondryFS         = Cell->AnonymousDataItem.SecondryFS        ;
	Dup->AnonymousDataItem.WeightedValueX     = Cell->AnonymousDataItem.WeightedValueX    ;
	Dup->AnonymousDataItem.WeightedProxyX     = Cell->AnonymousDataItem.WeightedProxyX    ;
	Dup->TotalNumberObservations     = Cell->TotalNumberObservations    ;
	Dup->TotalValue                  = Cell->TotalValue                 ;
	Dup->TotalShadow                 = Cell->TotalShadow                ;
	Dup->TotalProxy                  = Cell->TotalProxy                 ;
	Dup->TotalValuePt                = Cell->TotalValuePt               ;
	Dup->TotalValuePw                = Cell->TotalValuePw               ;
	Dup->TotalValueN                 = Cell->TotalValueN                ;
	Dup->TotalValueSn                = Cell->TotalValueSn               ;
	Dup->TotalValueX                 = Cell->TotalValueX                ;
	Dup->TotalProxyPt                = Cell->TotalProxyPt               ;
	Dup->TotalProxyPw                = Cell->TotalProxyPw               ;
	Dup->TotalProxyN                 = Cell->TotalProxyN                ;
	Dup->TotalProxySn                = Cell->TotalProxySn               ;
	Dup->TotalProxyX                 = Cell->TotalProxyX                ;
	Dup->TotalFT                     = Cell->TotalFT                    ;
	Dup->TotalFW                     = Cell->TotalFW                    ;
	Dup->TotalFS                     = Cell->TotalFS                    ;
	Dup->TotalWeight                 = Cell->TotalWeight                ;
	Dup->TotalWeightedValue          = Cell->TotalWeightedValue         ;
	Dup->TotalWeightedShadow         = Cell->TotalWeightedShadow        ;
	Dup->TotalWeightedProxy          = Cell->TotalWeightedProxy         ;
	Dup->TotalSecondryValuePt        = Cell->TotalSecondryValuePt       ;
	Dup->TotalSecondryValuePw        = Cell->TotalSecondryValuePw       ;
	Dup->TotalSecondryProxyPt        = Cell->TotalSecondryProxyPt       ;
	Dup->TotalSecondryProxyPw        = Cell->TotalSecondryProxyPw       ;
	Dup->TotalSecondryFT             = Cell->TotalSecondryFT            ;
	Dup->TotalSecondryFW             = Cell->TotalSecondryFW            ;
	Dup->TotalSecondryFS             = Cell->TotalSecondryFS            ;
	Dup->TotalWeightedValueX         = Cell->TotalWeightedValueX        ;
	Dup->TotalWeightedProxyX         = Cell->TotalWeightedProxyX        ;
	Dup->Sensitivity                 = Cell->Sensitivity                ;
	Dup->Sensitivity_nowaivers       = Cell->Sensitivity_nowaivers      ;
	Dup->Sensitivity_noproxy         = Cell->Sensitivity_noproxy        ;
	Dup->Sensitivity_noweights       = Cell->Sensitivity_noweights      ;
	Dup->WhichNoise                  = Cell->WhichNoise                 ;
	Dup->FavCost                     = Cell->FavCost                    ; /* ??? */
	Dup->LargestNumberEntries = 0;
	Dup->LargestPYNumberEntries = 0;
	Dup->LargestWVNumberEntries = 0;
	Dup->LargestWPNumberEntries = 0;
	Dup->LargestFTNumberEntries = 0;
	Dup->LargestFWNumberEntries = 0;
	Dup->LargestFSNumberEntries = 0;
	Dup->Largst2FWNumberEntries = 0;
	Dup->Largst2FTNumberEntries = 0;
	Dup->Largst2FSNumberEntries = 0;
	//Dup->Largest = Cell->Largest;    //do not set Largest because it would point to Items of Cell (instead of those of Dup)
	//Dup->LargestPY = Cell->LargestPY //do not set LargestPY because it would point to Items of Cell (instead of those of Dup);
	//Dup->LargestWV = Cell->LargestWV //do not set LargestWV because it would point to Items of Cell (instead of those of Dup);
	//Dup->LargestWP = Cell->LargestWP //do not set LargestWP because it would point to Items of Cell (instead of those of Dup);
	//Dup->LargestFT = Cell->LargestFT //do not set LargestFT because it would point to Items of Cell (instead of those of Dup);
	//Dup->LargestFW = Cell->LargestFW //do not set LargestFW because it would point to Items of Cell (instead of those of Dup);
	//Dup->LargestFS = Cell->LargestFS //do not set LargestFS because it would point to Items of Cell (instead of those of Dup);
	//Dup->Largst2FT = Cell->Largst2FT //do not set Largst2FT because it would point to Items of Cell (instead of those of Dup);
	//Dup->Largst2FW = Cell->Largst2FW //do not set Largst2FW because it would point to Items of Cell (instead of those of Dup);
	//Dup->Largst2FS = Cell->Largst2FS //do not set Largst2FS because it would point to Items of Cell (instead of those of Dup);
	/*-actually, I believe the following would correctly duplicate the "Largest"                */
	/* and "LargestNumberEntries" members:                                                      */
	/*     Dup->LargestNumberEntries = Cell->LargestNumberEntries;                              */
	/*     for (i = 0; i < Cell->LargestNumberEntries; i++) {                                   */
	/*         Dup->Largest[i] = ((Dup->Data).Item) + (Cell->Largest)[i] - ((Cell->Data).Item); */
	/*     }                                                                                    */
	/* or, to protect against some of the array elements                                        */
	/* Cell->Largest[0]...Cell->Largest[Cell->LargestNumberEntries-1] being NULL (even though   */
	/* that SHOULD'NT ever happen):                                                             */
	/*     Dup->LargestNumberEntries = Cell->LargestNumberEntries;                              */
	/*     for (i = 0; i < Cell->LargestNumberEntries; i++) {                                   */
	/*         Dup->Largest[i] = ((Cell->Largest)[i]==NULL)?NULL:                               */
	/*                           ((Dup->Data).Item) + (Cell->Largest)[i] - ((Cell->Data).Item); */
	/*     }                                                                                    */
	/* --note that the function "STC_Build.c::SetLargestRespondents()" (which calls the         */
	/* function "STC_Build.c::SearchLargestRespondents()" and is called only in the             */
	/* functions "STC_Build.c::FindSensitiveAggregatesForOneMarginalCell()" and                 */
	/* "STC_Build.c::STC_CalculateUserGroupsSensitivity()" (the latter of which isn't           */
	/* called anywhere because it was written to help implement the abandoned "group"           */
	/* parameter of proc sensitivity)), does exactly this in a more round-about and less        */
	/* efficient way, by looking up the index of each cell in the "Largest" array of the        */
	/* source cell in the source cell's "Data.Items" array and using that index to index        */
	/* the destination cell's "Data.Items" array to get the address of the corresponding        */
	/* cell in the destination cell's "Data.Items" array to assign to the corresponding         */
	/* element of the destination cell's "Largest" array--i.e., instead of the above,           */
	/* "STC_Build.c::SetLargestRespondents()" does:                                             */
	/*     Dup->LargestNumberEntries = Cell->LargestNumberEntries;                              */
	/*     for (i = 0; i < Cell->LargestNumberEntries; i++) {                                   */
	/*         Index = SearchLargestRespondents (&Cell->Data, Cell->Largest[i]->Key);           */
	/*             //can't return -1, because I know it exists                                  */
	/*         Dup->Largest[i] = Cell->Data.Item[Index];                                        */ /*     }                                                                                    */
	return Dup;
}
/*------------------------------------------------------------------------------
Fill a cell with random data. Debug function.
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_CellFill (
	STCT_CELL * Cell,
	int n,
	int KeyLength,
	int MaxKey,
	int MaxValue,
	int PourcentVide) /* 1 a 100 */
{
	int i;
	char Key[101];
	int r;
	EIT_RETURNCODE rc;
	double Value;

	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "n %d KeyLength %d MaxKey %d MaxValue %d PourcentVide %d\n",
		n, KeyLength, MaxKey, MaxValue, PourcentVide);
	for (i = 0; i < n; i++) {
		r = UTIL_Random (1, 100);
		if (r >= PourcentVide) {
			sprintf (Key, "%0*ld", KeyLength, UTIL_Random (1, MaxKey));
			Value = UTIL_Random (1, MaxValue);
			rc = STC_CellAdd (Cell, Key, Value, Value, Value, Value, 2, 0);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
		else {
			Value = UTIL_Random (1, MaxValue/2);
			rc = STC_CellAdd (Cell, "", Value, Value, Value, Value, 2, 0);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Free the STCT_CELL structure
------------------------------------------------------------------------------*/
void STC_CellFree (
	STCT_CELL * Cell)
{
	if (Cell != NULL) {
#ifdef _DEBUG
#ifdef DEBUGALLOCATION
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "STC_CellFree ne=%d/na=%d\n", Cell->Data.NumberEntries, Cell->Data.NumberAllocated);
#endif
#endif
		STC_CoordinateFree (Cell->Coordinate);
		STC_CellFreeItems (Cell);
		InitializeCell (Cell);
		STC_FreeMemory (Cell);
	}
}
/*------------------------------------------------------------------------------
Free the item part of the STCT_CELL structure
------------------------------------------------------------------------------*/
void STC_CellFreeItems (
	STCT_CELL * Cell)
{
	int i;

	if (Cell != NULL) {
		if (Cell->Data.Item != NULL) {
			for (i = 0; i < Cell->Data.NumberEntries; i++) {
				if (Cell->IsInternal) STC_FreeMemory (Cell->Data.Item[i]->Key);
				STC_FreeMemory (Cell->Data.Item[i]);
			}
			STC_FreeMemory (Cell->Data.Item);
		}
		Cell->Data.Item = NULL;
		Cell->Data.NumberAllocated = 0;
		Cell->Data.NumberEntries = 0;
	}
}
/*------------------------------------------------------------------------------
set the CellId of the cell, but only if was not previously set
------------------------------------------------------------------------------*/
void STC_CellNextCellId (
	STCT_CELL * Cell)
{
	if (Cell->CellId == 0) Cell->CellId = ++mCellId;
}
/*------------------------------------------------------------------------------
Check if the cell has anonymous observations
return EIE_TRUE if it has, EIE_FALSE otherwise.
------------------------------------------------------------------------------*/
EIT_BOOLEAN STC_CellHasAnonymous (
	STCT_CELL * Cell)
{
	return Cell->AnonymousDataItem.NumberObservations > 0 ? EIE_TRUE : EIE_FALSE;
}
/*------------------------------------------------------------------------------
Check if the cell has only anonymous observations
return EIE_TRUE if it has, EIE_FALSE otherwise.
------------------------------------------------------------------------------*/
EIT_BOOLEAN STC_CellHasAnonymousOnly (
	STCT_CELL * Cell)
{
	return (Cell->Data.NumberEntries == 0 && Cell->AnonymousDataItem.NumberObservations > 0) ? EIE_TRUE : EIE_FALSE;
}
/*------------------------------------------------------------------------------
Initialize the CellId 
------------------------------------------------------------------------------*/
void STC_CellInitCellId (void)
{
	mCellId = 0;
}
/*------------------------------------------------------------------------------
Check if the cell is sensitive
return EIE_TRUE if it is, EIE_FALSE otherwise.
------------------------------------------------------------------------------*/
EIT_BOOLEAN STC_CellIsSensitive (
	STCT_CELL * Cell)
{
#ifdef _DEBUG
	//developpement seulement
	if (Cell->Sensitivity == SENSITIVITY_NOT_CALCULATED) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "sensibilité non calculé pour ");
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
		STC_CellPrint (Cell);
		EI_PrintMessages ();
		assert (Cell->Sensitivity != SENSITIVITY_NOT_CALCULATED);
		exit (EXIT_FAILURE);
	}
#endif
	return Cell->Sensitivity > 0.0 ? EIE_TRUE : EIE_FALSE;
}
/*------------------------------------------------------------------------------
Merge the content of 2 cells in a newly created cell.
------------------------------------------------------------------------------*/
STCT_CELL * STC_CellMerge (
	STCT_CELL * Src1,
	STCT_CELL * Src2)
{
	int cmp;
	STCT_CELL * Dest;
	int i1;
	int i2;
	EIT_RETURNCODE rc;

	Dest = STC_CellAllocate (Src1->Data.NumberEntries+Src2->Data.NumberEntries);
	if (Dest == NULL) return NULL;

	i1 = i2 = 0;
	while (i1 < Src1->Data.NumberEntries && i2 < Src2->Data.NumberEntries) {
		cmp = strcmp (Src1->Data.Item[i1]->Key, Src2->Data.Item[i2]->Key);
		if (cmp > 0) {
			rc = STC_CellAdd (Dest, Src1->Data.Item[i1]->Key, Src1->Data.Item[i1]->Value, Src1->Data.Item[i1]->Shadow, Src1->Data.Item[i1]->Proxy, Src1->Data.Item[i1]->Weight, 2, Src1->Data.Item[i1]->waiver_flag);
			if (rc != EIE_SUCCEED) return NULL;
			i1++;
		}
		else if (cmp < 0) {
			rc = STC_CellAdd (Dest, Src2->Data.Item[i2]->Key, Src2->Data.Item[i2]->Value, Src2->Data.Item[i2]->Shadow, Src2->Data.Item[i2]->Proxy, Src2->Data.Item[i2]->Weight, 2, Src2->Data.Item[i2]->waiver_flag);
			if (rc != EIE_SUCCEED) return NULL;
			i2++;
		}
		else { /* cmp == 0 */
			rc = STC_CellAdd (Dest, Src1->Data.Item[i1]->Key, Src1->Data.Item[i1]->Value, Src1->Data.Item[i1]->Shadow, Src1->Data.Item[i1]->Proxy, Src1->Data.Item[i1]->Weight, 2, Src1->Data.Item[i1]->waiver_flag);
			if (rc != EIE_SUCCEED) return NULL;
			i1++;
			rc = STC_CellAdd (Dest, Src2->Data.Item[i2]->Key, Src2->Data.Item[i2]->Value, Src2->Data.Item[i2]->Shadow, Src2->Data.Item[i2]->Proxy, Src2->Data.Item[i2]->Weight, 2, Src2->Data.Item[i2]->waiver_flag);
			if (rc != EIE_SUCCEED) return NULL;
			i2++;
		}
	}
	while (i1 < Src1->Data.NumberEntries) {
		rc = STC_CellAdd (Dest, Src1->Data.Item[i1]->Key, Src1->Data.Item[i1]->Value, Src1->Data.Item[i1]->Shadow, Src1->Data.Item[i1]->Proxy, Src1->Data.Item[i1]->Weight, 2, Src1->Data.Item[i1]->waiver_flag);
		if (rc != EIE_SUCCEED) return NULL;
		i1++;
	}
	while (i2 < Src2->Data.NumberEntries) {
		rc = STC_CellAdd (Dest, Src2->Data.Item[i2]->Key, Src2->Data.Item[i2]->Value, Src2->Data.Item[i2]->Shadow, Src2->Data.Item[i2]->Proxy, Src2->Data.Item[i2]->Weight, 2, Src2->Data.Item[i2]->waiver_flag);
		if (rc != EIE_SUCCEED) return NULL;
		i2++;
	}
	Dest->AnonymousDataItem.NumberObservations = Src1->AnonymousDataItem.NumberObservations + Src2->AnonymousDataItem.NumberObservations;
	Dest->AnonymousDataItem.Value              = Src1->AnonymousDataItem.Value              + Src2->AnonymousDataItem.Value             ;
	Dest->AnonymousDataItem.Shadow             = Src1->AnonymousDataItem.Shadow             + Src2->AnonymousDataItem.Shadow            ;
	Dest->AnonymousDataItem.Proxy              = Src1->AnonymousDataItem.Proxy              + Src2->AnonymousDataItem.Proxy             ;
	Dest->AnonymousDataItem.ValuePt            = Src1->AnonymousDataItem.ValuePt            + Src2->AnonymousDataItem.ValuePt           ;
	Dest->AnonymousDataItem.ValuePw            = Src1->AnonymousDataItem.ValuePw            + Src2->AnonymousDataItem.ValuePw           ;
	Dest->AnonymousDataItem.ValueN             = Src1->AnonymousDataItem.ValueN             + Src2->AnonymousDataItem.ValueN            ;
	Dest->AnonymousDataItem.ValueSn            = Src1->AnonymousDataItem.ValueSn            + Src2->AnonymousDataItem.ValueSn           ;
	Dest->AnonymousDataItem.ValueX             = Src1->AnonymousDataItem.ValueX             + Src2->AnonymousDataItem.ValueX            ;
	Dest->AnonymousDataItem.ProxyPt            = Src1->AnonymousDataItem.ProxyPt            + Src2->AnonymousDataItem.ProxyPt           ;
	Dest->AnonymousDataItem.ProxyPw            = Src1->AnonymousDataItem.ProxyPw            + Src2->AnonymousDataItem.ProxyPw           ;
	Dest->AnonymousDataItem.ProxyN             = Src1->AnonymousDataItem.ProxyN             + Src2->AnonymousDataItem.ProxyN            ;
	Dest->AnonymousDataItem.ProxySn            = Src1->AnonymousDataItem.ProxySn            + Src2->AnonymousDataItem.ProxySn           ;
	Dest->AnonymousDataItem.ProxyX             = Src1->AnonymousDataItem.ProxyX             + Src2->AnonymousDataItem.ProxyX            ;
	Dest->AnonymousDataItem.FT                 = Src1->AnonymousDataItem.FT                 + Src2->AnonymousDataItem.FT                ;
	Dest->AnonymousDataItem.FW                 = Src1->AnonymousDataItem.FW                 + Src2->AnonymousDataItem.FW                ;
	Dest->AnonymousDataItem.FS                 = Src1->AnonymousDataItem.FS                 + Src2->AnonymousDataItem.FS                ;
	Dest->AnonymousDataItem.Weight             = Src1->AnonymousDataItem.Weight             + Src2->AnonymousDataItem.Weight            ;
	Dest->AnonymousDataItem.WeightedValue      = Src1->AnonymousDataItem.WeightedValue      + Src2->AnonymousDataItem.WeightedValue     ;
	Dest->AnonymousDataItem.WeightedShadow     = Src1->AnonymousDataItem.WeightedShadow     + Src2->AnonymousDataItem.WeightedShadow    ;
	Dest->AnonymousDataItem.WeightedProxy      = Src1->AnonymousDataItem.WeightedProxy      + Src2->AnonymousDataItem.WeightedProxy     ;
	Dest->AnonymousDataItem.SecondryValuePt    = Src1->AnonymousDataItem.SecondryValuePt    + Src2->AnonymousDataItem.SecondryValuePt   ;
	Dest->AnonymousDataItem.SecondryValuePw    = Src1->AnonymousDataItem.SecondryValuePw    + Src2->AnonymousDataItem.SecondryValuePw   ;
	Dest->AnonymousDataItem.SecondryProxyPt    = Src1->AnonymousDataItem.SecondryProxyPt    + Src2->AnonymousDataItem.SecondryProxyPt   ;
	Dest->AnonymousDataItem.SecondryProxyPw    = Src1->AnonymousDataItem.SecondryProxyPw    + Src2->AnonymousDataItem.SecondryProxyPw   ;
	Dest->AnonymousDataItem.SecondryFT         = Src1->AnonymousDataItem.SecondryFT         + Src2->AnonymousDataItem.SecondryFT        ;
	Dest->AnonymousDataItem.SecondryFW         = Src1->AnonymousDataItem.SecondryFW         + Src2->AnonymousDataItem.SecondryFW        ;
	Dest->AnonymousDataItem.SecondryFS         = Src1->AnonymousDataItem.SecondryFS         + Src2->AnonymousDataItem.SecondryFS        ;
	Dest->AnonymousDataItem.WeightedValueX     = Src1->AnonymousDataItem.WeightedValueX     + Src2->AnonymousDataItem.WeightedValueX    ;
	Dest->AnonymousDataItem.WeightedProxyX     = Src1->AnonymousDataItem.WeightedProxyX     + Src2->AnonymousDataItem.WeightedProxyX    ;
	Dest->TotalNumberObservations    += Src1->AnonymousDataItem.NumberObservations + Src2->AnonymousDataItem.NumberObservations;
	Dest->TotalValue                 += Src1->AnonymousDataItem.Value              + Src2->AnonymousDataItem.Value             ;
	Dest->TotalShadow                += Src1->AnonymousDataItem.Shadow             + Src2->AnonymousDataItem.Shadow            ;
	Dest->TotalProxy                 += Src1->AnonymousDataItem.Proxy              + Src2->AnonymousDataItem.Proxy             ;
	Dest->TotalValuePt               += Src1->AnonymousDataItem.ValuePt            + Src2->AnonymousDataItem.ValuePt           ;
	Dest->TotalValuePw               += Src1->AnonymousDataItem.ValuePw            + Src2->AnonymousDataItem.ValuePw           ;
	Dest->TotalValueN                += Src1->AnonymousDataItem.ValueN             + Src2->AnonymousDataItem.ValueN            ;
	Dest->TotalValueSn               += Src1->AnonymousDataItem.ValueSn            + Src2->AnonymousDataItem.ValueSn           ;
	Dest->TotalValueX                += Src1->AnonymousDataItem.ValueX             + Src2->AnonymousDataItem.ValueX            ;
	Dest->TotalProxyPt               += Src1->AnonymousDataItem.ProxyPt            + Src2->AnonymousDataItem.ProxyPt           ;
	Dest->TotalProxyPw               += Src1->AnonymousDataItem.ProxyPw            + Src2->AnonymousDataItem.ProxyPw           ;
	Dest->TotalProxyN                += Src1->AnonymousDataItem.ProxyN             + Src2->AnonymousDataItem.ProxyN            ;
	Dest->TotalProxySn               += Src1->AnonymousDataItem.ProxySn            + Src2->AnonymousDataItem.ProxySn           ;
	Dest->TotalProxyX                += Src1->AnonymousDataItem.ProxyX             + Src2->AnonymousDataItem.ProxyX            ;
	Dest->TotalFT                    += Src1->AnonymousDataItem.FT                 + Src2->AnonymousDataItem.FT                ;
	Dest->TotalFW                    += Src1->AnonymousDataItem.FW                 + Src2->AnonymousDataItem.FW                ;
	Dest->TotalFS                    += Src1->AnonymousDataItem.FS                 + Src2->AnonymousDataItem.FS                ;
	Dest->TotalWeight                += Src1->AnonymousDataItem.Weight             + Src2->AnonymousDataItem.Weight            ;
	Dest->TotalWeightedValue         += Src1->AnonymousDataItem.WeightedValue      + Src2->AnonymousDataItem.WeightedValue     ;
	Dest->TotalWeightedShadow        += Src1->AnonymousDataItem.WeightedShadow     + Src2->AnonymousDataItem.WeightedShadow    ;
	Dest->TotalWeightedProxy         += Src1->AnonymousDataItem.WeightedProxy      + Src2->AnonymousDataItem.WeightedProxy     ;
	Dest->TotalSecondryValuePt       += Src1->AnonymousDataItem.SecondryValuePt    + Src2->AnonymousDataItem.SecondryValuePt   ;
	Dest->TotalSecondryValuePw       += Src1->AnonymousDataItem.SecondryValuePw    + Src2->AnonymousDataItem.SecondryValuePw   ;
	Dest->TotalSecondryProxyPt       += Src1->AnonymousDataItem.SecondryProxyPt    + Src2->AnonymousDataItem.SecondryProxyPt   ;
	Dest->TotalSecondryProxyPw       += Src1->AnonymousDataItem.SecondryProxyPw    + Src2->AnonymousDataItem.SecondryProxyPw   ;
	Dest->TotalSecondryFT            += Src1->AnonymousDataItem.SecondryFT         + Src2->AnonymousDataItem.SecondryFT        ;
	Dest->TotalSecondryFW            += Src1->AnonymousDataItem.SecondryFW         + Src2->AnonymousDataItem.SecondryFW        ;
	Dest->TotalSecondryFS            += Src1->AnonymousDataItem.SecondryFS         + Src2->AnonymousDataItem.SecondryFS        ;
	Dest->TotalWeightedValueX        += Src1->AnonymousDataItem.WeightedValueX     + Src2->AnonymousDataItem.WeightedValueX    ;
	Dest->TotalWeightedProxyX        += Src1->AnonymousDataItem.WeightedProxyX     + Src2->AnonymousDataItem.WeightedProxyX    ;
	Dest->Sensitivity                 = SENSITIVITY_NOT_CALCULATED                                           ;
	Dest->Sensitivity_nowaivers       = SENSITIVITY_NOT_CALCULATED                                           ;
	Dest->Sensitivity_noproxy         = SENSITIVITY_NOT_CALCULATED                                           ;
	Dest->Sensitivity_noweights       = SENSITIVITY_NOT_CALCULATED                                           ;
	Dest->WhichNoise                  = ' '                                                                  ;
	/* Dest->FavCost = Src1->FavCost + Src2->FavCost; */
	Dest->FavCost = FAVCOST_NOT_CALCULATED;
	Dest->LargestNumberEntries = 0;
	Dest->LargestPYNumberEntries = 0;
	Dest->LargestWVNumberEntries = 0;
	Dest->LargestWPNumberEntries = 0;
	Dest->LargestFTNumberEntries = 0;
	Dest->LargestFWNumberEntries = 0;
	Dest->LargestFSNumberEntries = 0;
	Dest->Largst2FTNumberEntries = 0;
	Dest->Largst2FWNumberEntries = 0;
	Dest->Largst2FSNumberEntries = 0;
	return Dest;
}
/*------------------------------------------------------------------------------
Print the cell. Debug function.
------------------------------------------------------------------------------*/
void STC_CellPrint (
	STCT_CELL * Cell)
{
	int i;

	STC_CellPrintInfo (Cell, 1);
	for (i = 0; i < Cell->Data.NumberEntries; i++) {
		if (Cell->Data.Item == NULL)
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Cell->Data.Item == NULL\n");
		else
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "k %-8s v %7.2f o %7.2f %d\n",
				Cell->Data.Item[i]->Key, Cell->Data.Item[i]->Value, Cell->Data.Item[i]->Shadow,
				Cell->Data.Item[i]->NumberObservations);
	}
	if (STC_CellHasAnonymous (Cell) != EIE_FALSE)
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%-10s v %7.2f o %7.2f %d\n",
			"Anonymous", Cell->AnonymousDataItem.Value, Cell->AnonymousDataItem.Shadow, Cell->AnonymousDataItem.NumberObservations);
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
}
/*------------------------------------------------------------------------------
Print the cell metadata. Debug function.
------------------------------------------------------------------------------*/
void STC_CellPrintInfo (
	STCT_CELL * Cell,
	int PrintHeader)
{
#define CELLID_LENGTH 10
#define NUMBERENTRIES_LENGTH 10
#define SHADOW_LENGTH 12
#define SHADOW_DECIMAL 2
#define TOTAL_LENGTH 12
#define TOTAL_DECIMAL 2
#define SENSITIVITY_LENGTH 12
#define SENSITIVITY_DECIMAL 2
#define STATUS_LENGTH 10
#define TYPE_LENGTH 10
#define TAG_LENGTH 10

	int i;
	char s[101];
	char snwv[101];
	char snpr[101];
	char snwt[101];
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Cell ne=%d/na=%d\n", Cell->Data.NumberEntries, Cell->Data.NumberAllocated);
	if (PrintHeader)
		EI_AddMessage("", EIE_MESSAGESEVERITY_EMPTY, (
			"%*s "    /* CellId        */
			"%*s "    /* #Obs          */
			"%*s "    /* #Ent          */
			"%*s "    /* TotValue      */
			"%*s "    /* TotShadow     */
			"%*s "    /* TotProxy      */
			"%*s "    /* TotVPt        */
			"%*s "    /* TotVN         */
			"%*s "    /* TotVSn        */
			"%*s "    /* TotVX         */
			"%*s "    /* TotPPt        */
			"%*s "    /* TotPN         */
			"%*s "    /* TotPSn        */
			"%*s "    /* TotPX         */
			"%*s "    /* TotFT         */
			"%*s "    /* TotFS         */
			"%*s "    /* TotWeight     */
			"%*s "    /* TotWtdValue   */
			"%*s "    /* TotWtdShadow  */
			"%*s "    /* TotWtdProxy   */
			"%*s "    /* TotSecVPt     */
			"%*s "    /* TotSecPPt     */
			"%*s "    /* TotSecFT      */
			"%*s "    /* TotSecFS      */
			"%*s "    /* TotWtdVX      */
			"%*s "    /* TotWtdPX      */
			"%*s "    /* Sens          */
			"%*s "    /* SensNoWvrs    */
			"%*s "    /* SensNoPrxy    */
			"%*s "    /* SensNoWts     */
			"%*s "    /* WhichNoise    */
			"%*s "    /* Status        */
			"%*s\n"), /* Type          */
			CELLID_LENGTH, "CellId",
			NUMBERENTRIES_LENGTH, "#Obs",
			NUMBERENTRIES_LENGTH, "#Ent",
			TOTAL_LENGTH      , "TotValue"    ,
			SHADOW_LENGTH     , "TotShadow"   ,
			SHADOW_LENGTH     , "TotProxy"    ,
			SHADOW_LENGTH     , "TotVPt"      ,
			SHADOW_LENGTH     , "TotVN"       ,
			SHADOW_LENGTH     , "TotVSn"      ,
			SHADOW_LENGTH     , "TotVX"       ,
			SHADOW_LENGTH     , "TotPPt"      ,
			SHADOW_LENGTH     , "TotPN"       ,
			SHADOW_LENGTH     , "TotPSn"      ,
			SHADOW_LENGTH     , "TotPX"       ,
			SHADOW_LENGTH     , "TotFT"       ,
			SHADOW_LENGTH     , "TotFS"       ,
			SHADOW_LENGTH     , "TotWeight"   ,
			TOTAL_LENGTH      , "TotWtdValue" ,
			SHADOW_LENGTH     , "TotWtdShadow",
			SHADOW_LENGTH     , "TotWtdProxy" ,
			SHADOW_LENGTH     , "TotSecVPt"   ,
			SHADOW_LENGTH     , "TotSecPPt"   ,
			SHADOW_LENGTH     , "TotSecFT"    ,
			SHADOW_LENGTH     , "TotSecFS"    ,
			SHADOW_LENGTH     , "TotWtdVX"    ,
			SHADOW_LENGTH     , "TotWtdPX"    ,
			SENSITIVITY_LENGTH, "Sens",
			SENSITIVITY_LENGTH, "SensNoWvrs"  ,
			SENSITIVITY_LENGTH, "SensNoPrxy"  ,
			SENSITIVITY_LENGTH, "SensNoWts"   ,
			TYPE_LENGTH       , "WhichNoise"  ,
			STATUS_LENGTH, "Status",
			TYPE_LENGTH, "Type");
	if (Cell->Sensitivity == SENSITIVITY_NOT_CALCULATED)
		sprintf (s, "%*s",  SENSITIVITY_LENGTH, "NA");
	else
		sprintf (s, "%*.*f", SENSITIVITY_LENGTH, SENSITIVITY_DECIMAL, Cell->Sensitivity);
	if (Cell->Sensitivity_nowaivers == SENSITIVITY_NOT_CALCULATED)
		sprintf (snwv, "%*s",  SENSITIVITY_LENGTH, "NA");
	else
		sprintf (snwv, "%*.*f", SENSITIVITY_LENGTH, SENSITIVITY_DECIMAL, Cell->Sensitivity_nowaivers);
	if (Cell->Sensitivity_noproxy == SENSITIVITY_NOT_CALCULATED)
		sprintf (snpr, "%*s",  SENSITIVITY_LENGTH, "NA");
	else
		sprintf (snpr, "%*.*f", SENSITIVITY_LENGTH, SENSITIVITY_DECIMAL, Cell->Sensitivity_noproxy);
	if (Cell->Sensitivity_noweights == SENSITIVITY_NOT_CALCULATED)
		sprintf (snwt, "%*s",  SENSITIVITY_LENGTH, "NA");
	else
		sprintf (snwt, "%*.*f", SENSITIVITY_LENGTH, SENSITIVITY_DECIMAL, Cell->Sensitivity_noweights);
	EI_AddMessage("", EIE_MESSAGESEVERITY_EMPTY, (
		"%*d "   /* Cell->CellId                  */
		"%*d "   /* Cell->TotalNumberObservations */
		"%*d "   /* Cell->Data.NumberEntries      */
		"%*.*f " /* Cell->TotalValue              */
		"%*.*f " /* Cell->TotalShadow             */
		"%*.*f " /* Cell->TotalProxy              */
		"%*.*f " /* Cell->TotalValuePt            */
		"%*.*f " /* Cell->TotalValuePw            */
		"%*.*f " /* Cell->TotalValueN             */
		"%*.*f " /* Cell->TotalValueSn            */
		"%*.*f " /* Cell->TotalValueX             */
		"%*.*f " /* Cell->TotalProxyPt            */
		"%*.*f " /* Cell->TotalProxyPw            */
		"%*.*f " /* Cell->TotalProxyN             */
		"%*.*f " /* Cell->TotalProxySn            */
		"%*.*f " /* Cell->TotalProxyX             */
		"%*.*f " /* Cell->TotalFT                 */
		"%*.*f " /* Cell->TotalFW                 */
		"%*.*f " /* Cell->TotalFS                 */
		"%*.*f " /* Cell->TotalWeight             */
		"%*.*f " /* Cell->TotalWeightedValue      */
		"%*.*f " /* Cell->TotalWeightedShadow     */
		"%*.*f " /* Cell->TotalWeightedProxy      */
		"%*.*f " /* Cell->TotalSecondryValuePt    */
		"%*.*f " /* Cell->TotalSecondryValuePw    */
		"%*.*f " /* Cell->TotalSecondryProxyPt    */
		"%*.*f " /* Cell->TotalSecondryProxyPw    */
		"%*.*f " /* Cell->TotalSecondryFT         */
		"%*.*f " /* Cell->TotalSecondryFW         */
		"%*.*f " /* Cell->TotalSecondryFS         */
		"%*.*f " /* Cell->TotalWeightedValueX     */
		"%*.*f " /* Cell->TotalWeightedProxyX     */
		"%s "    /* s    = (Cell->Sensitivity          ==SENSITIVITY_NOT_CALCULATED)?"NA":<Cell->Sensitivity           formatted as a string> */
		"%s "    /* snwv = (Cell->Sensitivity_nowaivers==SENSITIVITY_NOT_CALCULATED)?"NA":<Cell->Sensitivity_nowaivers formatted as a string> */
		"%s "    /* snpr = (Cell->Sensitivity_noproxy  ==SENSITIVITY_NOT_CALCULATED)?"NA":<Cell->Sensitivity_noproxy   formatted as a string> */
		"%s "    /* snwt = (Cell->Sensitivity_noweights==SENSITIVITY_NOT_CALCULATED)?"NA":<Cell->Sensitivity_noweights formatted as a string> */
		"%c "    /* Cell->WhichNoise              */
		"%*c "   /* Cell->Sensitivity == SENSITIVITY_NOT_CALCULATED */
		         /* ? STC_CELLSTATUS_NOTSET                         */
		         /* : (Cell->Sensitivity > 0                        */
		         /*    ? STC_CELLSTATUS_SENSITIVE                   */
		         /*    : STC_CELLSTATUS_NOTSENSITIVE                */
		"%*c"),  /* Cell->Type                                      */
		CELLID_LENGTH, Cell->CellId,
		NUMBERENTRIES_LENGTH, Cell->TotalNumberObservations,
		NUMBERENTRIES_LENGTH, Cell->Data.NumberEntries,
		TOTAL_LENGTH , TOTAL_DECIMAL , Cell->TotalValue          ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalShadow         ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalProxy          ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalValuePt        ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalValuePw        ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalValueN         ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalValueSn        ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalValueX         ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalProxyPt        ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalProxyPw        ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalProxyN         ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalProxySn        ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalProxyX         ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalFT             ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalFW             ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalFS             ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalWeight         ,
		TOTAL_LENGTH , TOTAL_DECIMAL , Cell->TotalWeightedValue  ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalWeightedShadow ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalWeightedProxy  ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalSecondryValuePt,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalSecondryValuePw,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalSecondryProxyPt,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalSecondryProxyPw,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalSecondryFT     ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalSecondryFW     ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalSecondryFS     ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalWeightedValueX ,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalWeightedProxyX ,
		s,
		                               snwv                      ,
		                               snpr                      ,
		                               snwt                      ,
		TYPE_LENGTH                  , Cell->Type                ,
		STATUS_LENGTH, Cell->Sensitivity == SENSITIVITY_NOT_CALCULATED ? STC_CELLSTATUS_NOTSET : (Cell->Sensitivity > 0 ? STC_CELLSTATUS_SENSITIVE : STC_CELLSTATUS_NOTSENSITIVE),
		TYPE_LENGTH, Cell->Type);
	if (Cell->Coordinate != NULL) {
		for (i = 0; i < Cell->Coordinate->NumberEntries; i++)
			EI_AddMessage("", EIE_MESSAGESEVERITY_EMPTY, " %*d", TAG_LENGTH, Cell->Coordinate->Tag[i]);
	}
	EI_AddMessage("", EIE_MESSAGESEVERITY_EMPTY, "\n");
}
/*------------------------------------------------------------------------------
Print the cell largest respondents
------------------------------------------------------------------------------*/
void STC_CellPrintLargest (
	STCT_CELL * Cell)
{
	int i;

	STC_CellPrintInfo (Cell, 1);
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "LARGEST (%d)\n", Cell->LargestNumberEntries);
	for (i = 0; i < Cell->LargestNumberEntries; i++) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, ("Key %-8s "
		                                               "NumberObservations %d "
		                                               "Value %7.2f "
		                                               "Shadow %7.2f "
		                                               "Proxy %7.2f "
		                                               "ValuePt %7.2f "
		                                               "ValuePw %7.2f "
		                                               "ValueN %7.2f "
		                                               "ValueSn %7.2f "
		                                               "ValueX %7.2f "
		                                               "ProxyPt %7.2f "
		                                               "ProxyPw %7.2f "
		                                               "ProxyN %7.2f "
		                                               "ProxySn %7.2f "
		                                               "ProxyX %7.2f "
		                                               "FT %7.2f "
		                                               "FW %7.2f "
		                                               "FS %7.2f "
		                                               "Weight %7.2f "
		                                               "WeightedValue %7.2f "
		                                               "WeightedShadow %7.2f "
		                                               "WeightedProxy %7.2f "
		                                               "SecondryValuePt %7.2f "
		                                               "SecondryValuePw %7.2f "
		                                               "SecondryProxyPt %7.2f "
		                                               "SecondryProxyPw %7.2f "
		                                               "SecondryFT %7.2f "
		                                               "SecondryFW %7.2f "
		                                               "SecondryFS %7.2f "
		                                               "WeightedValueX %7.2f "
		                                               "WeightedProxyX %7.2f "
		                                               "MixedSignStatus %d "
		                                               "waiver_flag %d\n"),
		               Cell->Largest[i]->Key,
		               Cell->Largest[i]->NumberObservations,
		               Cell->Largest[i]->Value,
		               Cell->Largest[i]->Shadow,
		               Cell->Largest[i]->Proxy,
		               Cell->Largest[i]->ValuePt,
		               Cell->Largest[i]->ValuePw,
		               Cell->Largest[i]->ValueN,
		               Cell->Largest[i]->ValueSn,
		               Cell->Largest[i]->ValueX,
		               Cell->Largest[i]->ProxyPt,
		               Cell->Largest[i]->ProxyPw,
		               Cell->Largest[i]->ProxyN,
		               Cell->Largest[i]->ProxySn,
		               Cell->Largest[i]->ProxyX,
		               Cell->Largest[i]->FT,
		               Cell->Largest[i]->FW,
		               Cell->Largest[i]->FS,
		               Cell->Largest[i]->Weight,
		               Cell->Largest[i]->WeightedValue,
		               Cell->Largest[i]->WeightedShadow,
		               Cell->Largest[i]->WeightedProxy,
		               Cell->Largest[i]->SecondryValuePt,
		               Cell->Largest[i]->SecondryValuePw,
		               Cell->Largest[i]->SecondryProxyPt,
		               Cell->Largest[i]->SecondryProxyPw,
		               Cell->Largest[i]->SecondryFT,
		               Cell->Largest[i]->SecondryFW,
		               Cell->Largest[i]->SecondryFS,
		               Cell->Largest[i]->WeightedValueX,
		               Cell->Largest[i]->WeightedProxyX,
		               Cell->Largest[i]->MixedSignStatus,
		               Cell->Largest[i]->waiver_flag
		              );
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "LARGESTFT (%d)\n", Cell->LargestPYNumberEntries);
	for (i = 0; i < Cell->LargestPYNumberEntries; i++) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, ("Key %-8s "
		                                               "NumberObservations %d "
		                                               "Value %7.2f "
		                                               "Shadow %7.2f "
		                                               "Proxy %7.2f "
		                                               "ValuePt %7.2f "
		                                               "ValuePw %7.2f "
		                                               "ValueN %7.2f "
		                                               "ValueSn %7.2f "
		                                               "ValueX %7.2f "
		                                               "ProxyPt %7.2f "
		                                               "ProxyPw %7.2f "
		                                               "ProxyN %7.2f "
		                                               "ProxySn %7.2f "
		                                               "ProxyX %7.2f "
		                                               "FT %7.2f "
		                                               "FW %7.2f "
		                                               "FS %7.2f "
		                                               "Weight %7.2f "
		                                               "WeightedValue %7.2f "
		                                               "WeightedShadow %7.2f "
		                                               "WeightedProxy %7.2f "
		                                               "SecondryValuePt %7.2f "
		                                               "SecondryValuePw %7.2f "
		                                               "SecondryProxyPt %7.2f "
		                                               "SecondryProxyPw %7.2f "
		                                               "SecondryFT %7.2f "
		                                               "SecondryFW %7.2f "
		                                               "SecondryFS %7.2f "
		                                               "WeightedValueX %7.2f "
		                                               "WeightedProxyX %7.2f "
		                                               "MixedSignStatus %d "
		                                               "waiver_flag %d\n"),
		               Cell->LargestPY[i]->Key,
		               Cell->LargestPY[i]->NumberObservations,
		               Cell->LargestPY[i]->Value,
		               Cell->LargestPY[i]->Shadow,
		               Cell->LargestPY[i]->Proxy,
		               Cell->LargestPY[i]->ValuePt,
		               Cell->LargestPY[i]->ValuePw,
		               Cell->LargestPY[i]->ValueN,
		               Cell->LargestPY[i]->ValueSn,
		               Cell->LargestPY[i]->ValueX,
		               Cell->LargestPY[i]->ProxyPt,
		               Cell->LargestPY[i]->ProxyPw,
		               Cell->LargestPY[i]->ProxyN,
		               Cell->LargestPY[i]->ProxySn,
		               Cell->LargestPY[i]->ProxyX,
		               Cell->LargestPY[i]->FT,
		               Cell->LargestPY[i]->FW,
		               Cell->LargestPY[i]->FS,
		               Cell->LargestPY[i]->Weight,
		               Cell->LargestPY[i]->WeightedValue,
		               Cell->LargestPY[i]->WeightedShadow,
		               Cell->LargestPY[i]->WeightedProxy,
		               Cell->LargestPY[i]->SecondryValuePt,
		               Cell->LargestPY[i]->SecondryValuePw,
		               Cell->LargestPY[i]->SecondryProxyPt,
		               Cell->LargestPY[i]->SecondryProxyPw,
		               Cell->LargestPY[i]->SecondryFT,
		               Cell->LargestPY[i]->SecondryFW,
		               Cell->LargestPY[i]->SecondryFS,
		               Cell->LargestPY[i]->WeightedValueX,
		               Cell->LargestPY[i]->WeightedProxyX,
		               Cell->LargestPY[i]->MixedSignStatus,
		               Cell->LargestPY[i]->waiver_flag
		              );
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "LARGESTFT (%d)\n", Cell->LargestWVNumberEntries);
	for (i = 0; i < Cell->LargestWVNumberEntries; i++) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, ("Key %-8s "
		                                               "NumberObservations %d "
		                                               "Value %7.2f "
		                                               "Shadow %7.2f "
		                                               "Proxy %7.2f "
		                                               "ValuePt %7.2f "
		                                               "ValuePw %7.2f "
		                                               "ValueN %7.2f "
		                                               "ValueSn %7.2f "
		                                               "ValueX %7.2f "
		                                               "ProxyPt %7.2f "
		                                               "ProxyPw %7.2f "
		                                               "ProxyN %7.2f "
		                                               "ProxySn %7.2f "
		                                               "ProxyX %7.2f "
		                                               "FT %7.2f "
		                                               "FW %7.2f "
		                                               "FS %7.2f "
		                                               "Weight %7.2f "
		                                               "WeightedValue %7.2f "
		                                               "WeightedShadow %7.2f "
		                                               "WeightedProxy %7.2f "
		                                               "SecondryValuePt %7.2f "
		                                               "SecondryValuePw %7.2f "
		                                               "SecondryProxyPt %7.2f "
		                                               "SecondryProxyPw %7.2f "
		                                               "SecondryFT %7.2f "
		                                               "SecondryFW %7.2f "
		                                               "SecondryFS %7.2f "
		                                               "WeightedValueX %7.2f "
		                                               "WeightedProxyX %7.2f "
		                                               "MixedSignStatus %d "
		                                               "waiver_flag %d\n"),
		               Cell->LargestWV[i]->Key,
		               Cell->LargestWV[i]->NumberObservations,
		               Cell->LargestWV[i]->Value,
		               Cell->LargestWV[i]->Shadow,
		               Cell->LargestWV[i]->Proxy,
		               Cell->LargestWV[i]->ValuePt,
		               Cell->LargestWV[i]->ValuePw,
		               Cell->LargestWV[i]->ValueN,
		               Cell->LargestWV[i]->ValueSn,
		               Cell->LargestWV[i]->ValueX,
		               Cell->LargestWV[i]->ProxyPt,
		               Cell->LargestWV[i]->ProxyPw,
		               Cell->LargestWV[i]->ProxyN,
		               Cell->LargestWV[i]->ProxySn,
		               Cell->LargestWV[i]->ProxyX,
		               Cell->LargestWV[i]->FT,
		               Cell->LargestWV[i]->FW,
		               Cell->LargestWV[i]->FS,
		               Cell->LargestWV[i]->Weight,
		               Cell->LargestWV[i]->WeightedValue,
		               Cell->LargestWV[i]->WeightedShadow,
		               Cell->LargestWV[i]->WeightedProxy,
		               Cell->LargestWV[i]->SecondryValuePt,
		               Cell->LargestWV[i]->SecondryValuePw,
		               Cell->LargestWV[i]->SecondryProxyPt,
		               Cell->LargestWV[i]->SecondryProxyPw,
		               Cell->LargestWV[i]->SecondryFT,
		               Cell->LargestWV[i]->SecondryFW,
		               Cell->LargestWV[i]->SecondryFS,
		               Cell->LargestWV[i]->WeightedValueX,
		               Cell->LargestWV[i]->WeightedProxyX,
		               Cell->LargestWV[i]->MixedSignStatus,
		               Cell->LargestWV[i]->waiver_flag
		              );
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "LARGESTFT (%d)\n", Cell->LargestWPNumberEntries);
	for (i = 0; i < Cell->LargestWPNumberEntries; i++) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, ("Key %-8s "
		                                               "NumberObservations %d "
		                                               "Value %7.2f "
		                                               "Shadow %7.2f "
		                                               "Proxy %7.2f "
		                                               "ValuePt %7.2f "
		                                               "ValuePw %7.2f "
		                                               "ValueN %7.2f "
		                                               "ValueSn %7.2f "
		                                               "ValueX %7.2f "
		                                               "ProxyPt %7.2f "
		                                               "ProxyPw %7.2f "
		                                               "ProxyN %7.2f "
		                                               "ProxySn %7.2f "
		                                               "ProxyX %7.2f "
		                                               "FT %7.2f "
		                                               "FW %7.2f "
		                                               "FS %7.2f "
		                                               "Weight %7.2f "
		                                               "WeightedValue %7.2f "
		                                               "WeightedShadow %7.2f "
		                                               "WeightedProxy %7.2f "
		                                               "SecondryValuePt %7.2f "
		                                               "SecondryValuePw %7.2f "
		                                               "SecondryProxyPt %7.2f "
		                                               "SecondryProxyPw %7.2f "
		                                               "SecondryFT %7.2f "
		                                               "SecondryFW %7.2f "
		                                               "SecondryFS %7.2f "
		                                               "WeightedValueX %7.2f "
		                                               "WeightedProxyX %7.2f "
		                                               "MixedSignStatus %d "
		                                               "waiver_flag %d\n"),
		               Cell->LargestWP[i]->Key,
		               Cell->LargestWP[i]->NumberObservations,
		               Cell->LargestWP[i]->Value,
		               Cell->LargestWP[i]->Shadow,
		               Cell->LargestWP[i]->Proxy,
		               Cell->LargestWP[i]->ValuePt,
		               Cell->LargestWP[i]->ValuePw,
		               Cell->LargestWP[i]->ValueN,
		               Cell->LargestWP[i]->ValueSn,
		               Cell->LargestWP[i]->ValueX,
		               Cell->LargestWP[i]->ProxyPt,
		               Cell->LargestWP[i]->ProxyPw,
		               Cell->LargestWP[i]->ProxyN,
		               Cell->LargestWP[i]->ProxySn,
		               Cell->LargestWP[i]->ProxyX,
		               Cell->LargestWP[i]->FT,
		               Cell->LargestWP[i]->FW,
		               Cell->LargestWP[i]->FS,
		               Cell->LargestWP[i]->Weight,
		               Cell->LargestWP[i]->WeightedValue,
		               Cell->LargestWP[i]->WeightedShadow,
		               Cell->LargestWP[i]->WeightedProxy,
		               Cell->LargestWP[i]->SecondryValuePt,
		               Cell->LargestWP[i]->SecondryValuePw,
		               Cell->LargestWP[i]->SecondryProxyPt,
		               Cell->LargestWP[i]->SecondryProxyPw,
		               Cell->LargestWP[i]->SecondryFT,
		               Cell->LargestWP[i]->SecondryFW,
		               Cell->LargestWP[i]->SecondryFS,
		               Cell->LargestWP[i]->WeightedValueX,
		               Cell->LargestWP[i]->WeightedProxyX,
		               Cell->LargestWP[i]->MixedSignStatus,
		               Cell->LargestWP[i]->waiver_flag
		              );
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "LARGESTFT (%d)\n", Cell->LargestFTNumberEntries);
	for (i = 0; i < Cell->LargestFTNumberEntries; i++) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, ("Key %-8s "
		                                               "NumberObservations %d "
		                                               "Value %7.2f "
		                                               "Shadow %7.2f "
		                                               "Proxy %7.2f "
		                                               "ValuePt %7.2f "
		                                               "ValuePw %7.2f "
		                                               "ValueN %7.2f "
		                                               "ValueSn %7.2f "
		                                               "ValueX %7.2f "
		                                               "ProxyPt %7.2f "
		                                               "ProxyPw %7.2f "
		                                               "ProxyN %7.2f "
		                                               "ProxySn %7.2f "
		                                               "ProxyX %7.2f "
		                                               "FT %7.2f "
		                                               "FW %7.2f "
		                                               "FS %7.2f "
		                                               "Weight %7.2f "
		                                               "WeightedValue %7.2f "
		                                               "WeightedShadow %7.2f "
		                                               "WeightedProxy %7.2f "
		                                               "SecondryValuePt %7.2f "
		                                               "SecondryValuePw %7.2f "
		                                               "SecondryProxyPt %7.2f "
		                                               "SecondryProxyPw %7.2f "
		                                               "SecondryFT %7.2f "
		                                               "SecondryFW %7.2f "
		                                               "SecondryFS %7.2f "
		                                               "WeightedValueX %7.2f "
		                                               "WeightedProxyX %7.2f "
		                                               "MixedSignStatus %d "
		                                               "waiver_flag %d\n"),
		               Cell->LargestFT[i]->Key,
		               Cell->LargestFT[i]->NumberObservations,
		               Cell->LargestFT[i]->Value,
		               Cell->LargestFT[i]->Shadow,
		               Cell->LargestFT[i]->Proxy,
		               Cell->LargestFT[i]->ValuePt,
		               Cell->LargestFT[i]->ValuePw,
		               Cell->LargestFT[i]->ValueN,
		               Cell->LargestFT[i]->ValueSn,
		               Cell->LargestFT[i]->ValueX,
		               Cell->LargestFT[i]->ProxyPt,
		               Cell->LargestFT[i]->ProxyPw,
		               Cell->LargestFT[i]->ProxyN,
		               Cell->LargestFT[i]->ProxySn,
		               Cell->LargestFT[i]->ProxyX,
		               Cell->LargestFT[i]->FT,
		               Cell->LargestFT[i]->FW,
		               Cell->LargestFT[i]->FS,
		               Cell->LargestFT[i]->Weight,
		               Cell->LargestFT[i]->WeightedValue,
		               Cell->LargestFT[i]->WeightedShadow,
		               Cell->LargestFT[i]->WeightedProxy,
		               Cell->LargestFT[i]->SecondryValuePt,
		               Cell->LargestFT[i]->SecondryValuePw,
		               Cell->LargestFT[i]->SecondryProxyPt,
		               Cell->LargestFT[i]->SecondryProxyPw,
		               Cell->LargestFT[i]->SecondryFT,
		               Cell->LargestFT[i]->SecondryFW,
		               Cell->LargestFT[i]->SecondryFS,
		               Cell->LargestFT[i]->WeightedValueX,
		               Cell->LargestFT[i]->WeightedProxyX,
		               Cell->LargestFT[i]->MixedSignStatus,
		               Cell->LargestFT[i]->waiver_flag
		              );
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "LARGESTFT (%d)\n", Cell->LargestFWNumberEntries);
	for (i = 0; i < Cell->LargestFWNumberEntries; i++) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, ("Key %-8s "
		                                               "NumberObservations %d "
		                                               "Value %7.2f "
		                                               "Shadow %7.2f "
		                                               "Proxy %7.2f "
		                                               "ValuePt %7.2f "
		                                               "ValuePw %7.2f "
		                                               "ValueN %7.2f "
		                                               "ValueSn %7.2f "
		                                               "ValueX %7.2f "
		                                               "ProxyPt %7.2f "
		                                               "ProxyPw %7.2f "
		                                               "ProxyN %7.2f "
		                                               "ProxySn %7.2f "
		                                               "ProxyX %7.2f "
		                                               "FT %7.2f "
		                                               "FW %7.2f "
		                                               "FS %7.2f "
		                                               "Weight %7.2f "
		                                               "WeightedValue %7.2f "
		                                               "WeightedShadow %7.2f "
		                                               "WeightedProxy %7.2f "
		                                               "SecondryValuePt %7.2f "
		                                               "SecondryValuePw %7.2f "
		                                               "SecondryProxyPt %7.2f "
		                                               "SecondryProxyPw %7.2f "
		                                               "SecondryFT %7.2f "
		                                               "SecondryFW %7.2f "
		                                               "SecondryFS %7.2f "
		                                               "WeightedValueX %7.2f "
		                                               "WeightedProxyX %7.2f "
		                                               "MixedSignStatus %d "
		                                               "waiver_flag %d\n"),
		               Cell->LargestFW[i]->Key,
		               Cell->LargestFW[i]->NumberObservations,
		               Cell->LargestFW[i]->Value,
		               Cell->LargestFW[i]->Shadow,
		               Cell->LargestFW[i]->Proxy,
		               Cell->LargestFW[i]->ValuePt,
		               Cell->LargestFW[i]->ValuePw,
		               Cell->LargestFW[i]->ValueN,
		               Cell->LargestFW[i]->ValueSn,
		               Cell->LargestFW[i]->ValueX,
		               Cell->LargestFW[i]->ProxyPt,
		               Cell->LargestFW[i]->ProxyPw,
		               Cell->LargestFW[i]->ProxyN,
		               Cell->LargestFW[i]->ProxySn,
		               Cell->LargestFW[i]->ProxyX,
		               Cell->LargestFW[i]->FT,
		               Cell->LargestFW[i]->FW,
		               Cell->LargestFW[i]->FS,
		               Cell->LargestFW[i]->Weight,
		               Cell->LargestFW[i]->WeightedValue,
		               Cell->LargestFW[i]->WeightedShadow,
		               Cell->LargestFW[i]->WeightedProxy,
		               Cell->LargestFW[i]->SecondryValuePt,
		               Cell->LargestFW[i]->SecondryValuePw,
		               Cell->LargestFW[i]->SecondryProxyPt,
		               Cell->LargestFW[i]->SecondryProxyPw,
		               Cell->LargestFW[i]->SecondryFT,
		               Cell->LargestFW[i]->SecondryFW,
		               Cell->LargestFW[i]->SecondryFS,
		               Cell->LargestFW[i]->WeightedValueX,
		               Cell->LargestFW[i]->WeightedProxyX,
		               Cell->LargestFW[i]->MixedSignStatus,
		               Cell->LargestFW[i]->waiver_flag
		              );
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "LARGESTFS (%d)\n", Cell->LargestFSNumberEntries);
	for (i = 0; i < Cell->LargestFSNumberEntries; i++) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, ("Key %-8s "
		                                               "NumberObservations %d "
		                                               "Value %7.2f "
		                                               "Shadow) %7.2f "
		                                               "Proxy %7.2f "
		                                               "ValuePt %7.2f "
		                                               "ValuePw %7.2f "
		                                               "ValueN %7.2f "
		                                               "ValueSn %7.2f "
		                                               "ValueX %7.2f "
		                                               "ProxyPt %7.2f "
		                                               "ProxyPw %7.2f "
		                                               "ProxyN %7.2f "
		                                               "ProxySn %7.2f "
		                                               "ProxyX %7.2f "
		                                               "FT %7.2f "
		                                               "FW %7.2f "
		                                               "FS %7.2f "
		                                               "Weight %7.2f "
		                                               "WeightedValue %7.2f "
		                                               "WeightedShadow %7.2f "
		                                               "WeightedProxy %7.2f "
		                                               "SecondryValuePt %7.2f "
		                                               "SecondryValuePw %7.2f "
		                                               "SecondryProxyPt %7.2f "
		                                               "SecondryProxyPw %7.2f "
		                                               "SecondryFT %7.2f "
		                                               "SecondryFW %7.2f "
		                                               "SecondryFS %7.2f "
		                                               "WeightedValueX %7.2f "
		                                               "WeightedProxyX %7.2f "
		                                               "MixedSignStatus %d "
		                                               "waiver_flag %d\n"),
		               Cell->LargestFS[i]->Key,
		               Cell->LargestFS[i]->NumberObservations,
		               Cell->LargestFS[i]->Value,
		               Cell->LargestFS[i]->Shadow,
		               Cell->LargestFS[i]->Proxy,
		               Cell->LargestFS[i]->ValuePt,
		               Cell->LargestFS[i]->ValuePw,
		               Cell->LargestFS[i]->ValueN,
		               Cell->LargestFS[i]->ValueSn,
		               Cell->LargestFS[i]->ValueX,
		               Cell->LargestFS[i]->ProxyPt,
		               Cell->LargestFS[i]->ProxyPw,
		               Cell->LargestFS[i]->ProxyN,
		               Cell->LargestFS[i]->ProxySn,
		               Cell->LargestFS[i]->ProxyX,
		               Cell->LargestFS[i]->FT,
		               Cell->LargestFS[i]->FW,
		               Cell->LargestFS[i]->FS,
		               Cell->LargestFS[i]->Weight,
		               Cell->LargestFS[i]->WeightedValue,
		               Cell->LargestFS[i]->WeightedShadow,
		               Cell->LargestFS[i]->WeightedProxy,
		               Cell->LargestFS[i]->SecondryValuePt,
		               Cell->LargestFS[i]->SecondryValuePw,
		               Cell->LargestFS[i]->SecondryProxyPt,
		               Cell->LargestFS[i]->SecondryProxyPw,
		               Cell->LargestFS[i]->SecondryFT,
		               Cell->LargestFS[i]->SecondryFW,
		               Cell->LargestFS[i]->SecondryFS,
		               Cell->LargestFS[i]->WeightedValueX,
		               Cell->LargestFS[i]->WeightedProxyX,
		               Cell->LargestFS[i]->MixedSignStatus,
		               Cell->LargestFS[i]->waiver_flag
		              );
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "LARGESTFT (%d)\n", Cell->Largst2FTNumberEntries);
	for (i = 0; i < Cell->Largst2FTNumberEntries; i++) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, ("Key %-8s "
		                                               "NumberObservations %d "
		                                               "Value %7.2f "
		                                               "Shadow %7.2f "
		                                               "Proxy %7.2f "
		                                               "ValuePt %7.2f "
		                                               "ValuePw %7.2f "
		                                               "ValueN %7.2f "
		                                               "ValueSn %7.2f "
		                                               "ValueX %7.2f "
		                                               "ProxyPt %7.2f "
		                                               "ProxyPw %7.2f "
		                                               "ProxyN %7.2f "
		                                               "ProxySn %7.2f "
		                                               "ProxyX %7.2f "
		                                               "FT %7.2f "
		                                               "FW %7.2f "
		                                               "FS %7.2f "
		                                               "Weight %7.2f "
		                                               "WeightedValue %7.2f "
		                                               "WeightedShadow %7.2f "
		                                               "WeightedProxy %7.2f "
		                                               "SecondryValuePt %7.2f "
		                                               "SecondryValuePw %7.2f "
		                                               "SecondryProxyPt %7.2f "
		                                               "SecondryProxyPw %7.2f "
		                                               "SecondryFT %7.2f "
		                                               "SecondryFW %7.2f "
		                                               "SecondryFS %7.2f "
		                                               "WeightedValueX %7.2f "
		                                               "WeightedProxyX %7.2f "
		                                               "MixedSignStatus %d "
		                                               "waiver_flag %d\n"),
		               Cell->Largst2FT[i]->Key,
		               Cell->Largst2FT[i]->NumberObservations,
		               Cell->Largst2FT[i]->Value,
		               Cell->Largst2FT[i]->Shadow,
		               Cell->Largst2FT[i]->Proxy,
		               Cell->Largst2FT[i]->ValuePt,
		               Cell->Largst2FT[i]->ValuePw,
		               Cell->Largst2FT[i]->ValueN,
		               Cell->Largst2FT[i]->ValueSn,
		               Cell->Largst2FT[i]->ValueX,
		               Cell->Largst2FT[i]->ProxyPt,
		               Cell->Largst2FT[i]->ProxyPw,
		               Cell->Largst2FT[i]->ProxyN,
		               Cell->Largst2FT[i]->ProxySn,
		               Cell->Largst2FT[i]->ProxyX,
		               Cell->Largst2FT[i]->FT,
		               Cell->Largst2FT[i]->FW,
		               Cell->Largst2FT[i]->FS,
		               Cell->Largst2FT[i]->Weight,
		               Cell->Largst2FT[i]->WeightedValue,
		               Cell->Largst2FT[i]->WeightedShadow,
		               Cell->Largst2FT[i]->WeightedProxy,
		               Cell->Largst2FT[i]->SecondryValuePt,
		               Cell->Largst2FT[i]->SecondryValuePw,
		               Cell->Largst2FT[i]->SecondryProxyPt,
		               Cell->Largst2FT[i]->SecondryProxyPw,
		               Cell->Largst2FT[i]->SecondryFT,
		               Cell->Largst2FT[i]->SecondryFW,
		               Cell->Largst2FT[i]->SecondryFS,
		               Cell->Largst2FT[i]->WeightedValueX,
		               Cell->Largst2FT[i]->WeightedProxyX,
		               Cell->Largst2FT[i]->MixedSignStatus,
		               Cell->Largst2FT[i]->waiver_flag
		              );
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "LARGESTFT (%d)\n", Cell->Largst2FWNumberEntries);
	for (i = 0; i < Cell->Largst2FWNumberEntries; i++) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, ("Key %-8s "
		                                               "NumberObservations %d "
		                                               "Value %7.2f "
		                                               "Shadow %7.2f "
		                                               "Proxy %7.2f "
		                                               "ValuePt %7.2f "
		                                               "ValuePw %7.2f "
		                                               "ValueN %7.2f "
		                                               "ValueSn %7.2f "
		                                               "ValueX %7.2f "
		                                               "ProxyPt %7.2f "
		                                               "ProxyPw %7.2f "
		                                               "ProxyN %7.2f "
		                                               "ProxySn %7.2f "
		                                               "ProxyX %7.2f "
		                                               "FT %7.2f "
		                                               "FW %7.2f "
		                                               "FS %7.2f "
		                                               "Weight %7.2f "
		                                               "WeightedValue %7.2f "
		                                               "WeightedShadow %7.2f "
		                                               "WeightedProxy %7.2f "
		                                               "SecondryValuePt %7.2f "
		                                               "SecondryValuePw %7.2f "
		                                               "SecondryProxyPt %7.2f "
		                                               "SecondryProxyPw %7.2f "
		                                               "SecondryFT %7.2f "
		                                               "SecondryFW %7.2f "
		                                               "SecondryFS %7.2f "
		                                               "WeightedValueX %7.2f "
		                                               "WeightedProxyX %7.2f "
		                                               "MixedSignStatus %d "
		                                               "waiver_flag %d\n"),
		               Cell->Largst2FW[i]->Key,
		               Cell->Largst2FW[i]->NumberObservations,
		               Cell->Largst2FW[i]->Value,
		               Cell->Largst2FW[i]->Shadow,
		               Cell->Largst2FW[i]->Proxy,
		               Cell->Largst2FW[i]->ValuePt,
		               Cell->Largst2FW[i]->ValuePw,
		               Cell->Largst2FW[i]->ValueN,
		               Cell->Largst2FW[i]->ValueSn,
		               Cell->Largst2FW[i]->ValueX,
		               Cell->Largst2FW[i]->ProxyPt,
		               Cell->Largst2FW[i]->ProxyPw,
		               Cell->Largst2FW[i]->ProxyN,
		               Cell->Largst2FW[i]->ProxySn,
		               Cell->Largst2FW[i]->ProxyX,
		               Cell->Largst2FW[i]->FT,
		               Cell->Largst2FW[i]->FW,
		               Cell->Largst2FW[i]->FS,
		               Cell->Largst2FW[i]->Weight,
		               Cell->Largst2FW[i]->WeightedValue,
		               Cell->Largst2FW[i]->WeightedShadow,
		               Cell->Largst2FW[i]->WeightedProxy,
		               Cell->Largst2FW[i]->SecondryValuePt,
		               Cell->Largst2FW[i]->SecondryValuePw,
		               Cell->Largst2FW[i]->SecondryProxyPt,
		               Cell->Largst2FW[i]->SecondryProxyPw,
		               Cell->Largst2FW[i]->SecondryFT,
		               Cell->Largst2FW[i]->SecondryFW,
		               Cell->Largst2FW[i]->SecondryFS,
		               Cell->Largst2FW[i]->WeightedValueX,
		               Cell->Largst2FW[i]->WeightedProxyX,
		               Cell->Largst2FW[i]->MixedSignStatus,
		               Cell->Largst2FW[i]->waiver_flag
		              );
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "LARGESTFS (%d)\n", Cell->Largst2FSNumberEntries);
	for (i = 0; i < Cell->Largst2FSNumberEntries; i++) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, ("Key %-8s "
		                                               "NumberObservations %d "
		                                               "Value %7.2f "
		                                               "Shadow) %7.2f "
		                                               "Proxy %7.2f "
		                                               "ValuePt %7.2f "
		                                               "ValuePw %7.2f "
		                                               "ValueN %7.2f "
		                                               "ValueSn %7.2f "
		                                               "ValueX %7.2f "
		                                               "ProxyPt %7.2f "
		                                               "ProxyPw %7.2f "
		                                               "ProxyN %7.2f "
		                                               "ProxySn %7.2f "
		                                               "ProxyX %7.2f "
		                                               "FT %7.2f "
		                                               "FW %7.2f "
		                                               "FS %7.2f "
		                                               "Weight %7.2f "
		                                               "WeightedValue %7.2f "
		                                               "WeightedShadow %7.2f "
		                                               "WeightedProxy %7.2f "
		                                               "SecondryValuePt %7.2f "
		                                               "SecondryValuePw %7.2f "
		                                               "SecondryProxyPt %7.2f "
		                                               "SecondryProxyPw %7.2f "
		                                               "SecondryFT %7.2f "
		                                               "SecondryFW %7.2f "
		                                               "SecondryFS %7.2f "
		                                               "WeightedValueX %7.2f "
		                                               "WeightedProxyX %7.2f "
		                                               "MixedSignStatus %d "
		                                               "waiver_flag %d\n"),
		               Cell->Largst2FS[i]->Key,
		               Cell->Largst2FS[i]->NumberObservations,
		               Cell->Largst2FS[i]->Value,
		               Cell->Largst2FS[i]->Shadow,
		               Cell->Largst2FS[i]->Proxy,
		               Cell->Largst2FS[i]->ValuePt,
		               Cell->Largst2FS[i]->ValuePw,
		               Cell->Largst2FS[i]->ValueN,
		               Cell->Largst2FS[i]->ValueSn,
		               Cell->Largst2FS[i]->ValueX,
		               Cell->Largst2FS[i]->ProxyPt,
		               Cell->Largst2FS[i]->ProxyPw,
		               Cell->Largst2FS[i]->ProxyN,
		               Cell->Largst2FS[i]->ProxySn,
		               Cell->Largst2FS[i]->ProxyX,
		               Cell->Largst2FS[i]->FT,
		               Cell->Largst2FS[i]->FW,
		               Cell->Largst2FS[i]->FS,
		               Cell->Largst2FS[i]->Weight,
		               Cell->Largst2FS[i]->WeightedValue,
		               Cell->Largst2FS[i]->WeightedShadow,
		               Cell->Largst2FS[i]->WeightedProxy,
		               Cell->Largst2FS[i]->SecondryValuePt,
		               Cell->Largst2FS[i]->SecondryValuePw,
		               Cell->Largst2FS[i]->SecondryProxyPt,
		               Cell->Largst2FS[i]->SecondryProxyPw,
		               Cell->Largst2FS[i]->SecondryFT,
		               Cell->Largst2FS[i]->SecondryFW,
		               Cell->Largst2FS[i]->SecondryFS,
		               Cell->Largst2FS[i]->WeightedValueX,
		               Cell->Largst2FS[i]->WeightedProxyX,
		               Cell->Largst2FS[i]->MixedSignStatus,
		               Cell->Largst2FS[i]->waiver_flag
		              );
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
}


/*-calculate the values of:                                  */
/*     for i=0 to i=Cell->Data.NumberEntries:                */
/*         Cell->Data.Item[i]->FT                            */
/*         Cell->Data.Item[i]->FW                            */
/*         Cell->Data.Item[i]->FS                            */
/*         Cell->Data.Item[i]->SecondryFT                    */
/*         Cell->Data.Item[i]->SecondryFW                    */
/*         Cell->Data.Item[i]->SecondryFS                    */
/*     Cell->TotalFT                                         */
/*     Cell->TotalFW                                         */
/*     Cell->TotalFS                                         */
/*     Cell->TotalSecondryFT                                 */
/*     Cell->TotalSecondryFW                                 */
/*     Cell->TotalSecondryFS                                 */
/*     Cell->LargestFT and Cell->LargestFTNumberEntries      */
/*     Cell->LargestFW and Cell->LargestFWNumberEntries      */
/*     Cell->LargestFS and Cell->LargestFSNumberEntries      */
/*     Cell->Largst2FT and Cell->Largst2FTNumberEntries      */
/*     Cell->Largst2FW and Cell->Largst2FWNumberEntries      */
/*     Cell->Largst2FS and Cell->Largst2FSNumberEntries      */
/* :                                                         */
static int CalculateFXAndLargestFX (int               ProxySpecifiedParm,
                                    STCT_SRULE *      SRule,
                                    STCT_CELL *       Cell)
{
    /*-inputs       :                                 */
    /*     int               ProxySpecifiedParm;      */
    /*     STCT_SRULE *      SRule;                   */  /*-(actually only "SRule->Type") */
    /*     STCT_CELL *       Cell;                    */
    /* outputs      :                                 */
    /*     STCT_CELL *       Cell;                    */ /*-(actually only the "FT", "FS", "SecondryFT", and "SecondryFS" members of the elements of the array "Cell->Data.Item"   */
    /*                                                */ /*  and the values "Cell->LargestFT", "Cell->LargestFS", "Cell->LargestFTNumberEntries", "Cell->LargestFSNumberEntries",  */
    /*                                                */ /*                 "Cell->Largst2FT", "Cell->Largst2FS", "Cell->Largst2FTNumberEntries", "Cell->Largst2FSNumberEntries",  */
    /*                                                */ /*  "Cell->TotalFT", "Cell->TotalFS", "Cell->TotalSecondryFT", and "Cell->TotalSecondryFS"                                */
    /*                                                */ /* )                                                                                                                      */
    /*     int               <return code (always=1)> */
    /* intermediates:                                 */
    /*     int i;                                     */
    /*     int rc;                                    */
    /*     STCT_DATAITEM * DataItemPtr;               */
    
    int i;
    int rc;
    STCT_DATAITEM * DataItemPtr;
    
    Cell->TotalFT         = 0.0;
    Cell->TotalFS         = 0.0;
    Cell->TotalSecondryFT = 0.0;
    Cell->TotalSecondryFS = 0.0;
    for (i = 0; i <= Cell->Data.NumberEntries; i++)
    {
        if (i < Cell->Data.NumberEntries) {
            DataItemPtr = Cell->Data.Item[i];
        }
        else { /* (i == Cell->Data.NumberEntries) */
            if (Cell->AnonymousDataItem.NumberObservations == 0) {
                continue;
            }
            DataItemPtr = &(Cell->AnonymousDataItem);
        }
        if (ProxySpecifiedParm) {
            DataItemPtr->FT = DataItemPtr->ProxyPt + DataItemPtr->ProxyN ;
            DataItemPtr->FS = DataItemPtr->ProxyN  - DataItemPtr->ProxySn;
        }
        else {
            DataItemPtr->FT = DataItemPtr->ValuePt + DataItemPtr->ValueN ;
            DataItemPtr->FS = DataItemPtr->ValueN  - DataItemPtr->ValueSn;
        }
        Cell->TotalFT   = Cell->TotalFT        + DataItemPtr->FT     ;
        Cell->TotalFS   = Cell->TotalFS        + DataItemPtr->FS     ;
        if (SRule->NumberGroups == 2) {
            if (ProxySpecifiedParm) {
                DataItemPtr->SecondryFT = DataItemPtr->SecondryProxyPt + DataItemPtr->        ProxyN ;
                DataItemPtr->SecondryFS = DataItemPtr->        ProxyN  - DataItemPtr->        ProxySn;
            }
            else {
                DataItemPtr->SecondryFT = DataItemPtr->SecondryValuePt + DataItemPtr->        ValueN ;
                DataItemPtr->SecondryFS = DataItemPtr->        ValueN  - DataItemPtr->        ValueSn;
            }
            Cell->TotalSecondryFT   = Cell->TotalSecondryFT        + DataItemPtr->SecondryFT     ;
            Cell->TotalSecondryFS   = Cell->TotalSecondryFS        + DataItemPtr->SecondryFS     ;
        }
    }
    
    for (i = 0; i < STCM_MAXLARGESTS; i = i + 1) {
        Cell->LargestFT[i] = NULL;
        Cell->LargestFS[i] = NULL;
    }
    Cell->LargestFTNumberEntries = 0;
    Cell->LargestFSNumberEntries = 0;
    
    for (i = 0; i < STCM_MAXLARGESTS; i = i + 1) {
        Cell->Largst2FT[i] = NULL;
        Cell->Largst2FS[i] = NULL;
    }
    Cell->Largst2FTNumberEntries = 0;
    Cell->Largst2FSNumberEntries = 0;
    
    rc = FindLargestFX(Cell, Cell->LargestFT, &(Cell->LargestFTNumberEntries), 'T');
    if (rc != EIE_SUCCEED) {
        return 0;
    }
    rc = FindLargestFX(Cell, Cell->LargestFS, &(Cell->LargestFSNumberEntries), 'S');
    if (rc != EIE_SUCCEED) {
        return 0;
    }
    if (SRule->NumberGroups == 2) {
        rc = FindLargestFX(Cell, Cell->Largst2FT, &(Cell->Largst2FTNumberEntries), 'T');
        if (rc != EIE_SUCCEED) {
            return 0;
        }
        rc = FindLargestFX(Cell, Cell->Largst2FS, &(Cell->Largst2FSNumberEntries), 'S');
        if (rc != EIE_SUCCEED) {
            return 0;
        }
    }
    return 1;
}
/*--------------------------------------------------------------------------------------------------
Calculate the sensitivity variables for a cell (type STCT_CELL) or a dataitem (type STCT_DATAITEM)
--------------------------------------------------------------------------------------------------*/
static int CalculateSensitivityVariables(void * StructPtr,
	int C,
	STCT_SRULE * SRule,
	double alphaplus1,
	double alphaplus2,
	double ProxyRatio,
	char WeightProtectionLevel)
{
	int ReturnCode;
	
	double ValueAdj1;
	double ValueAdj2;
	double ProxyAdj1;
	double ProxyAdj2;
	
	double ValuePt;
	double ValueN;
	double ValueSn;
	double ValueX;
	double WeightedValueX;
	double ProxyPt;
	double ProxyN;
	double ProxySn;
	double ProxyX;
	double WeightedProxyX;
	double SecondryValuePt;
	double SecondryProxyPt;
	
	ReturnCode = 1;
	
	if (C != 0 && C != 1) {
		ReturnCode = 0;
		goto error_cleanup;
	}
	
	ValueAdj1           =  (WeightProtectionLevel == 'L')?                fabs(SENSVAR(StructPtr, C, WeightedValue) -     SENSVAR(StructPtr, C, Value        ))
	                      :(WeightProtectionLevel == 'M')?((alphaplus1/2)*fabs(SENSVAR(StructPtr, C, WeightedValue) -     SENSVAR(StructPtr, C, Value        )))
	                      :(WeightProtectionLevel == 'H')?((fabs(SENSVAR(StructPtr, C, WeightedValue) -     SENSVAR(StructPtr, C, Value        )) <
	                                                        ((2.0-alphaplus1)*fabs(SENSVAR(StructPtr, C, Value        )))
	                                                       )?0.0
	                                                        :((fabs(SENSVAR(StructPtr, C, WeightedValue) -     SENSVAR(StructPtr, C, Value        )) <=
	                                                           ((2.0           )*fabs(SENSVAR(StructPtr, C, Value        )))
	                                                          )?(fabs(SENSVAR(StructPtr, C, WeightedValue) -     SENSVAR(StructPtr, C, Value        )) -
	                                                             ((2.0-alphaplus1)*fabs(SENSVAR(StructPtr, C, Value        )))
	                                                            )
	                                                           :((    alphaplus1)*fabs(SENSVAR(StructPtr, C, Value        )))
	                                                         )
	                                                      )
	                      :                               0.0;
	ProxyAdj1           =  (WeightProtectionLevel == 'L')?                fabs(SENSVAR(StructPtr, C, WeightedValue) -     SENSVAR(StructPtr, C, Value        ))
	                      :(WeightProtectionLevel == 'M')?((alphaplus1/2)*fabs((ProxyRatio * SENSVAR(StructPtr, C, WeightedProxy)) -     (ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))))
	                      :(WeightProtectionLevel == 'H')?((fabs((ProxyRatio * SENSVAR(StructPtr, C, WeightedProxy)) -     (ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))) <
	                                                        ((2.0-alphaplus1)*fabs((ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))))
	                                                       )?0.0
	                                                        :((fabs((ProxyRatio * SENSVAR(StructPtr, C, WeightedProxy)) -     (ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))) <=
	                                                           ((2.0           )*fabs((ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))))
	                                                          )?(fabs((ProxyRatio * SENSVAR(StructPtr, C, WeightedProxy)) -     (ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))) -
	                                                             ((2.0-alphaplus1)*fabs((ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))))
	                                                            )
	                                                           :((    alphaplus1)*fabs((ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))))
	                                                         )
	                                                      )
	                      :                               0.0;
	ValuePt             =   alphaplus1 * fabs(             SENSVAR(StructPtr, C, Value        ))-ValueAdj1;
	ValueN              =                fabs(             SENSVAR(StructPtr, C, WeightedValue));
	ValueSn             =                fabs(             SENSVAR(StructPtr, C, WeightedValue)) - fabs(SENSVAR(StructPtr, C, Value)        );
	ProxyPt             =  (             fabs(             SENSVAR(StructPtr, C, Value        )) >= (ProxyRatio * fabs(SENSVAR(StructPtr, C, Proxy        ))))
	                      ?(alphaplus1 * fabs(             SENSVAR(StructPtr, C, Value        ))-ValueAdj1)
	                      :(alphaplus1 * fabs(ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))-ProxyAdj1);
	ProxyN              =  (             fabs(             SENSVAR(StructPtr, C, Value        )) >= (ProxyRatio * fabs(SENSVAR(StructPtr, C, Proxy        ))))
	                      ?              fabs(             SENSVAR(StructPtr, C, WeightedValue))
	                      :(             fabs(ProxyRatio * SENSVAR(StructPtr, C, WeightedProxy)));
	ProxySn             =  (             fabs(             SENSVAR(StructPtr, C, Value        )) >= (ProxyRatio * fabs(SENSVAR(StructPtr, C, Proxy        ))))
	                      ?(             fabs(             SENSVAR(StructPtr, C, WeightedValue))-
	                                     fabs(             SENSVAR(StructPtr, C, Value        )))
	                      :(             fabs(ProxyRatio * SENSVAR(StructPtr, C, WeightedProxy))-
	                                     fabs(ProxyRatio * SENSVAR(StructPtr, C, Proxy        )));
	ValueX              =   fabs(             SENSVAR(StructPtr, C, Value        ));
	WeightedValueX      =   fabs(             SENSVAR(StructPtr, C, WeightedValue));
	ProxyX              = ( fabs(             SENSVAR(StructPtr, C, Value        )) >= (ProxyRatio * fabs(SENSVAR(StructPtr, C, Proxy        ))))
	                      ? fabs(             SENSVAR(StructPtr, C, Value        ))
	                      :(fabs(ProxyRatio * SENSVAR(StructPtr, C, Proxy        )));
	WeightedProxyX      = ( fabs(             SENSVAR(StructPtr, C, Value        )) >= (ProxyRatio * fabs(SENSVAR(StructPtr, C, Proxy        ))))
	                      ? fabs(             SENSVAR(StructPtr, C, WeightedValue))
	                      :(fabs(ProxyRatio * SENSVAR(StructPtr, C, WeightedProxy)));
	
	ValuePt             = FMAX(0.0, ValuePt);
	ValueN              = FMAX(0.0, ValueN );
	ValueSn             = FMAX(0.0, ValueSn);
	ProxyPt             = FMAX(0.0, ProxyPt);
	ProxyN              = FMAX(0.0, ProxyN );
	ProxySn             = FMAX(0.0, ProxySn);
	
	if (ValuePt > ValueN) {
		ValuePt = ValueN;
	}
	if (ProxyPt > ProxyN) {
		ProxyPt = ProxyN;
	}
	if (ValueSn > ValueN) {
		ValueSn = ValueN;
	}
	if (ProxySn > ProxyN) {
		ProxySn = ProxyN;
	}
	if (ValuePt > (alphaplus1 * WeightedValueX)) {
		ValuePt = alphaplus1 * WeightedValueX;
	}
	if (ProxyPt > (alphaplus1 * WeightedProxyX)) {
		ProxyPt = alphaplus1 * WeightedProxyX;
	}
	
	if (C == 0 && (((STCT_DATAITEM *)StructPtr)->Key)[0] == '\0') {
		/*-StructPtr points to the STCT_DATAITEM instance for anonymous contributors: */
		ValuePt = 0.0;
		ValueSn = ValueN;
		ProxyPt = 0.0;
		ProxySn = ProxyN;
	}
	
	SecondryValuePt     = 0.0;
	SecondryProxyPt     = 0.0;
	
	if ((SRule->Type == STCE_SRULE_TYPE_NK) && SRule->NumberGroups == 2) {
		ValueAdj2       =  (WeightProtectionLevel == 'L')?                fabs(SENSVAR(StructPtr, C, WeightedValue) -     SENSVAR(StructPtr, C, Value        ))
		                  :(WeightProtectionLevel == 'M')?((alphaplus2/2)*fabs(SENSVAR(StructPtr, C, WeightedValue) -     SENSVAR(StructPtr, C, Value        )))
		                  :(WeightProtectionLevel == 'H')?((fabs(SENSVAR(StructPtr, C, WeightedValue) -     SENSVAR(StructPtr, C, Value        )) <
		                                                    ((2.0-alphaplus2)*fabs(SENSVAR(StructPtr, C, Value        )))
		                                                   )?0.0
		                                                    :((fabs(SENSVAR(StructPtr, C, WeightedValue) -     SENSVAR(StructPtr, C, Value        )) <=
		                                                       ((2.0           )*fabs(SENSVAR(StructPtr, C, Value        )))
		                                                      )?(fabs(SENSVAR(StructPtr, C, WeightedValue) -     SENSVAR(StructPtr, C, Value        )) -
		                                                         ((2.0-alphaplus2)*fabs(SENSVAR(StructPtr, C, Value        )))
		                                                        )
		                                                       :((    alphaplus2)*fabs(SENSVAR(StructPtr, C, Value        )))
		                                                     )
		                                                  )
		                  :                               0.0;
		ProxyAdj2       =  (WeightProtectionLevel == 'L')?                fabs(SENSVAR(StructPtr, C, WeightedValue) -     SENSVAR(StructPtr, C, Value        ))
		                  :(WeightProtectionLevel == 'M')?((alphaplus2/2)*fabs((ProxyRatio * SENSVAR(StructPtr, C, WeightedProxy)) -     (ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))))
		                  :(WeightProtectionLevel == 'H')?((fabs((ProxyRatio * SENSVAR(StructPtr, C, WeightedProxy)) -     (ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))) <
		                                                    ((2.0-alphaplus2)*fabs((ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))))
		                                                   )?0.0
		                                                    :((fabs((ProxyRatio * SENSVAR(StructPtr, C, WeightedProxy)) -     (ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))) <=
		                                                       ((2.0           )*fabs((ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))))
		                                                      )?(fabs((ProxyRatio * SENSVAR(StructPtr, C, WeightedProxy)) -     (ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))) -
		                                                         ((2.0-alphaplus2)*fabs((ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))))
		                                                        )
		                                                       :((    alphaplus2)*fabs((ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))))
		                                                     )
		                                                  )
		                  :                               0.0;
		SecondryValuePt =   alphaplus2 * fabs(             SENSVAR(StructPtr, C, Value        ))-ValueAdj2;
		SecondryProxyPt =  (             fabs(             SENSVAR(StructPtr, C, Value        )) >= (ProxyRatio * fabs(SENSVAR(StructPtr, C, Proxy        ))))
	                          ?(alphaplus2 * fabs(             SENSVAR(StructPtr, C, Value        ))-ValueAdj2)
	                          :(alphaplus2 * fabs(ProxyRatio * SENSVAR(StructPtr, C, Proxy        ))-ProxyAdj2);
		SecondryValuePt = FMAX(0.0, SecondryValuePt);
		SecondryProxyPt = FMAX(0.0, SecondryProxyPt);
		
		if (SecondryValuePt > ValueN) {
			SecondryValuePt = ValueN;
		}
		if (SecondryProxyPt > ProxyN) {
			SecondryProxyPt = ProxyN;
		}
		if (SecondryValuePt > (alphaplus2 * WeightedValueX)) {
			SecondryValuePt = alphaplus2 * WeightedValueX;
		}
		if (SecondryProxyPt > (alphaplus2 * WeightedProxyX)) {
			SecondryProxyPt = alphaplus2 * WeightedProxyX;
		}
		
		if (C == 0 && (((STCT_DATAITEM *)StructPtr)->Key)[0] == '\0') {
			/*-StructPtr points to the STCT_DATAITEM instance for anonymous contributors: */
			SecondryValuePt = 0.0;
			SecondryProxyPt = 0.0;
		}
	}
#ifdef STC_C2_OR_DUFFETT_IS_SUPPORTED
	#include "internal_code/src/STC_Cell_h1.h"
#endif
	if (C) {
		((STCT_CELL     *)StructPtr)->TotalValuePt         = ValuePt        ;
		((STCT_CELL     *)StructPtr)->TotalValueN          = ValueN         ;
		((STCT_CELL     *)StructPtr)->TotalValueSn         = ValueSn        ;
		((STCT_CELL     *)StructPtr)->TotalValueX          = ValueX         ;
		((STCT_CELL     *)StructPtr)->TotalWeightedValueX  = WeightedValueX ;
		((STCT_CELL     *)StructPtr)->TotalProxyPt         = ProxyPt        ;
		((STCT_CELL     *)StructPtr)->TotalProxyN          = ProxyN         ;
		((STCT_CELL     *)StructPtr)->TotalProxySn         = ProxySn        ;
		((STCT_CELL     *)StructPtr)->TotalProxyX          = ProxyX         ;
		((STCT_CELL     *)StructPtr)->TotalWeightedProxyX  = WeightedProxyX ;
		((STCT_CELL     *)StructPtr)->TotalSecondryValuePt = SecondryValuePt;
		((STCT_CELL     *)StructPtr)->TotalSecondryProxyPt = SecondryProxyPt;
	}
	else {
		((STCT_DATAITEM *)StructPtr)->ValuePt              = ValuePt        ;
		((STCT_DATAITEM *)StructPtr)->ValueN               = ValueN         ;
		((STCT_DATAITEM *)StructPtr)->ValueSn              = ValueSn        ;
		((STCT_DATAITEM *)StructPtr)->ValueX               = ValueX         ;
		((STCT_DATAITEM *)StructPtr)->WeightedValueX       = WeightedValueX ;
		((STCT_DATAITEM *)StructPtr)->ProxyPt              = ProxyPt        ;
		((STCT_DATAITEM *)StructPtr)->ProxyN               = ProxyN         ;
		((STCT_DATAITEM *)StructPtr)->ProxySn              = ProxySn        ;
		((STCT_DATAITEM *)StructPtr)->ProxyX               = ProxyX         ;
		((STCT_DATAITEM *)StructPtr)->WeightedProxyX       = WeightedProxyX ;
		((STCT_DATAITEM *)StructPtr)->SecondryValuePt      = SecondryValuePt;
		((STCT_DATAITEM *)StructPtr)->SecondryProxyPt      = SecondryProxyPt;
	}
	
	goto normal_cleanup;
	
error_cleanup:
	
normal_cleanup:
	
	return ReturnCode;
}
/*------------------------------------------------------------------------------
Calculate the sensitivity of a cell
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_CellSensitivity (
	STCT_CELL * Cell,
	STCT_SRULE * SRule,
	STCT_COORDINATE * Coordinate)
{
	double entvalue[STCM_MAXALPHA];
	double entproxy[STCM_MAXALPHA];
	double entwvalu[STCM_MAXALPHA];
	double entwprxy[STCM_MAXALPHA];
	STCT_DATAITEM * DataItemPtr;
	char  ptn_type1;
	char  ptn_type2;
	double alphaplus1;
	double alphaplus2;
	int    entwaivers[STCM_MAXALPHA];
	int    entwvproxy[STCM_MAXALPHA];
	int    entwvwvalu[STCM_MAXALPHA];
	int    entwvwprxy[STCM_MAXALPHA];
	int i;
	int n;
	int nPY;
	int nWV;
	int nWP;
	int nParm;

	int waiver_flags_present;

	int rc;
	EIT_RETURNCODE RetCode;
	EIT_RETURNCODE RetCodeSRuleSens;

	double DoubleValueToBeIgnored;

	int    weight_specified;
	int    proxyratio_specified;
	int    proxy_specified;
	int    additive_noise_specified;
	int    weight_diagnostics_specified;
	int    proxy_diagnostics_specified;
	char   WeightProtectionLevel;
	int    minrespw_specified;
	double MinRespW;
	double ProxyRatio;

	int NonAnonymousPositiveProxyNumberEntries;
	int NonAnonymousPositiveWValuNumberEntries;
	int NonAnonymousPositiveWPrxyNumberEntries;

	int      waiver_flags_present_parm;
	double * entvalParm;
	int    * entwaiversParm;
	double   TotalValueParm;
	double   AnonymousValueParm;
	int      NonAnonymousPositiveNumberEntriesParm;
	int      HasAnonymousParm;
	int      WeightSpecifiedParm;
	int      ProxySpecifiedParm;
	int      UsePtnSensitivityParm;
	int      UseAdditiveNoiseParm;
	int      NonPtnSensitivityVariableParm;



	RetCode = EIE_SUCCEED;


	if (Cell->Sensitivity != SENSITIVITY_NOT_CALCULATED) {return RetCode;}

	waiver_flags_present = SRule->waiver_flags_present;

	/*-options (parameters with a value of 1 (specified) or 0 (not specified): */
	weight_specified             = SRule->Parms->Weight;
	proxy_specified              = SRule->Parms->Proxy;
	proxyratio_specified         = SRule->Parms->ProxyRatioSpecified;
	additive_noise_specified     = SRule->Parms->AdditiveNoise;
	weight_diagnostics_specified = SRule->Parms->WeightDiagnostics;
	proxy_diagnostics_specified  = SRule->Parms->ProxyDiagnostics;
	minrespw_specified           = (SRule->Parms->MinRespW) != MINRESPW_UNSPECD_VALUE;

	/*-parameters with value besides specified or not specified: */
	MinRespW                     = SRule->Parms->MinRespW;
	WeightProtectionLevel        = SRule->Parms->WeightProtectionLevel;
	ProxyRatio                   = SRule->Parms->ProxyRatio;


	/* SRule->Parms->ProxyRatio            parm                                     */
	/* SRule->Parms->ProxyPercentile       parm (only used to calculate proxyratio) */
	/* SRule->Parms->ProxyDiagnostics      opt                                      */
	/* SRule->Parms->WeightDiagnostics     opt                                      */
	/* SRule->Parms->WeightProtectionLevel parm                                     */
	/* SRule->Parms->MinRespW              parm                                     */
	/* SRule->Parms->AdditiveNoise         opt                                      */

	/*-arguments to be passed to call to "STC_SRule.c::STC_SRuleSensitivity()"                            */
	/* that can change when:                                                                              */
	/*    -waiver_flags_present                                                                           */
	/*    -weight_specified                                                                               */
	/*    -proxy_specified                                                                                */
	/* are changed ("HasAnonymous" is 1 if the # of records input to cell with blank id (including any    */
	/* with zero values) is > 0, so it won't change when the "variable of interest" changes):             */
	/*    -waiver_flags_present:                                                                          */
	/*        -waiver_flags_present                                                                       */
	/*        -sensitivity_nowaivers                                                                      */
	/*    -weight_specified:                                                                              */
	/*        -weight_specified                                                                           */
	/*        -UsePtnSensitivity                                                                          */
	/*        -NonPtnSensitivityVariable                                                                  */
	/*        -entvalue                                                                                   */
	/*        -TotalValue                                                                                 */
	/*        -AnonymousValue                                                                             */
	/*        -NonAnonymousPositiveNumberEntries                                                          */
	/*    -proxy_specified:                                                                               */
	/*        -proxy_specified                                                                            */
	/*        -NonPtnSensitivityVariable                                                                  */
	/*        -entvalue                                                                                   */
	/*        -TotalValue                                                                                 */
	/*        -AnonymousValue                                                                             */
	/*        -NonAnonymousPositiveNumberEntries                                                          */

	/* if ((!Cell->IsInternal) || (!(proxy_specified && !proxyratio_specified))) {} */
	/*-the immediately preceding line was incorrect, because "sensitiv.c::CalculateProxyRatio()" is    */
	/* called to calculate Proxy->ProxyRatio before calculating any sensitivities iff the "PROXYRATIO" */
	/* option isn't specified, regardless of whether the "PROXYPERCENTILE" option is specified:        */
	/* if ((!Cell->IsInternal) || (Cell->IsInternal && (!proxyratio_specified))) {} */
	/*-the immediately preceding line was incorrect:                                                   */
	if ((!Cell->IsInternal) || (Cell->IsInternal && proxyratio_specified)) {
		SortKeys (Cell);
		RemoveDuplicateKeys (Cell);
	}

	/*-calculate sensitivity variables: */
	switch (SRule->Type) {
		case STCE_SRULE_TYPE_PQ:
			/*-for pq alpha      = [1.0 + pq, 1.0, 0.0, 0.0, 0.0] */
			/*        alphaplus1 = pq                             */
			/* so     alphaplus1 = SRule->Alpha[0][0]-1.0         */
			ptn_type1  = '1';
			ptn_type2  = ' ';
			alphaplus1 = SRule->Alpha[0][0]-1.0;
			alphaplus2 = 0.0;
			break;
		case STCE_SRULE_TYPE_NK:
			/*-for nk alpha      = [<100.0/k repeated n times> , <0.0 repeated (5-n) times>] */
			/*        alphaplus1 = (100.0/k)-1                                               */
			/* so     alphaplus1 = (100.0 / (100.0 / SRule->Alpha[0][0])) - 1.0              */
			/*                   =                   SRule->Alpha[0][0]   - 1.0              */
			ptn_type1  = 'n';
			alphaplus1 = (100.0 / (100.0 / SRule->Alpha[0][0])) - 1.0;
			if (SRule->NumberGroups == 2) {
			    ptn_type2  = 'n';
			    alphaplus2 = (100.0 / (100.0 / SRule->Alpha[1][0])) - 1.0;
			}
			else {
			    ptn_type2  = ' ';
			    alphaplus2 = 0.0;
			}
			break;
#ifdef STC_C2_IS_SUPPORTED
	#include "internal_code/src/STC_Cell_h2.h"
#endif
#ifdef STC_DUFFETT_IS_SUPPORTED
	#include "internal_code/src/STC_Cell_h3.h"
#endif
		default:
			ptn_type1  = ' ';
			ptn_type2  = ' ';
			alphaplus1 = 0.0;
			alphaplus2 = 0.0;
			break;
	}
	/*-if  (!Cell->IsInternal && additive_noise_specified) */
	/* then the values of:                                 */
	/*     Cell->TotalValuePt                              */
	/*     Cell->TotalValueN                               */
	/*     Cell->TotalValueSn                              */
	/*     Cell->TotalValueX                               */
	/*     Cell->TotalWeightedValueX                       */
	/*     Cell->TotalProxyPt                              */
	/*     Cell->TotalProxyN                               */
	/*     Cell->TotalProxySn                              */
	/*     Cell->TotalProxyX                               */
	/*     Cell->TotalWeightedProxyX                       */
	/*     Cell->TotalSecondryValuePt                      */
	/*     Cell->TotalSecondryProxyPt                      */
	/* and:                                                */
	/*     Cell->AnonymousDataItem[i].ValuePt              */
	/*     Cell->AnonymousDataItem[i].ValueN               */
	/*     Cell->AnonymousDataItem[i].ValueSn              */
	/*     Cell->AnonymousDataItem[i].ValueX               */
	/*     Cell->AnonymousDataItem[i].WeightedValueX       */
	/*     Cell->AnonymousDataItem[i].ProxyPt              */
	/*     Cell->AnonymousDataItem[i].ProxyN               */
	/*     Cell->AnonymousDataItem[i].ProxySn              */
	/*     Cell->AnonymousDataItem[i].ProxyX               */
	/*     Cell->AnonymousDataItem[i].WeightedProxyX       */
	/*     Cell->AnonymousDataItem[i].SecondryValuePt      */
	/*     Cell->AnonymousDataItem[i].SecondryProxyPt      */
	/* and (for i=0 to Cell->Data.NumberEntries):          */
	/*     Cell->Data.Item[i]->ValuePt                     */
	/*     Cell->Data.Item[i]->ValueN                      */
	/*     Cell->Data.Item[i]->ValueSn                     */
	/*     Cell->Data.Item[i]->ValueX                      */
	/*     Cell->Data.Item[i]->WeightedValueX              */
	/*     Cell->Data.Item[i]->ProxyPt                     */
	/*     Cell->Data.Item[i]->ProxyN                      */
	/*     Cell->Data.Item[i]->ProxySn                     */
	/*     Cell->Data.Item[i]->ProxyX                      */
	/*     Cell->Data.Item[i]->WeightedProxyX              */
	/*     Cell->Data.Item[i]->SecondryValuePt             */
	/*     Cell->Data.Item[i]->SecondryProxyPt             */
	/* have already been calculated by aggregating         */
	/* the values of the same field in the internal        */
	/* cells that make up this marginal cell or in         */
	/* the internal and/or marginal cells that make up     */
	/* this aggregate cell:                                */
	if (Cell->IsInternal || !additive_noise_specified) {
		for (i = 0; i <= Cell->Data.NumberEntries; i++) {
			if (i < Cell->Data.NumberEntries) {
				DataItemPtr = Cell->Data.Item[i];
			}
			else { /* (i == Cell->Data.NumberEntries) */
				if (Cell->AnonymousDataItem.NumberObservations == 0) {
					continue;
				}
				DataItemPtr = &(Cell->AnonymousDataItem);
			}
			rc = CalculateSensitivityVariables((void *)DataItemPtr, 0, SRule, alphaplus1, alphaplus2, ProxyRatio, WeightProtectionLevel);
		}
	}
	Cell->TotalValuePt         = 0.0;
	Cell->TotalValueN          = 0.0;
	Cell->TotalValueSn         = 0.0;
	Cell->TotalValueX          = 0.0;
	Cell->TotalWeightedValueX  = 0.0;
	Cell->TotalProxyPt         = 0.0;
	Cell->TotalProxyN          = 0.0;
	Cell->TotalProxySn         = 0.0;
	Cell->TotalProxyX          = 0.0;
	Cell->TotalWeightedProxyX  = 0.0;
	Cell->TotalSecondryValuePt = 0.0;
	Cell->TotalSecondryProxyPt = 0.0;

	for (i = 0; i <= Cell->Data.NumberEntries; i++) {
		if (i < Cell->Data.NumberEntries) {
			DataItemPtr = Cell->Data.Item[i];
		}
		else { /* (i == Cell->Data.NumberEntries) */
			if (Cell->AnonymousDataItem.NumberObservations == 0) {
				continue;
			}
			DataItemPtr = &(Cell->AnonymousDataItem);
		}
		Cell->TotalValuePt         = Cell->TotalValuePt         + DataItemPtr       ->ValuePt        ;
		Cell->TotalValueN          = Cell->TotalValueN          + DataItemPtr       ->ValueN         ;
		Cell->TotalValueSn         = Cell->TotalValueSn         + DataItemPtr       ->ValueSn        ;
		Cell->TotalValueX          = Cell->TotalValueX          + DataItemPtr       ->ValueX         ;
		Cell->TotalWeightedValueX  = Cell->TotalWeightedValueX  + DataItemPtr       ->WeightedValueX ;
		Cell->TotalProxyPt         = Cell->TotalProxyPt         + DataItemPtr       ->ProxyPt        ;
		Cell->TotalProxyN          = Cell->TotalProxyN          + DataItemPtr       ->ProxyN         ;
		Cell->TotalProxySn         = Cell->TotalProxySn         + DataItemPtr       ->ProxySn        ;
		Cell->TotalProxyX          = Cell->TotalProxyX          + DataItemPtr       ->ProxyX         ;
		Cell->TotalWeightedProxyX  = Cell->TotalWeightedProxyX  + DataItemPtr       ->WeightedProxyX ;
		Cell->TotalSecondryValuePt = Cell->TotalSecondryValuePt + DataItemPtr       ->SecondryValuePt;
		Cell->TotalSecondryProxyPt = Cell->TotalSecondryProxyPt + DataItemPtr       ->SecondryProxyPt;

	}
	/*-I'm pretty sure that the code that aggregates two or more cells togther to form a marginal or          */
	/* aggregate cell correctly updates not only the array of STCT_DATAITEMs in the cell but the cell totals  */
	/* too; if not, then this code could be used to do it here instead:                                       */
	/* else { /~ (!Cell->IsInternal && additive_noise_specified) ~/                                           */
	/*     Cell->TotalValuePt         = 0.0;                                                                  */
	/*     Cell->TotalValueN          = 0.0;                                                                  */
	/*     Cell->TotalValueSn         = 0.0;                                                                  */
	/*     Cell->TotalValueX          = 0.0;                                                                  */
	/*     Cell->TotalWeightedValueX  = 0.0;                                                                  */
	/*     Cell->TotalProxyPt         = 0.0;                                                                  */
	/*     Cell->TotalProxyN          = 0.0;                                                                  */
	/*     Cell->TotalProxySn         = 0.0;                                                                  */
	/*     Cell->TotalProxyX          = 0.0;                                                                  */
	/*     Cell->TotalWeightedProxyX  = 0.0;                                                                  */
	/*     Cell->TotalSecondryValuePt = 0.0;                                                                  */
	/*     Cell->TotalSecondryProxyPt = 0.0;                                                                  */
	/*     for (i = 0; i <= Cell->Data.NumberEntries; i++) {                                                  */
	/*         if (i < Cell->Data.NumberEntries) {                                                            */
	/*             DataItemPtr = Cell->Data.Item[i];                                                          */
	/*         }                                                                                              */
	/*         else { /~ (i == Cell->Data.NumberEntries) ~/                                                   */
	/*             if (Cell->AnonymousDataItem.NumberObservations == 0) {                                     */
	/*                 continue;                                                                              */
	/*             }                                                                                          */
	/*             DataItemPtr = &(Cell->AnonymousDataItem);                                                  */
	/*         }                                                                                              */
	/*         Cell->TotalValuePt         = Cell->TotalValuePt         + DataItemPtr       ->ValuePt        ; */
	/*         Cell->TotalValueN          = Cell->TotalValueN          + DataItemPtr       ->ValueN         ; */
	/*         Cell->TotalValueSn         = Cell->TotalValueSn         + DataItemPtr       ->ValueSn        ; */
	/*         Cell->TotalValueX          = Cell->TotalValueX          + DataItemPtr       ->ValueX         ; */
	/*         Cell->TotalWeightedValueX  = Cell->TotalWeightedValueX  + DataItemPtr       ->WeightedValueX ; */
	/*         Cell->TotalProxyPt         = Cell->TotalProxyPt         + DataItemPtr       ->ProxyPt        ; */
	/*         Cell->TotalProxyN          = Cell->TotalProxyN          + DataItemPtr       ->ProxyN         ; */
	/*         Cell->TotalProxySn         = Cell->TotalProxySn         + DataItemPtr       ->ProxySn        ; */
	/*         Cell->TotalProxyX          = Cell->TotalProxyX          + DataItemPtr       ->ProxyX         ; */
	/*         Cell->TotalWeightedProxyX  = Cell->TotalWeightedProxyX  + DataItemPtr       ->WeightedProxyX ; */
	/*         Cell->TotalSecondryValuePt = Cell->TotalSecondryValuePt + DataItemPtr       ->SecondryValuePt; */
	/*         Cell->TotalSecondryProxyPt = Cell->TotalSecondryProxyPt + DataItemPtr       ->SecondryProxyPt; */
	/*     }                                                                                                  */
	/* }                                                                                                      */

	FindLargest (Cell, STCM_MAXLARGESTS, Cell->Largest  , &(Cell->LargestNumberEntries  ), 'v', waiver_flags_present);
	FindLargest (Cell, STCM_MAXLARGESTS, Cell->LargestPY, &(Cell->LargestPYNumberEntries), 'p', waiver_flags_present);
	FindLargest (Cell, STCM_MAXLARGESTS, Cell->LargestWV, &(Cell->LargestWVNumberEntries), 'V', waiver_flags_present);
	FindLargest (Cell, STCM_MAXLARGESTS, Cell->LargestWP, &(Cell->LargestWPNumberEntries), 'P', waiver_flags_present);
	CountNonAnonymousPositiveNumberEntries (Cell);
	NonAnonymousPositiveProxyNumberEntries = 0;
	for (i = 0; i < Cell->Data.NumberEntries; i++) {
		if (Cell->Data.Item[i]->Proxy > 0.0)
			NonAnonymousPositiveProxyNumberEntries++;
	}
	NonAnonymousPositiveWValuNumberEntries = 0;
	for (i = 0; i < Cell->Data.NumberEntries; i++) {
		if (Cell->Data.Item[i]->WeightedValue > 0.0)
			NonAnonymousPositiveWValuNumberEntries++;
	}
	NonAnonymousPositiveWPrxyNumberEntries = 0;
	for (i = 0; i < Cell->Data.NumberEntries; i++) {
		if (Cell->Data.Item[i]->WeightedProxy > 0.0)
			NonAnonymousPositiveWPrxyNumberEntries++;
	}

	/*-calculate "entvalue" and "entwaivers": */
	if (Cell->LargestNumberEntries > STCM_MAXALPHA)
		n = STCM_MAXALPHA;
	else
		n = Cell->LargestNumberEntries;
	for (i = 0; i < n; i++)
	{
		entvalue[i] = Cell->Largest  [i]->ValueX;
		if (waiver_flags_present == 2 || waiver_flags_present == 1) {
			entwaivers[i] = Cell->Largest[i]->waiver_flag;
		}
		else
		{
			entwaivers[i] = 0;
		}
	}
	for (; i < STCM_MAXALPHA; i++)
	{
		entvalue[i] = 0.0;
		entwaivers[i] = 0;
	}

	/*-calculate "entproxy" and "entwvproxy": */
	if (Cell->LargestPYNumberEntries > STCM_MAXALPHA)
		nPY = STCM_MAXALPHA;
	else
		nPY = Cell->LargestPYNumberEntries;
	for (i = 0; i < nPY; i++) {
		entproxy[i] = Cell->LargestPY[i]->ProxyX;
		if (waiver_flags_present == 2 || waiver_flags_present == 1) {
			entwvproxy[i] = Cell->LargestPY[i]->waiver_flag;
		}
		else
		{
			entwvproxy[i] = 0;
		}
	}
	for (; i < STCM_MAXALPHA; i++) {
		entproxy[i] = 0.0;
		entwvproxy[i] = 0;
	}

	/*-calculate "entwvalu" and "entwvwvalu": */
	if (Cell->LargestWVNumberEntries > STCM_MAXALPHA)
		nWV = STCM_MAXALPHA;
	else
		nWV = Cell->LargestWVNumberEntries;
	for (i = 0; i < nWV; i++) {
		entwvalu[i] = Cell->LargestWV[i]->WeightedValueX;
		if (waiver_flags_present == 2 || waiver_flags_present == 1) {
			entwvwvalu[i] = Cell->LargestWV[i]->waiver_flag;
		}
		else
		{
			entwvwvalu[i] = 0;
		}
	}
	for (; i < STCM_MAXALPHA; i++) {
		entwvalu[i] = 0.0;
		entwvwvalu[i] = 0;
	}

	/*-calculate "entwprxy" and "entwvwprxy": */
	if (Cell->LargestWPNumberEntries > STCM_MAXALPHA)
		nWP = STCM_MAXALPHA;
	else
		nWP = Cell->LargestWPNumberEntries;
	for (i = 0; i < nWP; i++) {
		entwprxy[i] = Cell->LargestWP[i]->WeightedProxyX;
		if (waiver_flags_present == 2 || waiver_flags_present == 1) {
			entwvwprxy[i] = Cell->LargestWP[i]->waiver_flag;
		}
		else
		{
			entwvwprxy[i] = 0;
		}
	}
	for (; i < STCM_MAXALPHA; i++) {
		entwprxy[i] = 0.0;
		entwvwprxy[i] = 0;
	}


	HasAnonymousParm                      = STC_CellHasAnonymous (Cell);

	if   (waiver_flags_present != 0) {
		/*-waivers specified, so first also calculate sensitivity without waivers: */
		waiver_flags_present_parm             = 0;
		WeightSpecifiedParm                   = weight_specified;
		ProxySpecifiedParm                    = proxy_specified;
		entvalParm                            = WeightSpecifiedParm?(ProxySpecifiedParm?entwprxy:entwvalu)
		                                                           :(ProxySpecifiedParm?entproxy:entvalue);
		nParm                                 = WeightSpecifiedParm?(ProxySpecifiedParm?nWP:nWV)
		                                                           :(ProxySpecifiedParm?nPY:n  );
		/* TotalValueParm                        = WeightSpecifiedParm?(ProxySpecifiedParm?Cell->TotalWeightedProxyX:Cell->TotalWeightedValueX)  */
		/*                                                            :(ProxySpecifiedParm?Cell->TotalProxyX        :Cell->TotalValueX        ); */
		TotalValueParm                        = 0.0;
		for (i = 0; i <= Cell->Data.NumberEntries; i++) {
			if (i < Cell->Data.NumberEntries) {
				DataItemPtr = Cell->Data.Item[i];
			}
			else { /* (i == Cell->Data.NumberEntries) */
				if (Cell->AnonymousDataItem.NumberObservations == 0) {
					continue;
				}
				DataItemPtr = &(Cell->AnonymousDataItem);
			}
			TotalValueParm                       += WeightSpecifiedParm?(ProxySpecifiedParm?DataItemPtr->WeightedProxyX:DataItemPtr->WeightedValueX)
			                                                           :(ProxySpecifiedParm?DataItemPtr->ProxyX        :DataItemPtr->ValueX        );
		}
		AnonymousValueParm                    = WeightSpecifiedParm?(ProxySpecifiedParm?Cell->AnonymousDataItem.WeightedProxyX
		                                                                               :Cell->AnonymousDataItem.WeightedValueX
		                                                            )
		                                                           :(ProxySpecifiedParm?Cell->AnonymousDataItem.ProxyX
		                                                                               :Cell->AnonymousDataItem.ValueX        
		                                                            );
		NonAnonymousPositiveNumberEntriesParm = WeightSpecifiedParm?(ProxySpecifiedParm?      NonAnonymousPositiveWPrxyNumberEntries
		                                                                               :      NonAnonymousPositiveWValuNumberEntries
		                                                            )
		                                                           :(ProxySpecifiedParm?      NonAnonymousPositiveProxyNumberEntries
		                                                                               :Cell->NonAnonymousPositiveNumberEntries
		                                                            );
		entwaiversParm                        = WeightSpecifiedParm?(ProxySpecifiedParm?entwvwprxy:entwvwvalu)
		                                                           :(ProxySpecifiedParm?entwvproxy:entwaivers);
		UsePtnSensitivityParm                 = (WeightSpecifiedParm && (WeightProtectionLevel != 'E'));
		UseAdditiveNoiseParm                  = additive_noise_specified;
		NonPtnSensitivityVariableParm         = "vpVP"[(2 * WeightSpecifiedParm) + ProxySpecifiedParm];
		if (UsePtnSensitivityParm) {
		    rc = CalculateFXAndLargestFX(ProxySpecifiedParm,
		                                 SRule,
		                                 Cell
		                                );
		}
		RetCodeSRuleSens            = STC_SRuleSensitivity (SRule,
		                                                    entvalParm,
		                                                    nParm,
		                                                    TotalValueParm,
		                                                    HasAnonymousParm,
		                                                    AnonymousValueParm,
		                                                    NonAnonymousPositiveNumberEntriesParm,
		                                                    entwaiversParm,
		                                                    Cell,
		                                                    waiver_flags_present_parm,
		                                                    WeightSpecifiedParm,
		                                                    ProxySpecifiedParm,
		                                                    UsePtnSensitivityParm,
		                                                    UseAdditiveNoiseParm,
		                                                    NonPtnSensitivityVariableParm,
		                                                    ptn_type1,
		                                                    ptn_type2,
		                                                    &DoubleValueToBeIgnored,
		                                                    &(Cell->Sensitivity_nowaivers)
		                                                   );
		if (RetCodeSRuleSens != EIE_SUCCEED) {
		    RetCode = EIE_FAIL;
		    return RetCode;
		}
	}
	if   (SRule->Parms->Weight && SRule->Parms->WeightDiagnostics) {
		/*-weight and weight diagnostics specified, so first also calculate sensitivity without weight: */
		waiver_flags_present_parm             = waiver_flags_present;
		WeightSpecifiedParm                   = 0;
		ProxySpecifiedParm                    = proxy_specified;
		entvalParm                            = WeightSpecifiedParm?(ProxySpecifiedParm?entwprxy:entwvalu)
		                                                           :(ProxySpecifiedParm?entproxy:entvalue);
		nParm                                 = WeightSpecifiedParm?(ProxySpecifiedParm?nWP:nWV)
		                                                           :(ProxySpecifiedParm?nPY:n  );
		TotalValueParm                        = 0.0;
		for (i = 0; i <= Cell->Data.NumberEntries; i++) {
			if (i < Cell->Data.NumberEntries) {
				DataItemPtr = Cell->Data.Item[i];
			}
			else { /* (i == Cell->Data.NumberEntries) */
				if (Cell->AnonymousDataItem.NumberObservations == 0) {
					continue;
				}
				DataItemPtr = &(Cell->AnonymousDataItem);
			}
			TotalValueParm                       += WeightSpecifiedParm?(ProxySpecifiedParm?DataItemPtr->WeightedProxyX:DataItemPtr->WeightedValueX)
			                                                           :(ProxySpecifiedParm?DataItemPtr->ProxyX        :DataItemPtr->ValueX        );
		}
		AnonymousValueParm                    = WeightSpecifiedParm?(ProxySpecifiedParm?Cell->AnonymousDataItem.WeightedProxyX
		                                                                               :Cell->AnonymousDataItem.WeightedValueX
		                                                            )
		                                                           :(ProxySpecifiedParm?Cell->AnonymousDataItem.ProxyX
		                                                                               :Cell->AnonymousDataItem.ValueX        
		                                                            );
		NonAnonymousPositiveNumberEntriesParm = WeightSpecifiedParm?(ProxySpecifiedParm?       NonAnonymousPositiveWPrxyNumberEntries
		                                                                               :       NonAnonymousPositiveWValuNumberEntries
		                                                            )
		                                                           :(ProxySpecifiedParm?       NonAnonymousPositiveProxyNumberEntries
		                                                                                :Cell->NonAnonymousPositiveNumberEntries
		                                                            );
		entwaiversParm                        = WeightSpecifiedParm?(ProxySpecifiedParm?entwvwprxy:entwvwvalu)
		                                                           :(ProxySpecifiedParm?entwvproxy:entwaivers);
		UsePtnSensitivityParm                 = (0 && (WeightProtectionLevel != 'E'));
		UseAdditiveNoiseParm                  = additive_noise_specified;
		NonPtnSensitivityVariableParm         = "vpVP"[(2 * WeightSpecifiedParm) + ProxySpecifiedParm];
		if (UsePtnSensitivityParm) {
		    rc = CalculateFXAndLargestFX(ProxySpecifiedParm,
		                                 SRule,
		                                 Cell
		                                );
		}
		RetCodeSRuleSens            = STC_SRuleSensitivity (SRule,
		                                                    entvalParm,
		                                                    nParm,
		                                                    TotalValueParm,
		                                                    HasAnonymousParm,
		                                                    AnonymousValueParm,
		                                                    NonAnonymousPositiveNumberEntriesParm,
		                                                    entwaiversParm,
		                                                    Cell,
		                                                    waiver_flags_present_parm,
		                                                    WeightSpecifiedParm,
		                                                    ProxySpecifiedParm,
		                                                    UsePtnSensitivityParm,
		                                                    UseAdditiveNoiseParm,
		                                                    NonPtnSensitivityVariableParm,
		                                                    ptn_type1,
		                                                    ptn_type2,
		                                                    &(Cell->Sensitivity_noweights),
		                                                    &(DoubleValueToBeIgnored)
		                                                   );
		if (RetCodeSRuleSens != EIE_SUCCEED) {
		    RetCode = EIE_FAIL;
		    return RetCode;
		}
	}
	if   (SRule->Parms->Proxy  && SRule->Parms->ProxyDiagnostics ) {
		/*-proxy and proxy diagnositics specified, so first also calculate sensitivity without proxy: */
		waiver_flags_present_parm             = waiver_flags_present;
		WeightSpecifiedParm                   = weight_specified;
		ProxySpecifiedParm                    = 0;
		entvalParm                            = WeightSpecifiedParm?(ProxySpecifiedParm?entwprxy:entwvalu)
		                                                           :(ProxySpecifiedParm?entproxy:entvalue);
		nParm                                 = WeightSpecifiedParm?(ProxySpecifiedParm?nWP:nWV)
		                                                           :(ProxySpecifiedParm?nPY:n  );
		TotalValueParm                        = 0.0;
		for (i = 0; i <= Cell->Data.NumberEntries; i++) {
			if (i < Cell->Data.NumberEntries) {
				DataItemPtr = Cell->Data.Item[i];
			}
			else { /* (i == Cell->Data.NumberEntries) */
				if (Cell->AnonymousDataItem.NumberObservations == 0) {
					continue;
				}
				DataItemPtr = &(Cell->AnonymousDataItem);
			}
			TotalValueParm                       += WeightSpecifiedParm?(ProxySpecifiedParm?DataItemPtr->WeightedProxyX:DataItemPtr->WeightedValueX)
			                                                           :(ProxySpecifiedParm?DataItemPtr->ProxyX        :DataItemPtr->ValueX        );
		}
		AnonymousValueParm                    = WeightSpecifiedParm?(ProxySpecifiedParm?Cell->AnonymousDataItem.WeightedProxyX
		                                                                               :Cell->AnonymousDataItem.WeightedValueX
		                                                            )
		                                                           :(ProxySpecifiedParm?Cell->AnonymousDataItem.ProxyX
		                                                                               :Cell->AnonymousDataItem.ValueX        
		                                                            );
		NonAnonymousPositiveNumberEntriesParm = WeightSpecifiedParm?(ProxySpecifiedParm?      NonAnonymousPositiveWPrxyNumberEntries
		                                                                               :      NonAnonymousPositiveWValuNumberEntries
		                                                            )
		                                                           :(ProxySpecifiedParm?      NonAnonymousPositiveProxyNumberEntries
		                                                                               :Cell->NonAnonymousPositiveNumberEntries
		                                                            );
		entwaiversParm                        = WeightSpecifiedParm?(ProxySpecifiedParm?entwvwprxy:entwvwvalu)
		                                                           :(ProxySpecifiedParm?entwvproxy:entwaivers);
		UsePtnSensitivityParm                 = (WeightSpecifiedParm && (WeightProtectionLevel != 'E'));
		UseAdditiveNoiseParm                  = additive_noise_specified;
		NonPtnSensitivityVariableParm         = "vpVP"[(2 * WeightSpecifiedParm) + ProxySpecifiedParm];
		if (UsePtnSensitivityParm) {
		    rc = CalculateFXAndLargestFX(ProxySpecifiedParm,
		                                 SRule,
		                                 Cell
		                                );
		}
		RetCodeSRuleSens            = STC_SRuleSensitivity (SRule,
		                                                    entvalParm,
		                                                    nParm,
		                                                    TotalValueParm,
		                                                    HasAnonymousParm,
		                                                    AnonymousValueParm,
		                                                    NonAnonymousPositiveNumberEntriesParm,
		                                                    entwaiversParm,
		                                                    Cell,
		                                                    waiver_flags_present_parm,
		                                                    WeightSpecifiedParm,
		                                                    ProxySpecifiedParm,
		                                                    UsePtnSensitivityParm,
		                                                    UseAdditiveNoiseParm,
		                                                    NonPtnSensitivityVariableParm,
		                                                    ptn_type1,
		                                                    ptn_type2,
		                                                    &(Cell->Sensitivity_noproxy),
		                                                    &(DoubleValueToBeIgnored)
		                                                   );
		if (RetCodeSRuleSens != EIE_SUCCEED) {
		    RetCode = EIE_FAIL;
		    return RetCode;
		}
	}
	/*-now calculate cell sensitivity using all of the specified parameter values: */
	waiver_flags_present_parm             = waiver_flags_present;
	WeightSpecifiedParm                   = weight_specified;
	ProxySpecifiedParm                    = proxy_specified;
	entvalParm                            = WeightSpecifiedParm?(ProxySpecifiedParm?entwprxy:entwvalu)
	                                                           :(ProxySpecifiedParm?entproxy:entvalue);
	nParm                                 = WeightSpecifiedParm?(ProxySpecifiedParm?nWP:nWV)
	                                                           :(ProxySpecifiedParm?nPY:n  );
	TotalValueParm                        = 0.0;
	for (i = 0; i <= Cell->Data.NumberEntries; i++) {
		if (i < Cell->Data.NumberEntries) {
			DataItemPtr = Cell->Data.Item[i];
		}
		else { /* (i == Cell->Data.NumberEntries) */
			if (Cell->AnonymousDataItem.NumberObservations == 0) {
				continue;
			}
			DataItemPtr = &(Cell->AnonymousDataItem);
		}
		TotalValueParm                       += WeightSpecifiedParm?(ProxySpecifiedParm?DataItemPtr->WeightedProxyX:DataItemPtr->WeightedValueX)
		                                                           :(ProxySpecifiedParm?DataItemPtr->ProxyX        :DataItemPtr->ValueX        );
	}
	AnonymousValueParm                    = WeightSpecifiedParm?(ProxySpecifiedParm?Cell->AnonymousDataItem.WeightedProxyX
	                                                                               :Cell->AnonymousDataItem.WeightedValueX
	                                                            )
	                                                           :(ProxySpecifiedParm?Cell->AnonymousDataItem.ProxyX
	                                                                               :Cell->AnonymousDataItem.ValueX        
	                                                            );
	NonAnonymousPositiveNumberEntriesParm = WeightSpecifiedParm?(ProxySpecifiedParm?      NonAnonymousPositiveWPrxyNumberEntries
	                                                                               :      NonAnonymousPositiveWValuNumberEntries
	                                                            )
	                                                           :(ProxySpecifiedParm?      NonAnonymousPositiveProxyNumberEntries
	                                                                               :Cell->NonAnonymousPositiveNumberEntries
	                                                            );
	entwaiversParm                        = WeightSpecifiedParm?(ProxySpecifiedParm?entwvwprxy:entwvwvalu)
	                                                           :(ProxySpecifiedParm?entwvproxy:entwaivers);
	UsePtnSensitivityParm                 = (WeightSpecifiedParm && (WeightProtectionLevel != 'E'));
	UseAdditiveNoiseParm                  = additive_noise_specified;
	/*-NonPtnSensitivityVariableParm = 'v' means unweighted value ("ValueX"         at the "STCT_DATAITEM" level and "TotalValueX"         at the "STCT_CELL" level) */
	/* NonPtnSensitivityVariableParm = 'p' means unweighted proxy ("ProxyX"         at the "STCT_DATAITEM" level and "TotalProxyX"         at the "STCT_CELL" level) */
	/* NonPtnSensitivityVariableParm = 'V' means   weighted value ("WeightedValueX" at the "STCT_DATAITEM" level and "TotalWeightedValueX" at the "STCT_CELL" level) */
	/* NonPtnSensitivityVariableParm = 'P' means   weighted proxy ("WeightedProxyX" at the "STCT_DATAITEM" level and "TotalWeightedProxyX" at the "STCT_CELL" level) */
	NonPtnSensitivityVariableParm         = "vpVP"[(2 * WeightSpecifiedParm) + ProxySpecifiedParm];
	if (UsePtnSensitivityParm) {
	    rc = CalculateFXAndLargestFX(ProxySpecifiedParm,
	                                 SRule,
	                                 Cell
	                                );
	}
	RetCodeSRuleSens  = STC_SRuleSensitivity (SRule,
	                                          entvalParm,
	                                          nParm,
	                                          TotalValueParm,
	                                          HasAnonymousParm,
	                                          AnonymousValueParm,
	                                          NonAnonymousPositiveNumberEntriesParm,
	                                          entwaiversParm,
	                                          Cell,
	                                          waiver_flags_present_parm,
	                                          WeightSpecifiedParm,
	                                          ProxySpecifiedParm,
	                                          UsePtnSensitivityParm,
	                                          UseAdditiveNoiseParm,
	                                          NonPtnSensitivityVariableParm,
	                                          ptn_type1,
	                                          ptn_type2,
	                                          &(Cell->Sensitivity),
	                                          &(DoubleValueToBeIgnored)
	                                         );
	if (RetCodeSRuleSens != EIE_SUCCEED) {
	    RetCode = EIE_FAIL;
	    return RetCode;
	}
	/*-set the value of "Cell->WhichNoise" so that it will be easy to know which noise variable was used to calculate sensitivity: */
	if (WeightSpecifiedParm && (WeightProtectionLevel != 'E')) {
		if (ProxySpecifiedParm) {
			Cell->WhichNoise = 'N';
		}
		else {
			Cell->WhichNoise = 'n';
		}
	}
	else { /* (!WeightSpecifiedParm || (WeightProtectionLevel == 'E')) */
		if (WeightSpecifiedParm) {
			if (ProxySpecifiedParm) {
				Cell->WhichNoise = 'P';
			}
			else {
				Cell->WhichNoise = 'V';
			}
		}
		else {
			if (ProxySpecifiedParm) {
				Cell->WhichNoise = 'p';
			}
			else {
				Cell->WhichNoise = 'v';
			}
		}
	}

	return RetCode;
}
/*------------------------------------------------------------------------------
Add cell at index position of CellSet.
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_CellSetAddAtIndex (
	STCT_CELLSET * CellSet,
	STCT_CELL * Cell,
	int Index)
{
#ifdef _DEBUG
	EI_PrintMessages ();
	assert (Index >= 0 && Index <= CellSet->NumberEntries);
#endif
	/* need more space? */
	if (CellSet->NumberEntries == CellSet->NumberAllocated) {
		if (ReallocateCellSet (CellSet) == NULL) return EIE_FAIL;
	}
	memmove (CellSet->Cell[Index+1], CellSet->Cell[Index],
		(CellSet->NumberEntries-Index) * sizeof *CellSet->Cell[Index]);
	CellSet->Cell[Index] = Cell;
	CellSet->NumberEntries++;
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Add cell at end of CellSet.
we use this faster version of STC_CellSetAddAtIndex(cs, cs->NumberEntries) a lot in traverse()
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_CellSetAddLast (
	STCT_CELLSET * CellSet,
	STCT_CELL * Cell)
{
	/* need more space? */
	if (CellSet->NumberEntries == CellSet->NumberAllocated) {
		if (ReallocateCellSet (CellSet) == NULL) return EIE_FAIL;
	}
	CellSet->Cell[CellSet->NumberEntries] = Cell;
	CellSet->NumberEntries++;
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Reallocates Item member of Cell to the exact number of Items
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_CellSetAdjustAllocation (
	STCT_CELLSET * CellSet)
{
	void * Ptr;

	if (STC_CellSetAdjustCellAllocation (CellSet) != EIE_SUCCEED)
		return EIE_FAIL;

	if (CellSet->NumberAllocated == CellSet->NumberEntries) return EIE_SUCCEED;
#ifdef DEBUGALLOCATION
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "STC_CellSetAdjustAllocation ne=%d/na=%d\n", CellSet->NumberEntries, CellSet->NumberAllocated);
#endif
	Ptr = STC_ReallocateMemory (
		CellSet->NumberAllocated * sizeof *CellSet->Cell,
		CellSet->NumberEntries * sizeof *CellSet->Cell,
		CellSet->Cell);
	if (Ptr == NULL && CellSet->NumberEntries != 0)
		return EIE_FAIL;
	CellSet->Cell = Ptr;
	CellSet->NumberAllocated = CellSet->NumberEntries;
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Reallocates Item member of all cells of the cellset to the exact number of Items
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_CellSetAdjustCellAllocation (
	STCT_CELLSET * CellSet)
{
	int i;
	
	for (i = 0; i < CellSet->NumberEntries; i++) {
		if (STC_CellAdjustAllocation (CellSet->Cell[i]) != EIE_SUCCEED)
			return EIE_FAIL;
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Allocate the STCT_CELLSET structure
------------------------------------------------------------------------------*/
STCT_CELLSET * STC_CellSetAllocate (
	int AllocationIncrement)
{
	STCT_CELLSET * CellSet;
	/* allocate the structure */
	CellSet = STC_AllocateMemory (sizeof *CellSet);
	if (CellSet == NULL) return NULL;
	CellSet->AllocationIncrement = (AllocationIncrement < 1 ? 10 : AllocationIncrement);
	InitializeCellSet (CellSet);
	return CellSet;
}
/*------------------------------------------------------------------------------
Create a cell from a cell set. Concatenate all cells in the cell set.
------------------------------------------------------------------------------*/
STCT_CELL * STC_CellSetAsCell (
	STCT_CELLSET * CellSet)
{
	STCT_CELL * CellSetAsCell;
	int i;
	int n;
	EIT_RETURNCODE rc;

	n = 0;
	for (i = 0; i < CellSet->NumberEntries; i++) {
		n += CellSet->Cell[i]->Data.NumberEntries;
	}
	CellSetAsCell = STC_CellAllocate (n);
	if (CellSetAsCell == NULL) return NULL;
	for (i = 0; i < CellSet->NumberEntries; i++) {
		rc = STC_CellConcat (CellSetAsCell, CellSet->Cell[i]);
		if (rc != EIE_SUCCEED) return NULL;
	}
	return CellSetAsCell;
}
/*------------------------------------------------------------------------------
Free the STCT_CELLSET structure
------------------------------------------------------------------------------*/
void STC_CellSetFree (
	STCT_CELLSET * CellSet)
{
	int i;
	if (CellSet != NULL) {

#ifdef _DEBUG
#ifdef DEBUGALLOCATION
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "STC_CellSetFree ne=%d/na=%d\n", CellSet->NumberEntries, CellSet->NumberAllocated);
#endif
#endif
		if (CellSet->Cell != NULL)
			for (i = 0; i < CellSet->NumberEntries; i++) {
				STC_CellFree (CellSet->Cell[i]);
			}
		STC_FreeMemory (CellSet->Cell);
		STC_FreeMemory (CellSet);
	}
}
/*------------------------------------------------------------------------------
return the last ConstraintId
------------------------------------------------------------------------------*/
int STC_CellSetGetConstraintId (void)
{
	return mConstraintId;
}
/*------------------------------------------------------------------------------
Initialize the ConstraintId 
------------------------------------------------------------------------------*/
void STC_CellSetInitConstraintId (void)
{
	mConstraintId = 0;
}
/*------------------------------------------------------------------------------
return the next available ConstraintId
------------------------------------------------------------------------------*/
int STC_CellSetNextConstraintId (void)
{
	return ++mConstraintId;
}
/*------------------------------------------------------------------------------
Print the STCT_CELLSET structure
------------------------------------------------------------------------------*/
void STC_CellSetPrint (
	STCT_CELLSET * CellSet)
{
	int i;
	STC_CellSetPrintInfo (CellSet);
	for (i = 0; i < CellSet->NumberEntries; i++) {
		STC_CellPrint (CellSet->Cell[i]);
	}
}
/*------------------------------------------------------------------------------
Print the STCT_CELLSET structure metadata including the forming cells
------------------------------------------------------------------------------*/
void STC_CellSetPrintCellInfo (
	STCT_CELLSET * CellSet)
{
	int i;
	STC_CellSetPrintInfo (CellSet);
	for (i = 0; i < CellSet->NumberEntries; i++) {
		STC_CellPrintInfo (CellSet->Cell[i], !i);
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
}
/*------------------------------------------------------------------------------
Print the cell set coordinate
------------------------------------------------------------------------------*/
void STC_CellSetPrintCoordinate (
	STCT_CELLSET * CellSet,
	STCT_HTREEROOT * HTreeRoot)
{
	int i;
	//STC_CellSetPrintInfo (CellSet);
	for (i = 0; i < CellSet->NumberEntries; i++) {
		STC_CoordinatePrint (CellSet->Cell[i]->Coordinate, HTreeRoot);
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
}
/*------------------------------------------------------------------------------
Print the STCT_CELLSET structure metadata
------------------------------------------------------------------------------*/
void STC_CellSetPrintInfo (
	STCT_CELLSET * CellSet)
{
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "CellSet ne=%d/na=%d\n", CellSet->NumberEntries, CellSet->NumberAllocated);
}
/*------------------------------------------------------------------------------
Print the STCT_CELLSET largest respondents
------------------------------------------------------------------------------*/
void STC_CellSetPrintLargest (
	STCT_CELLSET * CellSet)
{
	int i;
	STC_CellSetPrintInfo (CellSet);
	for (i = 0; i < CellSet->NumberEntries; i++) {
		STC_CellPrintLargest (CellSet->Cell[i]);
	}
}
/*------------------------------------------------------------------------------
Remove cell from index position of CellSet. returns the removed cell.
------------------------------------------------------------------------------*/
STCT_CELL * STC_CellSetRemoveAtIndex (
	STCT_CELLSET * CellSet,
	int Index)
{
	STCT_CELL * Cell;

#ifdef _DEBUG
	EI_PrintMessages ();
	assert (Index >= 0 && Index < CellSet->NumberEntries);
#endif
	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "removing at index %d. ne = %d. moving %d items of size %d = %d\n",
	//	Index, CellSet->NumberEntries,
	//	(CellSet->NumberEntries-Index-1), sizeof CellSet->Cell[0],
	//	(CellSet->NumberEntries-Index-1) * sizeof CellSet->Cell[0]);

	Cell = CellSet->Cell[Index];
	memmove (CellSet->Cell+Index, CellSet->Cell+Index+1,
		(CellSet->NumberEntries-Index-1) * sizeof CellSet->Cell[0]);
	CellSet->NumberEntries--;
	return Cell;
}
/*------------------------------------------------------------------------------
Remove cell from end of CellSet. returns the removed cell.
we use this faster version of STC_CellSetRemoveAtIndex(cs, cs->NumberEntries-1) a lot in traverse()
------------------------------------------------------------------------------*/
STCT_CELL * STC_CellSetRemoveLast (
	STCT_CELLSET * CellSet)
{
	CellSet->NumberEntries--;
	return CellSet->Cell[CellSet->NumberEntries];
}
/*------------------------------------------------------------------------------
Calculate the sensitivity of a cell set.
To do so, it creates a cell from all cells in cell set and than it calculates
the sensitivity of that cell
------------------------------------------------------------------------------*/
STCT_CELL * STC_CellSetSensitivity (
	STCT_CELLSET * CellSet,
	STCT_SRULE * SRule,
	STCT_COORDINATE * Coordinate)
{
	STCT_CELL * CellSetAsCell;
	EIT_RETURNCODE rc;

	CellSetAsCell = NULL;
	rc = EIE_SUCCEED;

	CellSetAsCell = STC_CellSetAsCell (CellSet);
	if (CellSetAsCell != NULL) {
	    rc = STC_CellSensitivity (CellSetAsCell, SRule, Coordinate);
	    if (rc != EIE_SUCCEED) {
	        if (CellSetAsCell != NULL) {
	            STC_CellFree(CellSetAsCell);
	            CellSetAsCell = NULL;
	        }
	    }
	}
	return CellSetAsCell;
}
/*------------------------------------------------------------------------------
Allocate a new cell set and copies the cell of the duplicated cell set. The actual cells
are not duplicated.
------------------------------------------------------------------------------*/
STCT_CELLSET * STC_CellSetShallowDuplicate (
	STCT_CELLSET * CellSet)
{
	STCT_CELLSET * CellSetDuplicate;
	int i;
	EIT_RETURNCODE rc;

	CellSetDuplicate = STC_CellSetAllocate (CellSet->AllocationIncrement);
	if (CellSetDuplicate == NULL) return NULL;
	for (i = 0; i < CellSet->NumberEntries; i++) {
		rc = STC_CellSetAddLast (CellSetDuplicate, CellSet->Cell[i]);
		if (rc != EIE_SUCCEED) return NULL;
	}
	return CellSetDuplicate;
}
/*------------------------------------------------------------------------------
Free the cell set but not the actual cells forming it.
See STC_CellSetShallowDuplicate()
------------------------------------------------------------------------------*/
void STC_CellSetShallowFree (
	STCT_CELLSET * CellSet)
{
	if (CellSet != NULL) {
#ifdef DEBUGALLOCATION
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "STC_CellSetShallowFree ne=%d/na=%d\n", CellSet->NumberEntries, CellSet->NumberAllocated);
#endif
		STC_FreeMemory (CellSet->Cell);
		STC_FreeMemory (CellSet);
	}
}


/*------------------------------------------------------------------------------
Add an item to the cell's largest list. the item is placed in order (largest to smallestofthelargest)
------------------------------------------------------------------------------*/
static double AddLargest ( //rene: on peut optimiser. 1-recherche binaire 2-n est fixé a 4, mais devrait etre determiné selon la regle de sensibilite
	STCT_DATAITEM ** Largest,//index 0 to n-1
	int LargestNumberEntries,//number of largest in Largest array, 0 to MaxLargests
	int MaxLargests,//number of largests to keep
	STCT_DATAITEM * Item,//item to add
	char ValueType,
	int * SmallestOfTheLargest_waiver_flag_p,//pointer to the value of the waiver flag for the smallest of the items in the array "Largest" when this function returns (output parameter--input value is not used)
	int waiver_flags_present)
{
	int Index;
	int InsertionPoint;
	int j;

	//after this loop InsertionPoint is the location of the Item to add
	/* for (InsertionPoint = 0; InsertionPoint < LargestNumberEntries && SELECT_DI_VAL2(Item, ValueType) <= SELECT_DI_VAL2(Largest[InsertionPoint], ValueType);InsertionPoint++) */
	InsertionPoint = 0;
	while (InsertionPoint < LargestNumberEntries) {
		if ((                             SELECT_DI_VAL2(Item, ValueType) <  SELECT_DI_VAL2(Largest[InsertionPoint], ValueType)) || /* the value in the item to be added is  */
			                                                                                                                        /* strictly greater than the value in    */
			                                                                                                                        /* the item at Largest[InsertionPoint]   */
			(waiver_flags_present == 0 && SELECT_DI_VAL2(Item, ValueType) == SELECT_DI_VAL2(Largest[InsertionPoint], ValueType)) || /* no waiver flags present and the value */
			                                                                                                                        /* in the item to be added is equal to   */
			                                                                                                                        /* the value in the item at              */
			                                                                                                                        /* Largest[InsertionPoint]               */
			((waiver_flags_present == 2 || waiver_flags_present == 1) && SELECT_DI_VAL2(Item, ValueType) == SELECT_DI_VAL2(Largest[InsertionPoint], ValueType) &&
			 (!(Largest[InsertionPoint]->waiver_flag != 1 && Item->waiver_flag == 1))
			)    /* partial waiver flags present                                                                                    */
			     /* and the value in the item to be added is equal to the value in the item at Largest[InsertionPoint]              */
			     /* and it is NOT the case that:                                                                                    */
			     /*     the value of the waiver flag of the item to be added sorts after                                            */
			     /*     the value of the waiver flag of the item at Largest[InsertionPoint]                                         */
			     /*     (so either the waiver flags are equal for the purposes of sorting (both 1 or both not 1) or                 */
			     /*     the waiver flag of the item to be added sorts before the waiver flag of the item at Largest[InsertionPoint] */
			     /*     (which is true iff the value of the waiver flag of the item to be sorted is 0 or -1 and                     */
			     /*     the value of the waiver flag of the item at Largest[InsertionPoint] is 1))                                  */
		) {
			InsertionPoint = InsertionPoint + 1;
		}
		else {
			/*-insertion point found: */
			break;
		}
	}

#ifdef _DEBUG
	if (DEBUG) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "SELECT_DI_VAL2(Item, ValueType)=%.0f ", SELECT_DI_VAL2(Item, ValueType));
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "LargestNumberEntries=%d MaxLargests=%d InsertionPoint=%d\n", LargestNumberEntries, MaxLargests, InsertionPoint);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "avant V:");
		for (j = 0; j < LargestNumberEntries; j++)
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%7.0f,", SELECT_DI_VAL2(Largest[j], ValueType));
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\navant K:");
		for (j = 0; j < LargestNumberEntries; j++)
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%7s,", Largest[j]->Key);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	}
#endif

	if (LargestNumberEntries == MaxLargests) {
		if (LargestNumberEntries-InsertionPoint-1 > 0) {
			//already full, replace an item
#ifdef _DEBUG
			if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY,
				"full, memmove (Largest+%d, Largest+%d, (%d) * sizeof (STCT_DATAITEM *));\n",
				InsertionPoint+1, InsertionPoint, LargestNumberEntries-InsertionPoint-1);
#endif
			//le memmove() cause un bug que je ne peux pas trouver
			//memmove (Largest+InsertionPoint+1, Largest+InsertionPoint, (LargestNumberEntries-InsertionPoint-1) * sizeof (STCT_DATAITEM *));
			for (j = LargestNumberEntries-1-1; j >= InsertionPoint; j--) {
#ifdef _DEBUG
				if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%d to %d, ", j, j+1);
#endif
				Largest[j+1] = Largest[j];
			}
#ifdef _DEBUG
			if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
#endif
		}
#ifdef _DEBUG
		else {
			if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY,
				"full, but no memmove (Largest+%d, Largest+%d, (%d) * sizeof (STCT_DATAITEM *));\n",
				InsertionPoint+1, InsertionPoint, LargestNumberEntries-InsertionPoint-1);
		}
#endif
		Largest[InsertionPoint] = Item;
#ifdef _DEBUG
		if (DEBUG) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "apres V:");
			for (j = 0; j < MaxLargests; j++)
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%7.0f,", SELECT_DI_VAL2(Largest[j], ValueType));
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\napres K:");
			for (j = 0; j < MaxLargests; j++)
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%7s,", Largest[j]->Key);
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
		}
#endif
		Index = MaxLargests-1;
	}
	else {
		if (LargestNumberEntries-InsertionPoint > 0) {
			//not full, add an item
#ifdef _DEBUG
			if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY,
				"not full, memmove (Largest+%d, Largest+%d, (%d) * sizeof (STCT_DATAITEM *));\n",
				InsertionPoint+1, InsertionPoint, LargestNumberEntries-InsertionPoint);
#endif
			//le memmove() cause un bug que je ne peux pas trouver
			//memmove (Largest+InsertionPoint+1, Largest+InsertionPoint, (LargestNumberEntries-InsertionPoint) * sizeof (STCT_DATAITEM *));
			for (j = LargestNumberEntries-1; j >= InsertionPoint; j--) {
#ifdef _DEBUG
				if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%d to %d, ", j, j+1);
#endif
				Largest[j+1] = Largest[j];
			}
#ifdef _DEBUG
			if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
#endif
		}
#ifdef _DEBUG
		else {
			if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY,
				"not full, but no memmove (Largest+%d, Largest+%d, (%d) * sizeof (STCT_DATAITEM *));\n",
				InsertionPoint+1, InsertionPoint, LargestNumberEntries-InsertionPoint);
		}
#endif
		Largest[InsertionPoint] = Item;
#ifdef _DEBUG
		if (DEBUG) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "apres V:");
			for (j = 0; j < LargestNumberEntries+1; j++)
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%7.0f,", SELECT_DI_VAL2(Largest[j], ValueType));
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\napres K:");
			for (j = 0; j < LargestNumberEntries+1; j++)
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%7s,", Largest[j]->Key);
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
		}
#endif
		Index = LargestNumberEntries;
	}
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Index=%d minmax=%.0f\n", Index, SELECT_DI_VAL2(Largest[Index], ValueType));
#endif
	/*-calculate the value pointed to by the output parameter "SmallestOfTheLargest_waiver_flag_p": */
	if (waiver_flags_present == 2 || waiver_flags_present == 1)
	{
		(*SmallestOfTheLargest_waiver_flag_p) = Largest[Index]->waiver_flag;
	}
	else
	{
		(*SmallestOfTheLargest_waiver_flag_p) = 0;
	}
	return SELECT_DI_VAL2(Largest[Index], ValueType);
}
/*------------------------------------------------------------------------------
Low level function to add an item to a cell
------------------------------------------------------------------------------*/
static EIT_RETURNCODE CellAddItem (
	STCT_CELL * Cell,
	STCT_DATAITEM * Item)
{
	if (Item->Key[0] == '\0') {
		Cell->AnonymousDataItem.NumberObservations += Item->NumberObservations;
		Cell->AnonymousDataItem.AbsVar          += Item->AbsVar         ;
		Cell->AnonymousDataItem.WAbsVar         += Item->WAbsVar        ;
		Cell->AnonymousDataItem.Value           += Item->Value          ;
		Cell->AnonymousDataItem.Shadow          += Item->Shadow         ;
		Cell->AnonymousDataItem.Proxy           += Item->Proxy          ;
		Cell->AnonymousDataItem.ValuePt         += Item->ValuePt        ;
		Cell->AnonymousDataItem.ValuePw         += Item->ValuePw        ;
		Cell->AnonymousDataItem.ValueN          += Item->ValueN         ;
		Cell->AnonymousDataItem.ValueSn         += Item->ValueSn        ;
		Cell->AnonymousDataItem.ValueX          += Item->ValueX         ;
		Cell->AnonymousDataItem.ProxyPt         += Item->ProxyPt        ;
		Cell->AnonymousDataItem.ProxyPw         += Item->ProxyPw        ;
		Cell->AnonymousDataItem.ProxyN          += Item->ProxyN         ;
		Cell->AnonymousDataItem.ProxySn         += Item->ProxySn        ;
		Cell->AnonymousDataItem.ProxyX          += Item->ProxyX         ;
		Cell->AnonymousDataItem.FT              += Item->FT             ;
		Cell->AnonymousDataItem.FW              += Item->FW             ;
		Cell->AnonymousDataItem.FS              += Item->FS             ;
		Cell->AnonymousDataItem.Weight          += Item->Weight         ;
		Cell->AnonymousDataItem.WeightedValue   += Item->WeightedValue  ;
		Cell->AnonymousDataItem.WeightedShadow  += Item->WeightedShadow ;
		Cell->AnonymousDataItem.WeightedProxy   += Item->WeightedProxy  ;
		Cell->AnonymousDataItem.SecondryValuePt += Item->SecondryValuePt;
		Cell->AnonymousDataItem.SecondryValuePw += Item->SecondryValuePw;
		Cell->AnonymousDataItem.SecondryProxyPt += Item->SecondryProxyPt;
		Cell->AnonymousDataItem.SecondryProxyPw += Item->SecondryProxyPw;
		Cell->AnonymousDataItem.SecondryFT      += Item->SecondryFT     ;
		Cell->AnonymousDataItem.SecondryFW      += Item->SecondryFW     ;
		Cell->AnonymousDataItem.SecondryFS      += Item->SecondryFS     ;
		Cell->AnonymousDataItem.WeightedValueX  += Item->WeightedValueX ;
		Cell->AnonymousDataItem.WeightedProxyX  += Item->WeightedProxyX ;
		if      (Cell->AnonymousDataItem.MixedSignStatus == -1) {
			if      (Item->MixedSignStatus == -1) {
				Cell->AnonymousDataItem.MixedSignStatus = -1;
			}
			else if (Item->MixedSignStatus ==  0) {
				Cell->AnonymousDataItem.MixedSignStatus = -1;
			}
			else if (Item->MixedSignStatus ==  1) {
				Cell->AnonymousDataItem.MixedSignStatus =  2;
			}
			else if (Item->MixedSignStatus ==  2) {
				Cell->AnonymousDataItem.MixedSignStatus =  2;
			}
		}
		else if (Cell->AnonymousDataItem.MixedSignStatus ==  0) {
			Cell->AnonymousDataItem.MixedSignStatus = Item->MixedSignStatus;
		}
		else if (Cell->AnonymousDataItem.MixedSignStatus ==  1) {
			if      (Item->MixedSignStatus == -1) {
				Cell->AnonymousDataItem.MixedSignStatus =  2;
			}
			else if (Item->MixedSignStatus ==  0) {
				Cell->AnonymousDataItem.MixedSignStatus =  1;
			}
			else if (Item->MixedSignStatus ==  1) {
				Cell->AnonymousDataItem.MixedSignStatus =  1;
			}
			else if (Item->MixedSignStatus ==  2) {
				Cell->AnonymousDataItem.MixedSignStatus =  2;
			}
		}
		else if (Cell->AnonymousDataItem.MixedSignStatus ==  2) {
			/*-Cell->AnonymousDataItem.MixedSignStatus unchanged no change no matter what "Item->MixedSignStatus" is */
		}
	}
	else {
		/* need more space? */
		if (Cell->Data.NumberEntries == Cell->Data.NumberAllocated) {
			if (ReallocateCell (Cell) == NULL) return EIE_FAIL;
		}
		Cell->Data.Item[Cell->Data.NumberEntries] = STC_AllocateMemory (
			sizeof *Cell->Data.Item[Cell->Data.NumberEntries]);
		if (Cell->Data.Item[Cell->Data.NumberEntries] == NULL) return EIE_FAIL;
		if (Cell->IsInternal) {
			//allocate space for Key only when the Cell is an internal cell.
			//that way we minimize the memory usage.
			Cell->Data.Item[Cell->Data.NumberEntries]->Key = STC_StrDup (Item->Key);//allocate memory
			if (Cell->Data.Item[Cell->Data.NumberEntries]->Key == NULL) return EIE_FAIL;
		}
		else {//!Cell->IsInternal
			//only keep reference to Key
			//that way we minimize the memory usage.
			Cell->Data.Item[Cell->Data.NumberEntries]->Key = Item->Key;//do not allocate memory
		}
		Cell->Data.Item[Cell->Data.NumberEntries]->NumberObservations = Item->NumberObservations;
		Cell->Data.Item[Cell->Data.NumberEntries]->AbsVar             = Item->AbsVar            ;
		Cell->Data.Item[Cell->Data.NumberEntries]->WAbsVar            = Item->WAbsVar           ;
		Cell->Data.Item[Cell->Data.NumberEntries]->Value              = Item->Value             ;
		Cell->Data.Item[Cell->Data.NumberEntries]->Shadow             = Item->Shadow            ;
		Cell->Data.Item[Cell->Data.NumberEntries]->Proxy              = Item->Proxy             ;
		Cell->Data.Item[Cell->Data.NumberEntries]->ValuePt            = Item->ValuePt           ;
		Cell->Data.Item[Cell->Data.NumberEntries]->ValuePw            = Item->ValuePw           ;
		Cell->Data.Item[Cell->Data.NumberEntries]->ValueN             = Item->ValueN            ;
		Cell->Data.Item[Cell->Data.NumberEntries]->ValueSn            = Item->ValueSn           ;
		Cell->Data.Item[Cell->Data.NumberEntries]->ValueX             = Item->ValueX            ;
		Cell->Data.Item[Cell->Data.NumberEntries]->ProxyPt            = Item->ProxyPt           ;
		Cell->Data.Item[Cell->Data.NumberEntries]->ProxyPw            = Item->ProxyPw           ;
		Cell->Data.Item[Cell->Data.NumberEntries]->ProxyN             = Item->ProxyN            ;
		Cell->Data.Item[Cell->Data.NumberEntries]->ProxySn            = Item->ProxySn           ;
		Cell->Data.Item[Cell->Data.NumberEntries]->ProxyX             = Item->ProxyX            ;
		Cell->Data.Item[Cell->Data.NumberEntries]->FT                 = Item->FT                ;
		Cell->Data.Item[Cell->Data.NumberEntries]->FW                 = Item->FW                ;
		Cell->Data.Item[Cell->Data.NumberEntries]->FS                 = Item->FS                ;
		Cell->Data.Item[Cell->Data.NumberEntries]->Weight             = Item->Weight            ;
		Cell->Data.Item[Cell->Data.NumberEntries]->WeightedValue      = Item->WeightedValue     ;
		Cell->Data.Item[Cell->Data.NumberEntries]->WeightedShadow     = Item->WeightedShadow    ;
		Cell->Data.Item[Cell->Data.NumberEntries]->WeightedProxy      = Item->WeightedProxy     ;
		Cell->Data.Item[Cell->Data.NumberEntries]->SecondryValuePt    = Item->SecondryValuePt   ;
		Cell->Data.Item[Cell->Data.NumberEntries]->SecondryValuePw    = Item->SecondryValuePw   ;
		Cell->Data.Item[Cell->Data.NumberEntries]->SecondryProxyPt    = Item->SecondryProxyPt   ;
		Cell->Data.Item[Cell->Data.NumberEntries]->SecondryProxyPw    = Item->SecondryProxyPw   ;
		Cell->Data.Item[Cell->Data.NumberEntries]->SecondryFT         = Item->SecondryFT        ;
		Cell->Data.Item[Cell->Data.NumberEntries]->SecondryFW         = Item->SecondryFW        ;
		Cell->Data.Item[Cell->Data.NumberEntries]->SecondryFS         = Item->SecondryFS        ;
		Cell->Data.Item[Cell->Data.NumberEntries]->WeightedValueX     = Item->WeightedValueX    ;
		Cell->Data.Item[Cell->Data.NumberEntries]->WeightedProxyX     = Item->WeightedProxyX    ;
		Cell->Data.Item[Cell->Data.NumberEntries]->MixedSignStatus    = Item->MixedSignStatus   ;
		Cell->Data.Item[Cell->Data.NumberEntries]->waiver_flag        = Item->waiver_flag       ;
		Cell->Data.NumberEntries++;
	}
	Cell->TotalNumberObservations += Item->NumberObservations  ;
	Cell->TotalValue              += Item->Value               ;
	Cell->TotalShadow             += Item->Shadow              ;
	Cell->TotalProxy              += Item->Proxy               ;
	Cell->TotalValuePt            += Item->ValuePt             ;
	Cell->TotalValuePw            += Item->ValuePw             ;
	Cell->TotalValueN             += Item->ValueN              ;
	Cell->TotalValueSn            += Item->ValueSn             ;
	Cell->TotalValueX             += Item->ValueX              ;
	Cell->TotalProxyPt            += Item->ProxyPt             ;
	Cell->TotalProxyPw            += Item->ProxyPw             ;
	Cell->TotalProxyN             += Item->ProxyN              ;
	Cell->TotalProxySn            += Item->ProxySn             ;
	Cell->TotalProxyX             += Item->ProxyX              ;
	Cell->TotalFT                 += Item->FT                  ;
	Cell->TotalFW                 += Item->FW                  ;
	Cell->TotalFS                 += Item->FS                  ;
	Cell->TotalWeight             += Item->Weight              ;
	Cell->TotalWeightedValue      += Item->WeightedValue       ;
	Cell->TotalWeightedShadow     += Item->WeightedShadow      ;
	Cell->TotalWeightedProxy      += Item->WeightedProxy       ;
	Cell->TotalSecondryValuePt    += Item->SecondryValuePt     ;
	Cell->TotalSecondryValuePw    += Item->SecondryValuePw     ;
	Cell->TotalSecondryProxyPt    += Item->SecondryProxyPt     ;
	Cell->TotalSecondryProxyPw    += Item->SecondryProxyPw     ;
	Cell->TotalSecondryFT         += Item->SecondryFT          ;
	Cell->TotalSecondryFW         += Item->SecondryFW          ;
	Cell->TotalSecondryFS         += Item->SecondryFS          ;
	Cell->TotalWeightedValueX     += Item->WeightedValueX      ;
	Cell->TotalWeightedProxyX     += Item->WeightedProxyX      ;
	if      (Cell->TotalMixedSignStatus == -1) {
		if      (Item->MixedSignStatus == -1) {
			Cell->TotalMixedSignStatus = -1;
		}
		else if (Item->MixedSignStatus ==  0) {
			Cell->TotalMixedSignStatus = -1;
		}
		else if (Item->MixedSignStatus ==  1) {
			Cell->TotalMixedSignStatus =  2;
		}
		else if (Item->MixedSignStatus ==  2) {
			Cell->TotalMixedSignStatus =  2;
		}
	}
	else if (Cell->TotalMixedSignStatus ==  0) {
		Cell->TotalMixedSignStatus = Item->MixedSignStatus;
	}
	else if (Cell->TotalMixedSignStatus ==  1) {
		if      (Item->MixedSignStatus == -1) {
			Cell->TotalMixedSignStatus =  2;
		}
		else if (Item->MixedSignStatus ==  0) {
			Cell->TotalMixedSignStatus =  1;
		}
		else if (Item->MixedSignStatus ==  1) {
			Cell->TotalMixedSignStatus =  1;
		}
		else if (Item->MixedSignStatus ==  2) {
			Cell->TotalMixedSignStatus =  2;
		}
	}
	else if (Cell->TotalMixedSignStatus ==  2) {
		/*-Cell->TotalMixedSignStatus unchanged no change no matter what "Item->MixedSignStatus" is */
	}
	//the cell content changed, the sensitivity is no longer valid
	Cell->Sensitivity              = SENSITIVITY_NOT_CALCULATED;
	Cell->Sensitivity_nowaivers    = SENSITIVITY_NOT_CALCULATED;
	Cell->Sensitivity_noproxy      = SENSITIVITY_NOT_CALCULATED;
	Cell->Sensitivity_noweights    = SENSITIVITY_NOT_CALCULATED;
	Cell->WhichNoise               = ' '                       ;
	Cell->FavCost                  = FAVCOST_NOT_CALCULATED    ;
	Cell->LargestNumberEntries   = 0;
	Cell->LargestPYNumberEntries = 0;
	Cell->LargestWVNumberEntries = 0;
	Cell->LargestWPNumberEntries = 0;
	Cell->LargestFTNumberEntries = 0;
	Cell->LargestFWNumberEntries = 0;
	Cell->LargestFSNumberEntries = 0;
	Cell->Largst2FTNumberEntries = 0;
	Cell->Largst2FWNumberEntries = 0;
	Cell->Largst2FSNumberEntries = 0;
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
pour développement seulement
------------------------------------------------------------------------------*/
#ifdef _DEBUG
static void CheckSortKeys (
	STCT_CELL * Cell)
{
	int i;
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "CheckSortKeys() should be called only when testing!\n");
	for (i = 0; i < Cell->Data.NumberEntries-1; i++) {
		if (strcmp (Cell->Data.Item[i]->Key, Cell->Data.Item[i+1]->Key) > 0) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Keys not sorted correctly!\n");
			STC_CellPrint (Cell);
			exit (EXIT_FAILURE);//developpement seulement
		}
	}
}
#endif
/*------------------------------------------------------------------------------
Count non anonymous positive number entries
------------------------------------------------------------------------------*/
static void CountNonAnonymousPositiveNumberEntries (
	STCT_CELL * Cell)
{
	int i;

	Cell->NonAnonymousPositiveNumberEntries = 0;
	for (i = 0; i < Cell->Data.NumberEntries; i++) {
		if (Cell->Data.Item[i]->Value > 0.0)
			Cell->NonAnonymousPositiveNumberEntries++;
	}
}
/*------------------------------------------------------------------------------
Scan all item to find the largest items in the cell.
This function iterates over all of the items in the list Cell->Data.Item, and
puts the n largest in the list "Largest"; if the list "Largest" isn't
full (contains less than n items) the current item will be inserted in the list,
and the call to "AddLargest()" will figure out where, but if the list already
contains n items, then the current item should only be inserted in the list iff
"Largest" if sorts before the current member of "Largest" that sorts
last among the current members of "Largest" (and again the call to
"AddLargest()" will figure out where in the list the item will be inserted).
If waiver_flags_present is 0 (the user hasn't specified to take waivers into
account), then items are sorted in descending order of their Value members,
but if waiver_flags_present is 1, then items are sorted first in descending
order of their Value members and then items with the same value for their
Value members are sorted in ASCENDING order of their waiver flags (that is,
among items whose Value members have the same value, items whose waiver flag
is 0 sort before items whose waiver flag is 1)
------------------------------------------------------------------------------*/
static void FindLargest (
	STCT_CELL * Cell,
	int n,
	STCT_DATAITEM ** Largest,
	int * LargestNumberEntriesPtr,
	char ValueType,
	int waiver_flags_present)
{
	int i;
	double SmallestOfTheLargest;
	int    SmallestOfTheLargest_waiver_flag;
	double Value;

	SmallestOfTheLargest = -DBL_MAX;
	SmallestOfTheLargest_waiver_flag = 0;

	(*LargestNumberEntriesPtr) = 0;
	for (i = 0; i < Cell->Data.NumberEntries; i++) {
		Value =  ValueType=='v'?Cell->Data.Item[i]->ValueX
		        :ValueType=='p'?Cell->Data.Item[i]->ProxyX
		        :ValueType=='V'?Cell->Data.Item[i]->WeightedValueX
		        :               Cell->Data.Item[i]->WeightedProxyX;
		/* if ((*LargestNumberEntriesPtr) < n || Value > SmallestOfTheLargest) { */ /* } */
		if ((*LargestNumberEntriesPtr) < n ||
		    (                             Value >  SmallestOfTheLargest                                                                                    ) ||
		    ((waiver_flags_present == 2 || waiver_flags_present == 1) && Value == SmallestOfTheLargest && Cell->Data.Item[i]->waiver_flag == 1 && SmallestOfTheLargest_waiver_flag != 1)
		   ) {
			SmallestOfTheLargest = AddLargest (Largest, (*LargestNumberEntriesPtr), n, Cell->Data.Item[i], ValueType, &SmallestOfTheLargest_waiver_flag, waiver_flags_present);
			if ((*LargestNumberEntriesPtr) < n) (*LargestNumberEntriesPtr)++;
		}
	}
}
/*-find the min(STCM_MAXLARGESTS, Cell->Data.NumberEntries) STCT_DATAITEM instances in Cell->Data.Item        */
/* having the largest values for their FT (if "WhichF" is 'T'), FW (if "WhichF" is 'W'), or FS                */
/* (if "WhichF" is 'S') field and store their addresses in Cell->LargestFT (if "WhichF" is 'T'),              */
/* Cell->LargestFW (if "WhichF" is 'W'), or Cell->LargestFS (if "WhichF" is 'S')                              */
/* and the number of entries in the array in Cell->LargestFTNumberEntries (if "WhichF" is 'T'),               */
/* Cell->LargestFWNumberEntries (if "WhichF" is 'W'), or Cell->LargestFSNumberEntries (if "WhichF" is 'S')    */
/* (uses a heap sort algorithm to select the largest entries and then sort them in descending order)          */
/* (the heap invariant is that the FT, FW, or FS field of every parent is LESS than that of all of its        */
/*  (1 or 2) children, so that means that (when the heap invariant holds) the top-level parent                */
/*  (the first element of the heap) is always has the SMALLEST value (or tied for the smallest value)         */
/*  for its FT, FW, or FS field, and once the heap is full new elements are added (replacing the first entry) */
/*  only if their FT, FW, or FS field is LARGER than that of the first entry, which ensures that once         */
/*  all candidates have been considered the heap contains the "HeapSize" candidates with the LARGEST          */
/*  FT, FW, or FS values                                                                                      */
/* )                                                                                                          */
/*-example calls:                                                                                             */
/*     rc = FindLargestFX(Cell, Cell->LargestFX, &(Cell->LargestFTNumberEntries), 'T');                       */
/*     rc = FindLargestFX(Cell, Cell->LargestFX, &(Cell->LargestFWNumberEntries), 'W');                       */
/*     rc = FindLargestFX(Cell, Cell->LargestFX, &(Cell->LargestFSNumberEntries), 'S');                       */
EIT_RETURNCODE FindLargestFX (
    STCT_CELL * Cell,
    STCT_DATAITEM ** LargestFX,
    int * LargestFXNumberEntriesPtr,
    char WhichF)
{
    /* STCT_DATAITEM * LargestFX[STCM_MAXLARGESTS]; */
    /* STCT_DATAITEM ** LargestFX; */
    int i;
    int HeapSize;
    int HeapEntries;
    int parent_index;
    int child_index;
    int child_index_1;
    int child_index_2;
    int previous_parent_index;
    int previous_child_index;
    int previous_child_index_1;
    int previous_child_index_2;
    int heap_property_holds;
    int parent_ndx;
    int child_ndx;
    int final_sorted_list_length;
    STCT_DATAITEM * DataItemPtr;
    
    STCT_DATAITEM * temp;
    
    int verify_heap_property_after_each_insertion = 0;
    
    EIT_RETURNCODE ReturnCode = EIE_SUCCEED;
    
    HeapSize = STCM_MAXLARGESTS;
    HeapEntries = 0;
    for(i = 0; i <= Cell->Data.NumberEntries; i = i + 1) {
        if (i < Cell->Data.NumberEntries) {
            DataItemPtr = Cell->Data.Item[i];
        }
        else { /* (i == Cell->Data.NumberEntries) */
            if (Cell->AnonymousDataItem.NumberObservations == 0) {
                continue;
            }
            DataItemPtr = &(Cell->AnonymousDataItem);
        }
        if (HeapEntries < HeapSize) {
            /*-insert DataItemPtr                                                               */
            /* in the first unused entry in the heap immediately following the last used entry, */
            /* then sift it down until the heap property is restored:                           */
            LargestFX[HeapEntries] = DataItemPtr;
            HeapEntries = HeapEntries + 1;
            /*-sift down: */
            child_index = HeapEntries - 1;
            parent_index = (((child_index+1)-((child_index+1) % 2)) / 2)-1;
            while (child_index > 0 &&
                   COMPAR_LT_FT_FS(LargestFX[child_index], LargestFX[parent_index], WhichF)
                  ) {
                temp = LargestFX[parent_index];
                LargestFX[parent_index] = LargestFX[child_index];
                LargestFX[child_index] = temp;
                previous_child_index = child_index;
                previous_parent_index = parent_index;
                child_index = parent_index;
                parent_index = (((child_index+1)-((child_index+1) % 2)) / 2)-1;
            }
        }
        else { /* (HeapEntries == HeapSize) */
            /*-if the FT/FW/FS member of the first entry in the heap                                                    */
            /* ((WhichF=='T')?(LargestFX[0]->FT):(WhichF=='W')?(LargestFX[0]->FW):(LargestFX[0]->FS))                   */
            /* is larger than (WhichF=='T')?(DataItemPtr->FT):(WhichF=='W')?(DataItemPtr->FW):(DataItemPtr->FS)         */
            /* then replace it (the first entry in the heap) with DataItemPtr,                                          */
            /* then sift it up until the heap property is restored:                                                     */
            if (COMPAR_GT_FT_FS(DataItemPtr, LargestFX[0], WhichF)) {
                LargestFX[0] = DataItemPtr;
                /*-sift up: */
                parent_index = 0;
                child_index_1 = (2*(parent_index+1)+0)-1;
                child_index_2 = (2*(parent_index+1)+1)-1;
                while ((child_index_1 <= (HeapSize-1) &&
                       COMPAR_LT_FT_FS(LargestFX[child_index_1], LargestFX[parent_index], WhichF)) ||
                       (child_index_2 <= (HeapSize-1) &&
                         COMPAR_LT_FT_FS(LargestFX[child_index_2], LargestFX[parent_index], WhichF)
                       )
                      ) {
                    if      (!(child_index_2 <= (HeapSize-1) &&
                               COMPAR_LT_FT_FS(LargestFX[child_index_2], LargestFX[parent_index], WhichF)
                              )
                            ) {
                        temp = LargestFX[parent_index];
                        LargestFX[parent_index] = LargestFX[child_index_1];
                        LargestFX[child_index_1] = temp;
                        previous_parent_index = parent_index;
                        parent_index = child_index_1;
                    }
                    else if (!(child_index_1 <= (HeapSize-1) &&
                               COMPAR_LT_FT_FS(LargestFX[child_index_1], LargestFX[parent_index], WhichF)
                              )
                            ) {
                        temp = LargestFX[parent_index];
                        LargestFX[parent_index] = LargestFX[child_index_2];
                        LargestFX[child_index_2] = temp;
                        previous_parent_index = parent_index;
                        parent_index = child_index_2;
                    }
                    else { /* ((child_index_1 <= (HeapSize-1) &&                                              */
                           /*  COMPAR_LT_FT_FS(LargestFX[child_index_1], LargestFX[parent_index], WhichF)) && */
                           /*  (child_index_2 <= (HeapSize-1) &&                                              */
                           /*   COMPAR_LT_FT_FS(LargestFX[child_index_2], LargestFX[parent_index], WhichF)    */
                           /*  )                                                                              */
                           /* )                                                                               */
                        if (COMPAR_LT_FT_FS(LargestFX[child_index_1], LargestFX[child_index_2], WhichF)) {
                            temp = LargestFX[parent_index];
                            LargestFX[parent_index] = LargestFX[child_index_1];
                            LargestFX[child_index_1] = temp;
                            previous_parent_index = parent_index;
                            parent_index = child_index_1;
                        }
                        else { /* ((WhichF=='T')?(LargestFX[child_index_1]->FT):(WhichF=='W')?(LargestFX[child_index_1]->FW):(LargestFX[child_index_1]->FS) >= (WhichF=='T')?(LargestFX[child_index_2]->FT):(WhichF=='W')?(LargestFX[child_index_2]->FW):(LargestFX[child_index_2]->FS)) */
                            temp = LargestFX[parent_index];
                            LargestFX[parent_index] = LargestFX[child_index_2];
                            LargestFX[child_index_2] = temp;
                            previous_parent_index = parent_index;
                            parent_index = child_index_2;
                        }
                    }
                    previous_child_index_1 = child_index_1;
                    previous_child_index_2 = child_index_2;
                    child_index_1 = (2*(parent_index+1)+0)-1;
                    child_index_2 = (2*(parent_index+1)+1)-1;
                }
            }
        }
        /* if (i==0) {                                                                                                                                                                              */
        /*     for(ndx=0;ndx<HeapEntries;ndx=ndx+1) {                                                                                                                                               */
        /*         IO_PRINT_LINE("after : (WhichF=='T')?(LargestFX[%d]->FT):(WhichF=='W')?(LargestFX[%d]->FW):(LargestFX[%d]->FS) = %f", ndx,                                                          */
        /*                                                               (WhichF=='T')?(LargestFX[ndx]->FT):(WhichF=='W')?(LargestFX[ndx]->FW):(LargestFX[ndx]->FS));                               */
        /*     }                                                                                                                                                                                    */
        /*     IO_PRINT_LINE("after : DataItemPtr->F%s = %f", (WhichF=='T')?"T":"S", (WhichF=='T')?(DataItemPtr->FT):(WhichF=='W')?(DataItemPtr->FW):(DataItemPtr->FS));                               */
        /*     IO_PRINT_LINE("after : (child_index_1, child_index_2, parent_index) = (%d, %d, %d)", child_index_1, child_index_2, parent_index);                                                       */
        /*     IO_PRINT_LINE("after : (previous_child_index_1, previous_child_index_2, previous_parent_index) = (%d, %d, %d)", previous_child_index_1, previous_child_index_2, previous_parent_index); */
        /* }                                                                                                                                                                                        */
        if (verify_heap_property_after_each_insertion) {
            /*-verify that the heap property holds: */
            heap_property_holds = 1;
            for(child_ndx=1;child_ndx<HeapEntries;child_ndx=child_ndx+1) {
                parent_ndx = (((child_ndx+1)-((child_ndx+1) % 2)) / 2)-1;
                /* if (i == ((Cell->Data.NumberEntries)-1)) {                                                                                      */
                /*     IO_PRINT_LINE("parent_ndx = %d, child_ndx = %d, (WhichF=='T')?(LargestFX[parent_ndx]->FT):(WhichF=='W')?(LargestFX[parent_ndx]->FW):(LargestFX[parent_ndx]->FS) = %f, "  */
                /*                                                 "(WhichF=='T')?(LargestFX[child_ndx ]->FT):(WhichF=='W')?(LargestFX[child_ndx ]->FW):(LargestFX[child_ndx ]->FS) = %f",   */
                /*                parent_ndx, child_ndx,                                                                                           */
                /*                (WhichF=='T')?(LargestFX[parent_ndx]->FT):(WhichF=='W')?(LargestFX[parent_ndx]->FW):(LargestFX[parent_ndx]->FS), */
                /*                (WhichF=='T')?(LargestFX[child_ndx ]->FT):(WhichF=='W')?(LargestFX[child_ndx ]->FW):(LargestFX[child_ndx ]->FS)  */
                /*               );                                                                                                                */
                /* }                                                                                                                               */
                if (COMPAR_LT_FT_FS(LargestFX[child_ndx], LargestFX[parent_ndx], WhichF)) {
                    heap_property_holds = 0;
                    IO_PRINT_LINE(M30206
                               M30207,
                               i, child_ndx, parent_ndx,
                               (WhichF=='T')?(LargestFX[child_ndx ]->FT):(WhichF=='W')?(LargestFX[child_ndx ]->FW):(LargestFX[child_ndx ]->FS),
                               (WhichF=='T')?(LargestFX[parent_ndx]->FT):(WhichF=='W')?(LargestFX[parent_ndx]->FW):(LargestFX[parent_ndx]->FS)
                              );
                }
            }
        }
        /* for(ndx=0;ndx<HeapEntries;ndx=ndx+1) {                                                                                                     */
        /*     IO_PRINT_LINE("(WhichF=='T')?(LargestFX[%d]->FT):(WhichF=='W')?(LargestFX[%d]->FW):(LargestFX[%d]->FS) = %f", ndx, (WhichF=='T')?(LargestFX[ndx]->FT):(WhichF=='W')?(LargestFX[ndx]->FW):(LargestFX[ndx]->FS)); */
        /* }                                                                                                                                           */
    }
    /*-now sort the elements in the occupied entries (the first "HeapEntries" entries)    */
    /* of the array "LargestFX" in ascending order of the size of the  "FT", "FW", or     */
    /* "FS" field                                                                         */
    /* (by repeatedly:                                                                    */
    /*    -swapping the first and last elements in the heap                               */
    /*    -then treating the element that is then last as being no longer part of the     */
    /*     heap (i.e. reducing the heap size "HeapEntries" by 1)                          */
    /*    -then sifting the element that is now the first element of the heap up until    */
    /*     the heap property is  restored (at which point the first element in the        */
    /*     heap will again be the element with the smallest value of "FT", "FW", or "FS") */
    /*  until the heap size "HeapEntries" is reduced to 1                                 */
    /* ):                                                                                 */
    final_sorted_list_length = HeapEntries;
    while (HeapEntries > 1) {
        HeapEntries = HeapEntries - 1;
        /*-swap the first element in the heap and the first element after the end of the heap in the array containing the heap: */
        temp = LargestFX[0];
        LargestFX[0] = LargestFX[HeapEntries];
        LargestFX[HeapEntries] = temp;
        /*-sift up: */
        parent_index = 0;
        child_index_1 = (2*(parent_index+1)+0)-1;
        child_index_2 = (2*(parent_index+1)+1)-1;
        while ((child_index_1 <= (HeapEntries-1) &&
                COMPAR_LT_FT_FS(LargestFX[child_index_1], LargestFX[parent_index], WhichF)
               ) ||
               (child_index_2 <= (HeapEntries-1) &&
                COMPAR_LT_FT_FS(LargestFX[child_index_2], LargestFX[parent_index], WhichF)
               )
              ) {
            if   (!(child_index_2 <= (HeapEntries-1) &&
                    COMPAR_LT_FT_FS(LargestFX[child_index_2], LargestFX[parent_index], WhichF)
                   )
                 ) {
                temp = LargestFX[parent_index];
                LargestFX[parent_index] = LargestFX[child_index_1];
                LargestFX[child_index_1] = temp;
                previous_parent_index = parent_index;
                parent_index = child_index_1;
            }
            else if (!(child_index_1 <= (HeapEntries-1) &&
                       COMPAR_LT_FT_FS(LargestFX[child_index_1], LargestFX[parent_index], WhichF)
                      )
                    ) {
                temp = LargestFX[parent_index];
                LargestFX[parent_index] = LargestFX[child_index_2];
                LargestFX[child_index_2] = temp;
                previous_parent_index = parent_index;
                parent_index = child_index_2;
            }
            else { /* ((child_index_1 <= (HeapEntries-1) &&                                       */
                   /*   COMPAR_LT_FT_FS(LargestFX[child_index_1], LargestFX[parent_index], WhichF) */
                   /*  ) &&                                                                        */
                   /*  (child_index_2 <= (HeapEntries-1) &&                                       */
                   /*   COMPAR_LT_FT_FS(LargestFX[child_index_2], LargestFX[parent_index], WhichF) */
                   /*  )                                                                           */
                   /* )                                                                            */
                if (COMPAR_LT_FT_FS(LargestFX[child_index_1], LargestFX[child_index_2], WhichF)) {
                    temp = LargestFX[parent_index];
                    LargestFX[parent_index] = LargestFX[child_index_1];
                    LargestFX[child_index_1] = temp;
                    previous_parent_index = parent_index;
                    parent_index = child_index_1;
                }
                else { /* ((WhichF=='T')?(LargestFX[child_index_1]->FT):(WhichF=='W')?(LargestFX[child_index_1]->FW):(LargestFX[child_index_1]->FS) >= (WhichF=='T')?(LargestFX[child_index_2]->FT):(WhichF=='W')?(LargestFX[child_index_2]->FW):(LargestFX[child_index_2]->FS)) */
                    temp = LargestFX[parent_index];
                    LargestFX[parent_index] = LargestFX[child_index_2];
                    LargestFX[child_index_2] = temp;
                    previous_parent_index = parent_index;
                    parent_index = child_index_2;
                }
            }
            previous_child_index_1 = child_index_1;
            previous_child_index_2 = child_index_2;
            child_index_1 = (2*(parent_index+1)+0)-1;
            child_index_2 = (2*(parent_index+1)+1)-1;
        }
        if (verify_heap_property_after_each_insertion) {
            /*-verify that the heap property holds: */
            heap_property_holds = 1;
            for (child_ndx=1;child_ndx<HeapEntries;child_ndx=child_ndx+1) {
                parent_ndx = (((child_ndx+1)-((child_ndx+1) % 2)) / 2)-1;
                if (COMPAR_LT_FT_FS(LargestFX[child_ndx], LargestFX[parent_ndx], WhichF)) {
                    heap_property_holds = 0;
                    IO_PRINT_LINE(M30206
                               M30207,
                               i, child_ndx, parent_ndx,
                               (WhichF=='T')?(LargestFX[child_ndx ]->FT):(WhichF=='W')?(LargestFX[child_ndx ]->FW):(LargestFX[child_ndx ]->FS),
                               (WhichF=='T')?(LargestFX[parent_ndx]->FT):(WhichF=='W')?(LargestFX[parent_ndx]->FW):(LargestFX[parent_ndx]->FS)
                              );
                }
            }
        }
        /* print(LargestFX); */
    }
    /*-now the elements of the occupied part of the array "LargestFX" are sorted in DESCENDING order of the value of the "FT", "FW", or "FS" field; */
    
    /*-now the elements of the occupied part of the array "LargestFX" are sorted in ASCENDING  order of the value of the "FT", "FW", or "FS" field; */
    /* reverse their order by swapping (NO, THIS IS NOT TRUE, SO THE REVERSAL OF THE ORDERING ISN'T NEEDED AFTER ALL):                       */
    /*    -     the 1st element (element 0) and the     last element (element final_sorted_list_length-(0+1))                                */
    /*    -then the 2nd element (element 1) and the 2nd last element (element final_sorted_list_length-(1+1))                                */
    /*    -then the 3rd element (element 2) and the 3rd last element (element final_sorted_list_length-(2+1))                                */
    /*    ...                                                                                                                                */
    /*    -finally element ((final_sorted_list_length/2)-1) and element final_sorted_list_length-((final_sorted_list_length/2)-1)            */
    /*        -if "final_sorted_list_length" is even then these will be the two (consecutive) elements                                       */
    /*         in the middle of the list                                                                                                     */
    /*        -if "final_sorted_list_length" is odd  then these will be the two               elements                                       */
    /*         immediately before and after the element in the middle of the list                                                            */
    /* :                                                                                                                                     */
    /* for (ndx=0;ndx<(final_sorted_list_length/2);ndx=ndx+1) {                                  */
    /*     temp                                        = LargestFX[                          ndx   ]; */
    /*     LargestFX[                          ndx   ] = LargestFX[final_sorted_list_length-(ndx+1)]; */
    /*     LargestFX[final_sorted_list_length-(ndx+1)] = temp                                       ; */
    /* }                                                                                              */
    /* print(LargestFX); */
    
    /*-save the length of the sorted list in "Cell->LargestFTNumberEntries", "Cell->LargestFWNumberEntries", or "Cell->LargestFSNumberEntries": */
    (*LargestFXNumberEntriesPtr) = final_sorted_list_length;
    
/*     goto normal_cleanup; */
/*                          */
/* error_cleanup:           */
/*                          */
/* normal_cleanup:          */
    
    return ReturnCode;
}
/*------------------------------------------------------------------------------
initialize cell structure
------------------------------------------------------------------------------*/
static void InitializeCell (
	STCT_CELL * Cell)
{
	int i;

	Cell->CellId = 0;
	Cell->Data.Item = NULL; /* allocate the structure contents only when needed */
	Cell->Data.NumberAllocated = 0;
	Cell->Data.NumberEntries = 0;
	Cell->Coordinate = NULL;
	Cell->IsInternal = EIE_FALSE;
	Cell->Type = STC_CELLTYPE_CELL; /* default to CELL type */
	Cell->AnonymousKey[0]                      = (char)0; /*-this does the same thing as strcpy(Cell->AnonymousKey, "") but is more efficient */
	Cell->AnonymousDataItem.Key                = Cell->AnonymousKey;
	Cell->AnonymousDataItem.NumberObservations = 0  ;
	Cell->AnonymousDataItem.AbsVar             = 0.0;
	Cell->AnonymousDataItem.WAbsVar            = 0.0;
	Cell->AnonymousDataItem.Value              = 0.0;
	Cell->AnonymousDataItem.Shadow             = 0.0;
	Cell->AnonymousDataItem.Proxy              = 0.0;
	Cell->AnonymousDataItem.ValuePt            = 0.0;
	Cell->AnonymousDataItem.ValuePw            = 0.0;
	Cell->AnonymousDataItem.ValueN             = 0.0;
	Cell->AnonymousDataItem.ValueSn            = 0.0;
	Cell->AnonymousDataItem.ValueX             = 0.0;
	Cell->AnonymousDataItem.ProxyPt            = 0.0;
	Cell->AnonymousDataItem.ProxyPw            = 0.0;
	Cell->AnonymousDataItem.ProxyN             = 0.0;
	Cell->AnonymousDataItem.ProxySn            = 0.0;
	Cell->AnonymousDataItem.ProxyX             = 0.0;
	Cell->AnonymousDataItem.FT                 = 0.0;
	Cell->AnonymousDataItem.FW                 = 0.0;
	Cell->AnonymousDataItem.FS                 = 0.0;
	Cell->AnonymousDataItem.Weight             = 0.0;
	Cell->AnonymousDataItem.WeightedValue      = 0.0;
	Cell->AnonymousDataItem.WeightedShadow     = 0.0;
	Cell->AnonymousDataItem.WeightedProxy      = 0.0;
	Cell->AnonymousDataItem.SecondryValuePt    = 0.0;
	Cell->AnonymousDataItem.SecondryValuePw    = 0.0;
	Cell->AnonymousDataItem.SecondryProxyPt    = 0.0;
	Cell->AnonymousDataItem.SecondryProxyPw    = 0.0;
	Cell->AnonymousDataItem.SecondryFT         = 0.0;
	Cell->AnonymousDataItem.SecondryFW         = 0.0;
	Cell->AnonymousDataItem.SecondryFS         = 0.0;
	Cell->AnonymousDataItem.WeightedValueX     = 0.0;
	Cell->AnonymousDataItem.WeightedProxyX     = 0.0;
	Cell->AnonymousDataItem.MixedSignStatus    =   0;
	Cell->AnonymousDataItem.waiver_flag        =   0;
	Cell->TotalNumberObservations     = 0  ;
	Cell->TotalValue                  = 0.0;
	Cell->TotalShadow                 = 0.0;
	Cell->TotalProxy                  = 0.0;
	Cell->TotalValuePt                = 0.0;
	Cell->TotalValuePw                = 0.0;
	Cell->TotalValueN                 = 0.0;
	Cell->TotalValueSn                = 0.0;
	Cell->TotalValueX                 = 0.0;
	Cell->TotalProxyPt                = 0.0;
	Cell->TotalProxyPw                = 0.0;
	Cell->TotalProxyN                 = 0.0;
	Cell->TotalProxySn                = 0.0;
	Cell->TotalProxyX                 = 0.0;
	Cell->TotalFT                     = 0.0;
	Cell->TotalFW                     = 0.0;
	Cell->TotalFS                     = 0.0;
	Cell->TotalWeight                 = 0.0;
	Cell->TotalWeightedValue          = 0.0;
	Cell->TotalWeightedShadow         = 0.0;
	Cell->TotalWeightedProxy          = 0.0;
	Cell->TotalSecondryValuePt        = 0.0;
	Cell->TotalSecondryValuePw        = 0.0;
	Cell->TotalSecondryProxyPt        = 0.0;
	Cell->TotalSecondryProxyPw        = 0.0;
	Cell->TotalSecondryFT             = 0.0;
	Cell->TotalSecondryFW             = 0.0;
	Cell->TotalSecondryFS             = 0.0;
	Cell->TotalWeightedValueX         = 0.0;
	Cell->WeightedNbResp              = 0.0;
	Cell->TotalWeightedProxyX         = 0.0;
	Cell->TotalMixedSignStatus        =   0;
	Cell->Sensitivity = SENSITIVITY_NOT_CALCULATED;
	Cell->Sensitivity_nowaivers = SENSITIVITY_NOT_CALCULATED;
	Cell->Sensitivity_noproxy   = SENSITIVITY_NOT_CALCULATED;
	Cell->Sensitivity_noweights = SENSITIVITY_NOT_CALCULATED;
	Cell->WhichNoise            = ' '                       ;
	Cell->FavCost = FAVCOST_NOT_CALCULATED;
	Cell->LargestNumberEntries = 0;
	Cell->LargestPYNumberEntries = 0;
	Cell->LargestWVNumberEntries = 0;
	Cell->LargestWPNumberEntries = 0;
	Cell->LargestFTNumberEntries = 0;
	Cell->LargestFWNumberEntries = 0;
	Cell->LargestFSNumberEntries = 0;
	Cell->Largst2FTNumberEntries = 0;
	Cell->Largst2FWNumberEntries = 0;
	Cell->Largst2FSNumberEntries = 0;
	for (i=0;i<STCM_MAXLARGESTS;i=i+1) {
		Cell->Largest[i] = NULL;
	}
	for (i=0;i<STCM_MAXLARGESTS;i=i+1) {
		Cell->LargestPY[i] = NULL;
	}
	for (i=0;i<STCM_MAXLARGESTS;i=i+1) {
		Cell->LargestWV[i] = NULL;
	}
	for (i=0;i<STCM_MAXLARGESTS;i=i+1) {
		Cell->LargestWP[i] = NULL;
	}
	for (i=0;i<STCM_MAXLARGESTS;i=i+1) {
		Cell->LargestFT[i] = NULL;
	}
	for (i=0;i<STCM_MAXLARGESTS;i=i+1) {
		Cell->LargestFW[i] = NULL;
	}
	for (i=0;i<STCM_MAXLARGESTS;i=i+1) {
		Cell->LargestFS[i] = NULL;
	}
	for (i=0;i<STCM_MAXLARGESTS;i=i+1) {
		Cell->Largst2FT[i] = NULL;
	}
	for (i=0;i<STCM_MAXLARGESTS;i=i+1) {
		Cell->Largst2FW[i] = NULL;
	}
	for (i=0;i<STCM_MAXLARGESTS;i=i+1) {
		Cell->Largst2FS[i] = NULL;
	}
}
/*------------------------------------------------------------------------------
initialize cell set structure
------------------------------------------------------------------------------*/
static void InitializeCellSet (
	STCT_CELLSET * CellSet)
{
	CellSet->Cell = NULL; /* allocate the structure contents only when needed */
	CellSet->NumberAllocated = 0;
	CellSet->NumberEntries = 0;
}
/*------------------------------------------------------------------------------
re/allocate Cell->AllocationIncrement new item
------------------------------------------------------------------------------*/
static STCT_CELL * ReallocateCell (
	STCT_CELL * Cell)
{
	STCT_DATAITEM ** Ptr;

	Cell->Data.NumberAllocated += Cell->Data.AllocationIncrement;
#ifdef DEBUGALLOCATION
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "ReallocateCell ne=%d/na=%d\n", Cell->Data.NumberEntries, Cell->Data.NumberAllocated);
#endif
	Ptr = STC_ReallocateMemory (
		Cell->Data.NumberEntries * sizeof *Cell->Data.Item,
		Cell->Data.NumberAllocated * sizeof *Cell->Data.Item,
		Cell->Data.Item);
	if (Ptr == NULL) return NULL;
	Cell->Data.Item = Ptr;
	return Cell;
}
/*------------------------------------------------------------------------------
re/allocate CellSet->AllocationIncrement new cell
------------------------------------------------------------------------------*/
static STCT_CELLSET * ReallocateCellSet (
	STCT_CELLSET * CellSet)
{
	STCT_CELL ** Ptr;

	CellSet->NumberAllocated += CellSet->AllocationIncrement;
#ifdef DEBUGALLOCATION
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "ReallocateCellSet ne=%d/na=%d\n", CellSet->NumberEntries, CellSet->NumberAllocated);
#endif
	Ptr = STC_ReallocateMemory (
		CellSet->NumberEntries * sizeof *CellSet->Cell,
		CellSet->NumberAllocated * sizeof *CellSet->Cell,
		CellSet->Cell);
	if (Ptr == NULL) return NULL;
	CellSet->Cell = Ptr;
	return CellSet;
}
/*------------------------------------------------------------------------------
Merge consecutive items with the same key
once all item are added to the cell and the items are sorted by the value
------------------------------------------------------------------------------*/
void RemoveDuplicateKeys (
	STCT_CELL * Cell)
{
	int i, j;

	if (Cell->Data.NumberEntries < 2) return;
	i = 0;
	j = 1;
	while (j < Cell->Data.NumberEntries) {
		while (j < Cell->Data.NumberEntries &&
				strcmp (Cell->Data.Item[i]->Key, Cell->Data.Item[j]->Key) == 0) {
			//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "adding j to i -- i=%d, j=%d\n", i, j);
			Cell->Data.Item[i]->AbsVar             += Cell->Data.Item[j]->AbsVar            ;		  /* these 2 statements implement the summation AbsVar(i,c) and WAbsVar(i,c) as */
			Cell->Data.Item[i]->WAbsVar            += Cell->Data.Item[j]->WAbsVar           ;		  /*  described in the specs for WeightedNbRespondents */
			Cell->Data.Item[i]->NumberObservations += Cell->Data.Item[j]->NumberObservations;
			Cell->Data.Item[i]->Value              += Cell->Data.Item[j]->Value             ;
			Cell->Data.Item[i]->Shadow             += Cell->Data.Item[j]->Shadow            ;
			Cell->Data.Item[i]->Proxy              += Cell->Data.Item[j]->Proxy             ;
			Cell->Data.Item[i]->ValuePt            += Cell->Data.Item[j]->ValuePt           ;
			Cell->Data.Item[i]->ValuePw            += Cell->Data.Item[j]->ValuePw           ;
			Cell->Data.Item[i]->ValueN             += Cell->Data.Item[j]->ValueN            ;
			Cell->Data.Item[i]->ValueSn            += Cell->Data.Item[j]->ValueSn           ;
			Cell->Data.Item[i]->ValueX             += Cell->Data.Item[j]->ValueX            ;
			Cell->Data.Item[i]->ProxyPt            += Cell->Data.Item[j]->ProxyPt           ;
			Cell->Data.Item[i]->ProxyPw            += Cell->Data.Item[j]->ProxyPw           ;
			Cell->Data.Item[i]->ProxyN             += Cell->Data.Item[j]->ProxyN            ;
			Cell->Data.Item[i]->ProxySn            += Cell->Data.Item[j]->ProxySn           ;
			Cell->Data.Item[i]->ProxyX             += Cell->Data.Item[j]->ProxyX            ;
			Cell->Data.Item[i]->FT                 += Cell->Data.Item[j]->FT                ;
			Cell->Data.Item[i]->FW                 += Cell->Data.Item[j]->FW                ;
			Cell->Data.Item[i]->FS                 += Cell->Data.Item[j]->FS                ;
			Cell->Data.Item[i]->Weight             += Cell->Data.Item[j]->Weight            ;
			Cell->Data.Item[i]->WeightedValue      += Cell->Data.Item[j]->WeightedValue     ;
			Cell->Data.Item[i]->WeightedShadow     += Cell->Data.Item[j]->WeightedShadow    ;
			Cell->Data.Item[i]->WeightedProxy      += Cell->Data.Item[j]->WeightedProxy     ;
			Cell->Data.Item[i]->SecondryValuePt    += Cell->Data.Item[j]->SecondryValuePt   ;
			Cell->Data.Item[i]->SecondryValuePw    += Cell->Data.Item[j]->SecondryValuePw   ;
			Cell->Data.Item[i]->SecondryProxyPt    += Cell->Data.Item[j]->SecondryProxyPt   ;
			Cell->Data.Item[i]->SecondryProxyPw    += Cell->Data.Item[j]->SecondryProxyPw   ;
			Cell->Data.Item[i]->SecondryFT         += Cell->Data.Item[j]->SecondryFT        ;
			Cell->Data.Item[i]->SecondryFW         += Cell->Data.Item[j]->SecondryFW        ;
			Cell->Data.Item[i]->SecondryFS         += Cell->Data.Item[j]->SecondryFS        ;
			Cell->Data.Item[i]->WeightedValueX     += Cell->Data.Item[j]->WeightedValueX    ;
			Cell->Data.Item[i]->WeightedProxyX     += Cell->Data.Item[j]->WeightedProxyX    ;
			if      (Cell->Data.Item[i]->MixedSignStatus == -1) {
				if      (Cell->Data.Item[j]->MixedSignStatus == -1) {
					Cell->Data.Item[i]->MixedSignStatus = -1;
				}
				else if (Cell->Data.Item[j]->MixedSignStatus ==  0) {
					Cell->Data.Item[i]->MixedSignStatus = -1;
				}
				else if (Cell->Data.Item[j]->MixedSignStatus ==  1) {
					Cell->Data.Item[i]->MixedSignStatus =  2;
				}
				else if (Cell->Data.Item[j]->MixedSignStatus ==  2) {
					Cell->Data.Item[i]->MixedSignStatus =  2;
				}
			}
			else if (Cell->Data.Item[i]->MixedSignStatus ==  0) {
				Cell->Data.Item[i]->MixedSignStatus = Cell->Data.Item[j]->MixedSignStatus;
			}
			else if (Cell->Data.Item[i]->MixedSignStatus ==  1) {
				if      (Cell->Data.Item[j]->MixedSignStatus == -1) {
					Cell->Data.Item[i]->MixedSignStatus =  2;
				}
				else if (Cell->Data.Item[j]->MixedSignStatus ==  0) {
					Cell->Data.Item[i]->MixedSignStatus =  1;
				}
				else if (Cell->Data.Item[j]->MixedSignStatus ==  1) {
					Cell->Data.Item[i]->MixedSignStatus =  1;
				}
				else if (Cell->Data.Item[j]->MixedSignStatus ==  2) {
					Cell->Data.Item[i]->MixedSignStatus =  2;
				}
			}
			else if (Cell->Data.Item[i]->MixedSignStatus ==  2) {
				/*-Cell->Data.Item[i]->MixedSignStatus unchanged no change no matter what "Cell->Data.Item[j]->MixedSignStatus" is */
			}
			Cell->Data.Item[i]->waiver_flag = ((Cell->Data.Item[i]->waiver_flag == 1) &&
			                                   (Cell->Data.Item[j]->waiver_flag == 1)
			                                  )?1:0;
			if (Cell->IsInternal) STC_FreeMemory (Cell->Data.Item[j]->Key);
			STC_FreeMemory (Cell->Data.Item[j]);
			j++;
		}
		if (j < Cell->Data.NumberEntries) {
			//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "moving j to i -- i=%d, j=%d\n", i, j);
			Cell->Data.Item[++i] = Cell->Data.Item[j];
		}
		j++;
	}
	Cell->Data.NumberEntries = i+1;
}
/*------------------------------------------------------------------------------
Sort the items by value
once all item are added to the cell
------------------------------------------------------------------------------*/
void SortKeys (
	STCT_CELL * Cell)
{
#ifdef NEVERTOBEDEFINED
	int cmp;
	int gap;
	int i;
	int j;
	STCT_DATAITEM * TempItem;
	for (gap = Cell->Data.NumberEntries/2; gap > 0; gap /= 2) {
		for (i = gap; i < Cell->Data.NumberEntries; i++) {
			for (j = i-gap; j >= 0; j -= gap) {
				cmp = strcmp (Cell->Data.Item[j]->Key, Cell->Data.Item[j+gap]->Key);
				if (cmp <= 0)
					break;
				/* move the items */
				TempItem               = Cell->Data.Item[j];
				Cell->Data.Item[j]     = Cell->Data.Item[j+gap];
				Cell->Data.Item[j+gap] = TempItem;
			}
		}
	}
#endif
	qsort ((void*)&Cell->Data.Item[0], (size_t)Cell->Data.NumberEntries, sizeof (STCT_DATAITEM **), SortKeysCompare);
#ifdef _DEBUG
	//check if sort worked
	//CheckSortKeys (Cell);//developpement seulement
#endif
}
/*------------------------------------------------------------------------------
compare 2 keys. see SortKeys()
------------------------------------------------------------------------------*/
static int SortKeysCompare (
	const void * ppi1, //pointer to pointer to STCT_DATAITEM
	const void * ppi2)
{
	char * k1 = (*(STCT_DATAITEM **)ppi1)->Key;
	char * k2 = (*(STCT_DATAITEM **)ppi2)->Key;
	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "k1=%s, k2=%s. %d\n", k1, k2, strcmp (k1, k2));
	return strcmp (k1, k2);
}
