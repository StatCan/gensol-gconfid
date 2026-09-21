#include "sensitiv_JIO.h"

IO_RETURN_CODE DSR_indata_init(SP_sensitiv* sp, T_in_ds in_ds_indata) {
    IO_RETURN_CODE rc_io = IORC_SUCCESS;

    /* initialize DSR */
    DSR_indata* dsr = &sp->dsr_indata;
    rc_io = DSR_init(&sp->spg, DSIO_INDATA, 
        &dsr->dsr, DSN_INDATA, in_ds_indata, sp->unit_id.value, sp->by.value, 
        IOB_FALSE, 
        7,
            &sp->dsr_indata.VL_dimension,
            &sp->dsr_indata.VL_proxy_size,
            &sp->dsr_indata.VL_p_waiver,
            &sp->dsr_indata.VL_shadow,
            &sp->dsr_indata.VL_var,
            &sp->dsr_indata.VL_waiver,
            &sp->dsr_indata.VL_weight

    );
    if (rc_io != IORC_SUCCESS) {
        return rc_io;
    }

    /* initialize procedure-specific varlists */
    rc_io = VL_init_single(&dsr->VL_var, &dsr->dsr, GPN_VAR, sp->var.value, IOVT_NUM);
    if (rc_io != IORC_SUCCESS) {
        return rc_io;
    }
    rc_io = VL_init_single(&dsr->VL_shadow, &dsr->dsr, GPN_SHADOW, sp->shadow.value, IOVT_NUM);
    if (rc_io != IORC_SUCCESS) {
        return rc_io;
    }
    rc_io = VL_init(&dsr->VL_dimension, &dsr->dsr, GPN_DIMENSION, sp->dimension.value, IOVT_CHAR);
    if (rc_io != IORC_SUCCESS) {
        return rc_io;
    }
    rc_io = VL_init_single(&dsr->VL_waiver, &dsr->dsr, GPN_WAIVER, sp->waiver.value, IOVT_NUM);
    if (rc_io != IORC_SUCCESS) {
        return rc_io;
    }
    rc_io = VL_init_single(&dsr->VL_p_waiver, &dsr->dsr, GPN_P_WAIVER, sp->p_waiver.value, IOVT_NUM);
    if (rc_io != IORC_SUCCESS) {
        return rc_io;
    }
    rc_io = VL_init_single(&dsr->VL_proxy_size, &dsr->dsr, GPN_PROXY_SIZE, sp->proxy_size.value, IOVT_NUM);
    if (rc_io != IORC_SUCCESS) {
        return rc_io;
    }
    rc_io = VL_init_single(&dsr->VL_weight, &dsr->dsr, GPN_WEIGHT, sp->weight.value, IOVT_NUM);
    if (rc_io != IORC_SUCCESS) {
        return rc_io;
    }

    return IORC_SUCCESS;
}

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
) {
    TIME_WALL_START(SP_init);
    TIME_CPU_START(SP_init);

    /* INITIALIZE SP_generic struct */
    IO_RETURN_CODE rc_spg_init = SPG_init(&sp->spg, in_parms, UPO__COUNT, DSIO__COUNT, DSOO__COUNT);
    if (rc_spg_init != IORC_SUCCESS) {
        return rc_spg_init;
    }

    /* GET PARAMETERS */
    TIME_CPU_START(read_parms);
    /****** GENERATED CODE START: Procedure_IO_analysis.xlsx:Parameter Procedure ->[init code] ***************************/
    PARM_init(&sp->spg, UPO_UNIT_ID,             (PARM_generic*) &sp->unit_id,             GPN_UNIT_ID,             IOPT_STRING);
    PARM_init(&sp->spg, UPO_BY,                  (PARM_generic*) &sp->by,                  GPN_BY,                  IOPT_STRING);
    PARM_init(&sp->spg, UPO_VAR,                 (PARM_generic*) &sp->var,                 GPN_VAR,                 IOPT_STRING);
    PARM_init(&sp->spg, UPO_SHADOW,              (PARM_generic*) &sp->shadow,              GPN_SHADOW,              IOPT_STRING);
    PARM_init(&sp->spg, UPO_DIMENSION,           (PARM_generic*) &sp->dimension,           GPN_DIMENSION,           IOPT_STRING);
    PARM_init(&sp->spg, UPO_WAIVER,              (PARM_generic*) &sp->waiver,              GPN_WAIVER,              IOPT_STRING);
    PARM_init(&sp->spg, UPO_P_WAIVER,            (PARM_generic*) &sp->p_waiver,            GPN_P_WAIVER,            IOPT_STRING);
    PARM_init(&sp->spg, UPO_PROXY_SIZE,          (PARM_generic*) &sp->proxy_size,          GPN_PROXY_SIZE,          IOPT_STRING);
    PARM_init(&sp->spg, UPO_WEIGHT,              (PARM_generic*) &sp->weight,              GPN_WEIGHT,              IOPT_STRING);
    PARM_init(&sp->spg, UPO_S_RULE,              (PARM_generic*) &sp->s_rule,              GPN_S_RULE,              IOPT_STRING);
    PARM_init(&sp->spg, UPO_HIERARCHY,           (PARM_generic*) &sp->hierarchy,           GPN_HIERARCHY,           IOPT_STRING);
    PARM_init(&sp->spg, UPO_RANGE,               (PARM_generic*) &sp->range,               GPN_RANGE,               IOPT_STRING);
    PARM_init(&sp->spg, UPO_M,                   (PARM_generic*) &sp->m,                   GPN_M,                   IOPT_INT);
    PARM_init(&sp->spg, UPO_X,                   (PARM_generic*) &sp->x,                   GPN_X,                   IOPT_NUMERIC);
    PARM_init(&sp->spg, UPO_Y,                   (PARM_generic*) &sp->y,                   GPN_Y,                   IOPT_NUMERIC);
    PARM_init(&sp->spg, UPO_Z,                   (PARM_generic*) &sp->z,                   GPN_Z,                   IOPT_NUMERIC);
    PARM_init(&sp->spg, UPO_TOLERANCE,           (PARM_generic*) &sp->tolerance,           GPN_TOLERANCE,           IOPT_NUMERIC);
    PARM_init(&sp->spg, UPO_MIN_RESP,            (PARM_generic*) &sp->min_resp,            GPN_MIN_RESP,            IOPT_INT);
    PARM_init(&sp->spg, UPO_PROXY_RATIO,         (PARM_generic*) &sp->proxy_ratio,         GPN_PROXY_RATIO,         IOPT_NUMERIC);
    PARM_init(&sp->spg, UPO_PROXY_PERCENTILE,    (PARM_generic*) &sp->proxy_percentile,    GPN_PROXY_PERCENTILE,    IOPT_NUMERIC);
    PARM_init(&sp->spg, UPO_WEIGHT_PROT_LEVEL,   (PARM_generic*) &sp->weight_prot_level,   GPN_WEIGHT_PROT_LEVEL,   IOPT_STRING);
    PARM_init(&sp->spg, UPO_MIN_RESP_W,          (PARM_generic*) &sp->min_resp_w,          GPN_MIN_RESP_W,          IOPT_NUMERIC);
    PARM_init(&sp->spg, UPO_DEBUG_FILE_PREFIX,   (PARM_generic*) &sp->debug_file_prefix,   GPN_DEBUG_FILE_PREFIX,   IOPT_STRING);
    PARM_init(&sp->spg, UPO_DEBUG_WORK_DIR_PATH, (PARM_generic*) &sp->debug_work_dir_path, GPN_DEBUG_WORK_DIR_PATH, IOPT_STRING);
    PARM_init(&sp->spg, UPO_VERBOSE,             (PARM_generic*) &sp->verbose,             GPN_VERBOSE,             IOPT_FLAG);
    PARM_init(&sp->spg, UPO_TIMER,               (PARM_generic*) &sp->timer,               GPN_TIMER,               IOPT_FLAG);
    PARM_init(&sp->spg, UPO_PRINT_CODES,         (PARM_generic*) &sp->print_codes,         GPN_PRINT_CODES,         IOPT_FLAG);
    PARM_init(&sp->spg, UPO_LIMIT_WARNINGS,      (PARM_generic*) &sp->limit_warnings,      GPN_LIMIT_WARNINGS,      IOPT_FLAG);
    PARM_init(&sp->spg, UPO_ADDITIVE_NOISE,      (PARM_generic*) &sp->additive_noise,      GPN_ADDITIVE_NOISE,      IOPT_FLAG);
    PARM_init(&sp->spg, UPO_PROXY_DIAG,          (PARM_generic*) &sp->proxy_diag,          GPN_PROXY_DIAG,          IOPT_FLAG);
    PARM_init(&sp->spg, UPO_WEIGHT_DIAG,         (PARM_generic*) &sp->weight_diag,         GPN_WEIGHT_DIAG,         IOPT_FLAG);
    PARM_init(&sp->spg, UPO_ACCEPT_NEGATIVE,     (PARM_generic*) &sp->accept_negative,     GPN_ACCEPT_NEGATIVE,     IOPT_FLAG);
    /****** GENERATED CODE END: Procedure_IO_analysis.xlsx:Parameter Procedure ->[init code] *****************************/
    TIME_CPU_STOPDIFF(read_parms);

    /* INITIALIZE OUTPUT DATA */
    IO_RETURN_CODE rc_dsw_init = IORC_ERROR;

    rc_dsw_init = DSW_init(&sp->spg, DSOO_OUTCONSTRAINT,      &sp->dsw_outconstraint,       DSN_OUT_CONSTRAINT,      out_sch_outconstraint, out_arr_outconstraint);
    if (rc_dsw_init != IORC_SUCCESS) {
        return rc_dsw_init;
    }

    rc_dsw_init = DSW_init(&sp->spg, DSOO_OUTCELL,            &sp->dsw_outcell,             DSN_OUT_CELL,            out_sch_outcell, out_arr_outcell);
    if (rc_dsw_init != IORC_SUCCESS) {
        return rc_dsw_init;
    }

    rc_dsw_init = DSW_init(&sp->spg, DSOO_OUTLARGEST,         &sp->dsw_outlargest,          DSN_OUT_LARGEST,         out_sch_outlargest, out_arr_outlargest);
    if (rc_dsw_init != IORC_SUCCESS) {
        return rc_dsw_init;
    }

    rc_dsw_init = DSW_init(&sp->spg, DSOO_OUTPAIRS,           &sp->dsw_outpairs,            DSN_OUT_PAIRS,           out_sch_outpairs, out_arr_outpairs);
    if (rc_dsw_init != IORC_SUCCESS) {
        return rc_dsw_init;
    }

    rc_dsw_init = DSW_init(&sp->spg, DSOO_OUTTARGETS,         &sp->dsw_outtargets,          DSN_OUT_TARGETS,         out_sch_outtargets, out_arr_outtargets);
    if (rc_dsw_init != IORC_SUCCESS) {
        return rc_dsw_init;
    }


    DSW_set_by_var_reference(&sp->dsw_outconstraint, &sp->dsr_indata.dsr);
    DSW_set_by_var_reference(&sp->dsw_outcell, &sp->dsr_indata.dsr);
    DSW_set_by_var_reference(&sp->dsw_outlargest, &sp->dsr_indata.dsr);

    /* INITIALIZE INPUT DATA */
    IO_RETURN_CODE rc_indata_init = IORC_SUCCESS;
    TIME_WALL_START(decode_datasets);

    TIME_CPU_THREAD_START(dsr_indata_init);
    rc_indata_init = DSR_indata_init(sp, in_ds_indata);
    TIME_CPU_THREAD_STOPDIFF(dsr_indata_init);

    TIME_WALL_STOPDIFF(decode_datasets);

    /* stop timers: all expensive operations now complete */
    TIME_CPU_STOPDIFF(SP_init);
    TIME_WALL_STOPDIFF(SP_init);

    /* validate initialization return code(s) */
    if (IORC_SUCCESS != DSR_validate_init(&sp->dsr_indata.dsr, rc_indata_init, IOB_TRUE)) {
        return rc_indata_init;
    }

    return IORC_SUCCESS;
}

PROC_RETURN_CODE SP_wrap(SP_sensitiv* sp) {
    IO_RETURN_CODE rc = IORC_ERROR;
    TIME_WALL_START(SP_wrap);
    TIME_CPU_START(SP_wrap);

    TIME_CPU_THREAD_START(encode_outconstraint);
    rc = DSW_wrap(&sp->dsw_outconstraint);
    TIME_CPU_THREAD_STOPDIFF(encode_outconstraint);
    if (rc != IORC_SUCCESS) {
        return PRC_FAIL_WRITE_DATA;
    }

    TIME_CPU_THREAD_START(encode_outcell);
    rc = DSW_wrap(&sp->dsw_outcell);
    TIME_CPU_THREAD_STOPDIFF(encode_outcell);
    if (rc != IORC_SUCCESS) {
        return PRC_FAIL_WRITE_DATA;
    }

    TIME_CPU_THREAD_START(encode_outlargest);
    rc = DSW_wrap(&sp->dsw_outlargest);
    TIME_CPU_THREAD_STOPDIFF(encode_outlargest);
    if (rc != IORC_SUCCESS) {
        return PRC_FAIL_WRITE_DATA;
    }

    TIME_CPU_THREAD_START(encode_outpairs);
    rc = DSW_wrap(&sp->dsw_outpairs);
    TIME_CPU_THREAD_STOPDIFF(encode_outpairs);
    if (rc != IORC_SUCCESS) {
        return PRC_FAIL_WRITE_DATA;
    }

    TIME_CPU_THREAD_START(encode_outtargets);
    rc = DSW_wrap(&sp->dsw_outtargets);
    TIME_CPU_THREAD_STOPDIFF(encode_outtargets);
    if (rc != IORC_SUCCESS) {
        return PRC_FAIL_WRITE_DATA;
    }

    TIME_CPU_STOPDIFF(SP_wrap);
    TIME_WALL_STOPDIFF(SP_wrap);

    return PRC_SUCCESS;
}
