"""Define the wrapper class for the Optimized Rounding module."""

import logging
from pathlib import Path

import pandas as pd
import pulp
import pyarrow as pa

from gconfid import (
    io_util as io,
)
from gconfid.opt_rounding._opt_round import _opt_round
from gconfid.opt_rounding.opt_roundschema import OptRoundAdditiveBoundModel, OptRoundAdditiveConModel, OptRoundIncellModel
from gconfid.util._confidmodule import _ConfidModule
from gconfid.util.validation import GConfidValidator, ValidationContext


class OptRounding(_ConfidModule):
    """Wrapper implementation for the Opt_Round module."""

    # A global output format default is set in init. Set this to override the global setting for this proc only
    _default_output_format = None

    def __init__(
            self,
            # datasets
            incell : pd.DataFrame | pa.Table | str,
            additive_con: pd.DataFrame | pa.Table | str,
            additive_bound: pd.DataFrame | pa.Table | str,
            # execution parameter
            base: int,
            custom_solver: pulp.LpSolver | None             = None,
            # output dataset
            outround: str | Path | None                            = None,
            # wrapper parameters
            skip_validation: bool                           = False,
            trace: int | bool | None                        = None,
            logger: logging.Logger | None                   = None,
            capture: bool                                   = False,
        ) -> None:
        """Round data table based on rounding base and constraints.

        Find an optimal solution that is both additive and controlled (constraints are respected).

        :param incell: The data table to be rounded.
        :type incell: pd.DataFrame | pa.Table | str
        :param additive_con: Data table containing constraints related to additivity.
        :type additive_con: pd.DataFrame | pa.Table | str
        :param additive_bound: Data table containing bounds related to additivity constraints.
        :type additive_bound: pd.DataFrame | pa.Table | str
        :param base: The rounding base, Required.
        :type base: int
        :param custom_solver: A custom configured solver to use instead of the default solver, defaults to None
        :type custom_solver: pulp.LpSolver | None, optional
        :param outround: The output DataFrame that will contain the rounding results, defaults to None
        :type outround: str | Path | None, optional
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
        mp["base"] = base
        self._module_parameters = mp

        self._skip_validation = skip_validation

        # input datasets
        self._incell = io.StcInputTable("incell", incell)
        self._additive_con = io.StcInputTable("additive_con", additive_con)
        self._additive_bound = io.StcInputTable("additive_bound", additive_bound)
        # output datasets
        self._outround = io.StcOutputTable("outround", outround)

        super().__init__(
            name="Opt_Round",
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
    def additive_con(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the additive_con input parameter.

        :return: The additive_con dataset in the format originally provided to the wrapper.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_input_dataset(self._additive_con)
    @additive_con.setter
    def additive_con(self, value):
        self._set_input_dataset(self._additive_con, value=value)

    @property
    def additive_bound(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the additive_bound input parameter.

        :return: The additive_bound dataset in the format originally provided to the wrapper.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_input_dataset(self._additive_bound)
    @additive_bound.setter
    def additive_bound(self, value):
        self._set_input_dataset(self._additive_bound, value=value)

    @property
    def outround(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the outround output data table.

        :return: The outround table as produced by the execution of the optimized rounding proc.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_output_dataset(self._outround)
    @outround.setter
    def outround(self, value):
        self._set_output_dataset(self._outround, value=value)

    # required by superclass
    def _pre_execute(self) -> None:
        validator = ValidationContext()

        # ensure datasets in proper format
        for ds in self._input_datasets:
            if(not self._skip_validation):
                # Unfortunately the best way to apply our schemas to PyArrow Tables is to just convert to Pandas then back after
                # Fortunately this can all be skipped for efficiency's sake
                ds.to_pandas()
                if(ds.name == "additive_con"):
                    validator.strategy = GConfidValidator(schema=OptRoundAdditiveConModel.to_schema())
                    self.perform_input_validation(ds, validator)
                elif(ds.name == "additive_bound"):
                    validator.strategy = GConfidValidator(schema=OptRoundAdditiveBoundModel.to_schema())
                    self.perform_input_validation(ds, validator)
                elif(ds.name == "incell"):
                    validator.strategy = GConfidValidator(schema=OptRoundIncellModel.to_schema())
                    self.perform_input_validation(ds, validator)

            # Now convert to Arrow for our procs to work on
            ds.to_arrow()

    def _execute(self) -> None:
        # execute round
        self._outround.ds_intermediate = _opt_round(
            incell=self._incell.ds_intermediate,
            additive_con=self._additive_con.ds_intermediate,
            additive_bound=self._additive_bound.ds_intermediate,
            logger=self._log,
            **self._module_parameters,
        )

    def _post_execute(self) -> None:
        pass  # no post execution required

    def _init_input_datasets(self) -> None:
        super()._init_input_datasets(
            self._incell,
            self._additive_con,
            self._additive_bound,
        )

    def _init_output_datasets(self) -> None:
        super()._init_output_datasets(self._outround)
