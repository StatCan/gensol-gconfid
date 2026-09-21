#include "proc_common.h"

#include "PlatformSupport.h"


/* Statcan Procedurefunctions
    `SP_*` functions are procedure-specific */
    /* SP_validate_init
        generic function validates return code from procedure-specific `SP_init()` function */
GCONFID_RETURN_CODE SP_validate_init(IO_RETURN_CODE init_rc) {
    if (init_rc == IORC_SUCCESS) {
        return GRC_SUCCESS;
    }

    if (init_rc == IORC_VARLIST_NOT_FOUND
        || init_rc == IORC_BY_VARLIST_NOT_FOUND) {
        return GRC_FAIL_VARLIST_NOT_FOUND;
    }
    else if (init_rc == IORC_VAR_NAME_TOO_LONG) {
        return GRC_FAIL_NAME_TOO_LONG;
    }
    else if (init_rc == IORC_DATASET_MISSING) {
        return GRC_FAIL_MISSING_DATASET;
    }
    else if (init_rc == IORC_FAIL_INIT_IN_DATASET) {
        return GRC_FAIL_INIT_IN_DATASET;
    }
    else if (init_rc == IORC_FAIL_INIT_OUT_DATASET) {
        return GRC_FAIL_WRITE_GENERIC;
    }
    else if (init_rc == IORC_VARLIST_INVALID_COUNT) {
        return GRC_FAIL_VARLIST_INVALID_COUNT;
    }

    return GRC_FAIL_UNHANDLED;
}

/* convert_processing_rc
    This will probably be renamed and moved elsewhere, but here's the initial implementation */
GCONFID_RETURN_CODE convert_processing_rc(PROC_RETURN_CODE rc) {
    switch (rc) {
        // success
    case PRC_SUCCESS: // fall through
    case PRC_SUCCESS_NO_MORE_DATA:
        return GRC_SUCCESS;

        // sort order
    case PRC_FAIL_REC_NOT_SORTED: // fall through
    case PRC_FAIL_BY_NOT_SORTED:
        return GRC_FAIL_WRONG_SORT_ORDER;

        // dataset syncronization 
    case PRC_FAIL_SYNC_GENERIC: // fall through
    case PRC_FAIL_SYNC_NOT_SORTED:
        return GRC_FAIL_READ_SYNC;

        // dataset reading
    case PRC_FAIL_GET_REC: // fall through
    case PRC_FAIL_ADVANCE_REC: // fall through
    case PRC_FAIL_ADVANCE_BY:
        return GRC_FAIL_READ_GENERIC;

        // duplicate data
    case PRC_FAIL_DUPLICATE_REC:
        return GRC_FAIL_READ_DUPLICATE_DATA;

        // processing error (all fall through)
    case PRC_FAIL_PROCESSING_GENERIC:
    case PRC_FAIL_DETERMINISTIC:
    case PRC_FAIL_DONORIMP_RANDNUMVAR:
    case PRC_FAIL_ERRORLOC_DO_LOCALIZATION:
    case PRC_FAIL_ERRORLOC_BOUNDS:
    case PRC_FAIL_ERRORLOC_RANDOM_NUM:
    case PRC_FAIL_ESTIMATO_INVALID_WEIGHT_VARIANCE:
    case PRC_FAIL_ESTIMATO_COMPUTE_BETAS:
    case PRC_FAIL_ESTIMATO_DO_ESTIMATION:
    case PRC_FAIL_PRORATE_RANKING:
        return GRC_FAIL_PROCESSING_GENERIC;

        // writing data
    case PRC_FAIL_WRITE_DATA:
        return GRC_FAIL_WRITE_GENERIC;

        // unexpected error
    case PRC_FAIL_UNHANDLED: // fall through
    default:
        return GRC_FAIL_UNHANDLED;
    }
}

static UINT saved_codepage;

void deinit_runtime_env() {
    // restore original console encoding
    set_console_output_encoding(saved_codepage);

    /* Flush standard output: otherwise, it may become mixed with
        output from other calls/programs */
    flush_std_buffers();
}

void init_runtime_env() {
    // set console encoding to UTF-8
    saved_codepage = set_console_output_encoding(UTF8_ENCODING_ID);
	init_debug_env("GCONFID_DEBUG_STATS");

#ifdef _OPENMP
    omp_set_dynamic(1 == 1); /* TRUE: enable dynamic threads */
#endif
}

EXPORTED_FUNCTION const char* get_rc_description(GCONFID_RETURN_CODE brc) {
    switch (brc) {
    case GRC_SUCCESS:
        return RC_DESC_SUCCESS;
        break;
    case GRC_FAIL_UNHANDLED:
        return RC_DESC_FAIL_UNHANDLED;
        break;

    // setup related errors
    case GRC_FAIL_INIT_IN_DATASET:
        return RC_DESC_FAIL_INIT_IN_DATASET;
        break;
    case GRC_FAIL_READ_PARMS_LEGACY:
        return RC_DESC_FAIL_READ_PARMS_LEGACY;
        break;
    case GRC_FAIL_MISSING_DATASET:
        return RC_DESC_FAIL_MISSING_DATASET;
        break;
    case GRC_FAIL_VARLIST_NOT_FOUND:
        return RC_DESC_FAIL_VARLIST_NOT_FOUND;
        break;
    case GRC_FAIL_VARLIST_INVALID_COUNT:
        return RC_DESC_FAIL_VARLIST_INVALID_COUNT;
        break;
    case GRC_FAIL_VARLIST_SYSTEM_GENERATED:
        return RC_DESC_FAIL_VARLIST_SYSTEM_GENERATED;
        break;
    case GRC_FAIL_SETUP_DATASET_IN:
        return RC_DESC_FAIL_SETUP_DATASET_IN;
        break;
    case GRC_FAIL_SETUP_OTHER:
        return RC_DESC_FAIL_SETUP_OTHER;
        break;
    case GRC_FAIL_ALLOCATE_MEMORY:
        return RC_DESC_FAIL_ALLOCATE_MEMORY;
        break;
    case GRC_FAIL_LPI_INIT:
        return RC_DESC_FAIL_LPI_INIT;
        break;
    case GRC_FAIL_EDITS_PARSE:
        return RC_DESC_FAIL_EDITS_PARSE;
        break;
    // validation related errors
    case GRC_FAIL_VALIDATION_LEGACY:
        return RC_DESC_FAIL_VALIDATION_LEGACY;
        break;
    case GRC_FAIL_VALIDATION_NEW:
        return RC_DESC_FAIL_VALIDATION_NEW;
        break;
    case GRC_FAIL_EDITS_OTHER:
        return RC_DESC_FAIL_EDITS_OTHER;
        break;
    case GRC_FAIL_EDITS_CONSISTENCY:
        return RC_DESC_FAIL_EDITS_CONSISTENCY;
        break;
    case GRC_FAIL_EDITS_REDUNDANCY:
        return RC_DESC_FAIL_EDITS_REDUNDANCY;
        break;
    case GRC_FAIL_NAME_TOO_LONG:
        return RC_DESC_FAIL_NAME_TOO_LONG;
        break;
    // dataset related errors
    case GRC_FAIL_READ_GENERIC:
        return RC_DESC_FAIL_READ_GENERIC;
        break;
    case GRC_FAIL_WRONG_SORT_ORDER:
        return RC_DESC_FAIL_WRONG_SORT_ORDER;
        break;
    case GRC_FAIL_READ_SYNC:
        return RC_DESC_FAIL_READ_SYNC;
        break;
    case GRC_FAIL_READ_DUPLICATE_DATA:
        return RC_DESC_FAIL_READ_DUPLICATE_DATA;
        break;
        // output related errors
    case GRC_FAIL_WRITE_GENERIC:
        return RC_DESC_FAIL_WRITE_GENERIC;
        break;
    // processing related errors
    case GRC_FAIL_PROCESSING_GENERIC:
        return RC_DESC_FAIL_PROCESSING_GENERIC;
        break;
    case GRC_EIE_TRANSFORM_FAIL:
        return RC_DESC_EIE_TRANSFORM_FAIL;
        break;
    case GRC_EIE_KDTREE_FAIL:
        return RC_DESC_EIE_KDTREE_FAIL;
        break;
    case GRC_EIE_MATCHFIELDS_FAIL:
        return RC_DESC_EIE_MATCHFIELDS_FAIL;
        break;
    default:
        return "";
    }
}