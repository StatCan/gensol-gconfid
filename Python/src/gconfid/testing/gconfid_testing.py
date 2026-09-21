import gconfid
from gconfid._common.src.testing.assert_helper import (
    _GeneralizedTester,
    get_control_dataset_path,
)


class SensitivTester(_GeneralizedTester):
    """Wrapper for testing Sensitivity procedure.

    Call this wrapper as you would call the procedure.  The additional test parameters
    allow various assertions to be ran on the output log contents or datasets.
    """

    def __init__(
        self,
        #### Unit test parameters
        msg_list_contains       = None,
        msg_list_contains_exact = None,
        expected_error_count    = 0,
        expected_warning_count  = 0,
        rc_should_be_zero       = True,
        round_data              = None,
        drop_columns            = True,
        expected_outconstraint  = None,
        expected_outcell        = None,
        expected_outlargest     = None,
        expected_outpairs       = None,
        expected_outtargets     = None,

        #### G-Confid parameters
        # USER C code parameters
        unit_id             = None,
        by                  = None,
        var                 = None,
        shadow              = None,
        dimension           = None,
        waiver              = None,
        p_waiver            = None,
        proxy_size          = None,
        weight              = None,
        s_rule              = None,
        hierarchy           = None,
        code_range          = None,
        m                   = None,
        x                   = None,
        y                   = None,
        z                   = None,
        tolerance           = None,
        min_resp            = None,
        proxy_ratio         = None,
        proxy_percentile    = None,
        weight_prot_level   = None,
        min_resp_w          = None,
        debug_file_prefix   = None,
        debug_work_dir_path = None,
        verbose             = None,
        timer               = None,
        print_codes         = None,
        limit_warnings      = None,
        additive_noise      = None,
        proxy_diag          = None,
        weight_diag         = None,
        accept_negative     = None,
        # USER dataset references
        indata              = None,
        outconstraint       = None,
        outcell             = None,
        outlargest          = None,
        outpairs            = None,
        outtargets          = None,
        # Fancy New Options
        presort             = None,
        skip_validation     = False,
        # OPTIONS
        trace               = None,
        **kwargs,
    ):
        self.parms = {}
        self.parms["unit_id"]             = unit_id
        self.parms["by"]                  = by
        self.parms["var"]                 = var
        self.parms["shadow"]              = shadow
        self.parms["dimension"]           = dimension
        self.parms["waiver"]              = waiver
        self.parms["p_waiver"]            = p_waiver
        self.parms["proxy_size"]          = proxy_size
        self.parms["weight"]              = weight
        self.parms["s_rule"]              = s_rule
        self.parms["hierarchy"]           = hierarchy
        self.parms["code_range"]          = code_range
        self.parms["m"]                   = m
        self.parms["x"]                   = x
        self.parms["y"]                   = y
        self.parms["z"]                   = z
        self.parms["tolerance"]           = tolerance
        self.parms["min_resp"]            = min_resp
        self.parms["proxy_ratio"]         = proxy_ratio
        self.parms["proxy_percentile"]    = proxy_percentile
        self.parms["weight_prot_level"]   = weight_prot_level
        self.parms["min_resp_w"]          = min_resp_w
        self.parms["debug_file_prefix"]   = debug_file_prefix
        self.parms["debug_work_dir_path"] = debug_work_dir_path
        self.parms["verbose"]             = verbose
        self.parms["timer"]               = timer
        self.parms["print_codes"]         = print_codes
        self.parms["limit_warnings"]      = limit_warnings
        self.parms["additive_noise"]      = additive_noise
        self.parms["proxy_diag"]          = proxy_diag
        self.parms["weight_diag"]         = weight_diag
        self.parms["accept_negative"]     = accept_negative
        self.parms["indata"]              = get_control_dataset_path(indata)
        self.parms["outconstraint"]       = outconstraint
        self.parms["outcell"]             = outcell
        self.parms["outlargest"]          = outlargest
        self.parms["outpairs"]            = outpairs
        self.parms["outtargets"]          = outtargets
        self.parms["presort"]             = presort
        self.parms["skip_validation"]     = skip_validation
        self.parms["trace"]               = trace
        self.parms.update(**kwargs)

        self._expected_outconstraint = get_control_dataset_path(expected_outconstraint)
        self._expected_outcell = get_control_dataset_path(expected_outcell)
        self._expected_outlargest = get_control_dataset_path(expected_outlargest)
        self._expected_outpairs = get_control_dataset_path(expected_outpairs)
        self._expected_outtargets = get_control_dataset_path(expected_outtargets)

        # always enable capturing
        self.parms["capture"] = True

        # initialize in case procedure call fails
        self.call = None

        super().__init__(
            msg_list_contains=msg_list_contains,
            msg_list_contains_exact=msg_list_contains_exact,
            expected_error_count=expected_error_count,
            expected_warning_count=expected_warning_count,
            rc_should_be_zero=rc_should_be_zero,
            round_data=round_data,
            drop_columns=drop_columns,
        )

    def _action(self):
        """Execute procedure, handle certain exceptions."""
        try:
            self.call = gconfid.sensitiv(
                **self.parms,
            )
            # create list of associated actual and expected output datasets
            self.ds_compare_list = [
                [self.call._outconstraint, self._expected_outconstraint],
                [self.call._outcell, self._expected_outcell],
                [self.call._outlargest, self._expected_outlargest],
                [self.call._outpairs, self._expected_outpairs],
                [self.call._outtargets, self._expected_outtargets],
            ]
        except gconfid.exceptions.ProcedureCError as e:
            self.c_return_code = e.return_code
        else:
            self.c_return_code = self.call.rc

class SuppressionTester(_GeneralizedTester):
    """Wrapper for testing Suppression module.

    Call this wrapper as you would call the module.  The additional test parameters
    allow various assertions to be ran on the output log contents or datasets.
    """

    def __init__(
            self,
            #### Unit test parameters
            msg_list_contains       = None,
            msg_list_contains_exact = None,
            expected_error_count    = None,
            expected_warning_count  = None,
            round_data              = None,
            drop_columns            = None,
            expected_outsuppress    = None,
            expected_outcomplement  = None,
            expected_outsuppress_failed = None,
            expected_outcomplement_failed = None,
            # gconfid
            incell=None,
            inconstraint=None,
            outsuppress = None,
            outcomplement = None,
            cost_function1 = "Size",
            cost_function2 = None,
            cost_var1 = None,
            cost_var2 = None,
            scale_cost = None,
            size_roundingbase = None,
            sen_roundingbase = None,
            total_roundingbase = None,
            constraint_scale = 2,
            suppress_order = 0,
            ambiguity_tolerance = 0.00001,
            by = None,
            skip_validation = False,
            custom_solver = None,
            #multiprocess = False, #noqa: ERA001 - see below
            trace = None,
            **kwargs,
    ):
        self.parms = {}
        self.parms["capture"]= True
        self.parms["trace"]=trace
        self.parms["incell"]=get_control_dataset_path(incell)
        self.parms["inconstraint"]=get_control_dataset_path(inconstraint)
        self.parms["outsuppress"]=outsuppress
        self.parms["outcomplement"]=outcomplement
        self.parms["cost_function1"]=cost_function1
        self.parms["cost_function2"]=cost_function2
        self.parms["cost_var1"]=cost_var1
        self.parms["cost_var2"]=cost_var2
        self.parms["scale_cost"]=scale_cost
        self.parms["size_roundingbase"]=size_roundingbase
        self.parms["sen_roundingbase"]=sen_roundingbase
        self.parms["total_roundingbase"]=total_roundingbase
        self.parms["constraint_scale"]=constraint_scale
        self.parms["suppress_order"]=suppress_order
        self.parms["ambiguity_tolerance"]=ambiguity_tolerance
        self.parms["by"]=by
        self.parms["skip_validation"]=skip_validation
        self.parms["custom_solver"]=custom_solver
        # Currently deprecated until more efficient methodology found to reduce solver iterations
        #self.parms["multiprocess"]=multiprocess #noqa: ERA001
        self.parms.update(**kwargs)

        self._expected_outsuppress = get_control_dataset_path(expected_outsuppress)
        self._expected_outcomplement = get_control_dataset_path(expected_outcomplement)
        self._expected_outsuppress_failed = get_control_dataset_path(expected_outsuppress_failed)
        self._expected_outcomplement_failed = get_control_dataset_path(expected_outcomplement_failed)

        super().__init__(
            msg_list_contains=msg_list_contains,
            msg_list_contains_exact=msg_list_contains_exact,
            expected_error_count=expected_error_count,
            expected_warning_count=expected_warning_count,
            rc_should_be_zero=None,  # ignore, suppress has no return code
            round_data=round_data,
            drop_columns=drop_columns,
        )

    def _action(self):
        self.call = gconfid.Suppression(**self.parms)
        self.ds_compare_list = [
            [self.call._outsuppress, self._expected_outsuppress],
            [self.call._outcomplement, self._expected_outcomplement], #noqa: SLF001
            # unfortunately we can't re-use the expected datasets as the assert helper will always attempt
            # to compare both pairs of datasets, even though we can only have one or the other produced by
            # suppress at one time.
            [self.call._outsuppress_failed, self._expected_outsuppress_failed], #noqa: SLF001
            [self.call._outcomplement_failed, self._expected_outcomplement_failed], #noqa: SLF001
        ]

class AuditingTester(_GeneralizedTester):
    """Wrapper for testing Auditing module.

    Call this wrapper as you would call the module.  The additional test parameters
    allow various assertions to be ran on the output log contents or datasets.
    """

    def __init__(
        self,
        #### Unit test parameters
        msg_list_contains       = None,
        msg_list_contains_exact = None,
        expected_error_count    = None,
        expected_warning_count  = None,
        round_data              = None,
        drop_columns            = None,
        expected_outaudit       = None,
        # gconfid
        incell = None,
        inconstraint = None,
        outaudit = None,
        lb_factor = 0.5,
        ub_factor = 1.5,
        use_shuttle = False,
        report_level = 0,
        by = None,
        skip_validation = False,
        custom_solver = None,
        multiprocess = False,
        trace=None,
        **kwargs,
    ):
        self.parms = {}
        self.parms["capture"]= True
        self.parms["trace"]=trace
        self.parms["incell"]=get_control_dataset_path(incell)
        self.parms["inconstraint"]=get_control_dataset_path(inconstraint)
        self.parms["outaudit"]=outaudit
        self.parms["lb_factor"]=lb_factor
        self.parms["ub_factor"]=ub_factor
        self.parms["use_shuttle"]=use_shuttle
        self.parms["report_level"]=report_level
        self.parms["by"]=by
        self.parms["skip_validation"]=skip_validation
        self.parms["custom_solver"]=custom_solver
        self.parms["multiprocess"]=multiprocess
        self.parms.update(**kwargs)

        self._expected_outaudit = get_control_dataset_path(expected_outaudit)

        super().__init__(
            msg_list_contains=msg_list_contains,
            msg_list_contains_exact=msg_list_contains_exact,
            expected_error_count=expected_error_count,
            expected_warning_count=expected_warning_count,
            rc_should_be_zero=None,  # ignore, audit has no return code
            round_data=round_data,
            drop_columns=drop_columns,
        )

    def _action(self):
        self.call = gconfid.Auditing(**self.parms)
        self.ds_compare_list = [
            [self.call._outaudit, self._expected_outaudit],
        ]

class OptRoundingTester(_GeneralizedTester):
    """Wrapper for testing Opt_Rounding module.

    Call this wrapper as you would call the module.  The additional test parameters
    allow various assertions to be ran on the output log contents or datasets.
    """

    def __init__(
        self,
        #### Unit test parameters
        msg_list_contains       = None,
        msg_list_contains_exact = None,
        expected_error_count    = None,
        expected_warning_count  = None,
        round_data              = None,
        drop_columns            = None,
        expected_outround       = None,
        # gconfid
        incell = None,
        additive_con = None,
        additive_bound = None,
        outround = None,
        base = None,
        skip_validation = False,
        custom_solver = None,
        trace = None,
        **kwargs,
    ):
        self.parms = {}
        self.parms["capture"]= True
        self.parms["trace"]=trace
        self.parms["incell"]=get_control_dataset_path(incell)
        self.parms["additive_con"]=get_control_dataset_path(additive_con)
        self.parms["additive_bound"]=get_control_dataset_path(additive_bound)
        self.parms["outround"]=outround
        self.parms["base"]=base
        self.parms["skip_validation"]=skip_validation
        self.parms["custom_solver"]=custom_solver
        self.parms.update(**kwargs)

        self._expected_outround = get_control_dataset_path(expected_outround)

        super().__init__(
            msg_list_contains=msg_list_contains,
            msg_list_contains_exact=msg_list_contains_exact,
            expected_error_count=expected_error_count,
            expected_warning_count=expected_warning_count,
            rc_should_be_zero=None,  # ignore, round has no return code
            round_data=round_data,
            drop_columns=drop_columns,
        )

    def _action(self):
        self.call = gconfid.OptRounding(**self.parms)
        self.ds_compare_list = [
            [self.call._outround, self._expected_outround],
        ]
