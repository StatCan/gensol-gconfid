/*********************************************************************
Copyright (C) 2008 by Statistics Canada
Implements confids build program in the form of a SAS Procedure.
The details of the method are available in
'Documentation for the module Build', by
Jean-Louis Tambay, Statistics Canada.
Ioana Schiopu-Kratina, Statistics Canada.
Jean-Marc Fillion, Statistics Canada.
*********************************************************************/

#ifndef SI_SYSTEMNAME
#error "Please define macro SI_SYSTEMNAME."
#endif
#ifndef SI_SYSTEMVERSION
#error "Please define macro SI_SYSTEMVERSION."
#endif
#ifndef SI_PROCVERSION
#error "Please define macro SI_PROCVERSION."
#endif
#ifndef SI_EMAIL
#error "Please define macro SI_EMAIL."
#endif

 /* standard libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "sensitiv_JIO.h"

/* StatCan Libraries */
#include "EI_Message.h"
#include "STC_Build.h"
#include "STC_Cell.h"
#include "STC_GConfid.h"
#include "STC_Coordinate.h"
#include "STC_Group.h"
#include "STC_Hierarchy.h"
#include "STC_Memory.h"
#include "STC_Range.h"
#include "STC_SRule.h"
#include "STC_Write.h"
#include "SDataSet.h"
#include "ilist.h"
#include "slist.h"
#include "util.h"

#define MAX(x,y)   (int)y > (int)x ? (int)y : (int)x

/* SAS grammar (.grm) name */
#define DATA_GRM_NAME              DSN_INDATA
#define OUTCONSTRAINT_GRM_NAME     DSN_OUT_CONSTRAINT
#define OUTCELL_GRM_NAME           DSN_OUT_CELL
#define OUTLARGEST_GRM_NAME        DSN_OUT_LARGEST
#define OUTTARGETS_GRM_NAME        DSN_OUT_TARGETS
#define OUTPAIRS_GRM_NAME          DSN_OUT_PAIRS
#define SRULE_GRM_NAME             GPN_S_RULE
#define HIERARCHY_GRM_NAME         GPN_HIERARCHY
#define RANGE_GRM_NAME             GPN_RANGE
#define GROUP_GRM_NAME             "GROUP"  // no longer exposed to user, nor removed from internal code...
#define M_GRM_NAME                 GPN_M
#define X_GRM_NAME                 GPN_X
#define Y_GRM_NAME                 GPN_Y
#define Z_GRM_NAME                 GPN_Z
#define ID_GRM_NAME                GPN_UNIT_ID
#define VAR_GRM_NAME               GPN_VAR
#define SHADOW_GRM_NAME            GPN_SHADOW
#define DIMENSION_GRM_NAME         GPN_DIMENSION
#define TOLERANCE_GRM_NAME         GPN_TOLERANCE
#define MINRESP_GRM_NAME           GPN_MIN_RESP
#define WAIVER_GRM_NAME            GPN_WAIVER
#define PWAIVER_GRM_NAME           GPN_P_WAIVER
#define PROXYSIZE_GRM_NAME         GPN_PROXY_SIZE
#define WEIGHT_GRM_NAME            GPN_WEIGHT
#define PROXYRATIO_GRM_NAME        GPN_PROXY_RATIO
#define PROXYPERCENTILE_GRM_NAME   GPN_PROXY_PERCENTILE
#define WEIGHTPROTLEVEL_GRM_NAME   GPN_WEIGHT_PROT_LEVEL
#define MINRESPW_GRM_NAME          GPN_MIN_RESP_W
#define VERBOSE_GRM_NAME           GPN_VERBOSE
#define TIMER_GRM_NAME             GPN_TIMER
#define PRINTCODES_GRM_NAME        GPN_PRINT_CODES
#define LIMITWARNINGS_GRM_NAME     GPN_LIMIT_WARNINGS
#define ADDITIVENOISE_GRM_NAME     GPN_ADDITIVE_NOISE
#define PROXYDIAG_GRM_NAME         GPN_PROXY_DIAG
#define WEIGHTDIAG_GRM_NAME        GPN_WEIGHT_DIAG
#define ACCEPTNEGATIVE_GRM_NAME    GPN_ACCEPT_NEGATIVE
#define DBGFILEPREFIX_GRM_NAME     GPN_DEBUG_FILE_PREFIX   //NOTE: this debugging code removed in may 2022
#define DBGWORKDIRPATH_GRM_NAME    GPN_DEBUG_WORK_DIR_PATH  //NOTE: Before removing these, remove associated grammar (.grm) code

/* related to input DATA data set */
#define DATA_NBVARS_MANDATORY      2 //ID and VAR are mandatory
// Note that  the number of DIMENSION variables isn't known until run time
#define DATA_NBVARS_OPTIONAL       4//SHADOW, WAIVER, PROXYSIZE, and WEIGHT are optional

#define DATA_ID_INDEX              0
#define DATA_VAR_INDEX             1
#define DATA_SHADOW_INDEX          2
#define DATA_WAIVER_INDEX          3
#define DATA_PROXY_INDEX           4
#define DATA_WEIGHT_INDEX          5
#define DATA_DIMENSION_INDEX       6

#define DATA_ID_TYPE               SUTIL_VARIABLE_TYPE_CHARACTER
#define DATA_VAR_TYPE              SUTIL_VARIABLE_TYPE_NUMERIC
#define DATA_SHADOW_TYPE           SUTIL_VARIABLE_TYPE_NUMERIC
#define DATA_WAIVER_TYPE           SUTIL_VARIABLE_TYPE_NUMERIC
#define DATA_PROXY_TYPE            SUTIL_VARIABLE_TYPE_NUMERIC
#define DATA_WEIGHT_TYPE           SUTIL_VARIABLE_TYPE_NUMERIC
#define DATA_DIMENSION_TYPE        SUTIL_VARIABLE_TYPE_CHARACTER

/* related to output CONSTRAINT data set */
#define CONSTRAINT_NBVARS              3

#define CONSTRAINT_CONSTRAINTID_NAME   "ConstraintId"
#define CONSTRAINT_CELLID_NAME         "CellId"
#define CONSTRAINT_COEFFICIENT_NAME    "Coefficient"

#define CONSTRAINT_CONSTRAINTID_INDEX  0
#define CONSTRAINT_CELLID_INDEX        1
#define CONSTRAINT_COEFFICIENT_INDEX   2

#define CONSTRAINT_CONSTRAINTID_TYPE   SUTIL_VARIABLE_TYPE_NUMERIC
#define CONSTRAINT_CELLID_TYPE         SUTIL_VARIABLE_TYPE_NUMERIC
#define CONSTRAINT_COEFFICIENT_TYPE    SUTIL_VARIABLE_TYPE_NUMERIC

/* related to output CELL data set */
// The following macro function is used to calculate the # vars on output dataset OutCell
#define CELL_NBVARS(SHADOWTOTAL_INCLUDED, TOTALPROXY_INCLUDED, SENSITIVNWV_INCLUDED, FAVCOST_INCLUDED, NUMBER_OF_DIMENSIONS, WEIGHT_SPECIFIED) (9 + WEIGHT_SPECIFIED + SHADOWTOTAL_INCLUDED + TOTALPROXY_INCLUDED + SENSITIVNWV_INCLUDED + FAVCOST_INCLUDED + NUMBER_OF_DIMENSIONS)

#define CELL_CELLID_NAME                 "CellId"
#define CELL_NBANONYM_NAME               "NbAnonym"
#define CELL_ANONYM_NAME                 "AnonymTotalVar"
#define CELL_NBRESPONDENTS_NAME          "NbRespondents"
#define CELL_WEIGHTEDNBRESPONDENTS_NAME  "WeightedNbRespondents"
#define CELL_TOTAL_NAME                  "TotalVar"
#define CELL_SENSITIVITY_NAME            "Sensitivity"
#define CELL_STATUS_NAME                 "Status"
#define CELL_TYPE_NAME                   "Type"
#define CELL_TOTALNOISE_NAME             "TotalNoise"
#define CELL_SHADOWTOTAL_NAME            "TotalShadow"
#define CELL_TOTALPROXY_NAME             "TotalProxySize"
#define CELL_SENSITIVNWV_NAME            "SensitivityBeforeWaivers"
#define CELL_FAVCOST_NAME                "FavCost"

/* OutCell variable positions
   The following macro definitions and functions aid calculation of output var index, depending on parameters
*/
#define CELL_BASEVAR_INDEX(OPT_SPECIFIED, OFFSET) (OPT_SPECIFIED+OFFSET)

#define CELL_CELLID_INDEX        0
#define CELL_NBANONYM_INDEX      1
#define CELL_ANONYM_INDEX        2
#define CELL_NBRESPONDENTS_INDEX 3
#define CELL_WEIGHTEDNBRESPONDENTS_INDEX 4
#define CELL_TOTAL_INDEX(WEIGHT_SPECIFIED)         CELL_BASEVAR_INDEX(WEIGHT_SPECIFIED, 4)
#define CELL_SENSITIVITY_INDEX(WEIGHT_SPECIFIED)   CELL_BASEVAR_INDEX(WEIGHT_SPECIFIED, 5)
#define CELL_STATUS_INDEX(WEIGHT_SPECIFIED)        CELL_BASEVAR_INDEX(WEIGHT_SPECIFIED, 6)
#define CELL_TYPE_INDEX(WEIGHT_SPECIFIED)          CELL_BASEVAR_INDEX(WEIGHT_SPECIFIED, 7)
#define CELL_TOTALNOISE_INDEX(WEIGHT_SPECIFIED)    CELL_BASEVAR_INDEX(WEIGHT_SPECIFIED, 8)
#define CELL_SHADOWTOTAL_INDEX(WEIGHT_SPECIFIED)   CELL_BASEVAR_INDEX(WEIGHT_SPECIFIED, 9)
#define CELL_TOTALPROXY_INDEX(WEIGHT_SPECIFIED, SHADOWTOTAL_INCLUDED) (CELL_SHADOWTOTAL_INDEX(WEIGHT_SPECIFIED) + SHADOWTOTAL_INCLUDED)
#define CELL_SENSITIVNWV_INDEX(WEIGHT_SPECIFIED, SHADOWTOTAL_INCLUDED, TOTALPROXY_INCLUDED) (CELL_SHADOWTOTAL_INDEX(WEIGHT_SPECIFIED) + SHADOWTOTAL_INCLUDED + TOTALPROXY_INCLUDED)
#define CELL_FAVCOST_INDEX(WEIGHT_SPECIFIED, SHADOWTOTAL_INCLUDED, TOTALPROXY_INCLUDED, SENSITIVNWV_INCLUDED) (CELL_SHADOWTOTAL_INDEX(WEIGHT_SPECIFIED) + SHADOWTOTAL_INCLUDED + TOTALPROXY_INCLUDED + SENSITIVNWV_INCLUDED)
#define CELL_FIRST_CODE_INDEX(WEIGHT_SPECIFIED, SHADOWTOTAL_INCLUDED, TOTALPROXY_INCLUDED, SENSITIVNWV_INCLUDED, FAVCOST_INCLUDED) (CELL_SHADOWTOTAL_INDEX(WEIGHT_SPECIFIED) + SHADOWTOTAL_INCLUDED + TOTALPROXY_INCLUDED + SENSITIVNWV_INCLUDED + FAVCOST_INCLUDED)

#define CELL_CELLID_TYPE         SUTIL_VARIABLE_TYPE_NUMERIC
#define CELL_NBANONYM_TYPE       SUTIL_VARIABLE_TYPE_NUMERIC
#define CELL_ANONYM_TYPE         SUTIL_VARIABLE_TYPE_NUMERIC
#define CELL_NBRESPONDENTS_TYPE  SUTIL_VARIABLE_TYPE_NUMERIC
#define CELL_TOTAL_TYPE          SUTIL_VARIABLE_TYPE_NUMERIC
#define CELL_SENSITIVITY_TYPE    SUTIL_VARIABLE_TYPE_NUMERIC
#define CELL_STATUS_TYPE         SUTIL_VARIABLE_TYPE_CHARACTER
#define CELL_TYPE_TYPE           SUTIL_VARIABLE_TYPE_CHARACTER
#define CELL_TOTALNOISE_TYPE     SUTIL_VARIABLE_TYPE_NUMERIC
#define CELL_SHADOWTOTAL_TYPE    SUTIL_VARIABLE_TYPE_NUMERIC
#define CELL_TOTALPROXY_TYPE     SUTIL_VARIABLE_TYPE_NUMERIC
#define CELL_SENSITIVNWV_TYPE    SUTIL_VARIABLE_TYPE_NUMERIC
#define CELL_FAVCOST_TYPE        SUTIL_VARIABLE_TYPE_NUMERIC
#define CELL_CODE_TYPE           SUTIL_VARIABLE_TYPE_CHARACTER
#define CELL_WEIGHTEDNBRESPONDENTS_TYPE  SUTIL_VARIABLE_TYPE_NUMERIC

/* related to output LARGEST data set */
#define LARGEST_NBVAR_MANDATORY 5
#define LARGEST_NBVARS(SHADOWTOTAL_INCLUDED, SHADOWPERCENT_INCLUDED, WAIVERFLAG_INCLUDED) (LARGEST_NBVAR_MANDATORY + SHADOWTOTAL_INCLUDED + SHADOWPERCENT_INCLUDED + WAIVERFLAG_INCLUDED)

#define LARGEST_CELLID_NAME         "CellId"
#define LARGEST_NBRESPONDENTS_NAME  "NbRespondents"
#define LARGEST_TOTAL_NAME          "TotalVar"
#define LARGEST_TOTALPERCENT_NAME   "TotalPercent"
#define LARGEST_SHADOWTOTAL_NAME    "TotalShadow"
#define LARGEST_SHADOWPERCENT_NAME  "ShadowPercent"
#define LARGEST_WAIVERFLAG_NAME     "WaiverFlag"

/* OutCell variable positions
   The following macro definitions and functions aid calculation of output var index, depending on parameters
*/
#define LARGEST_CELLID_INDEX        0
#define LARGEST_ID_INDEX            1
#define LARGEST_NBRESPONDENTS_INDEX 2
#define LARGEST_TOTAL_INDEX         3
#define LARGEST_TOTALPERCENT_INDEX  4
#define LARGEST_SHADOWTOTAL_INDEX   5
#define LARGEST_SHADOWPERCENT_INDEX 6
#define LARGEST_WAIVERFLAG_INDEX(SHADOWTOTAL_INCLUDED, SHADOWPERCENT_INCLUDED) (LARGEST_SHADOWTOTAL_INDEX + SHADOWTOTAL_INCLUDED + SHADOWPERCENT_INCLUDED)

#define LARGEST_CELLID_TYPE         SUTIL_VARIABLE_TYPE_NUMERIC
#define LARGEST_ID_TYPE             SUTIL_VARIABLE_TYPE_CHARACTER
#define LARGEST_NBRESPONDENTS_TYPE  SUTIL_VARIABLE_TYPE_NUMERIC 
#define LARGEST_TOTAL_TYPE          SUTIL_VARIABLE_TYPE_NUMERIC
#define LARGEST_TOTALPERCENT_TYPE   SUTIL_VARIABLE_TYPE_NUMERIC
#define LARGEST_SHADOWTOTAL_TYPE    SUTIL_VARIABLE_TYPE_NUMERIC
#define LARGEST_SHADOWPERCENT_TYPE  SUTIL_VARIABLE_TYPE_NUMERIC
#define LARGEST_WAIVERFLAG_TYPE     SUTIL_VARIABLE_TYPE_NUMERIC

/* output dataset TARGETS */
#define TARGETS_NBVARS              4

#define TARGETS_CELLID_NAME         "CellId"
#define TARGETS_ID_NAME             "Id"
#define TARGETS_PTNVARIABLE_NAME    "PtnVariable"
#define TARGETS_VALUE_NAME          "Value"

#define TARGETS_CELLID_INDEX        0
#define TARGETS_ID_INDEX            1
#define TARGETS_PTNVARIABLE_INDEX   2
#define TARGETS_VALUE_INDEX         3

#define TARGETS_CELLID_TYPE         SUTIL_VARIABLE_TYPE_NUMERIC
#define TARGETS_ID_TYPE             SUTIL_VARIABLE_TYPE_CHARACTER
#define TARGETS_PTNVARIABLE_TYPE    SUTIL_VARIABLE_TYPE_CHARACTER
#define TARGETS_VALUE_TYPE          SUTIL_VARIABLE_TYPE_NUMERIC

/* output dataset PAIRS */
#define PAIRS_NBVARS                7

#define PAIRS_CELLID_NAME           "CellId"
#define PAIRS_TARGETID_NAME         "TargetId"
#define PAIRS_TARGETPT_NAME         "TargetPt"
#define PAIRS_ATTACKERID_NAME       "AttackerId"
#define PAIRS_ATTACKERSN_NAME       "AttackerSn"
#define PAIRS_REMAINDERCOUNT_NAME   "RemainderCount"
#define PAIRS_REMAINDERN_NAME       "RemainderN"

#define PAIRS_CELLID_INDEX          0
#define PAIRS_TARGETID_INDEX        1
#define PAIRS_TARGETPT_INDEX        2
#define PAIRS_ATTACKERID_INDEX      3
#define PAIRS_ATTACKERSN_INDEX      4
#define PAIRS_REMAINDERCOUNT_INDEX  5
#define PAIRS_REMAINDERN_INDEX      6

#define PAIRS_CELLID_TYPE           SUTIL_VARIABLE_TYPE_NUMERIC
#define PAIRS_TARGETID_TYPE         SUTIL_VARIABLE_TYPE_CHARACTER
#define PAIRS_TARGETPT_TYPE         SUTIL_VARIABLE_TYPE_NUMERIC
#define PAIRS_ATTACKERID_TYPE       SUTIL_VARIABLE_TYPE_CHARACTER
#define PAIRS_ATTACKERSN_TYPE       SUTIL_VARIABLE_TYPE_NUMERIC
#define PAIRS_REMAINDERCOUNT_TYPE   SUTIL_VARIABLE_TYPE_NUMERIC
#define PAIRS_REMAINDERN_TYPE       SUTIL_VARIABLE_TYPE_NUMERIC

#define PAIRS_ATTACKERID_SIZE_MIN   5

/* Default/unspecified values */
#define M_DEFAULT_VALUE                  (5)
#define X_DEFAULT_VALUE                  (0.0)
#define Y_DEFAULT_VALUE                  (0.0)
#define Z_DEFAULT_VALUE                  (0.0)
#define TOLERANCE_DEFAULT_VALUE          (0.0)
#define MINRESP_DEFAULT_VALUE            (0)
#define PROXYPERCENTILE_DEFAULT_VALUE    (0.1)
#define WEIGHTPROTLEVEL_DEFAULT_VALUE    ("LINEAR")
#define ADDITIVENOISE_DEFAULT_VALUE      (1)
#define PROXYDIAG_DEFAULT_VALUE          (0)
#define WEIGHTDIAG_DEFAULT_VALUE         (0)
#define ACCEPTNEGATIVE_DEFAULT_VALUE     (0)
#define PROXYRATIO_UNSPECD_VALUE         (-1.0)

/* number of decimals to print in header */
#define DEFAULT_NB_DECIMALS              5

/* maximum number of messages to print when user wants to limit the number of warning printed */
#define MESSAGE_QUOTA                    15
#define MESSAGE_NO_QUOTA                 2000000000 //this should be enough when there is no limit...

#define NOTFOUND                         -1

#define M_MIN                            (0)
#define M_MAX                            (10)
#define X_MIN                            (0.0)
#define X_MAX                            (100.0)
#define Y_MIN                            (0.0)
#define Y_MAX                            (100.0)
#define Z_MIN                            (0.0)
#define Z_MAX                            (100.0)
#define TOLERANCE_MIN                    (0.0)
#define TOLERANCE_MAX                    (1000.0)
#define TOLERANCE_WARNING                (1.0)
#define MINRESP_MIN                      (1)
#define MINRESP_MINCOMMONRANGE           (3)
#define MINRESP_MAXCOMMONRANGE           (5)
#define MINRESPW_MIN                     (0.0)


struct PROGRAM_COUNTER {
	int NumberObs;
	int NumberValidObs;

	int NumberMissingValues;
	int NumberNegativeValues;
	int NumberMissingShadows;
	int NumberNegativeOrMissingProxies;
	int NumberMissingWeights;
	int NumberNonPositiveWeights;
	int NumberInvalidWaivers;
	int NumberMissingDimensions;
	int NumberInvalidDimensionsJunkInCode;
	int NumberDimensionsNotInHierarchy;

	int NumberInternalCellsCalculated;
	int NumberInternalCellsSensitive;
	int NumberInternalCellsZero;

	int NumberMarginalCellsCalculated;
	int NumberMarginalCellsSensitive;
	int NumberMarginalCellsZero;

	int NumberAggregatesCalculated;
	int NumberAggregatesSensitive;

	int NumberUserGroupsCalculated;
	int NumberUserGroupsSensitive;

	int NumberAnonymous;
	int NumberZero;

	int NumberInternalCellsSensitivityChanged;
	int NumberMarginalCellsSensitivityChanged;
	int NumberAggregatesSensitivityChanged;
	int NumberUserGroupsSensitivityChanged;
};
typedef struct PROGRAM_COUNTER PROGRAM_COUNTER;

struct tReadInfo {
	tSDataSet * DataDataSet;
};
typedef struct tReadInfo tReadInfo;

struct tWriteInfo {
	tSDataSet * OutCellDataSet;
	tSDataSet * OutConstraintDataSet;
	tSDataSet * OutLargestDataSet;
	tSDataSet * OutTargetsDataSet;
	tSDataSet * OutPairsDataSet;
	SP_sensitiv* sp;
};
typedef struct tWriteInfo tWriteInfo;

/*
set DEBUG to 1 to activate the debugging print statements.
set DEBUG to 0 to deactivate the debugging print statements.
If DEBUG is zero, most compilers will not generate any code for the debugging
statements.
*/
enum {DEBUG = 0};


static void CountersIncrement (PROGRAM_COUNTER * CounterTotal, PROGRAM_COUNTER * Counter);
static void CountersInit (PROGRAM_COUNTER * Counter);
static void CountersPrint (PROGRAM_COUNTER * Counter, PROGRAM_PARM * Parms, int waiver_flags_present);
static EIT_RETURNCODE DefineInDataDataSet (SP_sensitiv* sp, tSDataSet * DataSet);
static void DefineOutConstraintDataSet (tSDataSet *);
static void DefineOutCellDataSet (SP_sensitiv* sp, tSDataSet *, STCT_HTREEROOT * HTreeRoot);
static void DefineOutLargestDataSet (SP_sensitiv* sp, tSDataSet *);
static void DefineOutTargetsDataSet (SP_sensitiv* sp, tSDataSet * DataSet);
static void DefineOutPairsDataSet (SP_sensitiv* sp, tSDataSet * DataSet);
static EIT_RETURNCODE ExclusivityBetweenLists (DSR_indata* dsr_indata);
static EIT_RETURNCODE ExclusivityBetweenListsOneAtATime (DSR_generic* dsr, DS_varlist* vl_1, DS_varlist* vl_2);
static EIT_RETURNCODE GetParms (SP_sensitiv* sp, tSDataSet *, tSDataSet *, tSDataSet *,
	tSDataSet *, tSDataSet *, tSDataSet *, PROGRAM_PARM * Parms);
static int Intersect (int *, int, int *, int);
static EIT_BOOLEAN IsCodeValid (char * Code, char * CodeFirstCharacterCharacterSet,
	char * CodeCharacterSet);
static size_t LargestCodeLenght (STCT_HTREEROOT * HTreeRoot, int iDimension);
static void PrintParms (SP_sensitiv* sp, PROGRAM_PARM * Parms);
static EIT_RETURNCODE CalculateProxyRatio (double ProxyPercentile, STCT_CELLSET * CellSet, double * ProxyRatioPtr);
static EIT_RETURNCODE GenerateWeightAndProxyDiagnostics (STCT_CELLSET * CellSet, int GenerateWeightDiagnostics, int GenerateProxyDiagnostics);
static EIT_RETURNCODE ReadData (SP_sensitiv* sp, tSDataSet * DataSet, STCT_CELLSET * CellSet,
	STCT_KDTREE ** KdTree, STCT_HTREEROOT * HTreeRoot, STCT_RANGEROOT * RangeRoot,
	PROGRAM_COUNTER * Counter, int * MessageQuota,
	int waiver_flags_present,
	tSList * waiver_flags_slist,
	tIList * waiver_flags_ilist,
	int AcceptNegativeValues);
static void ShowTime (SP_sensitiv* sp, char *);
static EIT_RETURNCODE ValidateParms (SP_sensitiv* sp, PROGRAM_PARM * Parms, STCT_SRULE ** SRule,
	STCT_HTREEROOT ** HTreeRoot, STCT_RANGEROOT ** RangeRoot, STCT_GROUPROOT ** GroupRoot, int * waiver_flags_present);
static EIT_RETURNCODE WriteCellObs (STCT_CELL * Cell, int NumberDimensions);
static EIT_RETURNCODE WriteConstraintObs (int ConstraintId, int CellId, int Coefficient);
static EIT_RETURNCODE WriteLargestObs (int CellId, char * Id, int NumberObs, double Value,
	double TotalValue, double Shadow, double TotalShadow, int WaiverFlag);
static EIT_RETURNCODE WriteTargetsObs (int CellId, char * Id, char * PtnVariable, double Value);
static EIT_RETURNCODE WritePairsObs (int CellId, char * TargetId, double TargetPt, char * AttackerId,
	double AttackerSn, int RemainderCount, double RemainderN);
static tSListReturn update_waiver_flags_lists(char * key, double val, tSList * waiver_flags_slist, tIList * waiver_flags_ilist);


/*
be carefull of the side effect on MessageQuota...
same define in STC_Build.c
*/
#define ADDMESSAGEQUOTA(MessageQuota, ...)\
	do {\
		if (*MessageQuota <= 0)\
			break;\
		EI_AddMessage (MsgReadingMicrodata, EIE_MESSAGESEVERITY_WARNING, __VA_ARGS__);\
		(*MessageQuota)--;\
		if (*MessageQuota <= 0)\
			EI_AddMessage (MsgReadingMicrodata, EIE_MESSAGESEVERITY_WARNING, MsgNoMoreMessages "\n");\
	} while (0)


static STCT_HTREEROOT * mHTreeRoot;
static tReadInfo  mReadInfo ;
static tWriteInfo mWriteInfo;


static tSListReturn update_waiver_flags_lists(char * key, double val, tSList * waiver_flags_slist, tIList * waiver_flags_ilist)
{
    /*-for each record read (the value of "val" (DataSet->Variable[DATA_WAIVER_INDEX].Value.d) must be either */
    /* "1" (waiver flag) or "0" (no waiver flag)), the value of an existing element of "waiver_flags_ilist"   */
    /* is set to "-1" if it differs from the value of "val":                                                  */
    int slist_index;
    int old_value;
    slist_index = SList_StringSearch (waiver_flags_slist, key);
    if (slist_index == SLIST_NOTFOUND)
    {
        if (SList_Add(key, waiver_flags_slist) == eSListFail)
        {
            IO_PRINT_LINE(SAS_MESSAGE_PREFIX_ERROR MsgWaiversKeyListFailed);
            IO_PRINT_LINE(SAS_MESSAGE_PREFIX_ERROR MsgWaiversKeyValuePairFailed, key, (int)val);
            return eSListFail;
        }
        if (IList_Add((int)val, waiver_flags_ilist) == eIListFail)
        {
            IO_PRINT_LINE(SAS_MESSAGE_PREFIX_ERROR MsgWaiversKeyValueListFailed);
            IO_PRINT_LINE(SAS_MESSAGE_PREFIX_ERROR MsgWaiversKeyValuePairFailed, key, (int)val);
            return eIListFail;
        }
#ifdef DISPLAY_WAIVER_FLAG_KVPAIRS
        IO_PRINT_LINE(MsgWaiversListKvPairAdded, key, (int)val);
#endif
    }
    else
    {
        if ((waiver_flags_ilist->l[slist_index] == 0 && val == 1.0) ||
            (waiver_flags_ilist->l[slist_index] == 1 && val == 0.0)
           )
        {
            old_value = waiver_flags_ilist->l[slist_index];
            waiver_flags_ilist->l[slist_index] = -1;
            IO_PRINT_LINE(SAS_MESSAGE_PREFIX_ERROR MsgWaiversWaiverFlagNotConsistent, key);
#ifdef DISPLAY_WAIVER_FLAG_KVPAIRS
            IO_PRINT_LINE(MsgWaiversListKvPairChanged, key, old_value, key, -1);
#endif
            return eIListFail;
        } else {
#ifdef DISPLAY_WAIVER_FLAG_KVPAIRS
            IO_PRINT_LINE(MsgWaiversListNoChangeNeeded, key, (int)val);
#endif
        }
    }
    return eIListSucceed;
}

/*-the declaration of "Parms" and "SRule" has been moved here from inside of the   */
/* main function "sensitiv()", so it is now accesible anywhere in this */
/* file, so now any code in this file can get from the members         */
/* of this structure the processed (validated and possibly imputed)    */
/* parameters, instead of having to recalculate them from the raw      */
/* (unprocessed) parameter values that were passed in by the calling   */
/* program:                                                            */
PROGRAM_PARM Parms;
STCT_SRULE * SRule = NULL;

EXPORTED_FUNCTION int sensitiv(
	T_in_parm in_parms,

	T_in_ds in_ds_indata,

	T_out_ds out_sch_outconstraint,
	T_out_ds out_arr_outconstraint,
	T_out_ds out_sch_outcell,
	T_out_ds out_arr_outcell,
	T_out_ds out_sch_outlargest,
	T_out_ds out_arr_outlargest,
	T_out_ds out_sch_outpairs,
	T_out_ds out_arr_outpairs,
	T_out_ds out_sch_outtargets,
	T_out_ds out_arr_outtargets
)
{
	STCT_CELLSET * CellSet = NULL;
	PROGRAM_COUNTER Counter;
	PROGRAM_COUNTER CounterTotal;
	tSDataSet DataDataSet;
	STCT_GROUPROOT * GroupRoot = NULL;
	STCT_HTREEROOT * HTreeRoot = NULL;
	STCT_KDTREE * KdTree = NULL;
	int MessageQuota;
	tSDataSet OutConstraintDataSet;
	tSDataSet OutCellDataSet;
	tSDataSet OutLargestDataSet;
	tSDataSet OutTargetsDataSet;
	tSDataSet OutPairsDataSet;
	STCT_RANGEROOT * RangeRoot = NULL;
	EIT_RETURNCODE rc;

	int waiver_flags_present;
	tSList * waiver_flags_slist = NULL;
	tIList * waiver_flags_ilist = NULL;
#ifdef DISPLAY_WAIVER_FLAG_KVPAIRS
	int ndx;
#endif

	error_occurred_in_expression_macro = 0;

	(&DataDataSet         )->Variable = NULL;
	(&OutConstraintDataSet)->Variable = NULL;
	(&OutCellDataSet      )->Variable = NULL;
	(&OutLargestDataSet   )->Variable = NULL;
	(&OutTargetsDataSet   )->Variable = NULL;
	(&OutPairsDataSet     )->Variable = NULL;
	EI_mMessageList_Message_to_NULL();

	GCONFID_RETURN_CODE proc_exit_code = GRC_SUCCESS;

	/* Initialize runtime environment */
	init_runtime_env();

	/* TIME MEASUREMENT */
	TIME_WALL_DECLARE(cleanup);
	TIME_CPU_DECLARE(cleanup);
	TIME_WALL_START(main);
	TIME_CPU_START(main);

	STC_AllocateMemorySetCB (SUtil_AllocateMemory);
	STC_ReallocationMemorySetCB (SUtil_ReallocateMemory);
	STC_FreeMemorySetCB (SUtil_FreeMemory);

	EI_PrintMessagesSetCB (SUtil_PrintMessages);


	/*-generate name for output file to which debugging information may be written: */
	
	EI_AllocateMessageList ();
	if (EI_mMessageList_Message_is_NULL()) {
		proc_exit_code = GRC_FAIL_ALLOCATE_MEMORY;
		goto error_cleanup;
	}

	/* SI : SYSTEM INFO. These defines should be define in the makefile */
	SUtil_PrintSystemInfo (SI_SYSTEMNAME, SI_SYSTEMVERSION, PROC_NAME,
		SI_PROCVERSION, SI_EMAIL, NULL);

	SP_sensitiv sp = { 0 };
	TIME_WALL_START(load_init);
	TIME_CPU_START(load_init);
	mem_usage("before SP_init");
	IO_RETURN_CODE rc_sp_init = SP_init(
		&sp,
		in_parms,
		in_ds_indata,
		out_sch_outconstraint,
		out_arr_outconstraint,
		out_sch_outcell,
		out_arr_outcell,
		out_sch_outlargest,
		out_arr_outlargest,
		out_sch_outpairs,
		out_arr_outpairs,
		out_sch_outtargets,
		out_arr_outtargets
	);
	mem_usage("after SP_init");

	proc_exit_code = SP_validate_init(rc_sp_init);
	if (proc_exit_code != GRC_SUCCESS) {
		goto error_cleanup;
	}

	IO_RETURN_CODE rc_validation = SPG_validate(&sp.spg);
	if (rc_validation != IORC_SUCCESS) {
		proc_exit_code = GRC_FAIL_VALIDATION_NEW;
		goto error_cleanup;
	}

	rc = GetParms (&sp, &DataDataSet, &OutConstraintDataSet, &OutCellDataSet,
		&OutLargestDataSet, &OutTargetsDataSet, &OutPairsDataSet, &Parms);
	if (rc != EIE_SUCCEED) {
		proc_exit_code = GRC_FAIL_READ_PARMS_LEGACY;
		goto error_cleanup;
	}

	ShowTime (&sp, "Kick start the timer.!");

	PrintParms (&sp, &Parms);

	// print any queued message (such as logging output)
	EI_PrintMessages();

	rc = ValidateParms (&sp, &Parms, &SRule, &HTreeRoot, &RangeRoot, &GroupRoot, &waiver_flags_present);
	if (rc != EIE_SUCCEED) {
		proc_exit_code = GRC_FAIL_VALIDATION_LEGACY;
		goto error_cleanup;
	}

	SRule->Tolerance = Parms.Tolerance;
	SRule->MinResp   = Parms.MinResp;
	SRule->Parms     = &Parms;

	mHTreeRoot = HTreeRoot;//pour WriteCell

	rc = DefineInDataDataSet (&sp, &DataDataSet);
	if (rc != EIE_SUCCEED) {
		proc_exit_code = GRC_FAIL_SETUP_DATASET_IN;
		goto error_cleanup;
	}

	SRule->waiver_flags_present = waiver_flags_present;

	mReadInfo.DataDataSet = &DataDataSet;	// is this used for anything?
	mWriteInfo.sp = &sp;

	IO_RETURN_CODE rc_io = IORC_ERROR;

	DefineOutConstraintDataSet (&OutConstraintDataSet);
	rc_io = SDataSetDefineForOutput (&OutConstraintDataSet, &sp.dsw_outconstraint);
	if (rc_io != IORC_SUCCESS) {
		proc_exit_code = GRC_FAIL_WRITE_GENERIC;
		goto error_cleanup;
	}
	mWriteInfo.OutConstraintDataSet = &OutConstraintDataSet;
	STC_WriteConstraintObsSetCB (WriteConstraintObs);

	DefineOutCellDataSet (&sp, &OutCellDataSet, HTreeRoot);
	rc_io = SDataSetDefineForOutput (&OutCellDataSet, &sp.dsw_outcell);
	if (rc_io != IORC_SUCCESS) {
		proc_exit_code = GRC_FAIL_WRITE_GENERIC;
		goto error_cleanup;
	}
	mWriteInfo.OutCellDataSet = &OutCellDataSet;
	STC_WriteCellObsSetCB (WriteCellObs);

	if (sp.dsw_outlargest.is_requested == IOB_TRUE) {
		DefineOutLargestDataSet (&sp, &OutLargestDataSet);
		rc_io = SDataSetDefineForOutput (&OutLargestDataSet, &sp.dsw_outlargest);
		if (rc_io != IORC_SUCCESS) {
			proc_exit_code = GRC_FAIL_WRITE_GENERIC;
			goto error_cleanup;
		}
		mWriteInfo.OutLargestDataSet = &OutLargestDataSet;
	}
	STC_WriteLargestObsSetCB (WriteLargestObs);

	if (sp.dsw_outtargets.is_requested == IOB_TRUE && Parms.Weight != 0 && Parms.WeightProtectionLevel != 'E' && SRule->Type == STCE_SRULE_TYPE_NK) {
	    DefineOutTargetsDataSet (&sp, &OutTargetsDataSet);
		rc_io = SDataSetDefineForOutput (&OutTargetsDataSet, &sp.dsw_outtargets);
		if (rc_io != IORC_SUCCESS) {
			proc_exit_code = GRC_FAIL_WRITE_GENERIC;
			goto error_cleanup;
		}
	    mWriteInfo.OutTargetsDataSet = &OutTargetsDataSet;
	}
	STC_WriteTargetsObsSetCB (WriteTargetsObs);
	if (sp.dsw_outpairs.is_requested == IOB_TRUE && Parms.Weight != 0 && Parms.WeightProtectionLevel != 'E' && SRule->Type == STCE_SRULE_TYPE_PQ) {
	    DefineOutPairsDataSet (&sp, &OutPairsDataSet);
		rc_io = SDataSetDefineForOutput (&OutPairsDataSet, &sp.dsw_outpairs);
		if (rc_io != IORC_SUCCESS) {
			proc_exit_code = GRC_FAIL_WRITE_GENERIC;
			goto error_cleanup;
		}
	    mWriteInfo.OutPairsDataSet = &OutPairsDataSet;
	}
	STC_WritePairsObsSetCB (WritePairsObs);

	if (sp.dsr_indata.dsr.VL_by_var.count) CountersInit (&CounterTotal);

	STC_CellInitCellId ();
	STC_CellSetInitConstraintId ();

	ShowTime (&sp, "Allocation et setup complété");
	TIME_CPU_STOPDIFF(load_init);
	TIME_WALL_STOPDIFF(load_init);

	TIME_WALL_START(processing);
	TIME_CPU_START(processing);

	for (;;) {//by groups processing

		EI_PrintMessages ();

		MessageQuota = Parms.MessageQuota;//reset the number of messages to print for each by group
		IO_DATASET_RC rc_next_by = DSR_cursor_next_by(&sp.dsr_indata.dsr);
		// check for errors
		if (rc_next_by == DSRC_NO_MORE_REC_IN_DS) { // no error
			break;
		}
		else if (rc_next_by == DSRC_BY_NOT_SORTED) {
			proc_exit_code = GRC_FAIL_WRONG_SORT_ORDER;
			goto error_cleanup;
		}
		else if (rc_next_by != DSRC_NEXT_BY_SUCCESS) {
			proc_exit_code = GRC_FAIL_READ_GENERIC;
			goto error_cleanup;
		} // else continue (no error)


		if (waiver_flags_present == 1) { //if FULL waiver specified, then create SList for validating flag consistency
			SList_New (&waiver_flags_slist);
			if (waiver_flags_slist == NULL) {
				proc_exit_code = GRC_FAIL_ALLOCATE_MEMORY;
				goto error_cleanup;
			}
			IList_New (&waiver_flags_ilist);
			if (waiver_flags_ilist == NULL) {
				proc_exit_code = GRC_FAIL_ALLOCATE_MEMORY;
				goto error_cleanup;
			}
		}

		CountersInit (&Counter);

		CellSet = STC_CellSetAllocate (10000);
		if (CellSet == NULL) {
			proc_exit_code = GRC_FAIL_ALLOCATE_MEMORY;
			goto error_cleanup;
		}
		KdTree = NULL;
		rc = ReadData (&sp, &DataDataSet, CellSet, &KdTree, HTreeRoot, RangeRoot, &Counter, &MessageQuota, waiver_flags_present, waiver_flags_slist, waiver_flags_ilist, Parms.AcceptNegativeValues);
		if (rc != EIE_SUCCEED) {
			proc_exit_code = GRC_FAIL_READ_GENERIC;
			goto error_cleanup;
		}
		STC_CellSetAdjustCellAllocation (CellSet);//shrink the data structure to save memory
		if (CellSet == NULL) {
			proc_exit_code = GRC_FAIL_ALLOCATE_MEMORY;
			goto error_cleanup;
		}

		if (!Parms.ProxyRatioSpecified) {
			rc = CalculateProxyRatio (Parms.ProxyPercentile, CellSet, &(Parms.ProxyRatio));
			if (rc != EIE_SUCCEED) {
				proc_exit_code = GRC_FAIL_PROCESSING_GENERIC;
				goto error_cleanup;
			}
			if (Parms.Proxy) {
				IO_PRINT_LINE(MsgProxyRatio , Parms.ProxyPercentile, Parms.ProxyRatio);
			}
		}

		//STC_CellSetPrint (CellSet);

		ShowTime (&sp, "Lecture complété");

		SRule->NumberSensitivityChanged = 0;
		rc = STC_CalculateInternalCellsSensitivity (CellSet, SRule,
			&Counter.NumberInternalCellsCalculated, &Counter.NumberInternalCellsSensitive,
			&Counter.NumberInternalCellsZero);
		//EI_PrintMessages ();
		if (rc != EIE_SUCCEED) {
			proc_exit_code = GRC_FAIL_PROCESSING_GENERIC;
			goto error_cleanup;
		}
/* STC_HTreeRootPrint(HTreeRoot); */
/* STC_RangeRootPrint(RangeRoot); */
/* STC_CellSetPrint  (CellSet  ); */
/* STC_KdTreePrint   (KdTree   ); */
/* EI_PrintMessages  ();          */
		ShowTime (&sp, "Calculate internal cells sensitivity");
		Counter.NumberInternalCellsSensitivityChanged = SRule->NumberSensitivityChanged;

		//STC_CellSetPrint (CellSet);

		if (waiver_flags_present != 0) {
			rc = STC_CalculateInternalCellsFavCost (HTreeRoot, CellSet, KdTree, SRule);
			if (rc != EIE_SUCCEED) {
				proc_exit_code = GRC_FAIL_PROCESSING_GENERIC;
			    goto error_cleanup;
			}
			rc = STC_WriteInternalCells (CellSet, SRule, waiver_flags_present);
			if (rc != EIE_SUCCEED) {
				proc_exit_code = GRC_FAIL_PROCESSING_GENERIC;
			    goto error_cleanup;
			}
		}

		SRule->NumberSensitivityChanged = 0;
		rc = STC_CalculateMarginalCellsSensitivity (HTreeRoot, CellSet, KdTree, SRule,
			&Counter.NumberMarginalCellsCalculated, &Counter.NumberMarginalCellsSensitive,
			&Counter.NumberMarginalCellsZero);
		//EI_PrintMessages ();
		if (rc != EIE_SUCCEED) {
			proc_exit_code = GRC_FAIL_PROCESSING_GENERIC;
			goto error_cleanup;
		}
		ShowTime (&sp, "Calculate marginal cells sensitivity");
		Counter.NumberMarginalCellsSensitivityChanged = SRule->NumberSensitivityChanged;

		STC_CellSetAdjustAllocation (CellSet);//shrink the data structure to save memory 

		SRule->NumberSensitivityChanged = 0;
		rc = STC_FindSensitiveAggregates (HTreeRoot, KdTree, SRule, Parms.M, Parms.X, Parms.Y, Parms.Z, Parms.Verbose,
			&Counter.NumberAggregatesCalculated, &Counter.NumberAggregatesSensitive);
		//EI_PrintMessages ();
		if (rc != EIE_SUCCEED) {
			proc_exit_code = GRC_FAIL_PROCESSING_GENERIC;
			goto error_cleanup;
		}
		ShowTime (&sp, "Calculate aggregates sensitivity");
		Counter.NumberAggregatesSensitivityChanged = SRule->NumberSensitivityChanged;

		CountersPrint (&Counter, &Parms, waiver_flags_present);
		DSR_cursor_print_by_message(&sp.dsr_indata.dsr, MSG_PREFIX_NOTE MsgHeaderForByGroupAbove_SAS_FREE, 1);

		EI_PrintMessages ();
		if ((Parms.Weight && Parms.WeightDiagnostics) || (Parms.Proxy && Parms.ProxyDiagnostics)) {
			rc = GenerateWeightAndProxyDiagnostics(CellSet, (Parms.Weight && Parms.WeightDiagnostics), (Parms.Proxy && Parms.ProxyDiagnostics));
			if (rc != EIE_SUCCEED) {
				proc_exit_code = GRC_FAIL_PROCESSING_GENERIC;
				goto error_cleanup;
			}
		}

		if (sp.dsr_indata.dsr.VL_by_var.count) CountersIncrement (&CounterTotal, &Counter);

		if (CellSet            != NULL) {STC_CellSetFree (CellSet           ); CellSet            = NULL;}
		if (KdTree             != NULL) {STC_KdTreeFree  (KdTree            ); KdTree             = NULL;}
		if (waiver_flags_slist != NULL) {SList_Free      (waiver_flags_slist); waiver_flags_slist = NULL;}
		if (waiver_flags_ilist != NULL) {IList_Free      (waiver_flags_ilist); waiver_flags_ilist = NULL;}
	}//end by groups processing

	TIME_CPU_STOPDIFF(processing);
	TIME_WALL_STOPDIFF(processing);

	if (sp.dsr_indata.dsr.VL_by_var.count) {
		CountersPrint (&CounterTotal, &Parms, waiver_flags_present);
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgFooterAllByGroup SAS_NEWLINE);
	}

	mem_usage("Before SP_wrap");
	SP_wrap(&sp);
	mem_usage("After SP_wrap");

	TIME_WALL_BEGIN(cleanup);
	TIME_CPU_BEGIN(cleanup);

	goto normal_cleanup;
error_cleanup:
	if (CellSet                       != NULL) {STC_CellSetFree   (CellSet              ); CellSet                       = NULL;}
	if (KdTree                        != NULL) {STC_KdTreeFree    (KdTree               ); KdTree                        = NULL;}
	if (waiver_flags_slist            != NULL) {SList_Free        (waiver_flags_slist   ); waiver_flags_slist            = NULL;}
	if (waiver_flags_ilist            != NULL) {IList_Free        (waiver_flags_ilist   ); waiver_flags_ilist            = NULL;}

normal_cleanup:
	if (HTreeRoot                     != NULL) {STC_HTreeRootFree (HTreeRoot            ); HTreeRoot                     = NULL;}
	if (RangeRoot                     != NULL) {STC_RangeRootFree (RangeRoot            ); RangeRoot                     = NULL;}
	if (SRule                         != NULL) {STC_SRuleFree     (SRule                ); SRule                         = NULL;}

	if ((&DataDataSet         )->Variable != NULL) {SDataSetFree    (&DataDataSet         ); (&DataDataSet         )->Variable = NULL;}
	if ((&OutConstraintDataSet)->Variable != NULL) {SDataSetFree    (&OutConstraintDataSet); (&OutConstraintDataSet)->Variable = NULL;}
	if ((&OutCellDataSet      )->Variable != NULL) {SDataSetFree    (&OutCellDataSet      ); (&OutCellDataSet      )->Variable = NULL;}
	if ((&OutLargestDataSet   )->Variable != NULL) {SDataSetFree    (&OutLargestDataSet   ); (&OutLargestDataSet   )->Variable = NULL;}
	if ((&OutTargetsDataSet   )->Variable != NULL) {SDataSetFree    (&OutTargetsDataSet   ); (&OutTargetsDataSet   )->Variable = NULL;}
	if ((&OutPairsDataSet     )->Variable != NULL) {SDataSetFree    (&OutPairsDataSet     ); (&OutPairsDataSet     )->Variable = NULL;}
	EI_PrintMessages ();

	ShowTime (&sp, "Clean up");

	if ((EI_mMessageList_Message_is_NULL()!= 1   )
	) {
		EI_FreeMessageList ();
		EI_mMessageList_Message_to_NULL();
	}
	/* free Statcan Procedureand children */
	mem_usage("Before SPG_free");
	SPG_free(&sp.spg);
	mem_usage("After SPG_free");

	/* TIME MEASUREMENT */
	TIME_CPU_STOPDIFF(cleanup);
	TIME_WALL_STOPDIFF(cleanup);
	TIME_CPU_STOPDIFF(main);
	TIME_WALL_STOPDIFF(main);

	deinit_runtime_env();

	if (error_occurred_in_expression_macro != 0 && proc_exit_code != GRC_FAIL_UNHANDLED) {
		proc_exit_code = GRC_FAIL_UNHANDLED;
	}

	return(proc_exit_code);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static void CountersIncrement (
	PROGRAM_COUNTER * CounterTotal,
	PROGRAM_COUNTER * Counter)
{
	CounterTotal->NumberObs += Counter->NumberObs;
	CounterTotal->NumberValidObs += Counter->NumberValidObs;
	CounterTotal->NumberMissingValues += Counter->NumberMissingValues;
	CounterTotal->NumberNegativeValues += Counter->NumberNegativeValues;
	CounterTotal->NumberMissingShadows += Counter->NumberMissingShadows;
	CounterTotal->NumberNegativeOrMissingProxies += Counter->NumberNegativeOrMissingProxies;
	CounterTotal->NumberMissingWeights += Counter->NumberMissingWeights;
	CounterTotal->NumberNonPositiveWeights += Counter->NumberNonPositiveWeights;
	CounterTotal->NumberInvalidWaivers += Counter->NumberInvalidWaivers;
	CounterTotal->NumberMissingDimensions += Counter->NumberMissingDimensions;
	CounterTotal->NumberInvalidDimensionsJunkInCode += Counter->NumberInvalidDimensionsJunkInCode;
	CounterTotal->NumberDimensionsNotInHierarchy += Counter->NumberDimensionsNotInHierarchy;

	CounterTotal->NumberInternalCellsCalculated += Counter->NumberInternalCellsCalculated;
	CounterTotal->NumberInternalCellsSensitive += Counter->NumberInternalCellsSensitive;
	CounterTotal->NumberInternalCellsZero += Counter->NumberInternalCellsZero;

	CounterTotal->NumberMarginalCellsCalculated += Counter->NumberMarginalCellsCalculated;
	CounterTotal->NumberMarginalCellsSensitive += Counter->NumberMarginalCellsSensitive;
	CounterTotal->NumberMarginalCellsZero += Counter->NumberMarginalCellsZero;

	CounterTotal->NumberAggregatesCalculated += Counter->NumberAggregatesCalculated;
	CounterTotal->NumberAggregatesSensitive += Counter->NumberAggregatesSensitive;

	CounterTotal->NumberUserGroupsCalculated += Counter->NumberUserGroupsCalculated;
	CounterTotal->NumberUserGroupsSensitive += Counter->NumberUserGroupsSensitive;

	CounterTotal->NumberAnonymous += Counter->NumberAnonymous;
	CounterTotal->NumberZero += Counter->NumberZero;

	CounterTotal->NumberInternalCellsSensitivityChanged += Counter->NumberInternalCellsSensitivityChanged;
	CounterTotal->NumberMarginalCellsSensitivityChanged += Counter->NumberMarginalCellsSensitivityChanged;
	CounterTotal->NumberAggregatesSensitivityChanged += Counter->NumberAggregatesSensitivityChanged;
	CounterTotal->NumberUserGroupsSensitivityChanged += Counter->NumberUserGroupsSensitivityChanged;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static void CountersInit (
	PROGRAM_COUNTER * Counter)
{
	Counter->NumberObs = 0;
	Counter->NumberValidObs = 0;
	Counter->NumberMissingValues = 0;
	Counter->NumberNegativeValues = 0;
	Counter->NumberMissingShadows = 0;
	Counter->NumberNegativeOrMissingProxies = 0;
	Counter->NumberMissingWeights = 0;
	Counter->NumberNonPositiveWeights = 0;
	Counter->NumberInvalidWaivers = 0;
	Counter->NumberMissingDimensions = 0;
	Counter->NumberInvalidDimensionsJunkInCode = 0;
	Counter->NumberDimensionsNotInHierarchy = 0;

	Counter->NumberInternalCellsCalculated = 0;
	Counter->NumberInternalCellsSensitive = 0;
	Counter->NumberInternalCellsZero = 0;

	Counter->NumberMarginalCellsCalculated = 0;
	Counter->NumberMarginalCellsSensitive = 0;
	Counter->NumberMarginalCellsZero = 0;

	Counter->NumberAggregatesCalculated = 0;
	Counter->NumberAggregatesSensitive = 0;

	Counter->NumberUserGroupsCalculated = 0;
	Counter->NumberUserGroupsSensitive = 0;

	Counter->NumberAnonymous = 0;
	Counter->NumberZero = 0;

	Counter->NumberInternalCellsSensitivityChanged = 0;
	Counter->NumberMarginalCellsSensitivityChanged = 0;
	Counter->NumberAggregatesSensitivityChanged = 0;
	Counter->NumberUserGroupsSensitivityChanged = 0;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static void CountersPrint (
	PROGRAM_COUNTER * Counter,
	PROGRAM_PARM * Parms,
	int waiver_flags_present
	)
{
	int Calculated;
	int Sensitive;
	double Percent;

#define PREFIX_SIZE 30
#define COLUMNSIZE 20
#define MSGSIZE (PREFIX_SIZE + COLUMNSIZE*2 + 2 + 1)
#define DECIMALS 2

#define MICRODATA_STATISTICS_FORMAT "%-*s %*d"
#define CELL_STATISTICS_HEADER_FORMAT "%-*s %*s %*s %*s"
#define CELL_STATISTICS_FORMAT "%-*s %*d %*d %*.*f"

	if (Counter->NumberInternalCellsSensitivityChanged > 0 ||
			Counter->NumberMarginalCellsSensitivityChanged > 0 ||
			Counter->NumberAggregatesSensitivityChanged > 0 ||
			Counter->NumberUserGroupsSensitivityChanged > 0) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MsgSensitivityChanged);
		if (Counter->NumberInternalCellsSensitivityChanged > 0) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MsgInternalCellsSensitivityChanged,
				Counter->NumberInternalCellsSensitivityChanged);
		}
		if (Counter->NumberMarginalCellsSensitivityChanged > 0) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MsgMarginalCellsSensitivityChanged,
				Counter->NumberMarginalCellsSensitivityChanged);
		}
		if (Counter->NumberAggregatesSensitivityChanged > 0) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MsgAggregatesSensitivityChanged,
				Counter->NumberAggregatesSensitivityChanged);
		}
		if (Counter->NumberUserGroupsSensitivityChanged > 0) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MsgUserGroupsSensitivityChanged,
				Counter->NumberUserGroupsSensitivityChanged);
		}
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");
	}

	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " ");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MsgMicrodataStatisticsHeader);
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, MsgTotalObs, COLUMNSIZE,                                  Counter->NumberObs                             );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg1       , COLUMNSIZE,                                  Counter->NumberValidObs                        );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg1a      , COLUMNSIZE,                                  Counter->NumberAnonymous                       );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg1b      , COLUMNSIZE,                                  Counter->NumberValidObs -
	                                                                                                                                              Counter->NumberAnonymous                       );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg1c      , COLUMNSIZE,     Parms->AcceptNegativeValues ?Counter->NumberNegativeValues          :0      );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg1d      , COLUMNSIZE,     Parms->Proxy                ?Counter->NumberNegativeOrMissingProxies:0      );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg1e      , COLUMNSIZE,     Parms->Weight               ?Counter->NumberNonPositiveWeights      :0      );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg1f      , COLUMNSIZE,     (waiver_flags_present != 0) ?Counter->NumberInvalidWaivers          :0      );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg2       , COLUMNSIZE,                                  Counter->NumberZero                            );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg3       , COLUMNSIZE,
		                                                                                                     (                                Counter->NumberMissingValues                  +
		                                                                                                      ((!Parms->AcceptNegativeValues)?Counter->NumberNegativeValues             :0) +
		                                                                                                                                      Counter->NumberMissingDimensions              +
		                                                                                                                                      Counter->NumberInvalidDimensionsJunkInCode    +
		                                                                                                      (  Parms->Shadow               ?Counter->NumberMissingShadows             :0) +
		                                                                                                      (  Parms->Weight               ?Counter->NumberMissingWeights             :0) +
		                                                                                                      (                               Counter->NumberDimensionsNotInHierarchy     )  
		                                                                                                     )                                                                              );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg3a      , COLUMNSIZE,                                  Counter->NumberMissingValues                   );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg3b      , COLUMNSIZE,   (!Parms->AcceptNegativeValues)?Counter->NumberNegativeValues:0                );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg3c      , COLUMNSIZE,                                  Counter->NumberMissingDimensions               );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg3d      , COLUMNSIZE,                                  Counter->NumberInvalidDimensionsJunkInCode     );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg3e      , COLUMNSIZE,    Parms->Shadow                ?Counter->NumberMissingShadows              :0  );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg3f      , COLUMNSIZE,    Parms->Weight                ?Counter->NumberMissingWeights              :0  );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MICRODATA_STATISTICS_FORMAT, MSGSIZE, Msg4       , COLUMNSIZE,                                  Counter->NumberDimensionsNotInHierarchy        );
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");

	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, MsgCellStatisticsHeader);
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");

	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, CELL_STATISTICS_HEADER_FORMAT,
		PREFIX_SIZE, "", COLUMNSIZE, MsgNumberCalculated, COLUMNSIZE, MsgNumberSensitive, COLUMNSIZE, MsgPercentSensitive);
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");

	Calculated = Counter->NumberInternalCellsZero+Counter->NumberMarginalCellsZero;
	Sensitive = 0;
	Percent = (Calculated == 0 ? 0.0 : 100.0 * Sensitive / Calculated);
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, CELL_STATISTICS_FORMAT,
		PREFIX_SIZE, MsgCellsZeroValue, COLUMNSIZE, Calculated, COLUMNSIZE, Sensitive, COLUMNSIZE, DECIMALS, Percent);

	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%-*s",
		PREFIX_SIZE, MsgCellsNonZeroValue);

	Calculated = Counter->NumberInternalCellsCalculated;
	Sensitive = Counter->NumberInternalCellsSensitive;
	Percent = (Calculated == 0 ? 0.0 : 100.0 * Sensitive / Calculated);
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, CELL_STATISTICS_FORMAT,
		PREFIX_SIZE, "  " MsgInternalCells, COLUMNSIZE, Calculated, COLUMNSIZE, Sensitive, COLUMNSIZE, DECIMALS, Percent);

	Calculated = Counter->NumberMarginalCellsCalculated;
	Sensitive = Counter->NumberMarginalCellsSensitive;
	Percent = (Calculated == 0 ? 0.0 : 100.0 * Sensitive / Calculated);
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, CELL_STATISTICS_FORMAT,
		PREFIX_SIZE, "  " MsgMarginalCells, COLUMNSIZE, Calculated, COLUMNSIZE, Sensitive, COLUMNSIZE, DECIMALS, Percent);

	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");

	Calculated = Counter->NumberInternalCellsCalculated+Counter->NumberMarginalCellsCalculated;
	Sensitive = Counter->NumberInternalCellsSensitive+Counter->NumberMarginalCellsSensitive;
	Percent = (Calculated == 0 ? 0.0 : 100.0 * Sensitive / Calculated);
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, CELL_STATISTICS_FORMAT,
		PREFIX_SIZE, MsgAllCells, COLUMNSIZE, Calculated, COLUMNSIZE, Sensitive, COLUMNSIZE, DECIMALS, Percent);

	Calculated = Counter->NumberAggregatesCalculated;
	Sensitive = Counter->NumberAggregatesSensitive;
	Percent = (Calculated == 0 ? 0.0 : 100.0 * Sensitive / Calculated);
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, CELL_STATISTICS_FORMAT,
		PREFIX_SIZE, MsgAggregates, COLUMNSIZE, Calculated, COLUMNSIZE, Sensitive, COLUMNSIZE, DECIMALS, Percent);

	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");

	Calculated = Counter->NumberInternalCellsCalculated+Counter->NumberMarginalCellsCalculated+Counter->NumberAggregatesCalculated+Counter->NumberUserGroupsCalculated;
	Sensitive = Counter->NumberInternalCellsSensitive+Counter->NumberMarginalCellsSensitive+Counter->NumberAggregatesSensitive+Counter->NumberUserGroupsSensitive;
	Percent = (Calculated == 0 ? 0.0 : 100.0 * Sensitive / Calculated);
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, CELL_STATISTICS_FORMAT,
		PREFIX_SIZE, MsgTotal, COLUMNSIZE, Calculated, COLUMNSIZE, Sensitive, COLUMNSIZE, DECIMALS, Percent);

	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");

	EI_PrintMessages ();
}
/*********************************************************************
Allocates variables for input data set DATA
*********************************************************************/
static EIT_RETURNCODE DefineInDataDataSet (
	SP_sensitiv* sp,
	tSDataSet * DataSet)
{
	int i, j;
	int * Position;

	IO_RETURN_CODE rc_io = IORC_SUCCESS;

	int num_dimensions = sp->dsr_indata.VL_dimension.count;

	/* SA: MAY-2022: The following comment is likely out of date information (so don't trust, double check yourself) 
	  -the variables in each record in the input "indata" dataset can appear in any order   */
	/* (this program accesses them by name, not by their positions in the input record      */
	/*  --of course the order of the variables will be the same in all of the records, but  */
	/*  the point is that this program will be able to access them no matter what that      */
	/*  order is                                                                            */
	/* )                                                                                    */
	/* and the order in which the variables in each record of the input "indata" dataset    */
	/* are stored in the struct DataSet (of type tSDataSet) is:                           */
	/*     var       (the variable whose confidentiality needs to be protected) (always included                                         ) */
	/*     id        (the contributor id   variable)                            (always included                                         ) */
	/*     shadow    (the shadow           variable)                            (if     included, otherwise this position is left unused ) */
	/*     proxy     (the proxy            variable)                            (if     included, otherwise this position is left unused ) */
	/*     weight    (the weight           variable)                            (if     included, otherwise this position is left unused ) */
	/*     dim1      (the first  dimension variable)                            (always included                                         ) */
	/*     dim2      (the second dimension variable)                            (if     included, otherwise this position isn't allocated) */
	/*     dim3      (the third  dimension variable)                            (if     included, otherwise this position isn't allocated) */
	/*     ...                                                                                                                             */
	/*     dimn      (the last   dimension variable)                            (if     included, otherwise this position isn't allocated) */
	/* --space is allocated for 5+<the number of dimensions> variables, and if an optional  */
	/* variable isn't specfied by the calling sas program then it's place in the list is    */
	/* left unused                                                                          */

	DSR_indata* dsr_indata = &sp->dsr_indata;
	/* this rough calculation meets the SAS_XVGETI(...) requirement that # vars specified be >= Number actually read */
	DataSet->NumberOfVariables =
		DATA_NBVARS_MANDATORY + // allocate space for the mandatory variables
		DATA_NBVARS_OPTIONAL  + // allocate space for optional variables even if not specified by user. 
		num_dimensions;  // allocate space for the 1 or more dimension variables (user MUST specify >= 1)

	DataSet->Type = SDATASET_TYPE_INPUT;

	DataSet->Variable = STC_AllocateMemory (
		DataSet->NumberOfVariables * sizeof *DataSet->Variable);

	i = DATA_ID_INDEX;
	Position = (int *) sp->dsr_indata.dsr.VL_unit_id.positions;
	IOUtil_copy_varname(DataSet->Variable[i].Name, sp->dsr_indata.dsr.VL_unit_id.names[0]);
	DataSet->Variable[i].Position = Position[0];
	DataSet->Variable[i].Type = DATA_ID_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Size = S_NOT_SET;
	rc_io = SVariableDefineForInput(DataSet, &DataSet->Variable[i], &sp->dsr_indata.dsr, &dsr_indata->dsr.VL_unit_id.ptrs[0]);
	if (rc_io != IORC_SUCCESS) {
		return EIE_FAIL;
	}

	i = DATA_VAR_INDEX;
	Position = (int*)sp->dsr_indata.VL_var.positions;
	IOUtil_copy_varname(DataSet->Variable[i].Name, sp->dsr_indata.VL_var.names[0]);
	DataSet->Variable[i].Position = Position[0];
	DataSet->Variable[i].Type = DATA_VAR_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Size = S_NOT_SET;
	rc_io = SVariableDefineForInput(DataSet, &DataSet->Variable[i], &sp->dsr_indata.dsr, &dsr_indata->VL_var.ptrs[0]);
	if (rc_io != IORC_SUCCESS) {
		return EIE_FAIL;
	}

	i = DATA_SHADOW_INDEX;
	if (sp->shadow.meta.is_specified == IOSV_SPECIFIED) {
		Position = sp->dsr_indata.VL_shadow.positions;
		IOUtil_copy_varname(DataSet->Variable[i].Name, sp->dsr_indata.VL_shadow.names[0]);
		DataSet->Variable[i].Position = Position[0];
		DataSet->Variable[i].Type = DATA_SHADOW_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_NOT_MANDATORY;
		DataSet->Variable[i].Size = S_NOT_SET;
		DataSet->Variable[i].Present = EIE_TRUE;
		rc_io = SVariableDefineForInput(DataSet, &DataSet->Variable[i], &sp->dsr_indata.dsr, &dsr_indata->VL_shadow.ptrs[0]);
		if (rc_io != IORC_SUCCESS) {
			return EIE_FAIL;
		}
	}
	else {
		DataSet->Variable[i].Position = S_NOT_SET;
		DataSet->Variable[i].Type = DATA_SHADOW_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_NOT_MANDATORY;
		DataSet->Variable[i].Size = S_NOT_SET;
		DataSet->Variable[i].Present = EIE_FALSE;
	}

	i = DATA_WAIVER_INDEX;
	if (sp->waiver.meta.is_specified == IOSV_SPECIFIED) {
		Position = sp->dsr_indata.VL_waiver.positions;
		IOUtil_copy_varname(DataSet->Variable[i].Name, sp->dsr_indata.VL_waiver.names[0]);
		DataSet->Variable[i].Position = Position[0];
		DataSet->Variable[i].Type = DATA_WAIVER_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_NOT_MANDATORY;
		DataSet->Variable[i].Size = S_NOT_SET;
		DataSet->Variable[i].Present = EIE_TRUE;
		rc_io = SVariableDefineForInput(DataSet, &DataSet->Variable[i], &sp->dsr_indata.dsr, &dsr_indata->VL_waiver.ptrs[0]);
		if (rc_io != IORC_SUCCESS) {
			return EIE_FAIL;
		}
	}
	else if (sp->p_waiver.meta.is_specified == IOSV_SPECIFIED ) {
		Position = sp->dsr_indata.VL_p_waiver.positions;
		IOUtil_copy_varname(DataSet->Variable[i].Name, sp->dsr_indata.VL_p_waiver.names[0]);
		DataSet->Variable[i].Position = Position[0];
		DataSet->Variable[i].Type = DATA_WAIVER_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_NOT_MANDATORY;
		DataSet->Variable[i].Size = S_NOT_SET;
		DataSet->Variable[i].Present = EIE_TRUE;
		rc_io = SVariableDefineForInput(DataSet, &DataSet->Variable[i], &sp->dsr_indata.dsr, &dsr_indata->VL_p_waiver.ptrs[0]);
		if (rc_io != IORC_SUCCESS) {
			return EIE_FAIL;
		}
	}
	else {
		DataSet->Variable[i].Position = S_NOT_SET;
		DataSet->Variable[i].Type = DATA_WAIVER_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_NOT_MANDATORY;
		DataSet->Variable[i].Size = S_NOT_SET;
		DataSet->Variable[i].Present = EIE_FALSE;
	}

	i = DATA_PROXY_INDEX;
	if (sp->proxy_size.meta.is_specified == IOSV_SPECIFIED ) {
		Position = sp->dsr_indata.VL_proxy_size.positions;
		IOUtil_copy_varname(DataSet->Variable[i].Name, sp->dsr_indata.VL_proxy_size.names[0]);
		DataSet->Variable[i].Position = Position[0];
		DataSet->Variable[i].Type = DATA_PROXY_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_NOT_MANDATORY;
		DataSet->Variable[i].Size = S_NOT_SET;
		DataSet->Variable[i].Present = EIE_TRUE;
		rc_io = SVariableDefineForInput(DataSet, &DataSet->Variable[i], &sp->dsr_indata.dsr, &dsr_indata->VL_proxy_size.ptrs[0]);
		if (rc_io != IORC_SUCCESS) {
			return EIE_FAIL;
		}
	}
	else {
		DataSet->Variable[i].Position = S_NOT_SET;
		DataSet->Variable[i].Type = DATA_PROXY_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_NOT_MANDATORY;
		DataSet->Variable[i].Size = S_NOT_SET;
		DataSet->Variable[i].Present = EIE_FALSE;
	}

	i = DATA_WEIGHT_INDEX;
	if (sp->weight.meta.is_specified == IOSV_SPECIFIED) {
		Position = sp->dsr_indata.VL_weight.positions;
		IOUtil_copy_varname(DataSet->Variable[i].Name, sp->dsr_indata.VL_weight.names[0]);
		DataSet->Variable[i].Position = Position[0];
		DataSet->Variable[i].Type = DATA_WEIGHT_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_NOT_MANDATORY;
		DataSet->Variable[i].Size = S_NOT_SET;
		DataSet->Variable[i].Present = EIE_TRUE;
		rc_io = SVariableDefineForInput(DataSet, &DataSet->Variable[i], &sp->dsr_indata.dsr, &dsr_indata->VL_weight.ptrs[0]);
		if (rc_io != IORC_SUCCESS) {
			return EIE_FAIL;
		}
	}
	else {
		DataSet->Variable[i].Position = S_NOT_SET;
		DataSet->Variable[i].Type = DATA_WEIGHT_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_NOT_MANDATORY;
		DataSet->Variable[i].Size = S_NOT_SET;
		DataSet->Variable[i].Present = EIE_FALSE;
	}

	i = DATA_DIMENSION_INDEX;
	Position = sp->dsr_indata.VL_dimension.positions;
	for (j = 0; j < num_dimensions; i++, j++) {
		IOUtil_copy_varname(DataSet->Variable[i].Name, sp->dsr_indata.VL_dimension.names[j]);
		DataSet->Variable[i].Type = DATA_DIMENSION_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
		DataSet->Variable[i].Size = S_NOT_SET;
		rc_io = SVariableDefineForInput(DataSet, &DataSet->Variable[i], &sp->dsr_indata.dsr, &dsr_indata->VL_dimension.ptrs[j]);
		if (rc_io != IORC_SUCCESS) {
			return EIE_FAIL;
		}
	}

	return EIE_SUCCEED;
}
/*********************************************************************
Allocates variables for output data set CONSTRAINT
*********************************************************************/
static void DefineOutConstraintDataSet (
	tSDataSet * DataSet)
{
	int i;

	DataSet->NumberOfVariables = CONSTRAINT_NBVARS;
	DataSet->Type = SDATASET_TYPE_OUTPUT;

	DataSet->Variable = STC_AllocateMemory (
		DataSet->NumberOfVariables * sizeof *DataSet->Variable);

	i = CONSTRAINT_CONSTRAINTID_INDEX;
	strcpy (DataSet->Variable[i].Name, CONSTRAINT_CONSTRAINTID_NAME);
	DataSet->Variable[i].Type = CONSTRAINT_CONSTRAINTID_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = CONSTRAINT_CELLID_INDEX;
	strcpy (DataSet->Variable[i].Name, CONSTRAINT_CELLID_NAME);
	DataSet->Variable[i].Type = CONSTRAINT_CELLID_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = CONSTRAINT_COEFFICIENT_INDEX;
	strcpy (DataSet->Variable[i].Name, CONSTRAINT_COEFFICIENT_NAME);
	DataSet->Variable[i].Type = CONSTRAINT_COEFFICIENT_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);
}
/*********************************************************************
Allocates variables for output data set CELL
*********************************************************************/
static void DefineOutCellDataSet (
	SP_sensitiv* sp,
	tSDataSet * DataSet,
	STCT_HTREEROOT * HTreeRoot)//for dimension variables
{
	int i;
	int * Position;

	int weight_specified      = 0;
	int weight_included       = 0;

	int shadow_specified      = 0;
	int shadowtotal_included  = 0;

	int proxysize_specified   = 0;
	int totalproxy_included   = 0;

	int waivers_specified     = 0;
	int sensitivnwv_included  = 0;
	int favcost_included      = 0;

	int number_of_dimensions  = 0;

	int cell_first_code_index = 0;

	weight_specified     = (sp->dsr_indata.VL_weight.count != 0);
	shadow_specified     = (sp->dsr_indata.VL_shadow.count != 0);
	proxysize_specified  = (sp->dsr_indata.VL_proxy_size.count != 0);
	waivers_specified    = (sp->dsr_indata.VL_waiver.count != 0) || (sp->dsr_indata.VL_p_waiver.count != 0);
	number_of_dimensions = sp->dsr_indata.VL_dimension.count;

	if (weight_specified) {
		weight_included = 1;
	}
	if (shadow_specified) {
		shadowtotal_included = 1;
	}
	if (proxysize_specified) {
	    totalproxy_included = 1;
	}
	if (waivers_specified) {
	    sensitivnwv_included = 1;
	    favcost_included     = 1;
	}

	DataSet->NumberOfVariables = CELL_NBVARS(shadowtotal_included, totalproxy_included, sensitivnwv_included, favcost_included, number_of_dimensions, weight_included);
	DataSet->Type = SDATASET_TYPE_OUTPUT;

	DataSet->Variable = STC_AllocateMemory (
		DataSet->NumberOfVariables * sizeof *DataSet->Variable);

	i = CELL_CELLID_INDEX;
	strcpy (DataSet->Variable[i].Name, CELL_CELLID_NAME);
	DataSet->Variable[i].Type = CELL_CELLID_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = CELL_NBANONYM_INDEX;
	strcpy (DataSet->Variable[i].Name, CELL_NBANONYM_NAME);
	DataSet->Variable[i].Type = CELL_NBANONYM_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = CELL_ANONYM_INDEX;
	strcpy (DataSet->Variable[i].Name, CELL_ANONYM_NAME);
	DataSet->Variable[i].Type = CELL_ANONYM_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = CELL_NBRESPONDENTS_INDEX;
	strcpy (DataSet->Variable[i].Name, CELL_NBRESPONDENTS_NAME);
	DataSet->Variable[i].Type = CELL_NBRESPONDENTS_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);
	
	if (weight_specified) {
		i = CELL_WEIGHTEDNBRESPONDENTS_INDEX;
		strcpy(DataSet->Variable[i].Name, CELL_WEIGHTEDNBRESPONDENTS_NAME);
		DataSet->Variable[i].Type = CELL_WEIGHTEDNBRESPONDENTS_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
		DataSet->Variable[i].Position = S_NOT_SET;
		DataSet->Variable[i].Size = sizeof(double);
	}

	i = CELL_TOTAL_INDEX(weight_included);
	strcpy (DataSet->Variable[i].Name, CELL_TOTAL_NAME);
	DataSet->Variable[i].Type = CELL_TOTAL_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = CELL_SENSITIVITY_INDEX(weight_included);
	strcpy (DataSet->Variable[i].Name, CELL_SENSITIVITY_NAME);
	DataSet->Variable[i].Type = CELL_SENSITIVITY_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = CELL_STATUS_INDEX(weight_included);
	strcpy (DataSet->Variable[i].Name, CELL_STATUS_NAME);
	DataSet->Variable[i].Type = CELL_STATUS_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = 1;

	i = CELL_TYPE_INDEX(weight_included);
	strcpy (DataSet->Variable[i].Name, CELL_TYPE_NAME);
	DataSet->Variable[i].Type = CELL_TYPE_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = 1;

	i = CELL_TOTALNOISE_INDEX(weight_included);
	strcpy (DataSet->Variable[i].Name, CELL_TOTALNOISE_NAME);
	DataSet->Variable[i].Type = CELL_TOTALNOISE_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);
	/* } */

	if (shadow_specified) {
		i = CELL_SHADOWTOTAL_INDEX(weight_included);
		strcpy (DataSet->Variable[i].Name, CELL_SHADOWTOTAL_NAME);
		DataSet->Variable[i].Type = CELL_SHADOWTOTAL_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
		DataSet->Variable[i].Position = S_NOT_SET;
		DataSet->Variable[i].Size = sizeof (double);
	}

	if (proxysize_specified) {
		i = CELL_TOTALPROXY_INDEX(weight_included, shadowtotal_included);
		strcpy (DataSet->Variable[i].Name, CELL_TOTALPROXY_NAME);
		DataSet->Variable[i].Type = CELL_TOTALPROXY_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
		DataSet->Variable[i].Position = S_NOT_SET;
		DataSet->Variable[i].Size = sizeof (double);
	}

	if (waivers_specified) { //if "full" OR "partial" waivers specified
		i = CELL_SENSITIVNWV_INDEX(weight_included, shadowtotal_included, totalproxy_included);
		strcpy (DataSet->Variable[i].Name, CELL_SENSITIVNWV_NAME);
		DataSet->Variable[i].Type = CELL_SENSITIVNWV_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
		DataSet->Variable[i].Position = S_NOT_SET;
		DataSet->Variable[i].Size = sizeof (double);

		i = CELL_FAVCOST_INDEX(weight_included, shadowtotal_included, totalproxy_included, sensitivnwv_included);
		strcpy (DataSet->Variable[i].Name, CELL_FAVCOST_NAME);
		DataSet->Variable[i].Type = CELL_FAVCOST_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
		DataSet->Variable[i].Position = S_NOT_SET;
		DataSet->Variable[i].Size = sizeof (double);
	}

	Position = sp->dsr_indata.VL_dimension.positions;
	cell_first_code_index = CELL_FIRST_CODE_INDEX(weight_included, shadowtotal_included, totalproxy_included, sensitivnwv_included, favcost_included);
	for (i = 0; i < sp->dsr_indata.VL_dimension.count; i++) {
		IOUtil_copy_varname(DataSet->Variable[cell_first_code_index + i].Name, sp->dsr_indata.dsr.col_names[Position[i]]);
		DataSet->Variable[cell_first_code_index+i].Type = CELL_CODE_TYPE;
		DataSet->Variable[cell_first_code_index+i].Mandatory = SVARIABLE_MANDATORY;
		DataSet->Variable[cell_first_code_index+i].Position = S_NOT_SET;
		DataSet->Variable[cell_first_code_index+i].Size = (int) LargestCodeLenght (HTreeRoot, i);
	}
}
/*********************************************************************
Allocates variables for output data set LARGEST
*********************************************************************/
static void DefineOutLargestDataSet (
	SP_sensitiv* sp,
	tSDataSet * DataSet)
{
	int i;
	int * Position;
	int shadow_specified        = 0;
	int shadowtotal_included    = 0;
	int shadowpercent_included  = 0;
	int waivers_specified       = 0;
	int waiverflag_included     = 0;

	shadow_specified  = (sp->shadow.meta.is_specified == IOSV_SPECIFIED);
	waivers_specified = (((sp->dsr_indata.VL_waiver.count != 0) ||
	                      (sp->dsr_indata.VL_p_waiver.count != 0)
	                     ) ? 1 : 0
	                    );
	if (shadow_specified) {
	    shadowtotal_included   = 1;
	    shadowpercent_included = 1;
	}
	if (waivers_specified) {
	    waiverflag_included = 1;
	}

	DataSet->NumberOfVariables = LARGEST_NBVARS(shadowtotal_included, shadowpercent_included, waiverflag_included);
	DataSet->Type = SDATASET_TYPE_OUTPUT;

	DataSet->Variable = STC_AllocateMemory (
		DataSet->NumberOfVariables * sizeof *DataSet->Variable);

	i = LARGEST_CELLID_INDEX;
	strcpy (DataSet->Variable[i].Name, LARGEST_CELLID_NAME);
	DataSet->Variable[i].Type = LARGEST_CELLID_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = LARGEST_ID_INDEX;
	Position = sp->dsr_indata.dsr.VL_unit_id.positions;
	IOUtil_copy_varname(DataSet->Variable[i].Name, sp->dsr_indata.dsr.col_names[Position[0]]);
	DataSet->Variable[i].Type = LARGEST_ID_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = MAX (5, DSR_get_column_length(&sp->dsr_indata.dsr, Position[0]));
	
	i = LARGEST_NBRESPONDENTS_INDEX;
	strcpy (DataSet->Variable[i].Name, LARGEST_NBRESPONDENTS_NAME);
	DataSet->Variable[i].Type = LARGEST_NBRESPONDENTS_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = LARGEST_TOTAL_INDEX;
	strcpy (DataSet->Variable[i].Name, LARGEST_TOTAL_NAME);
	DataSet->Variable[i].Type = LARGEST_TOTAL_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = LARGEST_TOTALPERCENT_INDEX;
	strcpy (DataSet->Variable[i].Name, LARGEST_TOTALPERCENT_NAME);
	DataSet->Variable[i].Type = LARGEST_TOTALPERCENT_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	if (sp->shadow.meta.is_specified == IOSV_SPECIFIED) {
		i = LARGEST_SHADOWTOTAL_INDEX;
		strcpy (DataSet->Variable[i].Name, LARGEST_SHADOWTOTAL_NAME);
		DataSet->Variable[i].Type = LARGEST_SHADOWTOTAL_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
		DataSet->Variable[i].Position = S_NOT_SET;
		DataSet->Variable[i].Size = sizeof (double);

		i = LARGEST_SHADOWPERCENT_INDEX;
		strcpy (DataSet->Variable[i].Name, LARGEST_SHADOWPERCENT_NAME);
		DataSet->Variable[i].Type = LARGEST_SHADOWPERCENT_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
		DataSet->Variable[i].Position = S_NOT_SET;
		DataSet->Variable[i].Size = sizeof (double);
	}

	if ((sp->waiver.meta.is_specified == IOSV_SPECIFIED) || (sp->p_waiver.meta.is_specified == IOSV_SPECIFIED)) {
		i = LARGEST_WAIVERFLAG_INDEX(shadowtotal_included, shadowpercent_included);
		strcpy (DataSet->Variable[i].Name, LARGEST_WAIVERFLAG_NAME);
		DataSet->Variable[i].Type = LARGEST_WAIVERFLAG_TYPE;
		DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
		DataSet->Variable[i].Position = S_NOT_SET;
		DataSet->Variable[i].Size = sizeof (double);
	}
}
/*********************************************************************
Allocates variables for output data set TARGETS
*********************************************************************/
static void DefineOutTargetsDataSet (
	SP_sensitiv* sp,
	tSDataSet * DataSet)
{
	int i;
	int * Position;

	DataSet->NumberOfVariables = TARGETS_NBVARS;
	DataSet->Type = SDATASET_TYPE_OUTPUT;

	DataSet->Variable = STC_AllocateMemory (
		DataSet->NumberOfVariables * sizeof *DataSet->Variable);

	i = TARGETS_CELLID_INDEX;
	strcpy (DataSet->Variable[i].Name, TARGETS_CELLID_NAME);
	DataSet->Variable[i].Type = TARGETS_CELLID_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = TARGETS_ID_INDEX;
	Position = sp->dsr_indata.dsr.VL_unit_id.positions;
	IOUtil_copy_varname(DataSet->Variable[i].Name, sp->dsr_indata.dsr.col_names[Position[0]]);
	strcpy (DataSet->Variable[i].Name, TARGETS_ID_NAME);
	DataSet->Variable[i].Type = TARGETS_ID_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = (size_t)MAX (9, DSR_get_column_length(&sp->dsr_indata.dsr, Position[0]));

	i = TARGETS_PTNVARIABLE_INDEX;
	strcpy (DataSet->Variable[i].Name, TARGETS_PTNVARIABLE_NAME);
	DataSet->Variable[i].Type = TARGETS_PTNVARIABLE_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = (size_t)2;

	i = TARGETS_VALUE_INDEX;
	strcpy (DataSet->Variable[i].Name, TARGETS_VALUE_NAME);
	DataSet->Variable[i].Type = TARGETS_VALUE_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);
}
/*********************************************************************
Allocates variables for output data set PAIRS
*********************************************************************/
static void DefineOutPairsDataSet (
	SP_sensitiv* sp,
	tSDataSet * DataSet)
{
	int i;
	int * Position;

	int* id_positions = sp->dsr_indata.dsr.VL_unit_id.positions;

	int id_length = DSR_get_column_length(&sp->dsr_indata.dsr, sp->dsr_indata.dsr.VL_unit_id.positions[0]);

	DataSet->NumberOfVariables = PAIRS_NBVARS;
	DataSet->Type = SDATASET_TYPE_OUTPUT;

	DataSet->Variable = STC_AllocateMemory (
		DataSet->NumberOfVariables * sizeof *DataSet->Variable);

	i = PAIRS_CELLID_INDEX;
	strcpy (DataSet->Variable[i].Name, PAIRS_CELLID_NAME);
	DataSet->Variable[i].Type = PAIRS_CELLID_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = PAIRS_TARGETID_INDEX;
	Position = id_positions;
	strcpy (DataSet->Variable[i].Name, PAIRS_TARGETID_NAME);
	DataSet->Variable[i].Type = PAIRS_TARGETID_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = MAX (5, id_length);

	i = PAIRS_TARGETPT_INDEX;
	strcpy (DataSet->Variable[i].Name, PAIRS_TARGETPT_NAME);
	DataSet->Variable[i].Type = PAIRS_TARGETPT_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = PAIRS_ATTACKERID_INDEX;
	Position = id_positions;
	strcpy (DataSet->Variable[i].Name, PAIRS_ATTACKERID_NAME);
	DataSet->Variable[i].Type = PAIRS_ATTACKERID_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = MAX (PAIRS_ATTACKERID_SIZE_MIN, id_length);

	i = PAIRS_ATTACKERSN_INDEX;
	strcpy (DataSet->Variable[i].Name, PAIRS_ATTACKERSN_NAME);
	DataSet->Variable[i].Type = PAIRS_ATTACKERSN_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = PAIRS_REMAINDERCOUNT_INDEX;
	strcpy (DataSet->Variable[i].Name, PAIRS_REMAINDERCOUNT_NAME);
	DataSet->Variable[i].Type = PAIRS_REMAINDERCOUNT_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);

	i = PAIRS_REMAINDERN_INDEX;
	strcpy (DataSet->Variable[i].Name, PAIRS_REMAINDERN_NAME);
	DataSet->Variable[i].Type = PAIRS_REMAINDERN_TYPE;
	DataSet->Variable[i].Mandatory = SVARIABLE_MANDATORY;
	DataSet->Variable[i].Position = S_NOT_SET;
	DataSet->Variable[i].Size = sizeof (double);
}
/*********************************************************************
Verify that the same variable do not appear in more then one statement.
I do not care about the type (numeric or character) a statement can hold,
because if the type don't match, the variable list will not intersect.
*********************************************************************/
static EIT_RETURNCODE ExclusivityBetweenLists (
	DSR_indata* dsr_indata)
{
    EIT_RETURNCODE crc;
    int i, j;
    EIT_RETURNCODE rc;

    crc = EIE_SUCCEED;

	DS_varlist* varlists[7];

	int num_of_varlists = sizeof(varlists) / sizeof(varlists[0]);

	varlists[0] = &dsr_indata->dsr.VL_by_var;

	varlists[1] = &dsr_indata->dsr.VL_unit_id;

	varlists[2] = &dsr_indata->VL_var;

	varlists[3] = &dsr_indata->VL_shadow;

	varlists[4] = &dsr_indata->VL_dimension;

	varlists[5] = &dsr_indata->VL_waiver;

	varlists[6] = &dsr_indata->VL_p_waiver;

	for (i = 0; i < num_of_varlists; i++) {
		for (j = i+1; j < num_of_varlists; j++) {
			rc = ExclusivityBetweenListsOneAtATime (&dsr_indata->dsr,
				varlists[i],
				varlists[j]);
			if (rc != EIE_SUCCEED) crc = EIE_FAIL;
		}
	}

	return crc;
}
/*********************************************************************
Verify that the same variable do not appear in more then one statement.
*********************************************************************/
static EIT_RETURNCODE ExclusivityBetweenListsOneAtATime (
	DSR_generic* dsr,
	DS_varlist* vl_1,
	DS_varlist* vl_2)
{
    EIT_RETURNCODE rc;
    int VariablePosition;

    VariablePosition = Intersect (
		vl_1->positions,
		vl_1->count,
        vl_2->positions,
		vl_2->count
	);
	rc = EIE_SUCCEED;
    if (VariablePosition != NOTFOUND) {
        IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgVarNameInTwoStatementsExclusive,
            dsr->col_names[VariablePosition], vl_1->varlist_name, vl_2->varlist_name);
        rc = EIE_FAIL;
    }
	return rc;
}
/*********************************************************************
Get procedure parameters.
Set default value to parameter not specified.
*********************************************************************/
static EIT_RETURNCODE GetParms (
	SP_sensitiv* sp,
	tSDataSet * DataDataSet,
	tSDataSet * OutConstraintDataSet,
	tSDataSet * OutCellDataSet,
	tSDataSet * OutLargestDataSet,
	tSDataSet * OutTargetsDataSet,
	tSDataSet * OutPairsDataSet,
	PROGRAM_PARM * Parms)
{
	EIT_RETURNCODE retcode = EIE_SUCCEED;
	char default_weightprotlevel_abbreviation;
	
	/* Initialize the input and output files */
	strcpy (DataDataSet->Name, DATA_GRM_NAME);

	strcpy (OutConstraintDataSet->Name, OUTCONSTRAINT_GRM_NAME);

	strcpy (OutCellDataSet->Name, OUTCELL_GRM_NAME);

	strcpy (OutLargestDataSet->Name, OUTLARGEST_GRM_NAME);

	strcpy (OutTargetsDataSet->Name, OUTTARGETS_GRM_NAME);

	strcpy (OutPairsDataSet->Name, OUTPAIRS_GRM_NAME);

	if (sp->s_rule.meta.is_specified == IOSV_NOT_SPECIFIED)
		Parms->SRuleString = "";
	else
		Parms->SRuleString = (char *) sp->s_rule.value;

	if (sp->hierarchy.meta.is_specified == IOSV_NOT_SPECIFIED)
		Parms->HierarchyString = "";
	else
		Parms->HierarchyString = (char *) sp->hierarchy.value;

	if (sp->range.meta.is_specified == IOSV_NOT_SPECIFIED)
		Parms->RangeString = "";
	else
		Parms->RangeString = (char *) sp->range.value;

	// looks like "GroupString" was disabled some time ago, but not removed
	// therefore we just set it to an empty string as-if it's a non-specified parameter
	Parms->GroupString = "";

	if (sp->m.meta.is_specified == IOSV_NOT_SPECIFIED)
		Parms->M = M_DEFAULT_VALUE;
	else
		Parms->M = sp->m.value;

	if (sp->x.meta.is_specified == IOSV_NOT_SPECIFIED)
		Parms->X = X_DEFAULT_VALUE;
	else
		Parms->X = sp->x.value;

	if (sp->y.meta.is_specified == IOSV_NOT_SPECIFIED)
		Parms->Y = Y_DEFAULT_VALUE;
	else
		Parms->Y = sp->y.value;

	if (sp->z.meta.is_specified == IOSV_NOT_SPECIFIED)
		Parms->Z = Z_DEFAULT_VALUE;
	else
		Parms->Z = sp->z.value;

	if (sp->tolerance.meta.is_specified == IOSV_NOT_SPECIFIED)
		Parms->Tolerance = TOLERANCE_DEFAULT_VALUE;
	else
		Parms->Tolerance = sp->tolerance.value;

	if (sp->min_resp.meta.is_specified == IOSV_NOT_SPECIFIED)
		Parms->MinResp = MINRESP_DEFAULT_VALUE; /* not active */
	else {
		double MinResp;
		MinResp = sp->min_resp.value;
		if (MinResp > INT_MAX)
			Parms->MinResp = INT_MAX;
		else
			Parms->MinResp = (int) MinResp;
	}

	/* VERBOSE option */
	Parms->Verbose = (sp->verbose.meta.is_specified == IOSV_SPECIFIED ? 3 : 0);

	/* SHADOW (variable) parameter */
	if (sp->shadow.meta.is_specified == IOSV_SPECIFIED) {
		Parms->Shadow = 1;
	}
	else {
		Parms->Shadow = 0;
	}

	/* WEIGHT (variable) parameter */
	if (sp->weight.meta.is_specified == IOSV_SPECIFIED) {
		Parms->Weight = 1;
	}
	else {
		Parms->Weight = 0;
	}

	/* PROXYSIZE (variable) parameter */
	if (sp->proxy_size.meta.is_specified == IOSV_SPECIFIED) {
		Parms->Proxy  = 1;
	}
	else {
		Parms->Proxy  = 0;
	}

	/* PROXYRATIO parameter */
	if (sp->proxy_ratio.meta.is_specified == IOSV_NOT_SPECIFIED) {
		Parms->ProxyRatioSpecified = 0;
		Parms->ProxyRatio = PROXYRATIO_UNSPECD_VALUE;
	}
	else {
		Parms->ProxyRatioSpecified = 1;
		Parms->ProxyRatio = sp->proxy_ratio.value;
	}

	/* PROXYPERCENTILE parameter */
	if (sp->proxy_percentile.meta.is_specified == IOSV_NOT_SPECIFIED) {
		Parms->ProxyPercentile = PROXYPERCENTILE_DEFAULT_VALUE;
	}
	else {
		Parms->ProxyPercentile = sp->proxy_percentile.value;
	}

	/* PROXYDIAG option */
	Parms->ProxyDiagnostics = PROXYDIAG_DEFAULT_VALUE;
	if (sp->proxy_size.meta.is_specified == IOSV_SPECIFIED && sp->proxy_diag.meta.is_specified == IOSV_SPECIFIED) {
		if (sp->proxy_diag.value == IOB_FALSE) {
			Parms->ProxyDiagnostics = 0;//set to FALSE if NOPROXYDIAG specified
		}
		else if (sp->proxy_diag.value == IOB_TRUE) {
			Parms->ProxyDiagnostics = 1;//set to TRUE if PROXYDIAG specified
		}
	}

	default_weightprotlevel_abbreviation = (strlen (       WEIGHTPROTLEVEL_DEFAULT_VALUE    )== 3  &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[0]))=='L' &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[1]))=='O' &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[2]))=='W'   )?'L':(
	                                       (strlen (       WEIGHTPROTLEVEL_DEFAULT_VALUE    )== 6  &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[0]))=='L' &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[1]))=='I' &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[2]))=='N' &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[3]))=='E' &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[4]))=='A' &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[5]))=='R'   )?'M':(
	                                       (strlen (       WEIGHTPROTLEVEL_DEFAULT_VALUE    )== 4  &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[0]))=='S' &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[1]))=='T' &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[2]))=='E' &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[3]))=='P'   )?'H':(
	                                       (strlen (       WEIGHTPROTLEVEL_DEFAULT_VALUE    )== 5  &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[0]))=='E' &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[1]))=='X' &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[2]))=='A' &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[3]))=='C' &&
	                                        ((char)toupper(WEIGHTPROTLEVEL_DEFAULT_VALUE[4]))=='T'   )?'E':(
	                                        ' '))));
	if (sp->weight_prot_level.meta.is_specified == IOSV_NOT_SPECIFIED) {
		Parms->WeightProtectionLevel = default_weightprotlevel_abbreviation;
	}
	else {
		char* wpl = (char*) sp->weight_prot_level.value;
		Parms->WeightProtectionLevel = (strlen (       ((char *) wpl)    )== 3  &&
		                                ((char)toupper(((char *) wpl)[0]))=='L' &&
		                                ((char)toupper(((char *) wpl)[1]))=='O' &&
		                                ((char)toupper(((char *) wpl)[2]))=='W'   )?'L':(
		                               (strlen (       ((char *) wpl)    )== 6  &&
		                                ((char)toupper(((char *) wpl)[0]))=='L' &&
		                                ((char)toupper(((char *) wpl)[1]))=='I' &&
		                                ((char)toupper(((char *) wpl)[2]))=='N' &&
		                                ((char)toupper(((char *) wpl)[3]))=='E' &&
		                                ((char)toupper(((char *) wpl)[4]))=='A' &&
		                                ((char)toupper(((char *) wpl)[5]))=='R'   )?'M':(
		                               (strlen (       ((char *) wpl)    )== 4  &&
		                                ((char)toupper(((char *) wpl)[0]))=='S' &&
		                                ((char)toupper(((char *) wpl)[1]))=='T' &&
		                                ((char)toupper(((char *) wpl)[2]))=='E' &&
		                                ((char)toupper(((char *) wpl)[3]))=='P'   )?'H':(
		                               (strlen (       ((char *) wpl)    )== 5  &&
		                                ((char)toupper(((char *) wpl)[0]))=='E' &&
		                                ((char)toupper(((char *) wpl)[1]))=='X' &&
		                                ((char)toupper(((char *) wpl)[2]))=='A' &&
		                                ((char)toupper(((char *) wpl)[3]))=='C' &&
		                                ((char)toupper(((char *) wpl)[4]))=='T'   )?'E':(
		                                default_weightprotlevel_abbreviation))));
	}

	/* MINRESPW parameter */
	if (sp->min_resp_w.meta.is_specified == IOSV_NOT_SPECIFIED) {
		Parms->MinRespW = MINRESPW_UNSPECD_VALUE;
	}
	else {
		Parms->MinRespW = sp->min_resp_w.value;
	}

	/* ADDITIVENOISE option */
	if (sp->additive_noise.meta.is_specified == IOSV_NOT_SPECIFIED) {
		Parms->AdditiveNoise = ADDITIVENOISE_DEFAULT_VALUE;
	}
	else {
		if (sp->additive_noise.value == IOB_FALSE) {
			Parms->AdditiveNoise = 0;//set to FALSE if NOADDITIVENOISE specified
		}
		else if (sp->additive_noise.value == IOB_TRUE) {
			Parms->AdditiveNoise = 1;//set to TRUE if ADDITIVENOISE specified
		}
	}

	/* WEIGHTDIAG option */
	if (sp->weight.meta.is_specified == IOSV_NOT_SPECIFIED) {
		Parms->WeightDiagnostics = WEIGHTDIAG_DEFAULT_VALUE;
	}
	else {
		if (sp->weight.value == IOB_FALSE) {
			Parms->WeightDiagnostics = 0;//set to FALSE if NOWEIGHTDIAG specified
		}
		else if (sp->weight.value == IOB_TRUE) {
			Parms->WeightDiagnostics = 1;//set to TRUE if WEIGHTDIAG specified
		}
	}

	/* ACCEPTNEGATIVE option */
	if (sp->accept_negative.meta.is_specified == IOSV_NOT_SPECIFIED) {
		Parms->AcceptNegativeValues = ACCEPTNEGATIVE_DEFAULT_VALUE;
	}
	else {
		if (sp->accept_negative.value == IOB_FALSE) {
			Parms->AcceptNegativeValues = 0;//set to FALSE if REJECTNEGATIVE specified
		}
		else if (sp->accept_negative.value == IOB_TRUE) {
			Parms->AcceptNegativeValues = 1;//set to TRUE if ACCEPTNEGATIVE specified
		}
	}

	/* PRINTCODES OPTION */
	if (sp->print_codes.meta.is_specified == IOSV_NOT_SPECIFIED) {
		Parms->PrintCodes = EIE_TRUE;//default to true
	}
	else {
		if (sp->print_codes.value == IOB_FALSE)
			Parms->PrintCodes = EIE_FALSE;//set to FALSE if NOPRINTCODES specified
		else if (sp->print_codes.value == IOB_TRUE)
			Parms->PrintCodes = EIE_TRUE;//set to TRUE if PRINTCODES specified, even if NOPRINTCODES was also specified
	}

	/* LIMITWARNINGS OPTION */
	if (sp->limit_warnings.meta.is_specified == IOSV_NOT_SPECIFIED) {
		Parms->MessageQuota = MESSAGE_QUOTA;//default is to limit the number of warnings
	}
	else {
		if (sp->limit_warnings.value == IOB_FALSE)
			Parms->MessageQuota = MESSAGE_NO_QUOTA;//if NOLIMITWARNINGS specified, do not limit the number of warning messages printed
		else if (sp->limit_warnings.value == IOB_TRUE)
			Parms->MessageQuota = MESSAGE_QUOTA;//if LIMITWARNINGS specified, limit the number of warning messages printed
	}
	
	return retcode;
}
/*------------------------------------------------------------------------------
Look for duplicate value that appear in both lists.
Returns the first duplicate value it finds,
NOTFOUND otherwise.
------------------------------------------------------------------------------*/
static int Intersect (
    int * List1,
    int nList1,
    int * List2,
    int nList2)
{
    int i;
    int j;

    for (i = 0; i < nList1; i++)
        for (j = 0; j < nList2; j++)
            if (List1[i] == List2[j])
                return List1[i];
    return NOTFOUND;
}
/*********************************************************************
validate code, make sure it contains the right code set
*********************************************************************/
static EIT_BOOLEAN IsCodeValid (
	char * Code,
	char * CodeFirstCharacterCharacterSet,
	char * CodeCharacterSet)
{
	size_t n;
	n = strspn (Code, CodeFirstCharacterCharacterSet);
	if (n > 0)
		n = strspn (Code+1, CodeCharacterSet) + 1;
	return n == strlen (Code) ? EIE_TRUE : EIE_FALSE;
}
/*********************************************************************
Get the length of the largest code
*********************************************************************/
static size_t LargestCodeLenght (
	STCT_HTREEROOT * HTreeRoot,
	int iDimension)
{
	int i;
	size_t MaxSize;
	size_t Size;

	MaxSize = 0;
	for (i = 0; i <= HTreeRoot->LargestTag[iDimension]; i++) {
		if (HTreeRoot->TagIndex[iDimension][i] != NULL) {
			Size = strlen (HTreeRoot->TagIndex[iDimension][i]->Code);
			if (Size > MaxSize)
				MaxSize = Size;
		}
	}
	return MaxSize;
}
/*********************************************************************
Print the values of the parameters to the LOG
*********************************************************************/
static void PrintParms (
	SP_sensitiv* sp,
	PROGRAM_PARM * Parms)
{
	SUtil_PrintInputDataSetInfo (&sp->dsr_indata.dsr);
	SUtil_PrintOutputDataSetInfo(&sp->dsw_outcell);
	SUtil_PrintOutputDataSetInfo(&sp->dsw_outconstraint);
	SUtil_PrintOutputDataSetInfo(&sp->dsw_outlargest);
	SUtil_PrintOutputDataSetInfo(&sp->dsw_outpairs);
	SUtil_PrintOutputDataSetInfo(&sp->dsw_outtargets);

	if (strcmp (Parms->SRuleString, "") == 0)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmNotSpecified, SRULE_GRM_NAME);
	else
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualString, SRULE_GRM_NAME, Parms->SRuleString);

	if (strcmp (Parms->HierarchyString, "") == 0)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmNotSpecified, HIERARCHY_GRM_NAME);
	else
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualString, HIERARCHY_GRM_NAME, Parms->HierarchyString);

	if (strcmp (Parms->RangeString, "") == 0)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmNotSpecified, RANGE_GRM_NAME);
	else
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualString, RANGE_GRM_NAME, Parms->RangeString);

	if (sp->m.meta.is_specified == IOSV_NOT_SPECIFIED)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualIntegerDefault, M_GRM_NAME, Parms->M);
	else
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualInteger, M_GRM_NAME, Parms->M);

	if (sp->x.meta.is_specified == IOSV_NOT_SPECIFIED)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualDoubleDefault, X_GRM_NAME, DEFAULT_NB_DECIMALS, Parms->X);
	else
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualDouble, X_GRM_NAME, DEFAULT_NB_DECIMALS, Parms->X);

	if (sp->y.meta.is_specified == IOSV_NOT_SPECIFIED)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualDoubleDefault, Y_GRM_NAME, DEFAULT_NB_DECIMALS, Parms->Y);
	else
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualDouble, Y_GRM_NAME, DEFAULT_NB_DECIMALS, Parms->Y);

	if (sp->z.meta.is_specified == IOSV_NOT_SPECIFIED)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualDoubleDefault, Z_GRM_NAME, DEFAULT_NB_DECIMALS, Parms->Z);
	else
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualDouble, Z_GRM_NAME, DEFAULT_NB_DECIMALS, Parms->Z);

	if (sp->tolerance.meta.is_specified == IOSV_NOT_SPECIFIED)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualDoubleDefault, TOLERANCE_GRM_NAME, DEFAULT_NB_DECIMALS, Parms->Tolerance);
	else
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualDouble, TOLERANCE_GRM_NAME, DEFAULT_NB_DECIMALS, Parms->Tolerance);

	if (sp->min_resp.meta.is_specified == IOSV_NOT_SPECIFIED)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmNotSpecified, MINRESP_GRM_NAME);
	else
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualInteger, MINRESP_GRM_NAME, Parms->MinResp);

	if (sp->proxy_size.meta.is_specified == IOSV_SPECIFIED) {
		/* PROXYRATIO parameter */
		if (sp->proxy_ratio.meta.is_specified == IOSV_NOT_SPECIFIED) {
			IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmNotSpecified, PROXYRATIO_GRM_NAME);
		}
		else {
			IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualDouble, PROXYRATIO_GRM_NAME, DEFAULT_NB_DECIMALS, Parms->ProxyRatio);
		}

		/* PROXYPERCENTILE parameter */
		if (sp->proxy_ratio.meta.is_specified == IOSV_NOT_SPECIFIED) {
			/*-the specification says not to display the value of the proxypercentile if the user specifies a valid value for the      */
			/* proxyratio; if the user specifies an invalid value for proxyratio then the program will be aborted in "ValidateParms()" */
			/* when the validity of the value specified for proxyratio is checked (if one was specified), so control won't reach here: */
			if (sp->proxy_percentile.meta.is_specified == IOSV_NOT_SPECIFIED) {
				IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualDoubleDefault, PROXYPERCENTILE_GRM_NAME, DEFAULT_NB_DECIMALS, Parms->ProxyPercentile);
			}
			else {
				IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualDouble, PROXYPERCENTILE_GRM_NAME, DEFAULT_NB_DECIMALS, Parms->ProxyPercentile);
			}
		}
	}

	/* PROXYDIAG option */
	if (Parms->ProxyDiagnostics == 0) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE PROXYDIAG_GRM_NAME " = False");
	}
	else {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE PROXYDIAG_GRM_NAME " = True");
	}

	/* WEIGHTPROTLEVEL parameter */
	if (sp->weight_prot_level.meta.is_specified == IOSV_NOT_SPECIFIED) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualString, WEIGHTPROTLEVEL_GRM_NAME, WEIGHTPROTLEVEL_DEFAULT_VALUE);
	}
	else {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualString, WEIGHTPROTLEVEL_GRM_NAME, ((char *) sp->weight_prot_level.value));
	}

	/* MINRESPW parameter */
	if (sp->min_resp_w.meta.is_specified == IOSV_NOT_SPECIFIED) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmNotSpecified, MINRESPW_GRM_NAME);
	}
	else {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgParmEqualDouble, MINRESPW_GRM_NAME, DEFAULT_NB_DECIMALS, Parms->MinRespW);
	}

	/* ADDITIVENOISE option */
	if (Parms->AdditiveNoise == 0) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE ADDITIVENOISE_GRM_NAME " = False");
	}
	else {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE ADDITIVENOISE_GRM_NAME " = True");
	}

	/* WEIGHTDIAG option */
	if (Parms->WeightDiagnostics == 0) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE WEIGHTDIAG_GRM_NAME " = False");
	}
	else {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE WEIGHTDIAG_GRM_NAME " = True");
	}

	/* ACCEPTNEGATIVE option */
	if (Parms->AcceptNegativeValues == 0) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE ACCEPTNEGATIVE_GRM_NAME " = False");
	}
	else {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE ACCEPTNEGATIVE_GRM_NAME " = True");
	}

	/* List variables */
	SUtil_PrintStatementVars (&sp->unit_id.meta, &sp->dsr_indata.dsr, &sp->dsr_indata.dsr.VL_unit_id, GPN_UNIT_ID);
	SUtil_PrintStatementVars (&sp->var.meta, &sp->dsr_indata.dsr, &sp->dsr_indata.VL_var, GPN_VAR);
	SUtil_PrintStatementVars (&sp->shadow.meta, &sp->dsr_indata.dsr, &sp->dsr_indata.VL_shadow, GPN_SHADOW);
	SUtil_PrintStatementVars (&sp->proxy_size.meta, &sp->dsr_indata.dsr, &sp->dsr_indata.VL_proxy_size, GPN_PROXY_SIZE);
	SUtil_PrintStatementVars (&sp->weight.meta, &sp->dsr_indata.dsr, &sp->dsr_indata.VL_weight, GPN_WEIGHT);
	SUtil_PrintStatementVars (&sp->dimension.meta, &sp->dsr_indata.dsr, &sp->dsr_indata.VL_dimension, GPN_DIMENSION);
	SUtil_PrintStatementVars (&sp->waiver.meta, &sp->dsr_indata.dsr, &sp->dsr_indata.VL_waiver, GPN_WAIVER);
	SUtil_PrintStatementVars (&sp->p_waiver.meta, &sp->dsr_indata.dsr, &sp->dsr_indata.VL_p_waiver, GPN_P_WAIVER);
	SUtil_PrintStatementVars (&sp->by.meta, &sp->dsr_indata.dsr, &sp->dsr_indata.dsr.VL_by_var, GPN_BY);

    /* VERBOSE option */
    if (Parms->Verbose) IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE VERBOSE_GRM_NAME);
    /* TIMER option */
    if (sp->timer.value) IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE TIMER_GRM_NAME);
    /* PRINTCODES option */
    if (Parms->PrintCodes) IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE PRINTCODES_GRM_NAME " = True");
	else IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE PRINTCODES_GRM_NAME " = False");
    /* LIMITWARNINGS option */
    if (Parms->MessageQuota == MESSAGE_QUOTA) IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE LIMITWARNINGS_GRM_NAME " = True");
	else IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE LIMITWARNINGS_GRM_NAME " = False");

	IO_PRINT_LINE ("");
}

static EIT_RETURNCODE CalculateProxyRatio (
    double ProxyPercentile,
    STCT_CELLSET * CellSet,
    double * ProxyRatioPtr)
{
    int    c;
    int    i;
    int    TotalEligibleEntries;
    double percentile_index_float;
    int    percentile_index_integer_part;
    double percentile_index_fraction_part;
    int    percentile_index;
    int    HeapSize;
    int    HeapEntries;
    int    parent_index;
    int    child_index;
    int    child_index_1;
    int    child_index_2;
    int    previous_parent_index;
    int    previous_child_index;
    int    previous_child_index_1;
    int    previous_child_index_2;
    int    heap_property_holds;
    int    parent_ndx;
    int    child_ndx;
    STCT_DATAITEM * DataItemPtr;
    
    double delta;
    double temp;
    double * Heap;
    double smallest_discarded_delta;
    
    int verify_heap_property_after_each_insertion = 0;
    
    EIT_RETURNCODE ReturnCode = EIE_SUCCEED;
    
    /*-iterate over the cells in CellSet (it contains only the internal cells at this point), */
    /* in each cell combine all STCT_DATAITEM instances in the cell's "Data" member having    */
    /* the same key into a single STCT_DATAITEM instance, then count the number of entries    */
    /* for which delta will have to be calculated:                                            */
    TotalEligibleEntries = 0;
    for(c=0;c<CellSet->NumberEntries;c=c+1) {
        SortKeys (CellSet->Cell[c]);
        RemoveDuplicateKeys(CellSet->Cell[c]);
        for(i=0;i<=CellSet->Cell[c]->Data.NumberEntries;i=i+1) {
            if (i==CellSet->Cell[c]->Data.NumberEntries) {
                DataItemPtr = &(CellSet->Cell[c]->AnonymousDataItem);
                if (DataItemPtr->NumberObservations == 0) {
                    continue;
                }
            }
            else {
                DataItemPtr = CellSet->Cell[c]->Data.Item[i];
            }
            TotalEligibleEntries = TotalEligibleEntries + 1;
        }
    }
    percentile_index_float         = TotalEligibleEntries * ProxyPercentile;
    percentile_index_integer_part  = (int)percentile_index_float;
    percentile_index_fraction_part = percentile_index_float - percentile_index_integer_part;
    if (percentile_index_fraction_part != .5) {
        if (percentile_index_fraction_part < .5) {
            /*-percentile_index_fraction_part < .5: */
            percentile_index = percentile_index_integer_part  ;
        }
        else {
            /*-percentile_index_fraction_part > .5: */
            percentile_index = percentile_index_integer_part+1;
        }
    }
    else {
        /*-percentile_index_fraction_part == .5: */
        if ((percentile_index_integer_part % 2) == 0) {
            /*-percentile_index_integer_part is even: */
            percentile_index = percentile_index_integer_part  ;
        }
        else {
            /*-percentile_index_integer_part is odd : */
            percentile_index = percentile_index_integer_part+1;
        }
    }
    HeapSize = (1>percentile_index)?1:percentile_index;
    
    Heap = STC_AllocateMemory(HeapSize * sizeof(double));
    if (Heap == NULL) {
        IO_PRINT_LINE(MsgMemoryErrorCalculateProxyRatio);
        ReturnCode = EIE_FAIL;
        goto error_cleanup;
    }
    HeapEntries = 0;
    smallest_discarded_delta = 1.0;
    for(c=0;c<CellSet->NumberEntries;c=c+1) {
        for(i=0;i<=CellSet->Cell[c]->Data.NumberEntries;i=i+1) {
            if (i==CellSet->Cell[c]->Data.NumberEntries) {
                DataItemPtr = &(CellSet->Cell[c]->AnonymousDataItem);
                if (DataItemPtr->NumberObservations == 0) {
                    continue;
                }
            }
            else {
                DataItemPtr = CellSet->Cell[c]->Data.Item[i];
            }
            if (fabs(DataItemPtr->Value) > fabs(DataItemPtr->Proxy)) {
                delta = 1.0;
            }
            else if (DataItemPtr->Value == 0.0 || DataItemPtr->Proxy == 0.0) {
                delta = 0.0;
            }
            else {
                delta = fabs(DataItemPtr->Value) / fabs(DataItemPtr->Proxy);
            }
            if (HeapEntries < HeapSize) {
                /*-insert delta in the first unused entry in the heap immediately following the last used entry, */
                /* then sift it down until the heap property is restored:                                        */
                Heap[HeapEntries] = delta;
                HeapEntries = HeapEntries + 1;
                /*-sift down: */
                child_index = HeapEntries - 1;
                parent_index = (((child_index+1)-((child_index+1) % 2)) / 2)-1;
                while (child_index > 0 && Heap[child_index] > Heap[parent_index]) {
                    temp = Heap[parent_index];
                    Heap[parent_index] = Heap[child_index];
                    Heap[child_index] = temp;
                    previous_child_index = child_index;
                    previous_parent_index = parent_index;
                    child_index = parent_index;
                    parent_index = (((child_index+1)-((child_index+1) % 2)) / 2)-1;
                }
            }
            else { /* (HeapEntries == HeapSize) */
                /*-if the first entry in the heap is larger than delta then replace it with delta, */
                /* then sift it up until the heap property is restored:                            */
                if (delta < Heap[0]) {
                    if (Heap[0] < smallest_discarded_delta) {
                        smallest_discarded_delta = Heap[0];
                    }
                    Heap[0] = delta;
                    /*-sift up: */
                    parent_index = 0;
                    child_index_1 = (2*(parent_index+1)+0)-1;
                    child_index_2 = (2*(parent_index+1)+1)-1;
                    while ((child_index_1 <= (HeapSize-1) && Heap[child_index_1] > Heap[parent_index]) || (child_index_2 <= (HeapSize-1) && Heap[child_index_2] > Heap[parent_index])) {
                        if      (!(child_index_2 <= (HeapSize-1) && Heap[child_index_2] > Heap[parent_index])) {
                            temp = Heap[parent_index];
                            Heap[parent_index] = Heap[child_index_1];
                            Heap[child_index_1] = temp;
                            previous_parent_index = parent_index;
                            parent_index = child_index_1;
                        }
                        else if (!(child_index_1 <= (HeapSize-1) && Heap[child_index_1] > Heap[parent_index])) {
                            temp = Heap[parent_index];
                            Heap[parent_index] = Heap[child_index_2];
                            Heap[child_index_2] = temp;
                            previous_parent_index = parent_index;
                            parent_index = child_index_2;
                        }
                        else { /* ((child_index_1 <= (HeapSize-1) && Heap[child_index_1] > Heap[parent_index]) && (child_index_2 <= (HeapSize-1) && Heap[child_index_2] > Heap[parent_index])) */
                            if (Heap[child_index_1] > Heap[child_index_2]) {
                                temp = Heap[parent_index];
                                Heap[parent_index] = Heap[child_index_1];
                                Heap[child_index_1] = temp;
                                previous_parent_index = parent_index;
                                parent_index = child_index_1;
                            }
                            else { /* (Heap[child_index_1] <= Heap[child_index_2]) */
                                temp = Heap[parent_index];
                                Heap[parent_index] = Heap[child_index_2];
                                Heap[child_index_2] = temp;
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
            if (verify_heap_property_after_each_insertion) {
                /*-verify that the heap property holds: */
                heap_property_holds = 1;
                for(child_ndx=1;child_ndx<HeapEntries;child_ndx=child_ndx+1) {
                    parent_ndx = (((child_ndx+1)-((child_ndx+1) % 2)) / 2)-1;
                    if (Heap[child_ndx] > Heap[parent_ndx]) {
                        heap_property_holds = 0;
                        IO_PRINT_LINE(MsgMemoryError, c, i, child_ndx, parent_ndx, Heap[child_ndx], Heap[parent_ndx]);
                    }
                }
            }
        }
    }
    *ProxyRatioPtr = Heap[0];
    
    goto normal_cleanup;
    
error_cleanup:
    
normal_cleanup:
    if (Heap != NULL) {
        STC_FreeMemory(Heap);
        Heap = NULL;
    }
    
    return ReturnCode;
}
static EIT_RETURNCODE GenerateWeightAndProxyDiagnostics (STCT_CELLSET * CellSet, int GenerateWeightDiagnostics, int GenerateProxyDiagnostics)
{
    EIT_RETURNCODE RetCode;
    STCT_CELL * Cell;
    
    /*-weight diagnostic sensitivity change counts: */
    int wght_diag_int_sens_decreased, wght_diag_int_sens_unchanged, wght_diag_int_sens_increased;
    int wght_diag_mrg_sens_decreased, wght_diag_mrg_sens_unchanged, wght_diag_mrg_sens_increased;
    int wght_diag_all_sens_decreased, wght_diag_all_sens_unchanged, wght_diag_all_sens_increased;
    
    /*-weight diagnostic status counts: */
    int wght_diag_int_unwghted_safe, wght_diag_int_unwghted_sens, wght_diag_int_weighted_safe, wght_diag_int_weighted_sens;
    int wght_diag_mrg_unwghted_safe, wght_diag_mrg_unwghted_sens, wght_diag_mrg_weighted_safe, wght_diag_mrg_weighted_sens;
    int wght_diag_agg_unwghted_safe, wght_diag_agg_unwghted_sens, wght_diag_agg_weighted_safe, wght_diag_agg_weighted_sens;
    int wght_diag_all_unwghted_safe, wght_diag_all_unwghted_sens, wght_diag_all_weighted_safe, wght_diag_all_weighted_sens;
    
    /*-proxy  diagnostic sensitivity change counts: */
    int prxy_diag_int_all4_sens_decreased, prxy_diag_int_all4_sens_unchanged, prxy_diag_int_all4_sens_increased;
    int prxy_diag_int_mixd_sens_decreased, prxy_diag_int_mixd_sens_unchanged, prxy_diag_int_mixd_sens_increased;
    int prxy_diag_int_nneg_sens_decreased, prxy_diag_int_nneg_sens_unchanged, prxy_diag_int_nneg_sens_increased;
    int prxy_diag_int_zero_sens_decreased, prxy_diag_int_zero_sens_unchanged, prxy_diag_int_zero_sens_increased;
    int prxy_diag_int_npos_sens_decreased, prxy_diag_int_npos_sens_unchanged, prxy_diag_int_npos_sens_increased;
    
    int prxy_diag_mrg_all4_sens_decreased, prxy_diag_mrg_all4_sens_unchanged, prxy_diag_mrg_all4_sens_increased;
    int prxy_diag_mrg_mixd_sens_decreased, prxy_diag_mrg_mixd_sens_unchanged, prxy_diag_mrg_mixd_sens_increased;
    int prxy_diag_mrg_nneg_sens_decreased, prxy_diag_mrg_nneg_sens_unchanged, prxy_diag_mrg_nneg_sens_increased;
    int prxy_diag_mrg_zero_sens_decreased, prxy_diag_mrg_zero_sens_unchanged, prxy_diag_mrg_zero_sens_increased;
    int prxy_diag_mrg_npos_sens_decreased, prxy_diag_mrg_npos_sens_unchanged, prxy_diag_mrg_npos_sens_increased;
    
    int prxy_diag_all_all4_sens_decreased, prxy_diag_all_all4_sens_unchanged, prxy_diag_all_all4_sens_increased;
    int prxy_diag_all_mixd_sens_decreased, prxy_diag_all_mixd_sens_unchanged, prxy_diag_all_mixd_sens_increased;
    int prxy_diag_all_nneg_sens_decreased, prxy_diag_all_nneg_sens_unchanged, prxy_diag_all_nneg_sens_increased;
    int prxy_diag_all_zero_sens_decreased, prxy_diag_all_zero_sens_unchanged, prxy_diag_all_zero_sens_increased;
    int prxy_diag_all_npos_sens_decreased, prxy_diag_all_npos_sens_unchanged, prxy_diag_all_npos_sens_increased;
    
    /*-proxy  diagnostic status counts: */
    int prxy_diag_int_all4_wnoproxy_safe, prxy_diag_int_all4_wnoproxy_sens, prxy_diag_int_all4_wthproxy_safe, prxy_diag_int_all4_wthproxy_sens;
    int prxy_diag_int_mixd_wnoproxy_safe, prxy_diag_int_mixd_wnoproxy_sens, prxy_diag_int_mixd_wthproxy_safe, prxy_diag_int_mixd_wthproxy_sens;
    int prxy_diag_int_nneg_wnoproxy_safe, prxy_diag_int_nneg_wnoproxy_sens, prxy_diag_int_nneg_wthproxy_safe, prxy_diag_int_nneg_wthproxy_sens;
    int prxy_diag_int_zero_wnoproxy_safe, prxy_diag_int_zero_wnoproxy_sens, prxy_diag_int_zero_wthproxy_safe, prxy_diag_int_zero_wthproxy_sens;
    int prxy_diag_int_npos_wnoproxy_safe, prxy_diag_int_npos_wnoproxy_sens, prxy_diag_int_npos_wthproxy_safe, prxy_diag_int_npos_wthproxy_sens;
    
    int prxy_diag_mrg_all4_wnoproxy_safe, prxy_diag_mrg_all4_wnoproxy_sens, prxy_diag_mrg_all4_wthproxy_safe, prxy_diag_mrg_all4_wthproxy_sens;
    int prxy_diag_mrg_mixd_wnoproxy_safe, prxy_diag_mrg_mixd_wnoproxy_sens, prxy_diag_mrg_mixd_wthproxy_safe, prxy_diag_mrg_mixd_wthproxy_sens;
    int prxy_diag_mrg_nneg_wnoproxy_safe, prxy_diag_mrg_nneg_wnoproxy_sens, prxy_diag_mrg_nneg_wthproxy_safe, prxy_diag_mrg_nneg_wthproxy_sens;
    int prxy_diag_mrg_zero_wnoproxy_safe, prxy_diag_mrg_zero_wnoproxy_sens, prxy_diag_mrg_zero_wthproxy_safe, prxy_diag_mrg_zero_wthproxy_sens;
    int prxy_diag_mrg_npos_wnoproxy_safe, prxy_diag_mrg_npos_wnoproxy_sens, prxy_diag_mrg_npos_wthproxy_safe, prxy_diag_mrg_npos_wthproxy_sens;
    
    int prxy_diag_agg_all4_wnoproxy_safe, prxy_diag_agg_all4_wnoproxy_sens, prxy_diag_agg_all4_wthproxy_safe, prxy_diag_agg_all4_wthproxy_sens;
    
    int prxy_diag_all_all4_wnoproxy_safe, prxy_diag_all_all4_wnoproxy_sens, prxy_diag_all_all4_wthproxy_safe, prxy_diag_all_all4_wthproxy_sens;
    int prxy_diag_all_mixd_wnoproxy_safe, prxy_diag_all_mixd_wnoproxy_sens, prxy_diag_all_mixd_wthproxy_safe, prxy_diag_all_mixd_wthproxy_sens;
    int prxy_diag_all_nneg_wnoproxy_safe, prxy_diag_all_nneg_wnoproxy_sens, prxy_diag_all_nneg_wthproxy_safe, prxy_diag_all_nneg_wthproxy_sens;
    int prxy_diag_all_zero_wnoproxy_safe, prxy_diag_all_zero_wnoproxy_sens, prxy_diag_all_zero_wthproxy_safe, prxy_diag_all_zero_wthproxy_sens;
    int prxy_diag_all_npos_wnoproxy_safe, prxy_diag_all_npos_wnoproxy_sens, prxy_diag_all_npos_wthproxy_safe, prxy_diag_all_npos_wthproxy_sens;
    
    char CellType;
    int  SensChngSignum;
    int  IsSens;
    int  IsSensWoWts;
    int  IsSensWoPxy;
    int  i;
    int  MxdSgnStatus;
    
    RetCode = EIE_SUCCEED;
    
    wght_diag_int_sens_decreased = 0; wght_diag_int_sens_unchanged = 0; wght_diag_int_sens_increased = 0;
    wght_diag_mrg_sens_decreased = 0; wght_diag_mrg_sens_unchanged = 0; wght_diag_mrg_sens_increased = 0;
    wght_diag_all_sens_decreased = 0; wght_diag_all_sens_unchanged = 0; wght_diag_all_sens_increased = 0;
    
    wght_diag_int_unwghted_safe = 0; wght_diag_int_unwghted_sens = 0; wght_diag_int_weighted_safe = 0; wght_diag_int_weighted_sens = 0;
    wght_diag_mrg_unwghted_safe = 0; wght_diag_mrg_unwghted_sens = 0; wght_diag_mrg_weighted_safe = 0; wght_diag_mrg_weighted_sens = 0;
    wght_diag_agg_unwghted_safe = 0; wght_diag_agg_unwghted_sens = 0; wght_diag_agg_weighted_safe = 0; wght_diag_agg_weighted_sens = 0;
    wght_diag_all_unwghted_safe = 0; wght_diag_all_unwghted_sens = 0; wght_diag_all_weighted_safe = 0; wght_diag_all_weighted_sens = 0;
    
    prxy_diag_int_all4_sens_decreased = 0; prxy_diag_int_all4_sens_unchanged = 0; prxy_diag_int_all4_sens_increased = 0;
    prxy_diag_int_mixd_sens_decreased = 0; prxy_diag_int_mixd_sens_unchanged = 0; prxy_diag_int_mixd_sens_increased = 0;
    prxy_diag_int_nneg_sens_decreased = 0; prxy_diag_int_nneg_sens_unchanged = 0; prxy_diag_int_nneg_sens_increased = 0;
    prxy_diag_int_zero_sens_decreased = 0; prxy_diag_int_zero_sens_unchanged = 0; prxy_diag_int_zero_sens_increased = 0;
    prxy_diag_int_npos_sens_decreased = 0; prxy_diag_int_npos_sens_unchanged = 0; prxy_diag_int_npos_sens_increased = 0;
    
    prxy_diag_mrg_all4_sens_decreased = 0; prxy_diag_mrg_all4_sens_unchanged = 0; prxy_diag_mrg_all4_sens_increased = 0;
    prxy_diag_mrg_mixd_sens_decreased = 0; prxy_diag_mrg_mixd_sens_unchanged = 0; prxy_diag_mrg_mixd_sens_increased = 0;
    prxy_diag_mrg_nneg_sens_decreased = 0; prxy_diag_mrg_nneg_sens_unchanged = 0; prxy_diag_mrg_nneg_sens_increased = 0;
    prxy_diag_mrg_zero_sens_decreased = 0; prxy_diag_mrg_zero_sens_unchanged = 0; prxy_diag_mrg_zero_sens_increased = 0;
    prxy_diag_mrg_npos_sens_decreased = 0; prxy_diag_mrg_npos_sens_unchanged = 0; prxy_diag_mrg_npos_sens_increased = 0;
    
    prxy_diag_all_all4_sens_decreased = 0; prxy_diag_all_all4_sens_unchanged = 0; prxy_diag_all_all4_sens_increased = 0;
    prxy_diag_all_mixd_sens_decreased = 0; prxy_diag_all_mixd_sens_unchanged = 0; prxy_diag_all_mixd_sens_increased = 0;
    prxy_diag_all_nneg_sens_decreased = 0; prxy_diag_all_nneg_sens_unchanged = 0; prxy_diag_all_nneg_sens_increased = 0;
    prxy_diag_all_zero_sens_decreased = 0; prxy_diag_all_zero_sens_unchanged = 0; prxy_diag_all_zero_sens_increased = 0;
    prxy_diag_all_npos_sens_decreased = 0; prxy_diag_all_npos_sens_unchanged = 0; prxy_diag_all_npos_sens_increased = 0;
    
    prxy_diag_int_all4_wnoproxy_safe = 0; prxy_diag_int_all4_wnoproxy_sens = 0; prxy_diag_int_all4_wthproxy_safe = 0; prxy_diag_int_all4_wthproxy_sens = 0;
    prxy_diag_int_mixd_wnoproxy_safe = 0; prxy_diag_int_mixd_wnoproxy_sens = 0; prxy_diag_int_mixd_wthproxy_safe = 0; prxy_diag_int_mixd_wthproxy_sens = 0;
    prxy_diag_int_nneg_wnoproxy_safe = 0; prxy_diag_int_nneg_wnoproxy_sens = 0; prxy_diag_int_nneg_wthproxy_safe = 0; prxy_diag_int_nneg_wthproxy_sens = 0;
    prxy_diag_int_zero_wnoproxy_safe = 0; prxy_diag_int_zero_wnoproxy_sens = 0; prxy_diag_int_zero_wthproxy_safe = 0; prxy_diag_int_zero_wthproxy_sens = 0;
    prxy_diag_int_npos_wnoproxy_safe = 0; prxy_diag_int_npos_wnoproxy_sens = 0; prxy_diag_int_npos_wthproxy_safe = 0; prxy_diag_int_npos_wthproxy_sens = 0;
    
    prxy_diag_mrg_all4_wnoproxy_safe = 0; prxy_diag_mrg_all4_wnoproxy_sens = 0; prxy_diag_mrg_all4_wthproxy_safe = 0; prxy_diag_mrg_all4_wthproxy_sens = 0;
    prxy_diag_mrg_mixd_wnoproxy_safe = 0; prxy_diag_mrg_mixd_wnoproxy_sens = 0; prxy_diag_mrg_mixd_wthproxy_safe = 0; prxy_diag_mrg_mixd_wthproxy_sens = 0;
    prxy_diag_mrg_nneg_wnoproxy_safe = 0; prxy_diag_mrg_nneg_wnoproxy_sens = 0; prxy_diag_mrg_nneg_wthproxy_safe = 0; prxy_diag_mrg_nneg_wthproxy_sens = 0;
    prxy_diag_mrg_zero_wnoproxy_safe = 0; prxy_diag_mrg_zero_wnoproxy_sens = 0; prxy_diag_mrg_zero_wthproxy_safe = 0; prxy_diag_mrg_zero_wthproxy_sens = 0;
    prxy_diag_mrg_npos_wnoproxy_safe = 0; prxy_diag_mrg_npos_wnoproxy_sens = 0; prxy_diag_mrg_npos_wthproxy_safe = 0; prxy_diag_mrg_npos_wthproxy_sens = 0;
    
    prxy_diag_agg_all4_wnoproxy_safe = 0; prxy_diag_agg_all4_wnoproxy_sens = 0; prxy_diag_agg_all4_wthproxy_safe = 0; prxy_diag_agg_all4_wthproxy_sens = 0;
    
    prxy_diag_all_all4_wnoproxy_safe = 0; prxy_diag_all_all4_wnoproxy_sens = 0; prxy_diag_all_all4_wthproxy_safe = 0; prxy_diag_all_all4_wthproxy_sens = 0;
    prxy_diag_all_mixd_wnoproxy_safe = 0; prxy_diag_all_mixd_wnoproxy_sens = 0; prxy_diag_all_mixd_wthproxy_safe = 0; prxy_diag_all_mixd_wthproxy_sens = 0;
    prxy_diag_all_nneg_wnoproxy_safe = 0; prxy_diag_all_nneg_wnoproxy_sens = 0; prxy_diag_all_nneg_wthproxy_safe = 0; prxy_diag_all_nneg_wthproxy_sens = 0;
    prxy_diag_all_zero_wnoproxy_safe = 0; prxy_diag_all_zero_wnoproxy_sens = 0; prxy_diag_all_zero_wthproxy_safe = 0; prxy_diag_all_zero_wthproxy_sens = 0;
    prxy_diag_all_npos_wnoproxy_safe = 0; prxy_diag_all_npos_wnoproxy_sens = 0; prxy_diag_all_npos_wthproxy_safe = 0; prxy_diag_all_npos_wthproxy_sens = 0;
    
    for (i = 0; i < CellSet->NumberEntries; i = i + 1) {
        Cell = CellSet->Cell[i];
        /*-cell is internal                          iff ( Cell->IsInternal                     )                                               */
        /* cell is marginal                          iff (!Cell->IsInternal && Cell->Type == 'C')                                               */
        /* cell is aggregate                         iff (                     Cell->Type == 'A')                                               */
        if      (Cell->IsInternal ) {
            /*-cell is internal : */
            CellType  = 'I';
        }
        else if (Cell->Type == 'C') {
            /*-cell is marginal : */
            CellType  = 'M';
        }
        else if (Cell->Type == 'A') {
            /*-cell is aggregate: */
            CellType  = 'A';
        }
        
        /*-calculate        diagnostic status             counts:                                                                           */
        /*    -cell is sensitive                         iff (Cell->Sensitivity           > 0.0     ) and otherwise is safe                 */
        IsSens      = (Cell->Sensitivity           > 0.0     );
        
        if (GenerateWeightDiagnostics) {
            /*-calculate weight diagnostics:                                                                                                        */
            /*    -calculate weight diagnostic sensitivity change counts:                                                                           */
            /*        -cell sensitivity is decreased due to weights iff ((Cell->Sensitivity - Cell->Sensitivity_noweights) <  0.0)                  */
            /*        -cell sensitivity is unchanged due to weights iff ((Cell->Sensitivity - Cell->Sensitivity_noweights) == 0.0)                  */
            /*        -cell sensitivity is increased due to weights iff ((Cell->Sensitivity - Cell->Sensitivity_noweights) >  0.0)                  */
            if      ((Cell->Sensitivity - Cell->Sensitivity_noweights) <  0.0) {
                SensChngSignum = -1;
            }
            else if ((Cell->Sensitivity - Cell->Sensitivity_noweights) == 0.0) {
                SensChngSignum =  0;
            }
            else if ((Cell->Sensitivity - Cell->Sensitivity_noweights) >  0.0) {
                SensChngSignum =  1;
            }
            if      (CellType == 'I') {wght_diag_int_sens_decreased += (SensChngSignum == -1); wght_diag_int_sens_unchanged += (SensChngSignum ==  0); wght_diag_int_sens_increased += (SensChngSignum ==  1);}
            else if (CellType == 'M') {wght_diag_mrg_sens_decreased += (SensChngSignum == -1); wght_diag_mrg_sens_unchanged += (SensChngSignum ==  0); wght_diag_mrg_sens_increased += (SensChngSignum ==  1);}
            else if (CellType == 'A') {wght_diag_all_sens_decreased += (SensChngSignum == -1); wght_diag_all_sens_unchanged += (SensChngSignum ==  0); wght_diag_all_sens_increased += (SensChngSignum ==  1);}
            
            /*    -calculate weight diagnostic status             counts:                                                                           */
            /*        -cell is sensitive without weights         iff (Cell->Sensitivity_noweights > 0.0     ) and otherwise is safe without weights */
            IsSensWoWts = (Cell->Sensitivity_noweights > 0.0     );
            if      (CellType == 'I') {wght_diag_int_unwghted_safe += !IsSensWoWts; wght_diag_int_unwghted_sens += IsSensWoWts; wght_diag_int_weighted_safe += !IsSens; wght_diag_int_weighted_sens += IsSens;}
            else if (CellType == 'M') {wght_diag_mrg_unwghted_safe += !IsSensWoWts; wght_diag_mrg_unwghted_sens += IsSensWoWts; wght_diag_mrg_weighted_safe += !IsSens; wght_diag_mrg_weighted_sens += IsSens;}
            else if (CellType == 'A') {wght_diag_agg_unwghted_safe += !IsSensWoWts; wght_diag_agg_unwghted_sens += IsSensWoWts; wght_diag_agg_weighted_safe += !IsSens; wght_diag_agg_weighted_sens += IsSens;}
                                       wght_diag_all_unwghted_safe += !IsSensWoWts; wght_diag_all_unwghted_sens += IsSensWoWts; wght_diag_all_weighted_safe += !IsSens; wght_diag_all_weighted_sens += IsSens;
        }
        
        if (GenerateProxyDiagnostics) {
            /*-calculate proxy  diagnostics:                                                                                                        */
            /*    -calculate proxy  diagnostic sensitivity change counts:                                                                           */
            /*        -cell sensitivity is decreased due to proxy   iff ((Cell->Sensitivity - Cell->Sensitivity_noproxy  ) <  0.0)                  */
            /*        -cell sensitivity is unchanged due to proxy   iff ((Cell->Sensitivity - Cell->Sensitivity_noproxy  ) == 0.0)                  */
            /*        -cell sensitivity is increased due to proxy   iff ((Cell->Sensitivity - Cell->Sensitivity_noproxy  ) >  0.0)                  */
            if      ((Cell->Sensitivity - Cell->Sensitivity_noproxy  ) <  0.0) {
                SensChngSignum = -1;
            }
            else if ((Cell->Sensitivity - Cell->Sensitivity_noproxy  ) == 0.0) {
                SensChngSignum =  0;
            }
            else if ((Cell->Sensitivity - Cell->Sensitivity_noproxy  ) >  0.0) {
                SensChngSignum =  1;
            }
            /*        -cell received only non-positive                  contributions some of which were non-zero iff (Cell->TotalMixedSignStatus == -1) */
            /*        -cell received only zero                          contributions                             iff (Cell->TotalMixedSignStatus ==  0) */
            /*        -cell received only non-negative                  contributions some of which were non-zero iff (Cell->TotalMixedSignStatus ==  1) */
            /*        -cell received both non-positive and non-negative contributions                             iff (Cell->TotalMixedSignStatus ==  2) */
            if      (Cell->TotalMixedSignStatus == -1) {
                MxdSgnStatus = -1;
            }
            else if (Cell->TotalMixedSignStatus ==  1) {
                MxdSgnStatus =  1;
            }
            else if (Cell->TotalMixedSignStatus ==  2) {
                MxdSgnStatus =  2;
            }
            else if (Cell->TotalMixedSignStatus ==  0) {
                MxdSgnStatus =  0;
            }
            else { /* (Cell->TotalMixedSignStatus isn't one of -1, 1, 2, or 0) */
                IO_PRINT_LINE(MsgCellInvalidContributions, Cell->TotalMixedSignStatus); RetCode = EIE_FAIL; goto error_cleanup;
            }
            
            if      (CellType=='I') {                                                prxy_diag_int_all4_sens_decreased += (SensChngSignum==-1); prxy_diag_int_all4_sens_unchanged += (SensChngSignum== 0); prxy_diag_int_all4_sens_increased += (SensChngSignum== 1);
                                          if (MxdSgnStatus== 2) {prxy_diag_int_mixd_sens_decreased += (SensChngSignum==-1); prxy_diag_int_mixd_sens_unchanged += (SensChngSignum== 0); prxy_diag_int_mixd_sens_increased += (SensChngSignum== 1);}
                                     else if (MxdSgnStatus== 1) {prxy_diag_int_nneg_sens_decreased += (SensChngSignum==-1); prxy_diag_int_nneg_sens_unchanged += (SensChngSignum== 0); prxy_diag_int_nneg_sens_increased += (SensChngSignum== 1);}
                                     else if (MxdSgnStatus== 0) {prxy_diag_int_zero_sens_decreased += (SensChngSignum==-1); prxy_diag_int_zero_sens_unchanged += (SensChngSignum== 0); prxy_diag_int_zero_sens_increased += (SensChngSignum== 1);}
                                     else if (MxdSgnStatus==-1) {prxy_diag_int_npos_sens_decreased += (SensChngSignum==-1); prxy_diag_int_npos_sens_unchanged += (SensChngSignum== 0); prxy_diag_int_npos_sens_increased += (SensChngSignum== 1);}
            }
            else if (CellType=='M') {                            prxy_diag_mrg_all4_sens_decreased += (SensChngSignum==-1); prxy_diag_mrg_all4_sens_unchanged += (SensChngSignum== 0); prxy_diag_mrg_all4_sens_increased += (SensChngSignum== 1);
                                          if (MxdSgnStatus== 2) {prxy_diag_mrg_mixd_sens_decreased += (SensChngSignum==-1); prxy_diag_mrg_mixd_sens_unchanged += (SensChngSignum== 0); prxy_diag_mrg_mixd_sens_increased += (SensChngSignum== 1);}
                                     else if (MxdSgnStatus== 1) {prxy_diag_mrg_nneg_sens_decreased += (SensChngSignum==-1); prxy_diag_mrg_nneg_sens_unchanged += (SensChngSignum== 0); prxy_diag_mrg_nneg_sens_increased += (SensChngSignum== 1);}
                                     else if (MxdSgnStatus== 0) {prxy_diag_mrg_zero_sens_decreased += (SensChngSignum==-1); prxy_diag_mrg_zero_sens_unchanged += (SensChngSignum== 0); prxy_diag_mrg_zero_sens_increased += (SensChngSignum== 1);}
                                     else if (MxdSgnStatus==-1) {prxy_diag_mrg_npos_sens_decreased += (SensChngSignum==-1); prxy_diag_mrg_npos_sens_unchanged += (SensChngSignum== 0); prxy_diag_mrg_npos_sens_increased += (SensChngSignum== 1);}
            }
                                                                 prxy_diag_all_all4_sens_decreased += (SensChngSignum==-1); prxy_diag_all_all4_sens_unchanged += (SensChngSignum== 0); prxy_diag_all_all4_sens_increased += (SensChngSignum== 1);
                                          if (MxdSgnStatus== 2) {prxy_diag_all_mixd_sens_decreased += (SensChngSignum==-1); prxy_diag_all_mixd_sens_unchanged += (SensChngSignum== 0); prxy_diag_all_mixd_sens_increased += (SensChngSignum== 1);}
                                     else if (MxdSgnStatus== 1) {prxy_diag_all_nneg_sens_decreased += (SensChngSignum==-1); prxy_diag_all_nneg_sens_unchanged += (SensChngSignum== 0); prxy_diag_all_nneg_sens_increased += (SensChngSignum== 1);}
                                     else if (MxdSgnStatus== 0) {prxy_diag_all_zero_sens_decreased += (SensChngSignum==-1); prxy_diag_all_zero_sens_unchanged += (SensChngSignum== 0); prxy_diag_all_zero_sens_increased += (SensChngSignum== 1);}
                                     else if (MxdSgnStatus==-1) {prxy_diag_all_npos_sens_decreased += (SensChngSignum==-1); prxy_diag_all_npos_sens_unchanged += (SensChngSignum== 0); prxy_diag_all_npos_sens_increased += (SensChngSignum== 1);}
            
            /*    -calculate proxy  diagnostic status             counts:                                                                           */
            /*        -cell is sensitive without proxy           iff (Cell->Sensitivity_noproxy   > 0.0     ) and otherwise is safe without proxy   */
            IsSensWoPxy = (Cell->Sensitivity_noproxy   > 0.0     );
            if      (CellType=='I') {                            prxy_diag_int_all4_wnoproxy_safe += !IsSensWoPxy; prxy_diag_int_all4_wnoproxy_sens += IsSensWoPxy; prxy_diag_int_all4_wthproxy_safe += !IsSens; prxy_diag_int_all4_wthproxy_sens += IsSens;
                                          if (MxdSgnStatus== 2) {prxy_diag_int_mixd_wnoproxy_safe += !IsSensWoPxy; prxy_diag_int_mixd_wnoproxy_sens += IsSensWoPxy; prxy_diag_int_mixd_wthproxy_safe += !IsSens; prxy_diag_int_mixd_wthproxy_sens += IsSens;}
                                     else if (MxdSgnStatus== 1) {prxy_diag_int_nneg_wnoproxy_safe += !IsSensWoPxy; prxy_diag_int_nneg_wnoproxy_sens += IsSensWoPxy; prxy_diag_int_nneg_wthproxy_safe += !IsSens; prxy_diag_int_nneg_wthproxy_sens += IsSens;}
                                     else if (MxdSgnStatus== 0) {prxy_diag_int_zero_wnoproxy_safe += !IsSensWoPxy; prxy_diag_int_zero_wnoproxy_sens += IsSensWoPxy; prxy_diag_int_zero_wthproxy_safe += !IsSens; prxy_diag_int_zero_wthproxy_sens += IsSens;}
                                     else if (MxdSgnStatus==-1) {prxy_diag_int_npos_wnoproxy_safe += !IsSensWoPxy; prxy_diag_int_npos_wnoproxy_sens += IsSensWoPxy; prxy_diag_int_npos_wthproxy_safe += !IsSens; prxy_diag_int_npos_wthproxy_sens += IsSens;}
            }
            else if (CellType=='M') {                            prxy_diag_mrg_all4_wnoproxy_safe += !IsSensWoPxy; prxy_diag_mrg_all4_wnoproxy_sens += IsSensWoPxy; prxy_diag_mrg_all4_wthproxy_safe += !IsSens; prxy_diag_mrg_all4_wthproxy_sens += IsSens;
                                          if (MxdSgnStatus== 2) {prxy_diag_mrg_mixd_wnoproxy_safe += !IsSensWoPxy; prxy_diag_mrg_mixd_wnoproxy_sens += IsSensWoPxy; prxy_diag_mrg_mixd_wthproxy_safe += !IsSens; prxy_diag_mrg_mixd_wthproxy_sens += IsSens;}
                                     else if (MxdSgnStatus== 1) {prxy_diag_mrg_nneg_wnoproxy_safe += !IsSensWoPxy; prxy_diag_mrg_nneg_wnoproxy_sens += IsSensWoPxy; prxy_diag_mrg_nneg_wthproxy_safe += !IsSens; prxy_diag_mrg_nneg_wthproxy_sens += IsSens;}
                                     else if (MxdSgnStatus== 0) {prxy_diag_mrg_zero_wnoproxy_safe += !IsSensWoPxy; prxy_diag_mrg_zero_wnoproxy_sens += IsSensWoPxy; prxy_diag_mrg_zero_wthproxy_safe += !IsSens; prxy_diag_mrg_zero_wthproxy_sens += IsSens;}
                                     else if (MxdSgnStatus==-1) {prxy_diag_mrg_npos_wnoproxy_safe += !IsSensWoPxy; prxy_diag_mrg_npos_wnoproxy_sens += IsSensWoPxy; prxy_diag_mrg_npos_wthproxy_safe += !IsSens; prxy_diag_mrg_npos_wthproxy_sens += IsSens;}
            }
            else if (CellType=='A') {                            prxy_diag_agg_all4_wnoproxy_safe += !IsSensWoPxy; prxy_diag_agg_all4_wnoproxy_sens += IsSensWoPxy; prxy_diag_agg_all4_wthproxy_safe += !IsSens; prxy_diag_agg_all4_wthproxy_sens += IsSens;
            }
                                                                 prxy_diag_all_all4_wnoproxy_safe += !IsSensWoPxy; prxy_diag_all_all4_wnoproxy_sens += IsSensWoPxy; prxy_diag_all_all4_wthproxy_safe += !IsSens; prxy_diag_all_all4_wthproxy_sens += IsSens;
                                          if (MxdSgnStatus== 2) {prxy_diag_all_mixd_wnoproxy_safe += !IsSensWoPxy; prxy_diag_all_mixd_wnoproxy_sens += IsSensWoPxy; prxy_diag_all_mixd_wthproxy_safe += !IsSens; prxy_diag_all_mixd_wthproxy_sens += IsSens;}
                                     else if (MxdSgnStatus== 1) {prxy_diag_all_nneg_wnoproxy_safe += !IsSensWoPxy; prxy_diag_all_nneg_wnoproxy_sens += IsSensWoPxy; prxy_diag_all_nneg_wthproxy_safe += !IsSens; prxy_diag_all_nneg_wthproxy_sens += IsSens;}
                                     else if (MxdSgnStatus== 0) {prxy_diag_all_zero_wnoproxy_safe += !IsSensWoPxy; prxy_diag_all_zero_wnoproxy_sens += IsSensWoPxy; prxy_diag_all_zero_wthproxy_safe += !IsSens; prxy_diag_all_zero_wthproxy_sens += IsSens;}
                                     else if (MxdSgnStatus==-1) {prxy_diag_all_npos_wnoproxy_safe += !IsSensWoPxy; prxy_diag_all_npos_wnoproxy_sens += IsSensWoPxy; prxy_diag_all_npos_wthproxy_safe += !IsSens; prxy_diag_all_npos_wthproxy_sens += IsSens;}
        }
    }
    
    if (GenerateWeightDiagnostics) {
        IO_PRINT_LINE(TableW1L1);
        IO_PRINT_LINE(TableW1LineShort);
        IO_PRINT_LINE(TableW1L2);
        IO_PRINT_LINE(TableW1L3);
        IO_PRINT_LINE(TableW1LineLong);
        IO_PRINT_LINE(TableW1L4 , wght_diag_int_sens_decreased, wght_diag_int_sens_unchanged, wght_diag_int_sens_increased);
        IO_PRINT_LINE(TableW1L5 , wght_diag_mrg_sens_decreased, wght_diag_mrg_sens_unchanged, wght_diag_mrg_sens_increased);
        IO_PRINT_LINE(TableW1L6 , wght_diag_all_sens_decreased, wght_diag_all_sens_unchanged, wght_diag_all_sens_increased);
        IO_PRINT_LINE(TableW1LineLong);
        IO_PRINT_LINE("");
        IO_PRINT_LINE(TableW2L1);
        IO_PRINT_LINE(TableW2LineShort);
        IO_PRINT_LINE(TableW2L2);
        IO_PRINT_LINE(TableW2L3);
        IO_PRINT_LINE(TableW2LineShort);
        IO_PRINT_LINE(TableW2L4);
        IO_PRINT_LINE(TableW2LineLong);   
        IO_PRINT_LINE(TableW2L5 , wght_diag_int_unwghted_safe, wght_diag_int_unwghted_sens, wght_diag_int_weighted_safe, wght_diag_int_weighted_sens);
        IO_PRINT_LINE(TableW2L6 , wght_diag_mrg_unwghted_safe, wght_diag_mrg_unwghted_sens, wght_diag_mrg_weighted_safe, wght_diag_mrg_weighted_sens);
        IO_PRINT_LINE(TableW2L7 , wght_diag_all_unwghted_safe, wght_diag_all_unwghted_sens, wght_diag_all_weighted_safe, wght_diag_all_weighted_sens);
        IO_PRINT_LINE(TableW2L8 , wght_diag_agg_unwghted_safe, wght_diag_agg_unwghted_sens, wght_diag_agg_weighted_safe, wght_diag_agg_weighted_sens);
        IO_PRINT_LINE(TableW2LineLong);  
        IO_PRINT_LINE("");
    }
    if (GenerateProxyDiagnostics) {
        IO_PRINT_LINE(TableP1L1);
        IO_PRINT_LINE(TableP1LineShort);
        IO_PRINT_LINE(TableP1L2);
        IO_PRINT_LINE(TableP1L3);
        IO_PRINT_LINE(TableP1LineLong);
        IO_PRINT_LINE(TableP1L4  , prxy_diag_int_all4_sens_decreased, prxy_diag_int_all4_sens_unchanged, prxy_diag_int_all4_sens_increased);
        IO_PRINT_LINE(TableP1L5  , prxy_diag_int_mixd_sens_decreased, prxy_diag_int_mixd_sens_unchanged, prxy_diag_int_mixd_sens_increased);
        IO_PRINT_LINE(TableP1L6  , prxy_diag_int_nneg_sens_decreased, prxy_diag_int_nneg_sens_unchanged, prxy_diag_int_nneg_sens_increased);
        IO_PRINT_LINE(TableP1L7  , prxy_diag_int_zero_sens_decreased, prxy_diag_int_zero_sens_unchanged, prxy_diag_int_zero_sens_increased);
        IO_PRINT_LINE(TableP1L8  , prxy_diag_int_npos_sens_decreased, prxy_diag_int_npos_sens_unchanged, prxy_diag_int_npos_sens_increased);
        IO_PRINT_LINE(TableP1L9  , prxy_diag_mrg_all4_sens_decreased, prxy_diag_mrg_all4_sens_unchanged, prxy_diag_mrg_all4_sens_increased);
        IO_PRINT_LINE(TableP1L10 , prxy_diag_mrg_mixd_sens_decreased, prxy_diag_mrg_mixd_sens_unchanged, prxy_diag_mrg_mixd_sens_increased);
        IO_PRINT_LINE(TableP1L11 , prxy_diag_mrg_nneg_sens_decreased, prxy_diag_mrg_nneg_sens_unchanged, prxy_diag_mrg_nneg_sens_increased);
        IO_PRINT_LINE(TableP1L12 , prxy_diag_mrg_zero_sens_decreased, prxy_diag_mrg_zero_sens_unchanged, prxy_diag_mrg_zero_sens_increased);
        IO_PRINT_LINE(TableP1L13 , prxy_diag_mrg_npos_sens_decreased, prxy_diag_mrg_npos_sens_unchanged, prxy_diag_mrg_npos_sens_increased);
        IO_PRINT_LINE(TableP1L14 , prxy_diag_all_all4_sens_decreased, prxy_diag_all_all4_sens_unchanged, prxy_diag_all_all4_sens_increased);
        IO_PRINT_LINE(TableP1L15 , prxy_diag_all_mixd_sens_decreased, prxy_diag_all_mixd_sens_unchanged, prxy_diag_all_mixd_sens_increased);
        IO_PRINT_LINE(TableP1L16 , prxy_diag_all_nneg_sens_decreased, prxy_diag_all_nneg_sens_unchanged, prxy_diag_all_nneg_sens_increased);
        IO_PRINT_LINE(TableP1L17 , prxy_diag_all_zero_sens_decreased, prxy_diag_all_zero_sens_unchanged, prxy_diag_all_zero_sens_increased);
        IO_PRINT_LINE(TableP1L18 , prxy_diag_all_npos_sens_decreased, prxy_diag_all_npos_sens_unchanged, prxy_diag_all_npos_sens_increased);
        IO_PRINT_LINE(TableP1LineLong);
        IO_PRINT_LINE("");
        IO_PRINT_LINE(TableP2L1);
        IO_PRINT_LINE(TableP2LineShort);
        IO_PRINT_LINE(TableP2L2);
        IO_PRINT_LINE(TableP2L3);
        IO_PRINT_LINE(TableP2L4);
        IO_PRINT_LINE(TableP2LineShort2);
        IO_PRINT_LINE(TableP2L5);
        IO_PRINT_LINE(TableP2LineLong);
        IO_PRINT_LINE(TableP2L6  , prxy_diag_int_all4_wnoproxy_safe, prxy_diag_int_all4_wnoproxy_sens, prxy_diag_int_all4_wthproxy_safe, prxy_diag_int_all4_wthproxy_sens);
        IO_PRINT_LINE(TableP2L7  , prxy_diag_int_mixd_wnoproxy_safe, prxy_diag_int_mixd_wnoproxy_sens, prxy_diag_int_mixd_wthproxy_safe, prxy_diag_int_mixd_wthproxy_sens);
        IO_PRINT_LINE(TableP2L8  , prxy_diag_int_nneg_wnoproxy_safe, prxy_diag_int_nneg_wnoproxy_sens, prxy_diag_int_nneg_wthproxy_safe, prxy_diag_int_nneg_wthproxy_sens);
        IO_PRINT_LINE(TableP2L9  , prxy_diag_int_zero_wnoproxy_safe, prxy_diag_int_zero_wnoproxy_sens, prxy_diag_int_zero_wthproxy_safe, prxy_diag_int_zero_wthproxy_sens);
        IO_PRINT_LINE(TableP2L10 , prxy_diag_int_npos_wnoproxy_safe, prxy_diag_int_npos_wnoproxy_sens, prxy_diag_int_npos_wthproxy_safe, prxy_diag_int_npos_wthproxy_sens);
        IO_PRINT_LINE(TableP2L11 , prxy_diag_mrg_all4_wnoproxy_safe, prxy_diag_mrg_all4_wnoproxy_sens, prxy_diag_mrg_all4_wthproxy_safe, prxy_diag_mrg_all4_wthproxy_sens);
        IO_PRINT_LINE(TableP2L12 , prxy_diag_mrg_mixd_wnoproxy_safe, prxy_diag_mrg_mixd_wnoproxy_sens, prxy_diag_mrg_mixd_wthproxy_safe, prxy_diag_mrg_mixd_wthproxy_sens);
        IO_PRINT_LINE(TableP2L13 , prxy_diag_mrg_nneg_wnoproxy_safe, prxy_diag_mrg_nneg_wnoproxy_sens, prxy_diag_mrg_nneg_wthproxy_safe, prxy_diag_mrg_nneg_wthproxy_sens);
        IO_PRINT_LINE(TableP2L14 , prxy_diag_mrg_zero_wnoproxy_safe, prxy_diag_mrg_zero_wnoproxy_sens, prxy_diag_mrg_zero_wthproxy_safe, prxy_diag_mrg_zero_wthproxy_sens);
        IO_PRINT_LINE(TableP2L15 , prxy_diag_mrg_npos_wnoproxy_safe, prxy_diag_mrg_npos_wnoproxy_sens, prxy_diag_mrg_npos_wthproxy_safe, prxy_diag_mrg_npos_wthproxy_sens);
        IO_PRINT_LINE(TableP2L16 , prxy_diag_all_all4_wnoproxy_safe, prxy_diag_all_all4_wnoproxy_sens, prxy_diag_all_all4_wthproxy_safe, prxy_diag_all_all4_wthproxy_sens);
        IO_PRINT_LINE(TableP2L17 , prxy_diag_all_mixd_wnoproxy_safe, prxy_diag_all_mixd_wnoproxy_sens, prxy_diag_all_mixd_wthproxy_safe, prxy_diag_all_mixd_wthproxy_sens);
        IO_PRINT_LINE(TableP2L18 , prxy_diag_all_nneg_wnoproxy_safe, prxy_diag_all_nneg_wnoproxy_sens, prxy_diag_all_nneg_wthproxy_safe, prxy_diag_all_nneg_wthproxy_sens);
        IO_PRINT_LINE(TableP2L19 , prxy_diag_all_zero_wnoproxy_safe, prxy_diag_all_zero_wnoproxy_sens, prxy_diag_all_zero_wthproxy_safe, prxy_diag_all_zero_wthproxy_sens);
        IO_PRINT_LINE(TableP2L20 , prxy_diag_all_npos_wnoproxy_safe, prxy_diag_all_npos_wnoproxy_sens, prxy_diag_all_npos_wthproxy_safe, prxy_diag_all_npos_wthproxy_sens);
        IO_PRINT_LINE(TableP2L21 , prxy_diag_agg_all4_wnoproxy_safe, prxy_diag_agg_all4_wnoproxy_sens, prxy_diag_agg_all4_wthproxy_safe, prxy_diag_agg_all4_wthproxy_sens);
        IO_PRINT_LINE(TableP2LineLong);
        IO_PRINT_LINE("");
    }
    
    goto normal_cleanup;
    
error_cleanup:
    
normal_cleanup:
    
    return RetCode;
}
/*********************************************************************
Reads DATA data sets
*********************************************************************/
static EIT_RETURNCODE ReadData (
	SP_sensitiv* sp,
	tSDataSet * DataSet,
	STCT_CELLSET * CellSet,
	STCT_KDTREE ** KdTree,
	STCT_HTREEROOT * HTreeRoot,
	STCT_RANGEROOT * RangeRoot,
	PROGRAM_COUNTER * Counter,
	int * MessageQuota,
	int waiver_flags_present,
	tSList * waiver_flags_slist,
	tIList * waiver_flags_ilist,
	int AcceptNegativeValues)
{
	STCT_COORDINATE * Coordinate = NULL;
	STCT_ADDMICRODATA_RETURNCODE amdrc;
	STCT_CELL * Cell = NULL;
	tSList ** Codes = NULL;
	int i;
	EIT_BOOLEAN InvalidCode;
	tSList ** MissingCodes = NULL;
	char ** ReadCode = NULL;
	double Shadow = 0.0;
	int waiver_flag;
	EIT_RETURNCODE ReturnCode;

	int counterWaiverFlagMissing = 0;
	int counterWaiverFlagInvalid = 0;

	ReturnCode = EIE_SUCCEED;

	ReadCode = STC_AllocateMemory (HTreeRoot->NumberEntries * sizeof *ReadCode);
	if (ReadCode     == NULL) {ReturnCode = EIE_FAIL; goto error_cleanup;}
	Codes = STC_AllocateMemory (HTreeRoot->NumberEntries * sizeof *Codes);
	if (Codes        == NULL) {ReturnCode = EIE_FAIL; goto error_cleanup;}
	MissingCodes = STC_AllocateMemory (HTreeRoot->NumberEntries * sizeof *MissingCodes);
	if (MissingCodes == NULL) {ReturnCode = EIE_FAIL; goto error_cleanup;}
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		SList_New (&Codes[i]);
		if (Codes       [i] == NULL) {ReturnCode = EIE_FAIL; goto error_cleanup;}
		SList_New (&MissingCodes[i]);
		if (MissingCodes[i] == NULL) {ReturnCode = EIE_FAIL; goto error_cleanup;}
	}
	Coordinate = STC_CoordinateAllocate (HTreeRoot->NumberEntries);
	if (Coordinate   == NULL) {ReturnCode = EIE_FAIL; goto error_cleanup;}

	STC_HTreeRootInitHasData (HTreeRoot);
	//STC_HTreeRootPrintDebug (HTreeRoot);
	IO_DATASET_RC rc_next_rec;

	while ((rc_next_rec = DSR_cursor_next_rec(&sp->dsr_indata.dsr)) == DSRC_NEXT_REC_SUCCESS) {

		Counter->NumberObs++;

		IO_RETURN_CODE rc_get_rec = DSR_rec_get(&sp->dsr_indata.dsr);
		if (rc_get_rec != IORC_SUCCESS) {
			ReturnCode = EIE_FAIL;
			goto error_cleanup;
		}

		SVariableNullTerminate (&DataSet->Variable[DATA_ID_INDEX]);
		if (IOUtil_is_missing(DataSet->Variable[DATA_VAR_INDEX].Value.d)) {
			Counter->NumberMissingValues++;
			continue;
		}
		if (!AcceptNegativeValues) {
			if (DataSet->Variable[DATA_VAR_INDEX].Value.d < 0.0) {
				Counter->NumberNegativeValues++;
				continue;
			}
		}
		if (sp->dsr_indata.VL_shadow.count == 1 &&
				IOUtil_is_missing (DataSet->Variable[DATA_SHADOW_INDEX].Value.d)) {
			Counter->NumberMissingShadows++;
			continue;
		}
		InvalidCode = EIE_FALSE;
		for (i = 0; i < HTreeRoot->NumberEntries; i++) {
			SVariableNullTerminate (&DataSet->Variable[DATA_DIMENSION_INDEX+i]);
			if (DataSet->Variable[DATA_DIMENSION_INDEX+i].Value.s[0] == '\0') {
				ADDMESSAGEQUOTA (MessageQuota, MsgCodeMissing, HTreeRoot->Name[i]);
				InvalidCode = EIE_TRUE;

				Counter->NumberMissingDimensions++;
				break;
			}
			else if (!IsCodeValid (DataSet->Variable[DATA_DIMENSION_INDEX+i].Value.s, STCM_CODE_FIRSTCHARACTER_CHARACTER_SET, STCM_CODE_CHARACTER_SET)) {
				ADDMESSAGEQUOTA (MessageQuota, MsgCodeHasIllegalCharacters, HTreeRoot->Name[i]);
				InvalidCode = EIE_TRUE;
				Counter->NumberInvalidDimensionsJunkInCode++;
				break;
			}
			ReadCode[i] = DataSet->Variable[DATA_DIMENSION_INDEX+i].Value.s;
		}
		if (InvalidCode) {
			continue;
		}
		/* WAIVER FLAGS: 2022-05 update
		Full waiver flags (waiver_flags_present == 1) and Partial waiver flags ( == 2) differ only slightly
		after this update.  For FULL waiver flags the SList is used to ensure consistency among flags from the same
		enterprise, while for PARTIAL waiver flags we simply validate and store the value without a consistency check.  
		Previously, FULL waivers used the SLIST exclusively (instead of adding the flag value to the DataItem).  Using the SList as
		a lookup table for FULL waiver flags was exceedingly inefficient, thus we now store store FULL waiver flags in the same
		manner as PARTIAL flags.  
		Therefore the SList "waiver flags list" here is essentially only used for validation
		*/
		if        (waiver_flags_present == 1) {
			if ((!IOUtil_is_missing(DataSet->Variable[DATA_WAIVER_INDEX].Value.d)       ) &&
			    ((((int)(DataSet->Variable[DATA_WAIVER_INDEX].Value.d)) == 0) ||
			     (((int)(DataSet->Variable[DATA_WAIVER_INDEX].Value.d)) == 1)
			    )
			   ) {
				if (eSListFail == update_waiver_flags_lists(DataSet->Variable[DATA_ID_INDEX].Value.s, DataSet->Variable[DATA_WAIVER_INDEX].Value.d, waiver_flags_slist, waiver_flags_ilist)) {
					IO_PRINT_LINE(SAS_MESSAGE_PREFIX_ERROR MsgWaiversWaiverFlagUnabletoUpdate);
					goto error_cleanup;
				}else {
					waiver_flag = (int)(DataSet->Variable[DATA_WAIVER_INDEX].Value.d);
				}
			}
			else {
				if (IOUtil_is_missing(DataSet->Variable[DATA_WAIVER_INDEX].Value.d)) {
				     counterWaiverFlagMissing = counterWaiverFlagMissing + 1;
				}
				else {
				     counterWaiverFlagInvalid = counterWaiverFlagInvalid + 1;
				}
				Counter->NumberInvalidWaivers++;
				if (eSListFail == update_waiver_flags_lists(DataSet->Variable[DATA_ID_INDEX].Value.s,                                            0, waiver_flags_slist, waiver_flags_ilist)) {
					IO_PRINT_LINE(SAS_MESSAGE_PREFIX_ERROR MsgWaiversWaiverFlagUnabletoUpdate);
					goto error_cleanup;
				}
				else {
					waiver_flag = 0;
				}
			}
		} else if (waiver_flags_present == 2) {
			if ((!IOUtil_is_missing(DataSet->Variable[DATA_WAIVER_INDEX].Value.d)       ) &&
			    ((((int)(DataSet->Variable[DATA_WAIVER_INDEX].Value.d)) == 0) ||
			     (((int)(DataSet->Variable[DATA_WAIVER_INDEX].Value.d)) == 1)
			    )
			   ) {
			    waiver_flag = (int)(DataSet->Variable[DATA_WAIVER_INDEX].Value.d);
			}
			else {
			    if (IOUtil_is_missing(DataSet->Variable[DATA_WAIVER_INDEX].Value.d)) {
			         counterWaiverFlagMissing = counterWaiverFlagMissing + 1;
			    }
			    else {
			         counterWaiverFlagInvalid = counterWaiverFlagInvalid + 1;
			    }
			    Counter->NumberInvalidWaivers++;
			    waiver_flag = 0;
			}
		}

		//combine all codes from all dimensions to form all combinaison of internal cells
		amdrc = STC_AddMicrodata (HTreeRoot, RangeRoot, CellSet, KdTree, ReadCode,
			DataSet->Variable[DATA_ID_INDEX].Value.s,
			DataSet->Variable[DATA_VAR_INDEX].Value.d,
			DataSet->Variable[DATA_SHADOW_INDEX].Value.d,
			(IOUtil_is_missing(DataSet->Variable[DATA_PROXY_INDEX].Value.d)?0.0
			                                                       :DataSet->Variable[DATA_PROXY_INDEX].Value.d
			),
			((sp->dsr_indata.VL_weight.count ==1)?((IOUtil_is_missing(DataSet->Variable[DATA_WEIGHT_INDEX].Value.d))?1.0
			                                                                                               :DataSet->Variable[DATA_WEIGHT_INDEX].Value.d
			                                     )
			                                    :1.0
			),
			Codes, MissingCodes, Coordinate, MessageQuota,
			waiver_flags_present,
			((waiver_flags_present == 2 || waiver_flags_present == 1)?waiver_flag:0));
		EI_PrintMessages ();
		switch (amdrc) {
		case STCE_ADDMICRODATA_INVALID:
			Counter->NumberDimensionsNotInHierarchy++;
			continue;
		case STCE_ADDMICRODATA_SUCCEED:
			break;
		case STCE_ADDMICRODATA_FAIL:
		default:
			/* return;//memory error... will not happen */
			IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgWaiversReadDataMemoryError);
			ReturnCode = EIE_FAIL;
			goto error_cleanup;
		}
		Counter->NumberValidObs++;
		if (AcceptNegativeValues) {
			if (DataSet->Variable[DATA_VAR_INDEX].Value.d < 0.0) {
				Counter->NumberNegativeValues++;
			}
		}
		if (sp->dsr_indata.VL_proxy_size.count == 1 && (IOUtil_is_missing (DataSet->Variable[DATA_PROXY_INDEX].Value.d) || DataSet->Variable[DATA_PROXY_INDEX].Value.d < 0.0)) {
			Counter->NumberNegativeOrMissingProxies++;
		}
		if (sp->dsr_indata.VL_weight.count == 1 ) {
			if (IOUtil_is_missing (DataSet->Variable[DATA_WEIGHT_INDEX].Value.d)) {
				Counter->NumberMissingWeights++;
			}
			else if (DataSet->Variable[DATA_WEIGHT_INDEX].Value.d <= 0.0) {
				Counter->NumberNonPositiveWeights++;
			}
		}
		if (DataSet->Variable[DATA_ID_INDEX].Value.s[0] == '\0')
			Counter->NumberAnonymous++;
		if (DataSet->Variable[DATA_VAR_INDEX].Value.d == 0.0)
			Counter->NumberZero++;
	}
	// check for errors
	if (rc_next_rec != DSRC_NO_MORE_REC_IN_BY) {
		ReturnCode = EIE_FAIL;
		goto error_cleanup;
	}

	STC_CoordinateFree (Coordinate);
	Coordinate = NULL;
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		SList_Free (Codes[i]);
		Codes[i] = NULL;
		SList_Free (MissingCodes[i]);
		MissingCodes[i] = NULL;
	}
	STC_FreeMemory (ReadCode);
	ReadCode = NULL;
	STC_FreeMemory (Codes);
	Codes = NULL;
	STC_FreeMemory (MissingCodes);
	MissingCodes = NULL;

	if (Counter->NumberMissingValues > 0)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_WARNING MsgNumberDroppedInDataSetMissingValueForVar,
			Counter->NumberMissingValues, DataSet->Name, DataSet->Variable[DATA_VAR_INDEX].Name);
	if (Counter->NumberNegativeValues > 0) {
		if (!AcceptNegativeValues) {
		    IO_PRINT_LINE (SAS_MESSAGE_PREFIX_WARNING MsgNumberDroppedInDataSetNegativeValueForVar,
		        Counter->NumberNegativeValues, DataSet->Name, DataSet->Variable[DATA_VAR_INDEX].Name);
		}
		else { /* AcceptNegativeValues */
		    IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgNumberReadInDataSetNegativeValueForVar,
		        Counter->NumberNegativeValues, DataSet->Name, DataSet->Variable[DATA_VAR_INDEX].Name);
		}
	}
	if (Counter->NumberMissingShadows > 0)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_WARNING MsgNumberDroppedInDataSetMissingValueForVar,
			Counter->NumberMissingShadows, DataSet->Name, DataSet->Variable[DATA_SHADOW_INDEX].Name);
	if (Counter->NumberNegativeOrMissingProxies  > 0)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgNumberReadInDataSetNegativeOrMissingValueForVar,
			Counter->NumberNegativeOrMissingProxies , DataSet->Name, DataSet->Variable[DATA_PROXY_INDEX ].Name);
	if (Counter->NumberMissingWeights > 0)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_WARNING MsgNumberReadInDataSetMissingValueForVar,
			Counter->NumberMissingWeights, DataSet->Name, DataSet->Variable[DATA_WEIGHT_INDEX].Name);
	if (Counter->NumberNonPositiveWeights > 0)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_WARNING MsgNumberReadInDataSetNonPositiveValueForVar,
			Counter->NumberNonPositiveWeights, DataSet->Name, DataSet->Variable[DATA_WEIGHT_INDEX].Name);
	if (counterWaiverFlagMissing > 0) {
		IO_PRINT_LINE(SAS_MESSAGE_PREFIX_WARNING MsgWaiversNumberWaiverFlagMissingDataset, counterWaiverFlagMissing);
	}		
	if (counterWaiverFlagInvalid > 0 ) {
        IO_PRINT_LINE(SAS_MESSAGE_PREFIX_WARNING MsgWaiversNumberWaiverFlagInvalidDataset, counterWaiverFlagInvalid);
	}
	if (Counter->NumberMissingDimensions > 0)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_WARNING MsgNumberDroppedMissingDimension,
			Counter->NumberMissingDimensions, DataSet->Name, DIMENSION_GRM_NAME);
	if (Counter->NumberInvalidDimensionsJunkInCode > 0)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_WARNING MsgNumberDroppedIllegalCharactersDimension,
			Counter->NumberInvalidDimensionsJunkInCode, DataSet->Name, DIMENSION_GRM_NAME);
	if (Counter->NumberDimensionsNotInHierarchy > 0)
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_WARNING MsgNumberDroppedDimensionNotInHierarchy,
			Counter->NumberDimensionsNotInHierarchy, DataSet->Name, DIMENSION_GRM_NAME);
	if (((                        Counter->NumberMissingValues              )+
	     ((!AcceptNegativeValues)?Counter->NumberNegativeValues:0           )+
	     (                        Counter->NumberMissingShadows             )+
	     (                        Counter->NumberMissingDimensions          )+
	     (                        Counter->NumberInvalidDimensionsJunkInCode)+
	     (                        Counter->NumberDimensionsNotInHierarchy   )
	    ) > 0
	   ) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_WARNING MsgObsNotUsedForSensitivityCalculation SAS_NEWLINE);
		IO_PRINT_LINE (SAS_NEWLINE);
	}

	if (Counter->NumberObs == 0) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_WARNING MsgNoObservationsInDataSet SAS_NEWLINE, DataSet->Name);
		ReturnCode = EIE_FAIL;
		goto error_cleanup;
	}
	//don't stop, continue with next by group
	if (Counter->NumberValidObs == 0) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_WARNING MsgNoValidObservationsInDataSet SAS_NEWLINE, DataSet->Name);
	}

	STC_HTreeRootSetHasData (HTreeRoot);

	goto normal_cleanup;

error_cleanup:
	if (Coordinate   != NULL) {
		STC_CoordinateFree (Coordinate);
		Coordinate = NULL;
	}
	if (Codes        != NULL) {
		for (i = 0; i < HTreeRoot->NumberEntries; i++) {
			if (Codes[i] != NULL) {
				SList_Free (Codes[i]);
				Codes[i] = NULL;
			}
		}
		STC_FreeMemory (Codes       );
		Codes = NULL;
	}
	if (MissingCodes != NULL) {
		for (i = 0; i < HTreeRoot->NumberEntries; i++) {
			if (MissingCodes[i] != NULL) {
				SList_Free (MissingCodes[i]);
				MissingCodes[i] = NULL;
			}
		}
		STC_FreeMemory (MissingCodes);
		MissingCodes = NULL;
	}
	if (ReadCode     != NULL) {
		STC_FreeMemory (ReadCode    );
		ReadCode = NULL;
	}

normal_cleanup:

	return ReturnCode;
}
/*********************************************************************
prints time.
help to analyse performance.
*********************************************************************/
static void ShowTime (
	SP_sensitiv* sp,
	char * Message)
{
	static clock_t StaticTime = 0;
	clock_t Time;

	if (sp->timer.meta.is_specified == IOSV_SPECIFIED) {
		Time = clock ();
		IO_PRINT_LINE ("Time: %10.4f %s.", (double) (Time-StaticTime)/CLOCKS_PER_SEC, Message);
		StaticTime = Time;
	}
}
/*********************************************************************
validates proc parameters.
does all validations before quitting.
*********************************************************************/
static EIT_RETURNCODE ValidateParms (
	SP_sensitiv* sp,
	PROGRAM_PARM * Parms,
	STCT_SRULE ** SRule,
	STCT_HTREEROOT ** HTreeRoot,
	STCT_RANGEROOT ** RangeRoot,
	STCT_GROUPROOT ** GroupRoot,
	int * waiver_flags_present)
{
	EIT_RETURNCODE crc; /* cumulative return code */
	int i;
	char Name[IO_COL_NAME_LEN+1];
	int * Position;
	EIT_RETURNCODE rc;
	int sruleparsed = 0;

	crc = EIE_SUCCEED;

	if (strcmp (Parms->SRuleString, "") == 0) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgParmMandatory, SRULE_GRM_NAME);
		crc = EIE_FAIL;
	}
	else {
		rc = STC_SRuleParse (Parms->SRuleString, SRule);
		if (rc != EIE_SUCCEED) {
			EI_PrintMessages ();
			crc = EIE_FAIL;
		}
		else {
			sruleparsed = 1;
			STC_SRulePrint (*SRule);
		}
	}

	//needed for hierarchy and range
	int dim_count = sp->dsr_indata.VL_dimension.count;

	if (strcmp (Parms->HierarchyString, "") == 0) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgParmMandatory, HIERARCHY_GRM_NAME);
		*HTreeRoot = NULL;
		crc = EIE_FAIL;
	}
	else {
		rc = STC_HTreeParse (Parms->HierarchyString, HTreeRoot, STCM_CODE_FIRSTCHARACTER_CHARACTER_SET, STCM_CODE_CHARACTER_SET);
		EI_PrintMessages ();
		if (rc != EIE_SUCCEED) {
			*HTreeRoot = NULL;
			crc = EIE_FAIL;
		}
		else {
			if (dim_count != (*HTreeRoot)->NumberEntries) {
				IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgNbHierarchiesNotEqualNbDimensions,
					(*HTreeRoot)->NumberEntries, HIERARCHY_GRM_NAME, dim_count, DIMENSION_GRM_NAME);
				crc = EIE_FAIL;
			}
			else {
				for (i = 0; i < dim_count; i++) {
					IOUtil_copy_varname(Name, sp->dsr_indata.VL_dimension.names[i]);
					STC_HTreeRootSetName (*HTreeRoot, Name, i);
				}
				if (Parms->PrintCodes == EIE_TRUE) {
					STC_HTreeRootPrint (*HTreeRoot);
				}
			}
		}
	}

	/* don't bother with range if there is a previous error */
	if (crc == EIE_SUCCEED) {
		if (strcmp (Parms->RangeString, "") == 0) {
			*RangeRoot = STC_RangeRootAllocate (dim_count);
		}
		else {
			rc = STC_RangeParse (Parms->RangeString, RangeRoot, STCM_CODE_FIRSTCHARACTER_CHARACTER_SET, STCM_CODE_CHARACTER_SET);
			EI_PrintMessages ();
			if (rc != EIE_SUCCEED) {
				*RangeRoot = NULL;
				crc = EIE_FAIL;
			}
		}
		if (*RangeRoot != NULL) {
			if (dim_count != (*RangeRoot)->NumberEntries) {
				IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgNbHierarchiesNotEqualNbDimensions,
					(*RangeRoot)->NumberEntries, RANGE_GRM_NAME, dim_count, DIMENSION_GRM_NAME);
				crc = EIE_FAIL;
			}
			else {
				for (i = 0; i < dim_count; i++) {
					IOUtil_copy_varname(Name, sp->dsr_indata.VL_dimension.names[i]);
					STC_RangeRootSetName (*RangeRoot, Name, i);
				}
				//print the range specified by user
				if (Parms->PrintCodes == EIE_TRUE) {
					STC_RangeRootPrint (*RangeRoot);
				}
				STC_PruneRange (*RangeRoot, *HTreeRoot);
				STC_AugmentRange (*RangeRoot, *HTreeRoot);
				rc = STC_ValidateCodes (*HTreeRoot, *RangeRoot);
				if (rc != EIE_SUCCEED) {
					EI_PrintMessages ();
					crc = EIE_FAIL;
				}
			}
		}
	}

	/* don't bother with group if there is a previous error */
	if (crc == EIE_SUCCEED) {
		if (strcmp (Parms->GroupString, "") == 0) {
			*GroupRoot = NULL;
		}
		else {
			rc = STC_GroupParse (Parms->GroupString, GroupRoot, *HTreeRoot, *RangeRoot, STCM_CODE_FIRSTCHARACTER_CHARACTER_SET, STCM_CODE_CHARACTER_SET);
			EI_PrintMessages ();
			if (rc != EIE_SUCCEED) {
				*GroupRoot = NULL;
				crc = EIE_FAIL;
			}
			else {
				STC_GroupRootPrint (*GroupRoot, *HTreeRoot);
			}
		}
	}
	EI_PrintMessages ();

	if (sp->unit_id.meta.is_specified == IOSV_NOT_SPECIFIED) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgParmMandatory, ID_GRM_NAME);
		crc = EIE_FAIL;
	}
	if (sp->var.meta.is_specified == IOSV_NOT_SPECIFIED) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgParmMandatory, VAR_GRM_NAME);
		crc = EIE_FAIL;
	}
	if (sp->dimension.meta.is_specified == IOSV_NOT_SPECIFIED) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgParmMandatory, DIMENSION_GRM_NAME);
		crc = EIE_FAIL;
	}
	if (Parms->M < M_MIN || Parms->M > M_MAX) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgIntegerNotInRange, M_GRM_NAME, M_MIN, M_MAX);
		crc = EIE_FAIL;
	}
	if (Parms->X < X_MIN || Parms->X > X_MAX) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgDoubleNotInRange, X_GRM_NAME, X_MIN, X_MAX);
		crc = EIE_FAIL;
	}
	if (Parms->Y < Y_MIN || Parms->Y > Y_MAX) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgDoubleNotInRange, Y_GRM_NAME, Y_MIN, Y_MAX);
		crc = EIE_FAIL;
	}
	if (Parms->Z < Z_MIN || Parms->Z > Z_MAX) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgDoubleNotInRange, Z_GRM_NAME, Z_MIN, Z_MAX);
		crc = EIE_FAIL;
	}
	if (Parms->Tolerance < TOLERANCE_MIN || Parms->Tolerance > TOLERANCE_MAX) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgDoubleNotInRange, TOLERANCE_GRM_NAME,
			TOLERANCE_MIN, TOLERANCE_MAX);
		crc = EIE_FAIL;
	}
	else if (Parms->Tolerance > TOLERANCE_WARNING) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_WARNING MsgToleranceTooBigForYourOwnGood,
			TOLERANCE_GRM_NAME, TOLERANCE_WARNING);
	}
	if (sp->min_resp.meta.is_specified == IOSV_SPECIFIED) {
		if ((double) Parms->MinResp != sp->min_resp.value )
			IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgMinRespTooBigChanged, INT_MAX);
		if (Parms->MinResp < MINRESP_MIN) {
			IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgMinRespGreaterZero);
			crc = EIE_FAIL;
		}
		else if (Parms->MinResp < MINRESP_MINCOMMONRANGE || Parms->MinResp > MINRESP_MAXCOMMONRANGE) {
			IO_PRINT_LINE (SAS_MESSAGE_PREFIX_NOTE MsgMinRespOutsideCommonRange);
		}
	}
	if (SUtil_AreDuplicateInListPosition (dim_count, sp->dsr_indata.VL_dimension.positions)) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgParmWithDuplicateVariable, DIMENSION_GRM_NAME);
		crc = EIE_FAIL;
	}
	if (SUtil_AreDuplicateInListPosition (sp->dsr_indata.dsr.VL_by_var.count,
				sp->dsr_indata.dsr.VL_by_var.positions)) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgParmWithDuplicateVariable, GPN_BY);
		crc = EIE_FAIL;
	}
	rc = ExclusivityBetweenLists (&sp->dsr_indata);
	if (rc != EIE_SUCCEED)
		crc = EIE_FAIL;
	if (Parms->Tolerance > 1.00 && Parms->MinResp > 0) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgToleranceMinRespNotCompatible);
		crc = EIE_FAIL;
	}
	if ((sp->waiver.meta.is_specified == IOSV_SPECIFIED) && (sp->p_waiver.meta.is_specified == IOSV_SPECIFIED)) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgStatementsExclusive, "WAIVER", "PWAIVER" ) ; 
		*waiver_flags_present = 0;
		crc = EIE_FAIL;
	} else {
		if      (sp->waiver.meta.is_specified == IOSV_SPECIFIED) {
			*waiver_flags_present = 1;
		}
		else if (sp->p_waiver.meta.is_specified == IOSV_SPECIFIED) {
			*waiver_flags_present = 2;
		}
		else    {
			*waiver_flags_present = 0;
		}
	}

	/*-at most one of proxyratio and proxypercentile can be specified */
	/*-if a proxy variable is included in the input dataset then either proxy ratio must be specified */
	/* or one will be calculated from the value of proxypercentile (whether a value is specified for  */
	/* proxypercentile or its default value of 0.1 is used)                                           */

	/*-proxydiagnostics cannot be requested if there is no proxy variable in the input dataset */
	/*-weightdiagnostics cannot be requested if there is no weight variable in the input dataset */
	/*-additivenoise is irrelevant if there are no weights (so should specifying it be an error?) unless there */

	/* PROXYRATIO parameter (a real number between 0.0 and 1.0 (no default value; not mandatory)) */
	if (sp->proxy_ratio.meta.is_specified == IOSV_SPECIFIED) {
		if (Parms->ProxyRatio < 0.0 || Parms->ProxyRatio > 1.0) {
			IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgDoubleNotInRange, PROXYRATIO_GRM_NAME, 0.0, 1.0);
			crc = EIE_FAIL;
		}
	}

	/* PROXYPERCENTILE parameter (a real number between 0.0 and 1.0) */
	if (sp->proxy_percentile.meta.is_specified == IOSV_SPECIFIED) {
		if (Parms->ProxyPercentile < 0.0 || Parms->ProxyPercentile > 1.0) {
			IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgDoubleNotInRange, PROXYPERCENTILE_GRM_NAME, 0.0, 1.0);
			crc = EIE_FAIL;
		}
	}

	if (sp->proxy_size.meta.is_specified == IOSV_SPECIFIED) {
		if ((sp->proxy_ratio.meta.is_specified == IOSV_SPECIFIED) && (sp->proxy_percentile.meta.is_specified == IOSV_SPECIFIED)) {
			/* IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR "Parameters \"%s\" and \"%s\" can't both be specified.", PROXYRATIO_GRM_NAME, PROXYPERCENTILE_GRM_NAME); */
			/* crc = EIE_FAIL; */
			IO_PRINT_LINE (SAS_MESSAGE_PREFIX_WARNING MsgProxypercAndProxyratioSpecified, PROXYPERCENTILE_GRM_NAME, PROXYRATIO_GRM_NAME, PROXYPERCENTILE_GRM_NAME);
		}
	}

	/* PROXYDIAG option (0 or 1, default = 0) */
	if (Parms->ProxyDiagnostics != 0 && Parms->ProxyDiagnostics != 1) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgIntegerNotInRange, PROXYDIAG_GRM_NAME, 0, 1);
		crc = EIE_FAIL;
	}

	/*-proxydiagnostics cannot be requested if there is no proxy variable in the input dataset */

	/*-there is no proxy variable in the input dataset, but at least one of the    */
	/* proxy-related parameters proxyratio, proxypercentile or proxydiagnostic was */
	/* specified; all proxy-related parameters will be ignored                     */
	if ( (sp->proxy_size.meta.is_specified == IOSV_NOT_SPECIFIED) &&
	    ((sp->proxy_ratio.meta.is_specified == IOSV_SPECIFIED) ||
	     (sp->proxy_percentile.meta.is_specified == IOSV_SPECIFIED) ||
	     (sp->proxy_diag.meta.is_specified == IOSV_SPECIFIED && sp->proxy_diag.value == IOB_TRUE)
	    )
	   ) {
	    IO_PRINT_LINE(SAS_MESSAGE_PREFIX_WARNING Msg3OptionsButNoStatement, PROXYSIZE_GRM_NAME, PROXYRATIO_GRM_NAME, PROXYPERCENTILE_GRM_NAME, PROXYDIAG_GRM_NAME, PROXYSIZE_GRM_NAME, PROXYSIZE_GRM_NAME);
	}

	/* WEIGHTPROTLEVEL parameter (a string equal to "LOW", "LINEAR", "STEP", or "EXACT", default = "MEDIUM" */
	/* (stored as 'L'(LOW), 'M'(MEDIUM), 'H'(HIGH), or 'E'(EXACT respectively))                             */
	if (sp->weight_prot_level.meta.is_specified == IOSV_SPECIFIED) {
		char* wpl = (char*) sp->weight_prot_level.value;
	    if (!(((strlen (       ((char *) wpl)    )== 3  &&
	            ((char)toupper(((char *) wpl)[0]))=='L' &&
	            ((char)toupper(((char *) wpl)[1]))=='O' &&
	            ((char)toupper(((char *) wpl)[2]))=='W'   )) ||
	          ((strlen (       ((char *) wpl)    )== 6  &&
	            ((char)toupper(((char *) wpl)[0]))=='L' &&
	            ((char)toupper(((char *) wpl)[1]))=='I' &&
	            ((char)toupper(((char *) wpl)[2]))=='N' &&
	            ((char)toupper(((char *) wpl)[3]))=='E' &&
	            ((char)toupper(((char *) wpl)[4]))=='A' &&
	            ((char)toupper(((char *) wpl)[5]))=='R'   )) ||
	          ((strlen (       ((char *) wpl)    )== 4  &&
	            ((char)toupper(((char *) wpl)[0]))=='S' &&
	            ((char)toupper(((char *) wpl)[1]))=='T' &&
	            ((char)toupper(((char *) wpl)[2]))=='E' &&
	            ((char)toupper(((char *) wpl)[3]))=='P'   )) ||
	          ((strlen (       ((char *) wpl)   ) == 5  &&
	            ((char)toupper(((char *) wpl)[0]))=='E' &&
	            ((char)toupper(((char *) wpl)[1]))=='X' &&
	            ((char)toupper(((char *) wpl)[2]))=='A' &&
	            ((char)toupper(((char *) wpl)[3]))=='C' &&
	            ((char)toupper(((char *) wpl)[4]))=='T'   ))
	         )
	       ) {
	        IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgInvalidOptionValue, ((char *) wpl), WEIGHTPROTLEVEL_GRM_NAME);
	        crc = EIE_FAIL;
	    }
	}

	/*-whether ptn sensitivity is used instead of linear sensitivity depends on whether a weight variable is specified, and doesn't */
	/* depend on whether a proxy variable is specified (if a proxy variable is specified it changes the details of how the linear   */
	/* or ptn sensitivity calculations are done, but not which type of sensitivity calculation (i.e. linear or ptn) is done)        */

	/*-when ptn sensitivity is used versus when linear sensitivity is used:                                                         */
	/*    -ptn sensitivity will NOT be used (linear sensitivity WILL     be used) iff                                               */
	/*        -a weight variable is NOT specified                                                                                   */
	/*         or a weight variable IS specified                                                                                    */
	/*         but weightprotectionlevel is specified to be "EXACT"                                                                 */
	/*    -ptn sensitivity WILL     be used (linear sensitivity will NOT be used) iff                                               */
	/*        -a weight variable IS specified                                                                                       */
	/*         and weightprotectionlevel is specified to be "STEP", "LINEAR"(default), or "LOW" (not the other possibility "EXACT") */
	/* or to summarize:                                                                                                             */
	/*     +---------+----------------------+-----------+-----------+                                                               */
	/*     |Weight   |                      |           |           |                                                               */
	/*     |variable |                      |Sensitivity|Sensitivity|                                                               */
	/*     |specified|WPL=                  |Measure    |Variables  |                                                               */
	/*     +---------+----------------------+-----------+-----------+                                                               */
	/*     |   No    |    -                 |  Linear   |    X      |                                                               */
	/*     +---------+----------------------+-----------+-----------+                                                               */
	/*     |         |  EXACT ('E'(EXACT  ) |  LINEAR   |    WX     |                                                               */
	/*     |         +----------------------+-----------+-----------+                                                               */
	/*     |         |  STEP  ('H'(HIGH   ) |           |           |                                                               */
	/*     |   Yes   +----------------------+           |           |                                                               */
	/*     |         | LINEAR ('M'(MEDIUM)) |   PTN     |           |                                                               */
	/*     |         |(default)             |           |           |                                                               */
	/*     |         +----------------------+           |           |                                                               */
	/*     |         |   LOW  ('L'(LOW    ) |           |           |                                                               */
	/*     +---------+----------------------+-----------+-----------+                                                               */
	if (sruleparsed                                &&
	    (Parms->Weight                == 1       ) &&
	    (Parms->WeightProtectionLevel != 'E'     ) &&
	    (((*SRule)->Type != STCE_SRULE_TYPE_PQ   ) &&
	     ((*SRule)->Type != STCE_SRULE_TYPE_NK   ) &&
	     ((*SRule)->Type != STCE_SRULE_TYPE_C2   )
	    )
	   ) {
	    IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgWeightSruleInvalid);
	    crc = EIE_FAIL;
	}
	if (sruleparsed                                &&
	    (Parms->Weight                == 1       ) &&
	    (Parms->WeightProtectionLevel != 'E'     ) &&
	    (((*SRule)->Type == STCE_SRULE_TYPE_NK
	     ) &&
	     ((*SRule)->NumberGroups > 2             )
	    )
	   ) {
	    IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgSruleNkPtnTooManyRules);
	    crc = EIE_FAIL;
	}


	

	if (Parms->MinRespW != MINRESPW_UNSPECD_VALUE) {
		if (Parms->MinRespW < MINRESPW_MIN) { /* MINRESPW parameter (a real number >= 0.0) */
			IO_PRINT_LINE(SAS_MESSAGE_PREFIX_ERROR MsgParameterMustBeGreaterThanX, MINRESPW_GRM_NAME, MINRESPW_MIN);
			crc = EIE_FAIL;
		}	// Note that the "else" here means that, in the case both validations fail, only the FIRST failed validation appears in the SAS log
		else if (sp->weight.meta.is_specified == IOSV_NOT_SPECIFIED) { /* Weight MANDATORY when valid MINRESPW specified*/
			IO_PRINT_LINE(SAS_MESSAGE_PREFIX_ERROR MsgParam1RequiresParam2, MINRESPW_GRM_NAME, WEIGHT_GRM_NAME);
			crc = EIE_FAIL;
		}
	}

	/* ADDITIVENOISE option (0 or 1, default = 1) */
	if (Parms->AdditiveNoise != 0 && Parms->AdditiveNoise != 1) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgIntegerNotInRange, ADDITIVENOISE_GRM_NAME, 0, 1);
		crc = EIE_FAIL;
	}

	/* WEIGHTDIAG option (0 or 1, default = 0) */
	if (Parms->WeightDiagnostics != 0 && Parms->WeightDiagnostics != 1) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgIntegerNotInRange, WEIGHTDIAG_GRM_NAME, 0, 1);
		crc = EIE_FAIL;
	}

	/* ACCEPTNEGATIVE option (0 or 1, default = 0) */
	if (Parms->AcceptNegativeValues != 0 && Parms->AcceptNegativeValues != 1) {
		IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgIntegerNotInRange, ACCEPTNEGATIVE_GRM_NAME, 0, 1);
		crc = EIE_FAIL;
	}

	/*-there must be a PROXYSIZE variable in the dataset if ACCEPTNEGATIVE is specified: */

	/*-ACCEPTNEGATIVE must be specified if there is a PROXYSIZE variable in the dataset: */
	if (Parms->AcceptNegativeValues == 0 && Parms->Proxy != 0) {
	    /* IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR "Parameter \"%s\" must be requested if there is a proxy variable in the input dataset.", ACCEPTNEGATIVE_GRM_NAME); */
	    IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgParameterRequired, ACCEPTNEGATIVE_GRM_NAME, PROXYSIZE_GRM_NAME);
	    crc = EIE_FAIL;
	}

	if (
		sp->weight.meta.is_specified == IOSV_NOT_SPECIFIED
		&& (
			sp->weight_prot_level.meta.is_specified == IOSV_SPECIFIED
			|| (sp->weight_diag.meta.is_specified == IOSV_SPECIFIED && sp->weight_diag.value == IOB_TRUE)
		)
	) {
		IO_PRINT_LINE(SAS_MESSAGE_PREFIX_WARNING Msg2OptionsButNoStatement, WEIGHT_GRM_NAME, WEIGHTPROTLEVEL_GRM_NAME, WEIGHTDIAG_GRM_NAME, WEIGHT_GRM_NAME, WEIGHT_GRM_NAME);
	}
	/*-weightdiagnostics cannot be requested if there is no weight variable in the input dataset */
	/* don't bother checking whether requests for outtargets and outpairs files are consistent with other parameters if srule wasn't successfully parsed: */
	if (sruleparsed) {
		if (sp->dsw_outtargets.is_requested == IOB_TRUE && !(Parms->Weight != 0 && Parms->WeightProtectionLevel != 'E' && (*SRule)->Type == STCE_SRULE_TYPE_NK)) {
		    IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgOuttargetsInvalid);
		    crc = EIE_FAIL;
		}
		if (sp->dsw_outpairs.is_requested == IOB_TRUE && !(Parms->Weight != 0 && Parms->WeightProtectionLevel != 'E' && (*SRule)->Type == STCE_SRULE_TYPE_PQ)) {
		    IO_PRINT_LINE (SAS_MESSAGE_PREFIX_ERROR MsgOutpairsInvalid);
		    crc = EIE_FAIL;
		}
	}

	return crc;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE WriteCellObs (
	STCT_CELL * Cell,
	int NumberDimensions)
{
	int i;
	int weight_specified     = 0;
	int weight_included      = 0;

	int shadow_specified     = 0;
	int shadowtotal_included = 0;

	int proxysize_specified  = 0;
	int totalproxy_included  = 0;

	int waivers_specified    = 0;
	int sensitivnwv_included = 0;
	int favcost_included     = 0;

	SP_sensitiv* sp = mWriteInfo.sp;

	shadow_specified    = (sp->shadow.meta.is_specified == IOSV_SPECIFIED);
	proxysize_specified = (sp->proxy_size.meta.is_specified == IOSV_SPECIFIED);
	waivers_specified   = (sp->waiver.meta.is_specified == IOSV_SPECIFIED) || (sp->p_waiver.meta.is_specified == IOSV_SPECIFIED);
	weight_specified    = (sp->weight.meta.is_specified == IOSV_SPECIFIED);

	if (weight_specified) {
		weight_included = 1;
	}
	if (shadow_specified) {
	    shadowtotal_included = 1;
	}
	if (proxysize_specified) {
	    totalproxy_included = 1;
	}
	if (waivers_specified) {
	    sensitivnwv_included = 1;
	    favcost_included     = 1;
	}

	mWriteInfo.OutCellDataSet->Variable[CELL_CELLID_INDEX].Value.d = Cell->CellId;
	mWriteInfo.OutCellDataSet->Variable[CELL_NBANONYM_INDEX].Value.d = Cell->AnonymousDataItem.NumberObservations;
	mWriteInfo.OutCellDataSet->Variable[CELL_ANONYM_INDEX].Value.d = (weight_specified)?(Cell->AnonymousDataItem.WeightedValue):(Cell->AnonymousDataItem.Value);
	mWriteInfo.OutCellDataSet->Variable[CELL_NBRESPONDENTS_INDEX].Value.d = Cell->Data.NumberEntries;
	if (weight_specified) {
		mWriteInfo.OutCellDataSet->Variable[CELL_WEIGHTEDNBRESPONDENTS_INDEX].Value.d = Cell->WeightedNbResp;
	}
	mWriteInfo.OutCellDataSet->Variable[CELL_TOTAL_INDEX(weight_included)].Value.d = (weight_specified) ? (Cell->TotalWeightedValue) : (Cell->TotalValue);
	mWriteInfo.OutCellDataSet->Variable[CELL_SENSITIVITY_INDEX(weight_included)].Value.d = Cell->Sensitivity;
	mWriteInfo.OutCellDataSet->Variable[CELL_STATUS_INDEX(weight_included)].Value.s[0] = (Cell->Sensitivity > 0 ? STC_CELLSTATUS_SENSITIVE : STC_CELLSTATUS_NOTSENSITIVE);
	mWriteInfo.OutCellDataSet->Variable[CELL_TYPE_INDEX(weight_included)].Value.s[0] = Cell->Type;
	mWriteInfo.OutCellDataSet->Variable[CELL_TOTALNOISE_INDEX(weight_included)].Value.d =
	     (Cell->WhichNoise=='v')?Cell->TotalValueX
	    :(Cell->WhichNoise=='p')?Cell->TotalProxyX
	    :(Cell->WhichNoise=='V')?Cell->TotalWeightedValueX
	    :(Cell->WhichNoise=='P')?Cell->TotalWeightedProxyX
	    :(Cell->WhichNoise=='n')?Cell->TotalValueN
	    :(Cell->WhichNoise=='N')?Cell->TotalProxyN
	    :0.0;
	
	if ((Cell->WhichNoise!='v') &&
	    (Cell->WhichNoise!='p') &&
	    (Cell->WhichNoise!='V') &&
	    (Cell->WhichNoise!='P') &&
	    (Cell->WhichNoise!='n') &&
	    (Cell->WhichNoise!='N')
	   ) 
	{
	    IO_PRINT_LINE(SAS_MESSAGE_PREFIX_ERROR MsgInvalidValueInFunctionCall, "Cell->WhichNoise", "WriteCellObs()");
	}
	if (shadow_specified) {
		mWriteInfo.OutCellDataSet->Variable[CELL_SHADOWTOTAL_INDEX(weight_included)].Value.d = (weight_specified)?(Cell->TotalWeightedShadow):(Cell->TotalShadow);
	}
	if (proxysize_specified) {
		mWriteInfo.OutCellDataSet->Variable[CELL_TOTALPROXY_INDEX(weight_included, shadowtotal_included)].Value.d = (weight_specified)?(Cell->TotalWeightedProxy):(Cell->TotalProxy);
	}
	if (waivers_specified) {
		mWriteInfo.OutCellDataSet->Variable[CELL_SENSITIVNWV_INDEX(weight_included, shadowtotal_included, totalproxy_included)].Value.d = Cell->Sensitivity_nowaivers;
		mWriteInfo.OutCellDataSet->Variable[CELL_FAVCOST_INDEX(weight_included, shadowtotal_included, totalproxy_included, sensitivnwv_included)].Value.d = Cell->FavCost;
	}
	if (Cell->Coordinate == NULL) { /* pour le cas d'aggregat sensible */
		for (i = 0; i < NumberDimensions; i++)
			SVariableCopy (&mWriteInfo.OutCellDataSet->Variable[CELL_FIRST_CODE_INDEX(weight_included, shadowtotal_included, totalproxy_included, sensitivnwv_included, favcost_included)+i], "");
	} else {
		for (i = 0; i < Cell->Coordinate->NumberEntries; i++)
			SVariableCopy (&mWriteInfo.OutCellDataSet->Variable[CELL_FIRST_CODE_INDEX(weight_included, shadowtotal_included, totalproxy_included, sensitivnwv_included, favcost_included)+i],
				mHTreeRoot->TagIndex[i][Cell->Coordinate->Tag[i]]->Code);
	}

	if (IORC_SUCCESS != DSW_add_record(&sp->dsw_outcell)) {
		return EIE_FAIL;
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE WriteConstraintObs (
	int ConstraintId,
	int CellId,
	int Coefficient)
{
	SP_sensitiv* sp = mWriteInfo.sp;

	mWriteInfo.OutConstraintDataSet->Variable[CONSTRAINT_CONSTRAINTID_INDEX].Value.d = ConstraintId;
	mWriteInfo.OutConstraintDataSet->Variable[CONSTRAINT_CELLID_INDEX].Value.d = CellId;
	mWriteInfo.OutConstraintDataSet->Variable[CONSTRAINT_COEFFICIENT_INDEX].Value.d = Coefficient;

	if (IORC_SUCCESS != DSW_add_record(&sp->dsw_outconstraint)) {
		return EIE_FAIL;
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE WriteLargestObs (
	int CellId,
	char * Id,
	int NumberObs,
	double TotalValue,
	double Value,
	double TotalShadow,
	double Shadow,
	int WaiverFlag)
{
	SP_sensitiv* sp = mWriteInfo.sp;

	int shadow_specified       = 0;
	int shadowtotal_included   = 0;
	int shadowpercent_included = 0;
	int waivers_specified      = 0;
	int waiverflag_included    = 0;
	
	shadow_specified  = (sp->shadow.meta.is_specified == IOSV_SPECIFIED);
	waivers_specified = (((sp->dsr_indata.VL_waiver.count != 0) ||
	                      (sp->dsr_indata.VL_p_waiver.count != 0)
	                     ) ? 1 : 0
	                    );
	if (shadow_specified) {
	    shadowtotal_included   = 1;
	    shadowpercent_included = 1;
	}
	if (waivers_specified) {
	    waiverflag_included = 1;
	}

	if (sp->dsw_outlargest.is_requested == IOB_FALSE)
		return EIE_SUCCEED;

	mWriteInfo.OutLargestDataSet->Variable[LARGEST_CELLID_INDEX].Value.d = CellId;
	SVariableCopy (&mWriteInfo.OutLargestDataSet->Variable[LARGEST_ID_INDEX], Id);
	mWriteInfo.OutLargestDataSet->Variable[LARGEST_NBRESPONDENTS_INDEX].Value.d = NumberObs;
	mWriteInfo.OutLargestDataSet->Variable[LARGEST_TOTAL_INDEX].Value.d = Value;
	mWriteInfo.OutLargestDataSet->Variable[LARGEST_TOTALPERCENT_INDEX].Value.d =
		(TotalValue == 0.0 ? 100.0 : 100.0 * Value / TotalValue);
	if (shadow_specified != 0) {
		mWriteInfo.OutLargestDataSet->Variable[LARGEST_SHADOWTOTAL_INDEX].Value.d = Shadow;
		mWriteInfo.OutLargestDataSet->Variable[LARGEST_SHADOWPERCENT_INDEX].Value.d =
			(TotalShadow == 0.0 ? 100.0 : 100.0 * Shadow / TotalShadow);
	}
	if (waivers_specified) {
	    mWriteInfo.OutLargestDataSet->Variable[LARGEST_WAIVERFLAG_INDEX(shadowtotal_included, shadowpercent_included)].Value.d = WaiverFlag;
	}

	if (IORC_SUCCESS != DSW_add_record(&sp->dsw_outlargest)) {
		return EIE_FAIL;
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE WriteTargetsObs (
	int CellId,
	char * Id,
	char * PtnVariable,
	double Value)
{
	SP_sensitiv* sp = mWriteInfo.sp;

	if (!(sp->dsw_outtargets.is_requested == IOB_TRUE && Parms.Weight != 0 && Parms.WeightProtectionLevel != 'E' && SRule->Type == STCE_SRULE_TYPE_NK)) {
		return EIE_SUCCEED;
	}

	mWriteInfo.OutTargetsDataSet->Variable[TARGETS_CELLID_INDEX].Value.d = CellId;
	SVariableCopy (&mWriteInfo.OutTargetsDataSet->Variable[TARGETS_ID_INDEX], Id);
	SVariableCopy (&mWriteInfo.OutTargetsDataSet->Variable[TARGETS_PTNVARIABLE_INDEX], PtnVariable);
	mWriteInfo.OutTargetsDataSet->Variable[TARGETS_VALUE_INDEX].Value.d = Value;

	if (IORC_SUCCESS != DSW_add_record(&sp->dsw_outtargets)) {
		return EIE_FAIL;
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE WritePairsObs (
	int CellId,
	char * TargetId,
	double TargetPt,
	char * AttackerId,
	double AttackerSn,
	int RemainderCount,
	double RemainderN)
{
	SP_sensitiv* sp = mWriteInfo.sp;

	if (!(sp->dsw_outpairs.is_requested == IOB_TRUE && Parms.Weight != 0 && Parms.WeightProtectionLevel != 'E' && SRule->Type == STCE_SRULE_TYPE_PQ)) {
		return EIE_SUCCEED;
	}

	mWriteInfo.OutPairsDataSet->Variable[PAIRS_CELLID_INDEX].Value.d = CellId;
	SVariableCopy (&mWriteInfo.OutPairsDataSet->Variable[PAIRS_TARGETID_INDEX], TargetId);
	mWriteInfo.OutPairsDataSet->Variable[PAIRS_TARGETPT_INDEX].Value.d = TargetPt;
	SVariableCopy (&mWriteInfo.OutPairsDataSet->Variable[PAIRS_ATTACKERID_INDEX], AttackerId);
	mWriteInfo.OutPairsDataSet->Variable[PAIRS_ATTACKERSN_INDEX].Value.d = AttackerSn;
	mWriteInfo.OutPairsDataSet->Variable[PAIRS_REMAINDERCOUNT_INDEX].Value.d = RemainderCount;
	mWriteInfo.OutPairsDataSet->Variable[PAIRS_REMAINDERN_INDEX].Value.d = RemainderN;

	if (IORC_SUCCESS != DSW_add_record(&sp->dsw_outpairs)) {
		return EIE_FAIL;
	}

	return EIE_SUCCEED;
}
