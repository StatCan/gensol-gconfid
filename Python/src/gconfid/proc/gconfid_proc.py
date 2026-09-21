"""The GConfidProcedure module contains the superclass for executing GConfid procs written in C."""
import contextlib

from gconfid._common.src.proc import GeneralizedProcedure
from gconfid._common.src.proc.validation import (
    validate_arg_type,
)
from gconfid.exceptions import GConfidInputDatasetValidationError
from gconfid.io_util import GensysInputDataset, interm_to_DF
from gconfid.nls import _
from gconfid.util.validation import ValidationContext


class GConfidProcedure(GeneralizedProcedure):
    """General superclass for executing GConfid procs written in C."""

    # create a copy of default args, so modifications do not affect superclass
    _default_args = GeneralizedProcedure.get_default().copy()

    # list of valid kwargs key words for this classes constructor
    # IMPORTANT: we need a copy of (as opposed to a reference to) the superclass' list
    # , so we don't modify the superclass' class attribute.  The `+` operator seems to do this.
    _valid_init_kwargs = GeneralizedProcedure._valid_init_kwargs + [  # noqa:  SLF001
        # no valid keyword arguments currently
    ]

    def __init__(
            self,
            trace,
            capture,
            logger,
            input_datasets,
            output_datasets,
            presort=None,
            prefill_by_vars=None,
            keyword_args=None,
    ):
        """Initialize Banff and execute Banff C procedure.

        After calling the superclass init method, performs Banff specific initialization tasks.
        Finally, it executes the procedure.

        Subclasses (i.e. Banff procedures) must store all procedure-specific arguments
        in `self` prior to calling this method, and pass a list of input and output datasets.
        """
        super().__init__(
            trace=trace,
            capture=capture,
            logger=logger,
            input_datasets=input_datasets,
            output_datasets=output_datasets,
            presort=presort,
            prefill_by_vars=prefill_by_vars,
            keyword_args=keyword_args,
        )

        # execute the procedure
        self._execute()

    ## Validation methods ##
    def _validate_c_parameters(self, log):
        """Perform basic validation of C parameter types.

        Strict validation happens in C code.
        """
        log_lcl = self._get_stack_logger(log)

        flag_parms = [
            "verbose",
            "timer",
            "print_codes",
            "limit_warnings",
            "additive_noise",
            "proxy_diag",
            "weight_diag",
            "accept_negative",
        ]

        for parm in flag_parms:
            with contextlib.suppress(KeyError):  # if exception, the parameter doesn't even exist
                validate_arg_type(
                    log=log_lcl,
                    arg_val=self.c_parms[parm],
                    parm_name=parm,
                    allowed_types=bool,
                    skip_none=True,
                )

        numeric_parms = [
            "m",
            "x",
            "y",
            "z",
            "tolerance",
            "min_resp",
            "proxy_ratio",
            "proxy_percentile",
            "min_resp_w",
        ]

        for parm in numeric_parms:
            with contextlib.suppress(KeyError):  # if exception, the parameter doesn't even exist
                validate_arg_type(
                    log=log_lcl,
                    arg_val=self.c_parms[parm],
                    parm_name=parm,
                    allowed_types=(int, float),
                    skip_none=True,
                )

        string_parms = [
            "s_rule",
            "hierarchy",
            "code_range",
            "weight_prot_level",
            "debug_file_prefix",
            "debug_work_dir_path",
        ]

        for parm in string_parms:
            with contextlib.suppress(KeyError):  # if exception, the parameter doesn't even exist
                validate_arg_type(
                    log=log_lcl,
                    arg_val=self.c_parms[parm],
                    parm_name=parm,
                    allowed_types=str,
                    skip_none=True,
                )

    def _validate_deprecations(self, log, keyword_args):
        """Check for use of deprecated options across all procedures."""
        super()._validate_deprecations(log=log, keyword_args=keyword_args)

        log_lcl = self._get_stack_logger(log)

        no_parms = [
            "no_print_codes",
            "no_limit_warnings",
            "no_additive_noise",
            "no_proxy_diag",
            "no_weight_diag",
        ]
        for parm in no_parms:
            if parm in keyword_args.keys():
                mesg = _("Option `{}` is deprecated, use `{}` instead").format(parm, f"{parm[3:]}=False")
                log_lcl.error(mesg)
                raise DeprecationWarning(mesg)

        if "reject_negative" in keyword_args.keys():
            mesg = _("Option `{}` is deprecated, use `{}` instead").format("reject_negative", "accept_negative=False")
            log_lcl.error(mesg)
            raise DeprecationWarning(mesg)

    @staticmethod
    def _get_bin_anchor():
        """Return package anchor for bin folder."""
        return "gconfid.proc.bin"

    @classmethod
    def _load_c(cls, debug=None, lang=None):
        if debug in (True, False):
            import os #noqa: PLC0415 I001
            if debug is True:
                os.environ["GCONFID_DEBUG_STATS"] = "TRUE"
            elif debug is False:
                with contextlib.suppress(KeyError):
                    os.environ.pop("GCONFID_DEBUG_STATS")

        super()._load_c(debug=debug, lang=lang)

    def perform_input_validation(self, ds: GensysInputDataset, validator: ValidationContext, logger = None) -> None:
        """Normalize column names and then validate an input dataset using the given `validator`.

        This method must be called prior to running execute(). The order of operations in C-based
        GConfid procs is different from the Python native ones, therefore the validation is done
        slightly differently. This also only supports datasets with a user_spec of an Arrow Table
        or Pandas DataFrame. Other formats need this validation to be performed by the C-specific
        submodule and cannot be done here.

        :param ds: The dataset object to validate.
        :type ds: io_util.GensysInputDataset
        :param validator: The ValidationContext containing the Strategy and Schema created for the
            input dataset to validate against.
        :type validator: ValidationContext
        :raises GConfidInputDatasetValidationError: if validation fails.
        """
        # Convert to the supported input format
        try:
            input_df = interm_to_DF(ds.user_spec)
        except TypeError:
            # If the input type is not supported, just return. We cannot validate here.
            return

        input_df = validator.execute_normalize(input_df)
        input_df = validator.execute_validate(input_df)
        if(validator.validation_result.is_valid):
            # Set the normalized and validated df to be validated by the C-code
            ds.user_spec = input_df
            if(logger):
                logger.debug(_("Validation for {} was successful!").format(ds.name))
        else:
            # Unfortunately we cannot use the log here yet as it is created by the parent class in
            # its init. Our validation must be performed before that, however. The fix for this is
            # to integrate the input validation into the shared submodule somehow, or just rely
            # on this exception to report the error.
            if(logger):
                logger.error(_("{} failed validation check.").format(ds.name))
                logger.error(validator.validation_result.error_msg)
            raise GConfidInputDatasetValidationError(validator.validation_result.error_msg)
