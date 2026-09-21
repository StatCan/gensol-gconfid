#ifndef PROC_COMMON_H
#define PROC_COMMON_H
/* This file contains code common to all procedures */

#include "MessageGConfid.h"
#include "PublicHelpers.h"
#include "ExportHelper.h"
#include "IOUtil.h"

#include "RuntimeDiagnostics.h"   // `init_debug_env()`
#include <omp.h>    // `omp_get_max_threads() 


/***** Macros *****/

#define PRINT_STATISTICS_WIDTH_DEFAULT 60

/* Proc Return Code
    Used internally at all levels within each procedure */
typedef enum __PRC_PROC_RETURN_CODE {
    PRC_SUCCESS,
    PRC_SUCCESS_NO_MORE_DATA,
    // generic error
    PRC_FAIL_UNHANDLED,
    // dataset reading related errors
    PRC_FAIL_BY_NOT_SORTED,
    PRC_FAIL_REC_NOT_SORTED,
    PRC_FAIL_SYNC_NOT_SORTED,
    PRC_FAIL_SYNC_GENERIC,
    PRC_FAIL_GET_REC,
    PRC_FAIL_ADVANCE_REC,
    PRC_FAIL_ADVANCE_BY,
    PRC_FAIL_DUPLICATE_REC,
    // dataset writing related errors
    PRC_FAIL_WRITE_DATA,
    // processing related errors
    PRC_FAIL_PROCESSING_GENERIC,
    PRC_FAIL_DETERMINISTIC,
    PRC_FAIL_DONORIMP_RANDNUMVAR,
    PRC_FAIL_ESTIMATO_INVALID_WEIGHT_VARIANCE,
    PRC_FAIL_ESTIMATO_COMPUTE_BETAS,
    PRC_FAIL_ESTIMATO_DO_ESTIMATION,
    PRC_FAIL_PRORATE_RANKING,
    PRC_FAIL_ERRORLOC_DO_LOCALIZATION,
    PRC_FAIL_ERRORLOC_BOUNDS,
    PRC_FAIL_ERRORLOC_RANDOM_NUM,
} PROC_RETURN_CODE;

/* Banff Return Code
        !!MUST BE NON-NEGATIVE!!
        negative values reserved for host application (Python)
    Returned to the calling process to indicate
    the Banff procedure's status */
typedef enum __GRC_BANFF_RETURN_CODE {
    // success
    GRC_SUCCESS,
    // generic unhandled error
    GRC_FAIL_UNHANDLED,
    // setup related errors
    GRC_FAIL_INIT_IN_DATASET,
    GRC_FAIL_READ_PARMS_LEGACY,
    GRC_FAIL_MISSING_DATASET,
    GRC_FAIL_VARLIST_NOT_FOUND,
    GRC_FAIL_VARLIST_INVALID_COUNT,
    GRC_FAIL_VARLIST_SYSTEM_GENERATED,
    GRC_FAIL_SETUP_DATASET_IN,
    GRC_FAIL_SETUP_OTHER,
    GRC_FAIL_ALLOCATE_MEMORY,
    GRC_FAIL_LPI_INIT,
    GRC_FAIL_EDITS_PARSE,
    // validation related errors
    GRC_FAIL_VALIDATION_LEGACY,
    GRC_FAIL_VALIDATION_NEW,
    GRC_FAIL_EDITS_OTHER,
    GRC_FAIL_EDITS_CONSISTENCY,
    GRC_FAIL_EDITS_REDUNDANCY,
    GRC_FAIL_NAME_TOO_LONG,
    // dataset related errors
    GRC_FAIL_READ_GENERIC,
    GRC_FAIL_WRONG_SORT_ORDER,
    GRC_FAIL_READ_SYNC,
    GRC_FAIL_READ_DUPLICATE_DATA,
    // output related errors
    GRC_FAIL_WRITE_GENERIC,
    // processing related errors
    GRC_FAIL_PROCESSING_GENERIC,
    GRC_EIE_TRANSFORM_FAIL,
    GRC_EIE_KDTREE_FAIL,
    GRC_EIE_MATCHFIELDS_FAIL,
} GCONFID_RETURN_CODE;

/***** Common Structures *****/

typedef struct __DSR_instatus {
    /* GENERIC INFO */
    DSR_generic dsr;

    /* VARIABLE LISTS */
    // for <unit_id>, will use `dsr.VL_unit_id
    DS_varlist VL_field_id;	//FIELD_ID: mandatory
    DS_varlist VL_status;	//STATUS: mandatory
} DSR_instatus;

/***** Common Functions *****/
GCONFID_RETURN_CODE SP_validate_init(IO_RETURN_CODE init_rc);
GCONFID_RETURN_CODE convert_processing_rc(PROC_RETURN_CODE rc);
void deinit_runtime_env();
void init_runtime_env();
EXPORTED_FUNCTION const char* get_rc_description(GCONFID_RETURN_CODE brc);

#endif