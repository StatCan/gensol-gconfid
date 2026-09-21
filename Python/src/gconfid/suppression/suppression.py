"""Define the wrapper class for the Suppression module."""
import logging
from pathlib import Path

import pandas as pd
import pulp
import pyarrow as pa

from gconfid import (
    io_util as io,
)
from gconfid.suppression._suppress import SuppressReturnCode, _suppress
from gconfid.suppression.suppressschema import SuppressIncellModel, SuppressInconstraintModel
from gconfid.util._confidmodule import _ConfidModule
from gconfid.util.validation import GConfidValidator, ValidationContext


class Suppression(_ConfidModule):
    """Wrapper implementation for the Suppression module."""

    # A global output format default is set in init. Set this to override the global setting for this proc only
    _default_output_format = None

    def __init__(
            self,
            # datasets
            incell: pd.DataFrame | pa.Table | str,
            inconstraint: pd.DataFrame | pa.Table | str,
            outsuppress: str | Path | None                  = None,
            outcomplement: str | Path | bool                = False, # outcomplement is a potentially large dataset, so don't create by default
            # execution parameters
            cost_function1: str                             = "Size",
            cost_function2: str | None                      = None,
            cost_var1: str | None                           = None,
            cost_var2: str | None                           = None,
            scale_cost: str | None                          = None,
            size_roundingbase: int | None                   = None,
            sen_roundingbase: int | None                    = None,
            total_roundingbase: int | None                  = None,
            constraint_scale: int                           = 2,
            suppress_order: int                             = 0,
            ambiguity_tolerance: float                      = 0.00001,
            by: str | None                                  = None,
            custom_solver: pulp.LpSolver | None             = None,
            #multiprocess: bool                              = False, #noqa: ERA001 - temporarily disabled
            # wrapper parameters
            skip_validation: bool                           = False,
            trace: int | bool | None                        = None,
            logger: logging.Logger | None                   = None,
            capture: bool                                   = False,
        ) -> None:
        """Create a Suppression pattern.

        :param incell: This dataset contains the actual table cells, and the sensitive aggregates.
        :type incell: pd.DataFrame | pa.Table | str
        :param inconstraint: The dataset containing the linear constraints coefficients.
        :type inconstraint: pd.DataFrame | pa.Table | str
        :param outsuppress: In addition to the fields contained in the incell dataset, this table contains the following two new fields;
            OutStatus and NetVariation, defaults to None
        :type outsuppress: str | Path | None, optional
        :param outcomplement: Contains the identification of all the complements identified, by sensitive cell, defaults to False
        :type outcomplement: str | Path | bool, optional
        :param cost_function1: Specifies the name of the cost function to be used in phase 1 of the Suppression process, defaults to "Size"
        :type cost_function1: str, optional
        :param cost_function2: Specifies the name of the cost function to be used in phase 2 of the Suppression process, defaults to None
        :type cost_function2: str | None, optional
        :param cost_var1: Specifies the name of the cost variable to be used in phase 1 of Suppression process, defaults to None
        :type cost_var1: str | None, optional
        :param cost_var2: Specifies the name of the cost variable to be used in phase 2 of Suppression process, defaults to None
        :type cost_var2: str | None, optional
        :param scale_cost: Specifies the method used to reduce the cost function coefficients. Possible values are None, MEAN and SCALE, defaults to None
        :type scale_cost: str | None, optional
        :param size_roundingbase: This is the rounding base to use in conjunction with the cost function ROUNDEDSIZE, defaults to None
        :type size_roundingbase: int | None, optional
        :param sen_roundingbase: This is the rounding based used to round sensitivity values, defaults to None
        :type sen_roundingbase: int | None, optional
        :param total_roundingbase: This is the rounding based used to round TotalNoise values., defaults to None
        :type total_roundingbase: int | None, optional
        :param constraint_scale: The constraint scale value, defaults to 2
        :type constraint_scale: int, optional
        :param suppress_order: Determines the order in which to suppress, defaults to 0
        :type suppress_order: int, optional
        :param ambiguity_tolerance: Tolerance used when considering if ambiguity is 0, defaults to 0.00001
        :type ambiguity_tolerance: float, optional
        :param by: Variable names separated by spaces used to create processing groups, defaults to None
        :type by: str | None, optional
        # :param multiprocess: Enables the use of multiple processors to distribute solver work, defaults to False
        # :type multiprocess: bool, optional
        :param skip_validation: Should the validation of fields on the input files be skipped?, defaults to False
        :type skip_validation: bool, optional
        :param custom_solver: A custom configured solver to use instead of the default solver, defaults to None
        :type custom_solver: pulp.LpSolver | None, optional
        :param trace: Use the trace parameter to control which log levels are printed, defaults to None
        :type trace: int | bool | None, optional
        :param logger: Specify a custom Logger that you have created, defaults to None
        :type logger: logging.Logger | None, optional
        :param capture: Controls whether messages are printed immediately to the log, defaults to False
        :type capture: bool, optional
        """
        mp = {}
        mp["cost_function1"]=cost_function1
        mp["cost_function2"]=cost_function2
        mp["cost_var1"]=cost_var1
        mp["cost_var2"]=cost_var2
        mp["scale_cost"]=scale_cost
        mp["size_roundingbase"]=size_roundingbase
        mp["sen_roundingbase"]=sen_roundingbase
        mp["total_roundingbase"]=total_roundingbase
        mp["constraint_scale"]=constraint_scale
        mp["suppress_order"]=suppress_order
        mp["ambiguity_tolerance"]=ambiguity_tolerance
        mp["by"]=by
        mp["skip_validation"]=skip_validation
        mp["outcomplement"]=bool(outcomplement) # Pass True whether a path is given or they just marked True
        # Currently deprecated until more efficient methodology found to reduce solver iterations
        mp["multiprocess"]=False #multiprocess
        self._module_parameters = mp

        # input datasets
        self._incell = io.StcInputTable("incell", incell)
        self._inconstraint = io.StcInputTable("inconstraint", inconstraint)
        # output datasets
        self._outsuppress = io.StcOutputTable("outsuppress", outsuppress)
        self._outcomplement = io.StcOutputTable("outcomplement", None if isinstance(outcomplement, bool) else outcomplement)
        # _failed datasets should just use the default output type due to their limited scope, but if you can figure out a clean
        # way to copy the output formats from the non-_failed parameters that fits every case now and into the future that would be better
        self._outsuppress_failed = io.StcOutputTable("outsuppress_failed", None)
        self._outcomplement_failed = io.StcOutputTable("outcomplement_failed", None)

        super().__init__(
            name="Suppress",
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
    def outsuppress(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the outsuppress output data table.

        :return: The outsuppress table as produced by the execution of the suppress proc.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_output_dataset(self._outsuppress)
    @outsuppress.setter
    def outsuppress(self, value):
        self._set_output_dataset(self._outsuppress, value=value)

    @property
    def outcomplement(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the outcomplement output data table.

        Will be empty if `outcomplement` was not provided or set to False.

        :return: The outcomplement table as produced by the execution of the suppress proc.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_output_dataset(self._outcomplement)
    @outcomplement.setter
    def outcomplement(self, value):
        self._set_output_dataset(self._outcomplement, value=value)

    @property
    def outsuppress_failed(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the outsuppress_failed output data table.

        :return: The outsuppress_failed table as produced by the execution of the suppress proc.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_output_dataset(self._outsuppress_failed)
    @outsuppress_failed.setter
    def outsuppress_failed(self, value):
        self._set_output_dataset(self._outsuppress_failed, value=value)

    @property
    def outcomplement_failed(self) -> pd.DataFrame | pa.Table | str:
        """Get or set the outcomplement_failed output data table.

        Will be empty if `outcomplement_failed` was not provided or set to False.

        :return: The outcomplement_failed table as produced by the execution of the suppress proc.
        :rtype:  pd.DataFrame | pa.Table | str
        """
        return self._get_output_dataset(self._outcomplement_failed)
    @outcomplement_failed.setter
    def outcomplement_failed(self, value):
        self._set_output_dataset(self._outcomplement_failed, value=value)

    # required by superclass
    def _pre_execute(self) -> None:
        validator = ValidationContext()
        # numeric columns on incell only
        cost_var1_field = self._module_parameters["cost_var1"]
        cost_var2_field = self._module_parameters["cost_var2"]

        # ensure datasets in proper format
        for ds in self._input_datasets:
            if(not self._module_parameters["skip_validation"]):
                # Unfortunately the best way to apply our schemas to PyArrow Tables is to just convert to Pandas then back after
                # Fortunately this can all be skipped for efficiency's sake
                # In future, time-permitting, the validation will be augmented with direct Arrow validation
                ds.to_pandas()
                if(ds.name == "inconstraint"):
                    validator.strategy = GConfidValidator(schema=SuppressInconstraintModel.to_schema(self._module_parameters["by"]))
                    self.perform_input_validation(ds, validator)
                elif(ds.name == "incell"):
                    validator.strategy = GConfidValidator(schema=SuppressIncellModel.to_schema(self._module_parameters["by"],
                                                                                               cost_var1_field,
                                                                                               cost_var2_field))
                    self.perform_input_validation(ds, validator)

            # Now convert to Arrow for our procs to work on
            ds.to_arrow()

    def _execute(self) -> None:
        # execute suppress
        # determine if the success or failed datasets should be returned
        #self._outsuppress.ds_intermediate, self._outcomplement.ds_intermediate
        ret_code, outsuppress, outcomplement = _suppress(
            incell=self._incell.ds_intermediate,
            inconstraint=self._inconstraint.ds_intermediate,
            logger=self._log,
            **self._module_parameters,
        )
        if ret_code is SuppressReturnCode.SOLVER_FAIL:
            # If the solver failed an iteration make sure we are setting the correct output datasets
            self._outsuppress_failed.ds_intermediate = outsuppress
            self._outcomplement_failed.ds_intermediate = outcomplement
        else:
            self._outsuppress.ds_intermediate = outsuppress
            self._outcomplement.ds_intermediate = outcomplement

    def _post_execute(self) -> None:
        pass  # no post execution required

    def _init_input_datasets(self) -> None:
        super()._init_input_datasets(
            self._incell,
            self._inconstraint,
        )

    def _init_output_datasets(self) -> None:
        super()._init_output_datasets(self._outsuppress, self._outcomplement,
                                      self._outsuppress_failed, self._outcomplement_failed)
