import logging
from abc import (
    ABC,
    abstractmethod,
)

import pulp

from gconfid import (
    _log as lg,
)
from gconfid import (
    diagnostics as diag,
)
from gconfid import (
    io_util as io,
)
from gconfid import (
    log_level,
)
from gconfid._common.src.proc.validation import (
    validate_arg_type,
)
from gconfid.exceptions import GConfidInputDatasetValidationError
from gconfid.nls import _
from gconfid.util.solver_utils import add_solver_threads
from gconfid.util.validation import ValidationContext


class _ConfidModule(ABC):
    def __init__(
            self,
            name: str,
            custom_solver=None,
            trace=None,
            logger=None,
            capture=False,
    ):
        """Abstract Base Class for G-Confid modules executing a particular G-Confid operation.

        :param name: The name of the module of the instance to be created
        :type name: str
        :param custom_solver: A custom configured solver to use instead of the default solver, defaults to None
        :type custom_solver: pulp.LpSolver | None, optional
        :param trace: Use the trace parameter to control which log levels are printed, defaults to None
        :type trace: int | bool | None, optional
        :param logger: Specify a custom Logger that you have created, defaults to None
        :type logger: logging.Logger | None, optional
        :param capture: Controls whether messages are printed immediately to the log, defaults to False
        :type capture: bool, optional
        """
        # `logger`: initialize logger for this object instance
        validate_arg_type(arg_val=logger, parm_name="logger", allowed_types=logging.Logger, skip_none=True)
        if isinstance(logger, logging.Logger):
            # if logger was passed, use it
            self._log = logger
            self._log.debug(_("Logging initialized using specified logger: '{}'").format(self._log.name))
        else:
            # get trace parameter
            validate_arg_type(arg_val=trace, parm_name="trace", allowed_types=(bool, int), skip_none=True)
            self._trace = log_level.INFO if trace is None else trace  # hardcoded default log level
            # Normal logger initialization
            self._log = lg.init_module_level(
                logger_name=name,
                trace_level=self._trace,
            )
            self._log.debug(_("Logging initialized with default parent: '{}'").format(self._log.parent.name))

        # `capture`: whether to capture/redirect console output
        if capture:
            self._py_log_redirect = lg.capture.PythonLogRedirect(enabled=True, reprint=True)
        else:
            self._py_log_redirect = lg.capture.PythonLogRedirect(enabled=False)

        # `custom_solver`: a pulp solver to use in place of the default for this module execution
        validate_arg_type(arg_val=custom_solver, parm_name="custom_solver", allowed_types=pulp.LpSolver, skip_none=True)
        # Set the default solver to the provided custom instance
        if(custom_solver):
            pulp.LpSolverDefault = custom_solver
            self._log.debug(_("LpSolverDefault updated to custom_solver {}").format(type(custom_solver)))
        else:
            # If we are using the default, users most likely aren't interested in this output, so disable it
            pulp.LpSolverDefault.msg = False
            # Set a threads value for the default solver to enable multithreading
            add_solver_threads(pulp.LpSolverDefault, self._log)

        # run the module
        with self._py_log_redirect:
            self._run_main()

    @property
    def captured_text(self):
        """Text captured during execution.

        When initialized with `capture=True`, this object attribute will contain all the console output text.
        """
        return self._py_log_redirect.all_captured_text

    def _run_main(self):
        """Run the module.

        Initialize and load datasets, run subclass execution methods, write output datsaets.
        """
        # print timezone info to log
        self._log.info(lg.get_timezone_message())

        # call subclass dataset init methods
        self._init_input_datasets()
        self._init_output_datasets()

        # load inputs into intermediate format
        self._log.info(_("Loading input datasets"))
        with diag.SystemStats(_("TOTAL load input"), logger=self._log, log_level=log_level.INFO):
            for ds in self._input_datasets:
                ds.ds_intermediate = io.load_input_dataset(ds.user_spec, log=self._log)

        # pre_execute (subclass method)
        self._log.info(_("Pre Execute"))
        with diag.SystemStats(_("TOTAL pre execute"), logger=self._log, log_level=log_level.INFO):
            self._pre_execute()

        # execute (subclass method)
        self._log.info(_("Execute"))
        with diag.SystemStats(_("TOTAL execute"), logger=self._log, log_level=log_level.INFO):
            self._execute()

        # post_execute (subclass method)
        self._log.info(_("Post Execute"))
        with diag.SystemStats(_("TOTAL post execute"), logger=self._log, log_level=log_level.INFO):
            self._post_execute()

        # free intermediate input references
        self._log.info(_("Free input datasets"))
        with diag.SystemStats(_("TOTAL free input"), logger=self._log, log_level=log_level.INFO):
            for ds in self._input_datasets:
                ds.ds_intermediate = None

        # handle outputs
        self._log.info(_("Write output datasets"))
        with diag.SystemStats(_("TOTAL write output"), logger=self._log, log_level=log_level.INFO):
            for ds in self._output_datasets:
                # If no table was ever created, just skip the output
                if ds.ds_intermediate is not None:
                    ds.user_output = io.write_output_dataset(
                        dataset=ds.ds_intermediate,
                        destination=ds.user_spec,
                        log=self._log,
                        default_format=self.get_default_output_format(),
                    )
                    # free intermediate output reference
                    ds.ds_intermediate = None

    # default output format
    def get_default_output_format(self):
        """Return the default output format."""
        if not hasattr(self, "_default_output_format"):
            mesg = _("Subclass {} must define `{}` class attribute").format(self, "_default_output_format")
            raise AttributeError(mesg)
        return self._default_output_format

    @classmethod
    def set_default_output_format(cls, value):
        """Validate and set the default output format."""
        from gconfid._common.src.io_util.type_converters import validate_output_spec #noqa: PLC0415 I001
        validate_output_spec(value)
        cls._default_output_format = value

    ## dataset property getter/setter methods ##
    def _get_input_dataset(self, ds):
        """Return the user-provided input dataset argument."""
        return ds.user_spec

    def _set_input_dataset(self, ds, value):  # noqa: ARG002  # `value` reserved for future/subclass use
        """Prevent users from modifying input dataset attributes directly.

        Inputs can only be set during initialization, they cannot be modified
        """
        mesg = _("'{}' dataset cannot be modified").format(ds.name)
        raise AttributeError(mesg)

    def _get_output_dataset(self, ds):
        """Return the C-code generated output dataset in user-requested format."""
        return ds.user_output

    def _set_output_dataset(self, ds, value):  # noqa: ARG002  # `value` reserved for future/subclass use
        """Prevent users from modifying output dataset attributes directly.

        Output dataset specification can only be set during initialization,
        they cannot be modified.
        """
        mesg = _("'{}' dataset cannot be modified").format(ds.name)
        raise AttributeError(mesg)

    def perform_input_validation(self, ds: io.StcInputTable, validator: ValidationContext) -> None:
        """Normalize column names and then validate an input dataset using the given `validator`.

        :param ds: The dataset object to validate.
        :type ds: io.StcInputTable
        :param validator: The ValidationContext containing the Strategy and Schema created for the
            input dataset to validate.
        :type validator: ValidationContext
        :raises GConfidInputDatasetValidationError: if validation fails.
        """
        # It would be nice to combine these but we would need to add the normalize stuff to the
        # Pandera model's validate() method which is not carried over/used when calling validate()
        # on the resultant schema. This may be changed in a future version but for now keep them discrete.
        ds.ds_intermediate = validator.execute_normalize(ds.ds_intermediate)
        ds.ds_intermediate = validator.execute_validate(ds.ds_intermediate)
        if(validator.validation_result.is_valid):
            msg = _("Validation for {} was successful!").format(ds.name)
            self._log.debug(msg)
            if(validator.validation_result.warnings):
                self._log.info(_("During validation of {} the following warnings occured:").format(ds.name))
                for warning_msg in validator.validation_result.warnings:
                    self._log.warning(warning_msg.message)
        else:
            # As return codes are not used for GConfid procs, we can only retrieve info during
            # tests from the final exception message, should a validation error occur, as we
            # can't read the captured log after the exception is raised. Therefore we want to
            # include all the retained exception info in the final message

            # The validator error_msg is already translated, so we can just grab it as-is
            msg = _("{} failed validation. ").format(ds.name) + validator.validation_result.error_msg
            self._log.error(msg)

            # This will have more detail than can be parsed from the various exception fields
            self._log.error(str(validator.validation_result.exception))

            raise GConfidInputDatasetValidationError(msg)

    # methods for subclass to implement
    @abstractmethod
    def _pre_execute(self):
        """Subclass specific method called just before execution."""

    @abstractmethod
    def _execute(self):
        """Subclass specific execution method."""

    @abstractmethod
    def _post_execute(self):
        """Subclass specific method called just after execution."""

    @abstractmethod
    def _init_input_datasets(self, *args):
        """Store a list of all input datasets.

        Generates a list containing all non-keyword arguments this function receives
        and stores it in the `_input_datasets` member.

        Abstract Method:
            In subclass implementation, pass all input datasets in a call to this implementation
        """
        self._input_datasets: list[io.StcInputTable] = [*args]

    @abstractmethod
    def _init_output_datasets(self, *args):
        """Store a list of all output datasets.

        Generates a list containing all non-keyword arguments this function receives
        and stores it in the `_output_datasets` member.

        Abstract Method:
            In subclass implementation, pass all output datasets in a call to this implementation
        """
        self._output_datasets: list[io.StcOutputTable] = [*args]
