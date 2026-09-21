#ifndef DETERMIN_JIO_H
#define DETERMIN_JIO_H


#include "proc_common.h"
#include "GConfidIdentifiers.h"
#include "IOUtil.h"
#include "SUtil.h"
#include "RuntimeDiagnostics.h"   // header defines macros for measuring execution time (CPU and WALL)

/******************************** MACRO DEFINITIONS **************************/
#define PROC_NAME         "SENSITIVITY"

/******************************** ENUMERATIONS *******************************/
/****** GENERATED CODE START: Procedure_IO_analysis.xlsx:Parameter Procedure ->[enum code] **************************
    These enumeration MUST NOT assign values explicitly.  
    `***__COUNT` MUST be the final element listed, ensuring it
    indicates the number of other elements in the enumeration.  */

typedef enum __user_parameter_order {
    UPO_UNIT_ID,
    UPO_BY,
    UPO_VAR,
    UPO_SHADOW,
    UPO_DIMENSION,
    UPO_WAIVER,
    UPO_P_WAIVER,
    UPO_PROXY_SIZE,
    UPO_WEIGHT,
    UPO_S_RULE,
    UPO_HIERARCHY,
    UPO_RANGE,
    UPO_M,
    UPO_X,
    UPO_Y,
    UPO_Z,
    UPO_TOLERANCE,
    UPO_MIN_RESP,
    UPO_PROXY_RATIO,
    UPO_PROXY_PERCENTILE,
    UPO_WEIGHT_PROT_LEVEL,
    UPO_MIN_RESP_W,
    UPO_DEBUG_FILE_PREFIX,
    UPO_DEBUG_WORK_DIR_PATH,
    UPO_VERBOSE,
    UPO_TIMER,
    UPO_PRINT_CODES,
    UPO_LIMIT_WARNINGS,
    UPO_ADDITIVE_NOISE,
    UPO_PROXY_DIAG,
    UPO_WEIGHT_DIAG,
    UPO_ACCEPT_NEGATIVE,
    UPO__COUNT,
} user_parameter_order;

typedef enum __input_dataset_order {
    DSIO_INDATA,
    DSIO__COUNT,
} input_dataset_order;

typedef enum __output_dataset_order {
    DSOO_OUTCONSTRAINT,
    DSOO_OUTCELL,
    DSOO_OUTLARGEST,
    DSOO_OUTPAIRS,
    DSOO_OUTTARGETS,
    DSOO__COUNT,
} output_dataset_order;
/****** GENERATED CODE END: Procedure_IO_analysis.xlsx:Parameter Procedure ->[enum code] *****************************/

/******************************** STRUCTURES *********************************/
typedef struct __DSR_indata {
    /* GENERIC INFO */
    DSR_generic dsr;

    DS_varlist VL_var;
    DS_varlist VL_shadow;
    DS_varlist VL_dimension;
    DS_varlist VL_waiver;
    DS_varlist VL_p_waiver;
    DS_varlist VL_proxy_size;
    DS_varlist VL_weight;

} DSR_indata;

typedef struct __SP_SENSITIV {
    /* GENERIC INFO */
    SP_generic spg;

    /* INPUT DATA */
    DSR_indata dsr_indata;

    /* OUTPUT DATA*/
    DSW_generic dsw_outconstraint;
    DSW_generic dsw_outcell;
    DSW_generic dsw_outlargest;
    DSW_generic dsw_outpairs;
    DSW_generic dsw_outtargets;

    /* INPUT PARAMETERS */
/****** GENERATED CODE START: Procedure_IO_analysis.xlsx:Parameter Procedure ->[struct code] ***************************/
    UP_QS unit_id;
    UP_QS by;
    UP_QS var;
    UP_QS shadow;
    UP_QS dimension;
    UP_QS waiver;
    UP_QS p_waiver;
    UP_QS proxy_size;
    UP_QS weight;
    UP_QS s_rule;
    UP_QS hierarchy;
    UP_QS range;
    UP_INT m;
    UP_INTNBR x;
    UP_INTNBR y;
    UP_INTNBR z;
    UP_INTNBR tolerance;
    UP_INT min_resp;
    UP_INTNBR proxy_ratio;
    UP_INTNBR proxy_percentile;
    UP_QS weight_prot_level;
    UP_INTNBR min_resp_w;
    UP_QS debug_file_prefix;
    UP_QS debug_work_dir_path;
    UP_FLAG verbose;
    UP_FLAG timer;
    UP_FLAG print_codes;
    UP_FLAG limit_warnings;
    UP_FLAG additive_noise;
    UP_FLAG proxy_diag;
    UP_FLAG weight_diag;
    UP_FLAG accept_negative;
/****** GENERATED CODE END: Procedure_IO_analysis.xlsx:Parameter Procedure ->[struct code] *****************************/
} SP_sensitiv;

/*************************** FUNCTION PROTOTYPES ******************************/
IO_RETURN_CODE DSR_indata_init(SP_sensitiv* sp, T_in_ds in_ds_indata);

IO_RETURN_CODE SP_init(
    SP_sensitiv* sp,

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
);
PROC_RETURN_CODE SP_wrap(SP_sensitiv* sp);

#endif