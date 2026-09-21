#include <stdlib.h>
#include <stdio.h>

#include "EI_Message.h"
#include "STC_Cell.h"
#include "STC_Write.h"
#include "MessageGConfidAPI.h"

#include "ilist.h"
#include "slist.h"

static EIT_RETURNCODE DefaultWriteCellObs (STCT_CELL * Cell, int NumberDimensions);
static EIT_RETURNCODE DefaultWriteConstraintObs (int ConstraintId, int CellId, int Coefficient);
static EIT_RETURNCODE DefaultWriteLargestObs (int CellId, char * Key, int NumberObservations,
	double Value, double TotalValue, double Shadow, double TotalShadow, int WaiverFlag);
static EIT_RETURNCODE DefaultWriteTargetsObs (int CellId, char * Id, char * PtnVariable, double Value);
static EIT_RETURNCODE DefaultWritePairsObs (int CellId, char * TargetId, double TargetPt,
	char * AttackerId, double AttackerSn, int RemainderCount, double RemainderN);


static STCT_WRITECELLOBSCALLBACK mWriteCellObs = DefaultWriteCellObs;
static STCT_WRITECONSTRAINTOBSCALLBACK mWriteConstraintObs = DefaultWriteConstraintObs;
static STCT_WRITELARGESTOBSCALLBACK mWriteLargestObs = DefaultWriteLargestObs;
static STCT_WRITETARGETSOBSCALLBACK mWriteTargetsObs = DefaultWriteTargetsObs;
static STCT_WRITEPAIRSOBSCALLBACK mWritePairsObs = DefaultWritePairsObs;


/*------------------------------------------------------------------------------
write the cell info. this function should be overwritten by the user of the API.
use STC_WriteCellSetCB() to overwrite.
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_WriteCell (
	STCT_CELL * Cell,
	int NumberDimensions)
{
    EIT_RETURNCODE rc = EIE_FAIL;

	/* if (Cell->TotalValue == 0.0) */
	if (SELECT_CE_VAL2(Cell, Cell->WhichNoise) == 0.0)
		return EIE_SUCCEED;

	STC_CellNextCellId (Cell);

    rc = mWriteCellObs(Cell, NumberDimensions);
    if (rc != EIE_SUCCEED) return EIE_FAIL;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
function to set the callback function to write a cell
------------------------------------------------------------------------------*/
void STC_WriteCellObsSetCB (
	STCT_WRITECELLOBSCALLBACK f)
{
	mWriteCellObs = f;
}
/*------------------------------------------------------------------------------
write the constraint info. this function should be overwritten by the user of the API.
use STC_WriteConstraintSetCB() to overwrite.
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_WriteConstraint (
	STCT_CELLSET * CellSet,
	STCT_CELL * RHSCell)
{
	int ConstraintId;
	int i;
    EIT_RETURNCODE rc = EIE_FAIL;

	/* if (RHSCell->TotalValue == 0.0) */
	if (SELECT_CE_VAL2(RHSCell, RHSCell->WhichNoise) == 0.0)
		return EIE_SUCCEED;

	ConstraintId = STC_CellSetNextConstraintId ();

	for (i = 0; i < CellSet->NumberEntries; i++) {
		/* if (CellSet->Cell[i]->TotalValue != 0.0) {} */
		if (SELECT_CE_VAL2(CellSet->Cell[i], CellSet->Cell[i]->WhichNoise) != 0.0) {

			STC_CellNextCellId (CellSet->Cell[i]);

            rc = mWriteConstraintObs (ConstraintId, CellSet->Cell[i]->CellId, 1);
            if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
	}

	STC_CellNextCellId (RHSCell);

    rc = mWriteConstraintObs(ConstraintId, RHSCell->CellId, -1);
    if (rc != EIE_SUCCEED) return EIE_FAIL;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
function to set the callback function to write a constraint
------------------------------------------------------------------------------*/
void STC_WriteConstraintObsSetCB (
	STCT_WRITECONSTRAINTOBSCALLBACK f)
{
	mWriteConstraintObs = f;
}
/*------------------------------------------------------------------------------
write the largest cell contributors info. this function should be overwritten by the user of the API.
use STC_WriteLargestSetCB() to overwrite.
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_WriteLargest (
	STCT_CELL * Cell,
	int waiver_flags_present,
	PROGRAM_PARM * Parms)
{
	int i;
	int PoolNumberObservations;
	double PoolShadow;
	double PoolValue;
	int NumOfContributorsWithWaivers;
	int NumOfContributors;
	int NumOfLargestContribsWithWaivers;
	int NumOfLargestContribs;
	int LargestWaiverFlag;
	int PoolWaiverFlag;
    EIT_RETURNCODE rc = EIE_FAIL;

	/* if (Cell->TotalValue == 0.0) */
	if (SELECT_CE_VAL2(Cell, Cell->WhichNoise) == 0.0)
		return EIE_SUCCEED;

	PoolNumberObservations = Cell->TotalNumberObservations - Cell->AnonymousDataItem.NumberObservations;
	PoolValue  = ((Parms->Weight != 0)?(Cell->TotalWeightedValue              ):(Cell->TotalValue              )) -
	             ((Parms->Weight != 0)?(Cell->AnonymousDataItem.WeightedValue ):(Cell->AnonymousDataItem.Value ));
	PoolShadow = ((Parms->Weight != 0)?(Cell->TotalWeightedShadow             ):(Cell->TotalShadow             )) -
	             ((Parms->Weight != 0)?(Cell->AnonymousDataItem.WeightedShadow):(Cell->AnonymousDataItem.Shadow));
	NumOfContributors = Cell->Data.NumberEntries;
	NumOfLargestContribs = Cell->LargestNumberEntries;
	NumOfContributorsWithWaivers = 0;
	if (waiver_flags_present == 2 || waiver_flags_present == 1) {
		for (i = 0; i < Cell->Data.NumberEntries; i++) {
			NumOfContributorsWithWaivers = NumOfContributorsWithWaivers + Cell->Data.Item[i]->waiver_flag;
		}
	}

	NumOfLargestContribsWithWaivers = 0;
	for (i = 0; i < Cell->LargestNumberEntries; i++) {
		if (waiver_flags_present == 2 || waiver_flags_present == 1) {
			LargestWaiverFlag = Cell->Largest[i]->waiver_flag;
		}
		else {
			LargestWaiverFlag = 0;
		}
		rc = mWriteLargestObs (Cell->CellId, Cell->Largest[i]->Key,
			Cell->Largest[i]->NumberObservations,
			((Parms->Weight != 0)?(Cell->TotalWeightedValue        ):(Cell->TotalValue        )),
			((Parms->Weight != 0)?(Cell->Largest[i]->WeightedValue ):(Cell->Largest[i]->Value )),
			((Parms->Weight != 0)?(Cell->TotalWeightedShadow       ):(Cell->TotalShadow       )),
			((Parms->Weight != 0)?(Cell->Largest[i]->WeightedShadow):(Cell->Largest[i]->Shadow)), LargestWaiverFlag);
        if (rc != EIE_SUCCEED) return EIE_FAIL;

		PoolNumberObservations -= Cell->Largest[i]->NumberObservations;
		PoolShadow -= ((Parms->Weight != 0)?(Cell->Largest[i]->WeightedShadow):(Cell->Largest[i]->Shadow));
		PoolValue  -= ((Parms->Weight != 0)?(Cell->Largest[i]->WeightedValue ):(Cell->Largest[i]->Value ));
		NumOfLargestContribsWithWaivers = NumOfLargestContribsWithWaivers + LargestWaiverFlag;
	}
	if (waiver_flags_present != 0) {
		if ((NumOfContributorsWithWaivers - NumOfLargestContribsWithWaivers) ==
		    (NumOfContributors            - NumOfLargestContribs           )
		   ) {
			PoolWaiverFlag = 1;
		}
		else {
			PoolWaiverFlag = 0;
		}
	}
	else {
		PoolWaiverFlag = 0;
	}

	rc = mWriteLargestObs (Cell->CellId, "pool", PoolNumberObservations,
		((Parms->Weight != 0)?(Cell->TotalWeightedValue              ):(Cell->TotalValue)),
		PoolValue,
		((Parms->Weight != 0)?(Cell->TotalWeightedShadow             ):(Cell->TotalShadow)),
		PoolShadow, PoolWaiverFlag);
    if (rc != EIE_SUCCEED) return EIE_FAIL;

	rc = mWriteLargestObs (Cell->CellId, "anon", Cell->AnonymousDataItem.NumberObservations,
		((Parms->Weight != 0)?(Cell->TotalWeightedValue              ):(Cell->TotalValue              )),
		((Parms->Weight != 0)?(Cell->AnonymousDataItem.WeightedValue ):(Cell->AnonymousDataItem.Value )),
		((Parms->Weight != 0)?(Cell->TotalWeightedShadow             ):(Cell->TotalShadow             )),
		((Parms->Weight != 0)?(Cell->AnonymousDataItem.WeightedShadow):(Cell->AnonymousDataItem.Shadow)), 0);
    if (rc != EIE_SUCCEED) return EIE_FAIL;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
function to set the callback function to write a the largest respondents in a cell
------------------------------------------------------------------------------*/
void STC_WriteLargestObsSetCB (
	STCT_WRITELARGESTOBSCALLBACK f)
{
	mWriteLargestObs = f;
}

/*------------------------------------------------------------------------------
write the targets cell info. this function should be overwritten by the user of the API.
use STC_WriteLargestSetCB() to overwrite.
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_WriteTargets (
    STCT_CELL     * Cell,
    int             Lrgst12FTNumberEntries,
    STCT_DATAITEM** Lrgst12FT,
    int             Lrgst12FSNumberEntries,
    STCT_DATAITEM** Lrgst12FS,
    int             SRuleNumberEntries01,
    int             UsePrimary,
    int             SRule_waiver_flags_present)
{
    int i;
    double CumulativeN;
    int ProxySpecified;
    int NumberOfEntriesToOutput;
    EIT_RETURNCODE rc = EIE_FAIL;

    /* if (Cell->TotalValue == 0.0) { } */
    if (SELECT_CE_VAL2(Cell, Cell->WhichNoise) == 0.0) {
        return EIE_SUCCEED;
    }
    
    if      (Cell->WhichNoise == 'n') {
        ProxySpecified = 0;
    }
    else if (Cell->WhichNoise == 'N') {
        ProxySpecified = 1;
    }
    else {
        IO_PRINT_LINE(M30205);
        return EIE_FAIL;
    }
    
    NumberOfEntriesToOutput = (Lrgst12FTNumberEntries<=SRuleNumberEntries01)?Lrgst12FTNumberEntries:SRuleNumberEntries01;
    CumulativeN = 0.0;
    for (i=0;i<NumberOfEntriesToOutput;i=i+1) {
        if (SRule_waiver_flags_present == 0) {
            rc = mWriteTargetsObs (Cell->CellId, Lrgst12FT[i]->Key, "PT", ProxySpecified?(UsePrimary?(Lrgst12FT[i]->ProxyPt):(Lrgst12FT[i]->SecondryProxyPt))
                                                                                   :(UsePrimary?(Lrgst12FT[i]->ValuePt):(Lrgst12FT[i]->SecondryValuePt))
                             );
        }
        else {
            rc = mWriteTargetsObs (Cell->CellId, Lrgst12FT[i]->Key, "PT", ProxySpecified?(UsePrimary?(Lrgst12FT[i]->ProxyPw):(Lrgst12FT[i]->SecondryProxyPw))
                                                                                   :(UsePrimary?(Lrgst12FT[i]->ValuePw):(Lrgst12FT[i]->SecondryValuePw))
                             );
        }
        if (rc != EIE_SUCCEED) return EIE_FAIL;

        CumulativeN = CumulativeN + (ProxySpecified?Lrgst12FT[i]->ProxyN:Lrgst12FT[i]->ValueN);
    }
    rc = mWriteTargetsObs (Cell->CellId, "Remainder"            , "N" , ProxySpecified?((Cell->TotalProxyN)-CumulativeN):((Cell->TotalValueN)-CumulativeN));
    if (rc != EIE_SUCCEED) return EIE_FAIL;
    
    return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
function to set the callback function to write a the targets in a cell
------------------------------------------------------------------------------*/
void STC_WriteTargetsObsSetCB (
	STCT_WRITETARGETSOBSCALLBACK f)
{
	mWriteTargetsObs = f;
}

/*------------------------------------------------------------------------------
write the pairs cell info. this function should be overwritten by the user of the API.
use STC_WriteLargestSetCB() to overwrite.
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_WritePairs (
    STCT_CELL     * Cell,
    int             Lrgst12FTNumberEntries,
    STCT_DATAITEM** Lrgst12FT,
    int             Lrgst12FSNumberEntries,
    STCT_DATAITEM** Lrgst12FS,
    int             UsePrimary,
    int             SRule_waiver_flags_present)
{
    int    TargetIndex;
    int    AttackerIndex;
    int    TotalCount;
    int    CumulativeCount;
    double CumulativeN;
    int    ProxySpecified;
    
    int    CellId;
    char * TargetId;
    double TargetPt;
    char * AttackerId;
    double AttackerSn;
    int    RemainderCount;
    double RemainderN;
    EIT_RETURNCODE rc = EIE_FAIL;
    
    /* if (Cell->TotalValue == 0.0) { } */
    if (SELECT_CE_VAL2(Cell, Cell->WhichNoise) == 0.0) {
        return EIE_SUCCEED;
    }
    
    if      (Cell->WhichNoise == 'n') {
        ProxySpecified = 0;
    }
    else if (Cell->WhichNoise == 'N') {
        ProxySpecified = 1;
    }
    
    else {
        IO_PRINT_LINE("error: value of Cell->WhichNoise is neither 'n' nor 'N' in call to STC_Writepairs()");
        return EIE_FAIL;
    }
    
    /* if      (Cell->Data.NumberEntries == 0 && Cell->AnonymousDataItem.NumberObservations != 0) { */
    /*     TargetIndex      =  0;                                                                   */
    /*     AttackerIndex    = -1;                                                                   */
    /*     TotalCount       = Lrgst12FTNumberEntries;                                         */
    /*     CumulativeCount  = 1;                                                                    */
    /*     CumulativeN      = ProxySpecified?Lrgst12FT[TargetIndex]->ProxyN                   */
    /*                                      :Lrgst12FT[TargetIndex]->ValueN;                  */
    /* }                                                                                            */
    if      ((Cell->Data.NumberEntries == 1 && Cell->AnonymousDataItem.NumberObservations == 0) ||
             (Cell->Data.NumberEntries == 0 && Cell->AnonymousDataItem.NumberObservations != 0)
            ) {
        TargetIndex      = 0;
        AttackerIndex    = -1;
        TotalCount       = Lrgst12FTNumberEntries;
        CumulativeCount  = 1;
        CumulativeN      = ProxySpecified?Lrgst12FT[TargetIndex]->ProxyN
                                         :Lrgst12FT[TargetIndex]->ValueN;
    }
    else if (strcmp(Lrgst12FT[0]->Key, Lrgst12FS[0]->Key) != 0){
        TargetIndex      = 0;
        AttackerIndex    = 0;
        TotalCount       = Lrgst12FTNumberEntries;
        CumulativeCount  = 2;
        CumulativeN      = ProxySpecified?(Lrgst12FT[TargetIndex]->ProxyN+Lrgst12FS[AttackerIndex]->ProxyN)
                                         :(Lrgst12FT[TargetIndex]->ValueN+Lrgst12FS[AttackerIndex]->ValueN);
    }
    else { /* ((Lrgst12FTNumberEntries > 1) &&                                 */
           /*  (strcmp(Lrgst12FT[0]->Key, Lrgst12FS[0]->Key) == 0)             */
           /* )                                                                */
        
        if (((((SRule_waiver_flags_present == 0)?(Lrgst12FT[0]->FT):(Lrgst12FT[0]->FW))+Lrgst12FS[1]->FS-(ProxySpecified?Cell->TotalProxyN:Cell->TotalValueN)) >
             (((SRule_waiver_flags_present == 0)?(Lrgst12FT[1]->FT):(Lrgst12FT[1]->FW))+Lrgst12FS[0]->FS-(ProxySpecified?Cell->TotalProxyN:Cell->TotalValueN))
            ) || 
            ((((SRule_waiver_flags_present == 0)?(Lrgst12FT[0]->FT):(Lrgst12FT[0]->FW))+Lrgst12FS[1]->FS-(ProxySpecified?Cell->TotalProxyN:Cell->TotalValueN)) ==
             (((SRule_waiver_flags_present == 0)?(Lrgst12FT[1]->FT):(Lrgst12FT[1]->FW))+Lrgst12FS[0]->FS-(ProxySpecified?Cell->TotalProxyN:Cell->TotalValueN))
             &&
             (strcmp(Lrgst12FT[0]->Key, Lrgst12FT[1]->Key) > 0)
            )
           ) {
            TargetIndex     = 0;
            AttackerIndex   = 1;
            TotalCount      = Lrgst12FTNumberEntries;
            CumulativeCount = 2;
            CumulativeN     = ProxySpecified?(Lrgst12FT[TargetIndex]->ProxyN+Lrgst12FS[AttackerIndex]->ProxyN)
                                            :(Lrgst12FT[TargetIndex]->ValueN+Lrgst12FS[AttackerIndex]->ValueN);
        }
        else {
            TargetIndex     = 1;
            AttackerIndex   = 0;
            TotalCount      = Lrgst12FTNumberEntries;
            CumulativeCount = 2;
            CumulativeN     = ProxySpecified?(Lrgst12FT[TargetIndex]->ProxyN+Lrgst12FS[AttackerIndex]->ProxyN)
                                            :(Lrgst12FT[TargetIndex]->ValueN+Lrgst12FS[AttackerIndex]->ValueN);
        }
    }
    
    /* mWritePairsObs (                       Cell->CellId                                                                                                                                 , */
    /*                 ((TargetIndex   != -1)?(Lrgst12FT[TargetIndex  ]->Key    ):"" )                                                                                                     , */
    /*                 ((TargetIndex   != -1)?(UsePrimary?((SRule_waiver_flags_present == 0)?(Lrgst12FT[TargetIndex  ]->ValuePt        ):(Lrgst12FT[TargetIndex  ]->ValuePw        ))        */
    /*                                                   :((SRule_waiver_flags_present == 0)?(Lrgst12FT[TargetIndex  ]->SecondryValuePt):(Lrgst12FT[TargetIndex  ]->SecondryValuePw))):0.0), */
    /*                 ((TargetIndex   != -1)?(UsePrimary?((SRule_waiver_flags_present == 0)?(Lrgst12FT[TargetIndex  ]->ProxyPt        ):(Lrgst12FT[TargetIndex  ]->ProxyPw        ))        */
    /*                                                   :((SRule_waiver_flags_present == 0)?(Lrgst12FT[TargetIndex  ]->SecondryProxyPt):(Lrgst12FT[TargetIndex  ]->SecondryProxyPw))):0.0), */
    /*                 ((AttackerIndex != -1)?(Lrgst12FS[AttackerIndex]->Key    ):""  )                                                                                                    , */
    /*                 ((AttackerIndex != -1)?(Lrgst12FS[AttackerIndex]->ValueSn):0.0 )                                                                                                    , */
    /*                 ((AttackerIndex != -1)?(Lrgst12FS[AttackerIndex]->ProxySn):0.0 )                                                                                                    , */
    /*                                        (TotalCount          - CumulativeCount  )                                                                                                    , */
    /*                                        ((Cell->TotalValueN) - CumulativeValueN )                                                                                                    , */
    /*                                        ((Cell->TotalValueN) - CumulativeProxyN )                                                                                                      */
    /* );                                                                                                                                                                                    */
    
    /*-from:                                                     */
    /*     int    TargetIndex                                    */
    /*     int    AttackerIndex                                  */
    /*     int    TotalCount                                     */
    /*     int    CumulativeCount                                */
    /*     double CumulativeN                                    */
    /* generate the arguments to the call to "mWritePairsObs()": */
    /*     int    CellId                                         */
    /*     char * TargetId                                       */
    /*     double TargetPt                                       */
    /*     char * AttackerId                                     */
    /*     double AttackerSn                                     */
    /*     int    RemainderCount                                 */
    /*     double RemainderN                                     */
    /* and then pass them to the call to "mWritePairsObs()":     */
    CellId         = Cell->CellId;
    if (TargetIndex   != -1) {
        TargetId   =                 Lrgst12FT[TargetIndex  ]->Key     ;
        if (SRule_waiver_flags_present == 0) {
            TargetPt   = ProxySpecified?(UsePrimary?(Lrgst12FT[TargetIndex  ]->ProxyPt):(Lrgst12FT[TargetIndex  ]->SecondryProxyPt))
                                       :(UsePrimary?(Lrgst12FT[TargetIndex  ]->ValuePt):(Lrgst12FT[TargetIndex  ]->SecondryValuePt));
        }
        else {
            TargetPt   = ProxySpecified?(UsePrimary?(Lrgst12FT[TargetIndex  ]->ProxyPw):(Lrgst12FT[TargetIndex  ]->SecondryProxyPw))
                                       :(UsePrimary?(Lrgst12FT[TargetIndex  ]->ValuePw):(Lrgst12FT[TargetIndex  ]->SecondryValuePw));
        }
    }
    if (AttackerIndex != -1) {
        AttackerId =                 Lrgst12FS[AttackerIndex]->Key     ;
        AttackerSn = ProxySpecified?(Lrgst12FS[AttackerIndex]->ProxySn)
                                   :(Lrgst12FS[AttackerIndex]->ValueSn);
    }
    RemainderCount = TotalCount                                           - CumulativeCount;
    RemainderN     = (ProxySpecified?Cell->TotalProxyN:Cell->TotalValueN) - CumulativeN    ;
    rc = mWritePairsObs (                      CellId            ,
                    (TargetIndex   != -1)?TargetId      :"" ,
                    (TargetIndex   != -1)?TargetPt      :0.0,
                    (AttackerIndex != -1)?AttackerId    :"" ,
                    (AttackerIndex != -1)?AttackerSn    :0.0,
                                          RemainderCount    ,
                                          RemainderN
                   );
    if (rc != EIE_SUCCEED) return EIE_FAIL;
    
    return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
function to set the callback function to write a the pairs in a cell
------------------------------------------------------------------------------*/
void STC_WritePairsObsSetCB (
	STCT_WRITEPAIRSOBSCALLBACK f)
{
	mWritePairsObs = f;
}


/*------------------------------------------------------------------------------
write the cell info. this function should be overwritten by the user of the API.
use STC_WriteCellSetCB() to overwrite.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE DefaultWriteCellObs (
	STCT_CELL * Cell,
	int NumberDimensions)
{
#define CELLID_LENGTH 10
#define SHADOW_LENGTH 12
#define SHADOW_DECIMAL 2
#define TOTAL_LENGTH 12
#define TOTAL_DECIMAL 2
#define SENSITIVITY_LENGTH 12
#define SENSITIVITY_DECIMAL 2
#define STATUS_LENGTH 10
#define TYPE_LENGTH 10
#define TAG_LENGTH 10
#define NUMBERENTRIES_LENGTH 10

#define WRITECELL_FORMAT "C%*d %*d %*.*f %*d %*.*f %*.*f %*.*f %*c %*c"

#define WRITETAG_FORMAT " %*d"

#define WRITETAG(Tag) \
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, WRITETAG_FORMAT, TAG_LENGTH, Tag);

	int i;

	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY,
		WRITECELL_FORMAT,
		CELLID_LENGTH, Cell->CellId,
		NUMBERENTRIES_LENGTH, Cell->AnonymousDataItem.NumberObservations,
		TOTAL_LENGTH, TOTAL_DECIMAL, Cell->AnonymousDataItem.Value,
		NUMBERENTRIES_LENGTH, Cell->Data.NumberEntries,
		SHADOW_LENGTH, SHADOW_DECIMAL, Cell->TotalShadow,
		TOTAL_LENGTH, TOTAL_DECIMAL, Cell->TotalValue,
		SENSITIVITY_LENGTH, SENSITIVITY_DECIMAL, Cell->Sensitivity,
		STATUS_LENGTH, Cell->Sensitivity > 0 ? STC_CELLSTATUS_SENSITIVE : STC_CELLSTATUS_NOTSENSITIVE,
		TYPE_LENGTH, Cell->Type);

	if (Cell->Coordinate == NULL) { /* pour le cas d'aggr sensible */
		for (i = 0; i < NumberDimensions; i++)
			WRITETAG (-1);
	}
	else {
		for (i = 0; i < Cell->Coordinate->NumberEntries; i++)
			WRITETAG (Cell->Coordinate->Tag[i]);
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
write the constraint info. this function should be overwritten by the user of the API.
use STC_WriteConstraintSetCB() to overwrite.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE DefaultWriteConstraintObs (
	int ConstraintId,
	int CellId,
	int Coefficient)
{
#define CONSTRAINTID_LENGTH 10
#define CELLID_LENGTH 10
#define COEFFICIENT_LENGTH 10

#define WRITECONSTRAINTOBS_FORMAT "A%*d %*d %*d\n"

#define WRITECONSTRAINTOBS(ConstraintId, CellId, Coefficient) \
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, \
		WRITECONSTRAINTOBS_FORMAT, \
		CONSTRAINTID_LENGTH, ConstraintId, \
		CELLID_LENGTH, CellId, \
		COEFFICIENT_LENGTH, Coefficient)

	WRITECONSTRAINTOBS (ConstraintId, CellId, Coefficient);

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
write the largest cell contributors info. this function should be overwritten by the user of the API.
use STC_WriteLargestSetCB() to overwrite.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE DefaultWriteLargestObs (
	int CellId,
	char * Key,
	int NumberObservations,
	double TotalValue,
	double Value,
	double TotalShadow,
	double Shadow,
	int WaiverFlag)
{
#define CELLID_LENGTH 10
#define NUMBEROBSERVATIONS_LENGTH 10
#define KEY_LENGTH 10
#define VALUE_LENGTH 12
#define VALUE_DECIMAL 2
#define PERCENT_LENGTH 6
#define PERCENT_DECIMAL 2
#define SHADOW_LENGTH 12
#define SHADOW_DECIMAL 2
#define WAIVERFLAG_LENGTH 1

#define WRITELARGESTOBS_FORMAT "L%*d %*s %*d %*.*f %*.*f %*.*f %*.*f %*d\n"

#define WRITELARGESTOBS(CellId, Key, NumberObservations, TotalValue, Value, TotalShadow, Shadow, WaiverFlag) \
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, \
		WRITELARGESTOBS_FORMAT, \
		CELLID_LENGTH, CellId, \
		KEY_LENGTH, Key, \
		NUMBEROBSERVATIONS_LENGTH, NumberObservations, \
		PERCENT_LENGTH, PERCENT_DECIMAL, TotalValue == 0.0 ? 100.0 : Value*100.0/TotalValue, \
		VALUE_LENGTH, VALUE_DECIMAL, Value, \
		PERCENT_LENGTH, PERCENT_DECIMAL, TotalShadow == 0.0 ? 100.0 : Shadow*100.0/TotalShadow, \
		SHADOW_LENGTH, SHADOW_DECIMAL, Shadow, \
		WAIVERFLAG_LENGTH, WaiverFlag)

	WRITELARGESTOBS (CellId, Key, NumberObservations, TotalValue, Value, TotalShadow, Shadow, WaiverFlag);

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
write the targets cell info. this function should be overwritten by the user of the API.
use STC_WriteTargetsSetCB() to overwrite.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE DefaultWriteTargetsObs (
	int CellId,
	char * Id,
	char * PtnVariable,
	double Value)
{
#define CELLID_LENGTH 10
#define ID_LENGTH 10
#define PTNVARIABLE_LENGTH 2
#define VALUE2_LENGTH 10

#define WRITETARGETSOBS_FORMAT "L%*d %*s %*s %*d\n"

#define WRITETARGETSOBS(CellId, Id, PtnVariable, Value) \
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, \
		WRITETARGETSOBS_FORMAT, \
		CELLID_LENGTH, CellId, \
		ID_LENGTH, Id, \
		PTNVARIABLE_LENGTH, PtnVariable, \
		VALUE2_LENGTH, Value)

	WRITETARGETSOBS (CellId, Id, PtnVariable, Value);

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
write the pairs cell info. this function should be overwritten by the user of the API.
use STC_WritePairsSetCB() to overwrite.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE DefaultWritePairsObs (
	int CellId,
	char * TargetId,
	double TargetPt,
	char * AttackerId,
	double AttackerSn,
	int RemainderCount,
	double RemainderN)
{
#define CELLID_LENGTH 10
#define TARGETID_LENGTH 10
#define TARGETPT_LENGTH 10
#define ATTACKERID_LENGTH 10
#define ATTACKERSN_LENGTH 10
#define REMAINDERCOUNT_LENGTH 10
#define REMAINDERN_LENGTH 10

#define WRITEPAIRSOBS_FORMAT "L%*d %*s %*d %*s %*d %*d %*d\n"

#define WRITEPAIRSOBS(CellId, TargetId, TargetPt, AttackerId, AttackerSn, RemainderCount, RemainderN) \
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, \
		WRITEPAIRSOBS_FORMAT, \
		CELLID_LENGTH, CellId, \
		TARGETID_LENGTH, TargetId, \
		TARGETPT_LENGTH, TargetPt, \
		ATTACKERID_LENGTH, AttackerId, \
		ATTACKERSN_LENGTH, AttackerSn, \
		REMAINDERCOUNT_LENGTH, RemainderCount, \
		REMAINDERN_LENGTH, RemainderN)

	WRITEPAIRSOBS (CellId, TargetId, TargetPt, AttackerId, AttackerSn, RemainderCount, RemainderN);

	return EIE_SUCCEED;
}
