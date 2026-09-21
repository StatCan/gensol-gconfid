"""Define the wrapper class for the Auditing module."""

import logging
from pathlib import Path

import pandas as pd
import pulp
import pyarrow as pa

from gconfid import (
    io_util as io,
)
from gconfid.auditing._audit import _audit
from gconfid.auditing.auditschema import AuditIncellModel, AuditInconstraintModel
from gconfid.util._confidmodule import _ConfidModule
from gconfid.util.validation import GConfidValidator, ValidationContext


class Auditing(_ConfidModule):
    """Wrapper implementation for the Audit module."""

    # A global output format default is set in init. Set this to override the global setting for this proc only
    _default_output_format = None

    def __init__(
            self,
            # datasets
            incell: pd.DataFrame | pa.Table | str,
            inconstraint: pd.DataFrame | pa.Table | str,
            outaudit: str | Path | None             = None,
            # execution parameters
            lb_factor: float                                = 0.5,
            ub_factor: float                                = 1.5,
            use_shuttle: bool                               = False,
            report_level: int                               = 0,
            by: str | None                                  = None,
            custom_solver: pulp.LpSolver | None             = None,
            multiprocess: bool                              = False,
            # wrapper parameters
            skip_validation: bool            = False,
            trace: int | bool | None         = None,
            logger: logging.Logger | None    = None,
            capture: bool                    = False,
        ):
        """Construct an instance of the Auditing wrapper class.

        :param incell: The data table to be audited.
        :type incell: pd.DataFrame | pa.Table | str
        :param inconstraint: The dataset containing the linear constraints coefficients.
        :type inconstraint: pd.DataFrame | pa.Table | str
        :param outaudit: The output DataFrame that will contain the auditing results, defaults to None
        :type outaudit: str | Path | None, optional
        :param lb_factor: Used to calculate lower bounds based off the input data table, defaults to 0.5
        :type lb_factor: float, optional
        :param ub_factor: Used to calculate upper bounds based off the input data table, defaults to 1.5
        :type ub_factor: float, optional
        :param use_shuttle: Should the Shuttle algorithm be used to calculate the lower and upper bounds of your data table?
            If False the standard method is used, defaults to False
        :type use_shuttle: bool, optional
        :param report_level: Any value other than 0 will log a more detailed report about the completed audit operation, defaults to 0
        :type report_level: int, optional
        :param by: Variable names separated by spaces used to create processing groups, defaults to None
        :type by: str | None, optional
        :param custom_solver: A custom configured solver to use instead of the default solver, defaults to None
        :type custom_solver: pulp.LpSolver | None, optional
        :param multiprocess: Enables the use of multiple processors to distribute solver work, defaults to False
        :type multiprocess: bool, optional
        :param skip_validation: Should the validation of fields on the input files be skipped?, defaults to False
        :type skip_validation: bool, optional
        :param trace: Use the trace parameter to control which log levels are printed, defaults to None
        :type trace: int | bool | None, optional
        :param logger: Specify a custom Logger that you have created, defaults to None
        :type logger: logging.Logger | None, optional
        :param capture: Controls whether messages are printed immediately to the log, defaults to False
        :type capture: bool, optional
        """
        # store parameters in dictionary
        mp = {}
        mp["lb_factor"] = lb_factor
        mp["ub_factor"] = ub_factor
        mp["use_shuttle"] = use_shuttle
        mp["report_level"] = report_level
        mp["by"] = by
        mp["multiprocess"] = multiprocess
        self._module_parameters = mp

        self._skip_validation = skip_validation

        # input datasets
        self._incell = io.StcInputTable("incell", incell)
        self._inconstraint = io.StcInputTable("inconstraint", inconstraint)
        # output datasets
        self._outaudit = io.StcOutputTable("outaudit", outaudit)

        super().__init__(
            name="Audit",
            custom_solver=custom_solver,
            trace=trace,
            logger=logger,
            capture=capture,
        )

    ##### dataset property methods
    @property
    def incell(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the incell input parameter.

        :return: The incell dataset in the format originally provided to the wrapper.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_input_dataset(self._incell)
    @incell.setter
    def incell(self, value):
        self._set_input_dataset(self._incell, value=value)

    @property
    def inconstraint(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the inconstraint input parameter.

        :return: The inconstraint dataset in the format originally provided to the wrapper.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_input_dataset(self._inconstraint)
    @inconstraint.setter
    def inconstraint(self, value):
        self._set_input_dataset(self._inconstraint, value=value)

    @property
    def outaudit(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the outaudit output data table.

        :return: The outaudit table as produced by the execution of the audit proc.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_output_dataset(self._outaudit)
    @outaudit.setter
    def outaudit(self, value):
        self._set_output_dataset(self._outaudit, value=value)

    # required by superclass
    def _pre_execute(self) -> None:
        validator = ValidationContext()

        # ensure datasets in proper format
        for ds in self._input_datasets:
            if(not self._skip_validation):
                # Unfortunately the best way to apply our schemas to PyArrow Tables is to just convert to Pandas then back after
                # Fortunately this can all be skipped for efficiency's sake
                ds.to_pandas()
                if(ds.name == "inconstraint"):
                    validator.strategy = GConfidValidator(schema=AuditInconstraintModel.to_schema(self._module_parameters["by"]))
                    self.perform_input_validation(ds, validator)
                elif(ds.name == "incell"):
                    validator.strategy = GConfidValidator(schema=AuditIncellModel.to_schema(self._module_parameters["by"]))
                    self.perform_input_validation(ds, validator)

            # Now convert to Arrow for our procs to work on
            ds.to_arrow()

    def _execute(self) -> None:
        # execute audit
        self._outaudit.ds_intermediate = _audit(
            incell=self._incell.ds_intermediate,
            inconstraint=self._inconstraint.ds_intermediate,
            logger=self._log,
            **self._module_parameters,
        )

    def _post_execute(self) -> None:
        pass  # no post execution required

    def _init_input_datasets(self) -> None:
        super()._init_input_datasets(
            self._incell,
            self._inconstraint,
        )

    def _init_output_datasets(self) -> None:
        super()._init_output_datasets(self._outaudit)
