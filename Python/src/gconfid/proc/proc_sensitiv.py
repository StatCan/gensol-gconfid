"""Define the wrapper class for the Sensitivity module."""

from logging import Logger
from pathlib import Path

import pandas as pd
import pyarrow as pa

from gconfid.io_util import (
    GensysInputDataset,
    GensysOutputDataset,
    c_argtype_input_dataset,
    c_argtype_output_dataset,
    c_argtype_parameters,
)
from gconfid.nls import _
from gconfid.proc import GConfidProcedure
from gconfid.proc.sensitivityschema import SensitivityInDataModel
from gconfid.util.validation import GConfidValidator, ValidationContext

#******CLASS DEFINITIONS************************************************************

class ProcSensitiv(GConfidProcedure):
    """Wrapper for Sensitivity procedure.

    This subclass of `GConfidProcedure` implements the procedure-specific parameters, datasets,
    preprocessing, and C code communication and execution.

    Initialize this object by providing all necessary module parameters and input datasets.
    """

    # static variables
    _proc_name = {"short": "sensitiv", "long": "Sensitivity"}
    _arg_types = [
        c_argtype_parameters(),  # parameters
        c_argtype_input_dataset(),  # indata

        c_argtype_output_dataset(),  # outconstraint
        c_argtype_output_dataset(),  # outconstraint
        c_argtype_output_dataset(),  # outcell
        c_argtype_output_dataset(),  # outcell
        c_argtype_output_dataset(),  # outlargest
        c_argtype_output_dataset(),  # outlargest
        c_argtype_output_dataset(),  # outpairs
        c_argtype_output_dataset(),  # outpairs
        c_argtype_output_dataset(),  # outtargets
        c_argtype_output_dataset(),  # outtargets
    ]

    # create a copy of default args, so modifications do not affect superclass
    # Make sure default_output_type is re-set here, so future modifications can be made without having to mess with submodule
    _default_args = {**GConfidProcedure.get_default().copy(), "default_output_type": None}

    def __init__(self,
            # USER C code parameters
            unit_id: str | None                 = None,
            by: str | None                      = None,
            var: str | None                     = None,
            shadow: str | None                  = None,
            dimension: str | None               = None,
            waiver: str | None                  = None,
            p_waiver: str | None                = None,
            proxy_size: str | None              = None,
            weight: str | None                  = None,
            s_rule: str | None                  = None,
            hierarchy: str | None               = None,
            code_range: str | None              = None,
            m: int | None                       = None,
            x: float | None                     = None,
            y: float | None                     = None,
            z: float | None                     = None,
            tolerance: float | None             = None,
            min_resp: int | None                = None,
            proxy_ratio: float | None           = None,
            proxy_percentile: float | None      = None,
            weight_prot_level: str | None       = None,
            min_resp_w: float | None            = None,
            debug_file_prefix: str | None       = None,
            debug_work_dir_path: str | None     = None,
            verbose: bool | None                = None,
            timer: bool | None                  = None,
            print_codes: bool | None            = None,
            limit_warnings: bool | None         = None,
            additive_noise: bool | None         = None,
            proxy_diag: bool | None             = None,
            weight_diag: bool | None            = None,
            accept_negative: bool | None        = None,
            # USER dataset references
            indata: pd.DataFrame | None         = None,
            outconstraint: pd.DataFrame | str | Path | None  = None,
            outcell: pd.DataFrame | str | Path | None        = None,
            outlargest: pd.DataFrame | str | Path | None     = None,
            outpairs: pd.DataFrame | str | Path | None       = None,
            outtargets: pd.DataFrame | str | Path | None     = None,
            # Fancy New Options
            presort: bool | None                = None,
            skip_validation: bool               = False,
            # super class options
            trace: int | bool | None            = None,
            capture: bool                       = False,
            logger: Logger | None               = None,
            **kwargs,
        ) -> None:
        """Initialize procedure-specific data and call superclass initializer.

        Store parameters, input datasets, and output dataset specifications.
        Construct list of input datasets and output datasets.
        Call super initializer function.

        :param unit_id: Name of the variable which is the key of th input data set, defaults to None
        :type unit_id: str, optional
        :param by: Variable names separated by spaces used to create processing groups, defaults to None
        :type by: str | None, optional
        :param var: Name of the variable containing the respondent data, defaults to None
        :type var: str | None, optional
        :param shadow: Name of the shadow variable, defaults to None
        :type shadow: str | None, optional
        :param dimension: Name(s) of the variable(s) that represent the hierarchy dimensions, defaults to None
        :type dimension: str | None, optional
        :param waiver: Name of the variable containing the full waiver flag, defaults to None
        :type waiver: str | None, optional
        :param p_waiver: Name of the variable containing the partial waiver flag, defaults to None
        :type p_waiver: str | None, optional
        :param proxy_size: Name of the auxiliary size variable used in the calculation, defaults to None
        :type proxy_size: str | None, optional
        :param weight: Name of the variable that represents the estimation weight, defaults to None
        :type weight: str | None, optional
        :param s_rule: The sensitivity rule to use, defaults to None
        :type s_rule: str | None, optional
        :param hierarchy: The hierarchy used for each dimension, defaults to None
        :type hierarchy: str | None, optional
        :param code_range: The interval of codes associated with the lowest level of the hierarchy, defaults to None
        :type code_range: str | None, optional
        :param m: The M parameter, defaults to None
        :type m: int | None, optional
        :param x: The X parameter to adjust the number of cell combinations, defaults to None
        :type x: float | None, optional
        :param y: The Y parameter to adjust the number of cell combinations, defaults to None
        :type y: float | None, optional
        :param z: The Z parameter to adjust the number of cell combinations, defaults to None
        :type z: float | None, optional
        :param tolerance: The upper bound value below which the sensitivity is deemed too small (below this value,
            the sensitivity is set to 0), defaults to None
        :type tolerance: float | None, optional
        :param min_resp: The minimum number of respondents with a non-zero value in a cell, defaults to None
        :type min_resp: int | None, optional
        :param proxy_ratio: The threshold below which observations are overridden by the proxy variable, defaults to None
        :type proxy_ratio: float | None, optional
        :param proxy_percentile: The percentage of observations overridden by the proxy variable, defaults to None
        :type proxy_percentile: float | None, optional
        :param weight_prot_level: The desired level of protection in the presence of weights, which will affect the
            sensitivity calculations, defaults to None
        :type weight_prot_level: str | None, optional
        :param min_resp_w: The weighted minimum number of respondents with a non-zero value in a cell, defaults to None
        :type min_resp_w: float | None, optional
        :param debug_file_prefix: Specification of debug prefix, defaults to None
        :type debug_file_prefix: str | None, optional
        :param debug_work_dir_path: Specification of debug directory, defaults to None
        :type debug_work_dir_path: str | None, optional
        :param verbose: Print additional information to the log, defaults to None
        :type verbose: bool | None, optional
        :param timer: Should the execution time of this proc should be recorded and reported, defaults to None
        :type timer: bool | None, optional
        :param print_codes: Should hierarchies and ranges be printed to the log or not, defaults to None
        :type print_codes: bool | None, optional
        :param limit_warnings: Should the printing of the warnings generated while reading the microdata be limited or not, defaults to None
        :type limit_warnings: bool | None, optional
        :param additive_noise: How the system calculates sensitivity variables at the marginal (or aggregate) cell level, defaults to None
        :type additive_noise: bool | None, optional
        :param proxy_diag: Should a diagnostic report related to the use of a proxy variable be produced, defaults to None
        :type proxy_diag: bool | None, optional
        :param weight_diag: Should a diagnostic report related to the use of a weight variable be produced, defaults to None
        :type weight_diag: bool | None, optional
        :param accept_negative: Should negative values be included when calculating sensitivity. This option is required when
            using the PROXYSIZE statement, defaults to None
        :type accept_negative: bool | None, optional
        :param indata: The input microdata file, defaults to None
        :type indata: pd.DataFrame | None, optional
        :param outconstraint: The output DataFrame or file location that will contain the constraints, defaults to None
        :type outconstraint: pd.DataFrame | str | Path | None, optional
        :param outcell: The output DataFrame or file location that will contain the sensitivity of the cells, defaults to None
        :type outcell: pd.DataFrame | str | Path | None, optional
        :param outlargest: The output DataFrame or file location that will contain information about the largest contributors to a cell,
            defaults to None
        :type outlargest: pd.DataFrame | str | Path | None, optional
        :param outpairs: The output DataFrame or file location that will contain information about the most sensitive observation pairs in each cell,
            defaults to None
        :type outpairs: pd.DataFrame | str | Path | None, optional
        :param outtargets: The output DataFrame or file location that will contain information about the most sensitive combination of
            observations in each cell, defaults to None
        :type outtargets: pd.DataFrame | str | Path | None, optional
        :param presort: Should indata be sorted prior to the execution of the proc, defaults to None
        :type presort: bool | None, optional
        :param skip_validation: Should the validation of fields on the input files be skipped?, defaults to False
        :type skip_validation: bool | None, optional
        :param trace: Controls which log levels are printed, defaults to None
        :type trace: int | bool | None, optional
        :param capture: Controls whether messages are printed immediately to the log, defaults to False
        :type capture: bool, optional
        :param logger: Specify a custom Logger that you have created, defaults to None
        :type logger: Logger | None, optional
        """
        # USER C code parameters
        parm_dict = {}
        parm_dict["unit_id"]             = unit_id
        parm_dict["by"]                  = by
        parm_dict["var"]                 = var
        parm_dict["shadow"]              = shadow
        parm_dict["dimension"]           = dimension
        parm_dict["waiver"]              = waiver
        parm_dict["p_waiver"]            = p_waiver
        parm_dict["proxy_size"]          = proxy_size
        parm_dict["weight"]              = weight
        parm_dict["s_rule"]              = s_rule
        parm_dict["hierarchy"]           = hierarchy
        parm_dict["code_range"]          = code_range
        parm_dict["m"]                   = m
        parm_dict["x"]                   = x
        parm_dict["y"]                   = y
        parm_dict["z"]                   = z
        parm_dict["tolerance"]           = tolerance
        parm_dict["min_resp"]            = min_resp
        parm_dict["proxy_ratio"]         = proxy_ratio
        parm_dict["proxy_percentile"]    = proxy_percentile
        parm_dict["weight_prot_level"]   = weight_prot_level
        parm_dict["min_resp_w"]          = min_resp_w
        parm_dict["debug_file_prefix"]   = debug_file_prefix
        parm_dict["debug_work_dir_path"] = debug_work_dir_path
        parm_dict["verbose"]             = verbose
        parm_dict["timer"]               = timer
        parm_dict["print_codes"]         = print_codes
        parm_dict["limit_warnings"]      = limit_warnings
        parm_dict["additive_noise"]      = additive_noise
        parm_dict["proxy_diag"]          = proxy_diag
        parm_dict["weight_diag"]         = weight_diag
        parm_dict["accept_negative"]     = accept_negative

        self.c_parms = parm_dict

        # INTERNAL dataset components (they store USER datasets/output specifications)
        self._indata              = GensysInputDataset("indata", indata)
        self._outconstraint       = GensysOutputDataset("outconstraint", outconstraint)
        self._outcell             = GensysOutputDataset("outcell", outcell)
        self._outlargest          = GensysOutputDataset("outlargest", outlargest, mandatory=False, requested_by_default=False)
        self._outpairs            = GensysOutputDataset("outpairs", outpairs, mandatory=False, requested_by_default=False)
        self._outtargets          = GensysOutputDataset("outtargets", outtargets, mandatory=False, requested_by_default=False)

        # This validation must be performed before the superclass init calls execute
        if(not skip_validation):
            # ensure indata is in a format we can validate
            # no need to convert back, it should be passed to the C-proc in a suitable format
            self._indata.to_pandas()
            validator = ValidationContext()
            validator.strategy = GConfidValidator(
                SensitivityInDataModel.to_schema(by,
                                                 dimension,
                                                 char_fields=[unit_id] if unit_id else None,
                                                 num_fields=[name for name in [var, waiver, p_waiver, weight, shadow, proxy_size] if name is not None]))
            self.perform_input_validation(self._indata, validator, logger=logger)

        # call super constructor
        super().__init__(
            trace=trace, capture=capture, logger=logger,
            input_datasets=[
                self._indata,
            ],
            output_datasets=[
                self._outconstraint,
                self._outcell,
                self._outlargest,
                self._outpairs,
                self._outtargets,
            ],
            presort=presort,
            keyword_args=kwargs,
        )

    ##### property methods
    @property
    def indata(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the indata input parameter.

        :return: The incell dataset in the format originally provided to the wrapper.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_input_dataset(self._indata)
    @indata.setter
    def indata(self, value):
        self._set_input_dataset(ds=self._indata, value=value)

    @property
    def outconstraint(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the outconstraint output data table.

        :return: The outconstraint table as produced by the execution of the sensitivity proc.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_output_dataset(self._outconstraint)
    @outconstraint.setter
    def outconstraint(self, value):
        self._set_output_dataset(ds=self._outconstraint, value=value)

    @property
    def outcell(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the outcell output data table.

        :return: The outcell table as produced by the execution of the sensitivity proc.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_output_dataset(self._outcell)
    @outcell.setter
    def outcell(self, value):
        self._set_output_dataset(ds=self._outcell, value=value)

    @property
    def outlargest(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the outlargest output data table.

        :return: The outlargest table as produced by the execution of the sensitivity proc.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_output_dataset(self._outlargest)
    @outlargest.setter
    def outlargest(self, value):
        self._set_output_dataset(ds=self._outlargest, value=value)

    @property
    def outpairs(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the outpairs output data table.

        :return: The outpairs table as produced by the execution of the sensitivity proc.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_output_dataset(self._outpairs)
    @outpairs.setter
    def outpairs(self, value):
        self._set_output_dataset(ds=self._outpairs, value=value)

    @property
    def outtargets(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the outtargets output data table.

        :return: The outtargets table as produced by the execution of the sensitivity proc.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_output_dataset(self._outtargets)
    @outtargets.setter
    def outtargets(self, value):
        self._set_output_dataset(ds=self._outtargets, value=value)

    def _call_c_code(self):
        return self._cproc_func(
            self._parm_dict,

            self._indata.c_arg,

            self._outconstraint.c_schema,
            self._outconstraint.c_array,
            self._outcell.c_schema,
            self._outcell.c_array,
            self._outlargest.c_schema,
            self._outlargest.c_array,
            self._outpairs.c_schema,
            self._outpairs.c_array,
            self._outtargets.c_schema,
            self._outtargets.c_array,
        )

    def _validate_deprecations(self, log, keyword_args):
        """Procedure specific check for use of deprecated options."""
        super()._validate_deprecations(log=log, keyword_args=keyword_args)

        log_lcl = self._get_stack_logger(log)

        # Version 2.0.0b2: 'range' flag replaced with optional output dataset 'code_range'
        if "range" in keyword_args.keys():
            mesg = _("Option `{}` has been renamed to `{}`").format("range", "code_range")
            log_lcl.error(mesg)
            raise ValueError(mesg)
