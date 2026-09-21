"""G-Confid suppress module."""

import math
import multiprocessing
import os
import uuid
import warnings
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor, as_completed
from enum import Enum
from logging import DEBUG, ERROR, INFO, Logger
from pathlib import Path

import numpy as np
import pulp
import pyarrow as pa
import pyarrow.compute as pc

import gconfid.exceptions
from gconfid.nls import SupportedLanguage, _
from gconfid.util.solver_utils import _MULTIPROCESS_LOG_DIR, clone_solver, combine_solver_logs, get_log_path

_global_state = {}

class SuppressReturnCode(Enum):
    """Identify the success or failure of a set of suppression iterations."""

    SUCCESS = 0
    SOLVER_FAIL = 1

def _round_up(n: float, decimals: int = 0) -> int:
    """Round up `n` to the specified number of decimal places.

    :param n: The number to round
    :type n: float
    :param decimals: The number of decimal places to round to, defaults to 0
    :type decimals: int, optional
    :return: `n` rounded up to the nearest decimal place in accordance with `decimals`
    :rtype: int
    """
    multiplier = 10**decimals
    return math.ceil(n * multiplier) / multiplier

def _round_down(n: float, decimals: int = 0) -> int:
    """Round down `n` to the specified number of decimal places.

    :param n: The number to round
    :type n: float
    :param decimals: The number of decimal places to round to, defaults to 0
    :type decimals: int, optional
    :return: `n` rounded down to the nearest decimal place in accordance with `decimals`
    :rtype: int
    """
    multiplier = 10**decimals
    return math.floor(n * multiplier) / multiplier

def _select_next_current_cell_suppress(active_cell_ids: list, ambiguity: dict, net_variation: dict) -> int:
    """Output the next sensitive cells to solve the corresponding LP problem.

    :param active_cell_ids: List of IDs for the active cells in the Linear Programming problem
    :type active_cell_ids: list
    :param ambiguity: Mapping of ambiguity values for each CellID
    :type ambiguity: dict
    :param net_variation: Mapping of Net Variation values for each CellID
    :type net_variation: dict
    :return: The cell ID of the next cell to suppress
    :rtype: int
    """
    # Pick the first cell with positive ambiguity
    for i in active_cell_ids:
        if ambiguity[i] > 0:
            current_cell_id = i
            current_cell_ambiguity = ambiguity[i]
            break

    for i in active_cell_ids:
        if ambiguity[i] > 0: # if the cell need to be protected
            if net_variation[i] > 0: # Cell i has already changed
                if net_variation[current_cell_id] == 0: # currently selected cells have not changed
                    current_cell_id = i           # Then we select the cell i to be suppressed
                    current_cell_ambiguity = ambiguity[i]
                elif ambiguity[i] > current_cell_ambiguity: # Compare the ambiguity of the cells and pick the one with larger ambiguity
                    current_cell_id = i
                    current_cell_ambiguity = ambiguity[i]
            elif net_variation[current_cell_id] == 0 and ambiguity[i] > current_cell_ambiguity: #Neither cell changed, pick the larger ambiguity
                current_cell_id = i
                current_cell_ambiguity = ambiguity[i]
    return current_cell_id

def _select_first_current_cell_amb(active_cell_ids: list, ambiguity: dict) -> int | None:
    """Return the active cell id with the highest ambiguity value.

    :param active_cell_ids: List of IDs for the active cells in the Linear Programming problem
    :type active_cell_ids: list
    :param ambiguity: Mapping of ambiguity values for each CellID
    :type ambiguity: dict
    :return: The CellID to be selected (has the highest ambiguity value) or None if no positive ambiguity values are present.
    :rtype: int | None
    """
    current_cell_ambiguity = 0
    current_cell_id = None
    for i in active_cell_ids:
        if ambiguity[i] > current_cell_ambiguity:
            current_cell_id = i
            current_cell_ambiguity = ambiguity[i]
    return current_cell_id

def _select_first_current_cell_sens(active_cell_ids: list, ambiguity: dict, sensitivity: dict) -> int | None:
    """Return the active cell id with the highest ambiguity value.

    :param active_cell_ids: List of IDs for the active cells in the Linear Programming problem
    :type active_cell_ids: list
    :param ambiguity: Mapping of ambiguity values for each CellID
    :type ambiguity: dict
    :param sensitivity: Mapping of sensitivity values for each CellID
    :type sensitivity: dict
    :return: The cell ID to be selected (has the highest sensitivity value) or None if no positive
        ambiguity or sensitivity values are present.
    :rtype: int | None
    """
    current_cell_sensitivity = 0
    current_cell_id = None
    for i in active_cell_ids:
        if ambiguity[i] > 0 and sensitivity[i] > current_cell_sensitivity:
            current_cell_id = i
            current_cell_sensitivity = sensitivity[i]
    return current_cell_id

def _update_ambiguity(x: dict | None, y: dict | None, active_cell_ids: list, net_variation: dict, sensitivity_dic: dict,
                      ambiguity: dict, constraint_scale: int, ambiguity_tolerance: float, current_sen_cell: float | None,
                      multi_results: dict[float, tuple[float, float]] | None = None) -> tuple[dict, dict, list]:
    """Update ambiguity and net variation values and find complementary cells for the current sensitive cell, from the results of a solver execution.

    :param x: x-value results from a solver iteration.
    :type x: dict | None
    :param y: y-value results from a solver iteration.
    :type y: dict | None
    :param active_cell_ids: List of IDs for the active cells in the Linear Programming problem
    :type active_cell_ids: list
    :param net_variation: Mapping of net variation values for each CellID
    :type net_variation: dict
    :param sensitivity_dic: Mapping of sensitivity values for each CellID
    :type sensitivity_dic: dict
    :param ambiguity: Mapping of ambiguity values for each CellID
    :type ambiguity: dict
    :param constraint_scale: The ambiguity factor to use when computing ambiguity
    :type constraint_scale: int
    :param ambiguity_tolerance: The level of tolerance for determining a cell's complement status and ambiguity value
    :type ambiguity_tolerance: float
    :param current_sen_cell: The CellID of the sensitive cell for which the solver was run
    :type current_sen_cell: sloat | None
    :param logger: Instance of logger to use to record information during execution
    :type logger: Logger
    :param multi_results: A mapping of cellIDs to their respective x and y result, solved for in parallel, defaults to None
    :type multi_results: dict[float, tuple[float, float]] | None, optional
    :return: A tuple of the updated net variation and ambiguity dictionaries and the list of CellIDs for the cells
        that were foudn to be complements for the current sensitive cell
    :rtype: tuple[dict, dict, list]
    """
    complement_cells = []
    # Update the ambiguity value and net variation after solving the LP problem.
    for i in active_cell_ids:
        if multi_results is not None and i not in multi_results:
            continue # A non-optimal result was obtained and therefore no value exists for this cell, just return what we have

        x_val = x[i].varValue if x is not None else multi_results[i][0]
        y_val = y[i].varValue if y is not None else multi_results[i][1]
        abs_x_minus_y = abs(x_val - y_val)
        if abs_x_minus_y > ambiguity_tolerance:
            net_variation[i] = max(net_variation[i], abs_x_minus_y) # update and keep the maximum variation for each cell
            if sensitivity_dic[i] > 0:
                # Calculate new ambiguity of the cell
                ambiguity[i] = (sensitivity_dic[i]/constraint_scale) - net_variation[i] # sensitivity/ ambiguity factor(=2) - Net Variation.
                if ambiguity[i] > 0.0 and ambiguity[i] < ambiguity_tolerance:
                    ambiguity[i] = 0
            # If current_sen_cell is None, we are processing a parallel solve so we have no complements
            if current_sen_cell not in (i, None):
                complement_cells.append(i)

    return net_variation, ambiguity, complement_cells

def _count_cells_to_treat(ambiguity: dict) -> int:
    """Output the number of sensitive cells to treat.

    :param ambiguity: Mapping of ambiguity values for each CellID
    :type ambiguity: dict
    :return: The number of cells to treat
    :rtype: int
    """
    return sum(1 for value in ambiguity.values() if value > 0)

def _cost_coef_zero(instatus_dic: dict, active_cell_ids: list, cost: dict) -> dict:
    """Revise the `cost` values to 0 for any cells that originally began with a Status value of 'S' or 'X'.

    :param instatus_dic: Mapping of "Status" values for each CellID as they were originally in incell, with
        only the normalization changes applied (i.e. instatus_dic not status_dic)
    :type instatus_dic: dict
    :param active_cell_ids: List of IDs for the active cells in the Linear Programming problem
    :type active_cell_ids: list
    :param cost: Mapping of cost values for each CellID
    :type cost: dict
    :return: The updated cost mappings
    :rtype: dict
    """
    for i in active_cell_ids:
        if instatus_dic[i] == "S" or instatus_dic[i] == "X":
            cost[i] = 0
    return cost

def _cost_coef_zero_complements(active_cell_ids: list, net_variation: dict, cost: dict) -> dict:
    """Revise the `cost` values to 0 for any cells that have a positive Net Variation.

    :param active_cell_ids: List of IDs for the active cells in the Linear Programming problem
    :type active_cell_ids: list
    :param net_variation: Mapping of Net Variation values for each CellID
    :type net_variation: dict
    :param cost: Mapping of cost values for each CellID
    :type cost: dict
    :return: The updated cost mappings
    :rtype: dict
    """
    for i in active_cell_ids:
        if net_variation[i] > 0:
            cost[i] = 0
    return cost

def _cost_coefficients(active_cell_ids: list, cost_function: str, c_var: dict, size_roundingbase: float | None) -> dict:
    """Calculate cost coefficients.

    :param active_cell_ids: List of IDs for the active cells in the Linear Programming problem
    :type active_cell_ids: list
    :param cost_function: The name of the cost function to use
    :type cost_function: str
    :param c_var: Mapping of cost variables to CellIDs
    :type c_var: dict
    :param size_roundingbase: The rounding base to use for `cost_function == "ROUNDEDSIZE"`
    :type size_roundingbase: float | None
    :return: A mapping of computed cost values to their CellIDs
    :rtype: dict
    :raises gconfid.exceptions.ProcessingError: if the `cost_function` holds an unsupported or unexpected value
    """
    cost = {}
    for i in active_cell_ids:
        if cost_function == "DIGITS":
            cost[i] = math.log10(c_var[i] + 1)

        elif cost_function == "INFORMATION":
            cost[i] = math.log10(c_var[i]+1)/ (c_var[i] + 1)

        elif cost_function == "SCALEDINFORMATION":
            cost[i] = round(np.mean(list(c_var.values()))*math.log10(c_var[i]+1)/ (c_var[i] + 1), 3)

        elif cost_function == "CONSTANT":
            cost[i] = 1

        elif cost_function == "SIZE":
            cost[i] = c_var[i]

        elif cost_function == "ROUNDEDSIZE":
            cost[i] = round(c_var[i], size_roundingbase)

        elif cost_function == "INVERSE":
            if c_var[i] == 0:
                cost[i] = 0
            else:
                cost[i] = 1/(c_var[i]+1)

        elif cost_function == "SCALEDINVERSE":
            if c_var[i] == 0:
                cost[i] = 0
            else:
                cost[i] = round(np.mean(list(c_var.values()))/(c_var[i]+1), 3)
        else:
            msg = _("Error in Cost function")
            raise gconfid.exceptions.ProcessingError(msg)

    return cost

def _cost_coefficients_mean(cost: dict, active_cell_ids: list) -> dict:
    """Divide all cost coefficients by their mean.

    :param cost: Mapping of cost values for each CellID
    :type cost: dict
    :param active_cell_ids: List of IDs for the active cells in the Linear Programming problem
    :type active_cell_ids: list
    :return: The updated cost mappings
    :rtype: dict
    """
    avg_cost_coefficient = sum(cost.values())/len(cost)
    if avg_cost_coefficient != 0:
        for i in active_cell_ids:
            cost[i] = cost[i]/avg_cost_coefficient
    return cost

def _cost_coefficients_scale(cost: dict, active_cell_ids: list) -> dict:
    """Scale cost coefficients.

    :param cost: Mapping of cost values for each CellID
    :type cost: dict
    :param active_cell_ids: List of IDs for the active cells in the Linear Programming problem
    :type active_cell_ids: list
    :return: The updated cost mappings
    :rtype: dict
    """
    scalecost_upperbound = 1000 # hidden variable in SAS, defaults to 1000
    scalecost_lowerbound = 1
    max_cost_coefficient = max(cost.values())
    min_cost_coefficient = min(cost.values())

    if max_cost_coefficient != min_cost_coefficient:
        scaling_factor = (scalecost_upperbound - scalecost_lowerbound) / (max_cost_coefficient - min_cost_coefficient)
        for i in active_cell_ids:
            if cost[i] != 0:
                cost[i] = scalecost_lowerbound + ((cost[i] - min_cost_coefficient) * scaling_factor)
    return cost

def _suppression_solver(prob: pulp.LpProblem, x: dict, y: dict, sensitivity_dic: dict, total_noise_dic: dict,
                        current_sen_cell: float, constraint_scale: int, prev_sen_cell: float | None = None,
                        solver: pulp.LpSolver | None = None) -> tuple[str, dict, dict]:
    """Execute the solver to solve the LP suppresion problem.

    :param prob: The PuLP LpProblem containing all the parameters and settings necessary to solve the problem
    :type prob: pulp.LpProblem
    :param x: One of the LpVariables
    :type x: dict
    :param y: One of the LpVariables
    :type y: dict
    :param sensitivity_dic: Mapping of sensitivity values for each CellID
    :type sensitivity_dic: dict
    :param total_noise_dic: Mapping of TotaNoise values for each CellID
    :type total_noise_dic: dict
    :param current_sen_cell: The CellID of the sensitive cell to run the solver on
    :type current_sen_cell: float
    :param prev_sen_cell: The CellID of the previous sensitive cell the solver was run on
    :type prev_sen_cell: float | None
    :param constraint_scale: This is divided by the TotalNoise or Sensitivity values to determine bounds on the LpVariables
    :type constraint_scale: int
    :return: The results of the solver via a status identifier. The LPVariables in x and y should be updated in-place
    :rtype: str
    """
    # If there is a previous sensitive cell, revert the bounds
    if prev_sen_cell is not None:
        x[prev_sen_cell].lowBound = 0
        y[prev_sen_cell].upBound = total_noise_dic[prev_sen_cell]/constraint_scale

    # Initialize bounds of current sensitive cell
    x[current_sen_cell].lowBound = sensitivity_dic[current_sen_cell]/constraint_scale
    y[current_sen_cell].upBound = 0

    # If the user supplied a custom_solver to the wrapper, it will be set as the default already
    solver = pulp.LpSolverDefault if solver is None else solver

    prob.solve(solver)

    return pulp.const.LpStatus[prob.status]

def _suppress_main(incell: pa.Table, inconstraint: pa.Table, logger: Logger, cost_function1: str = "Size", cost_function2: str | None = None,
                   cost_var1: str | None = None, cost_var2: str | None = None, scale_cost: str | None = None, size_roundingbase: int | None = None,
                   sen_roundingbase: int | None = None, total_roundingbase: int | None = None, constraint_scale: int = 2, suppress_order: int = 0,
                   ambiguity_tolerance: float = 0.00001, outcomplement: bool = False, by_group_info: str | None = None, multiprocess: bool = False,
                   ) -> tuple[SuppressReturnCode, pa.Table, pa.Table | None]:
    """Perform the actual suppression work in iterating with the solver and computing suppression information.

    :param incell: Dataset containing the input cell information
    :type incell: pa.Table
    :param inconstraint: Dataset containing the linear constraints
    :type inconstraint: pa.Table
    :param logger: Instance of logger to use to record information during execution
    :type logger: Logger
    :param cost_function1: Specifies the name of the cost function to be used in phase 1, defaults to "Size"
    :type cost_function1: str, optional
    :param cost_function2: Specifies the name of the cost function to be used in phase 2, defaults to None
    :type cost_function2: str | None, optional
    :param cost_var1: Specifies the name of the cost variable to be used in phase 1. It must be a numeric variable
        present in the incell data set. This variable must have only positive values, and missing values are not.
        allowed. When this parameter is provided, the variable specified is used to calculate the cost function.
        coefficients, instead of TOTAL, defaults to None
    :type cost_var1: str | None, optional
    :param cost_var2: Specifies the name of the cost variable to be used in phase 2. It must be a numeric variable
        present in the incell data set. This variable must have only positive values, and missing values are not.
        allowed. When this parameter is provided, the variable specified is used to calculate the cost function.
        coefficients, instead of TOTAL, defaults to None, defaults to None, defaults to None
    :type cost_var2: str | None, optional
    :param scale_cost: Specifies the method used to reduce the cost function coefficients, defaults to None
    :type scale_cost: str | None, optional
    :param size_roundingbase: Rounding base used for the cost function ROUNDEDSIZE, defaults to None
    :type size_roundingbase: int | None, optional
    :param sen_roundingbase: Rounding base used to round the sensitivity value, defaults to None
    :type sen_roundingbase: int | None, optional
    :param total_roundingbase: Rounding base used to round the total value, defaults to None
    :type total_roundingbase: int | None, optional
    :param constraint_scale: Used for scaling various values, defaults to 2
    :type constraint_scale: int, optional
    :param suppress_order: How to choose the next cell, defaults to 0
    :type suppress_order: int, optional
    :param ambiguity_tolerance: Tolerance when considering ambiguity values equal, defaults to 0.00001
    :type ambiguity_tolerance: float, optional
    :param outcomplement: Should an outcomplement dataset be generated?, defaults to False
    :type outcomplement: bool, optional
    :param by_group_info: The By groups this function is being called on, printed in suppress report, defaults to None
    :type by_group_info: str | None, optional
    :param multiprocess: Enables the use of multiple processors to distribute solver work, defaults to False
    :type multiprocess: bool, optional
    :return: The return code indicating success or failure of the solver iterations, outsuppress and outcomplements if requested
    :rtype: tuple[SuppressReturnCode, pa.Table, pa.Table | None]
    """
    start_time = gconfid.get_time()

    cell_id = incell.column("CellId").to_pylist()
    cost1 = None
    cost2 = None
    if cost_var1 is not None:
        cost1 = dict(zip(cell_id, incell.column(cost_var1).to_pylist(), strict=True))
    if cost_var2 is not None:
        cost2 = dict(zip(cell_id, incell.column(cost_var2).to_pylist(), strict=True))
    conci = inconstraint.column("CellId").to_pylist()
    ni = inconstraint.column("ConstraintId").to_pylist()
    a = inconstraint.column("Coefficient").to_pylist()

    coef = defaultdict(list) # {constraint ID : (CellId, coefficient)}

    for cid, cell, coeff in zip(ni, conci, a, strict=True):
        coef[cid].append([cell, coeff])

    # put everything into dicts with Cell ID as key
    status_upper = pc.utf8_upper(incell.column("Status")).to_pylist()
    sensitivity_dic = dict(zip(cell_id, incell.column("Sensitivity").to_pylist(), strict=True))
    total_noise_dic = dict(zip(cell_id, incell.column("TotalNoise").to_pylist(), strict=True))
    in_status_dic = dict(zip(cell_id, status_upper, strict=True))
    status_dic =  in_status_dic.copy()

    for key in in_status_dic:
        if total_roundingbase is not None:
            total_noise_dic[key] = _round_down(total_noise_dic[key], total_roundingbase)

        if sen_roundingbase is not None:
            sensitivity_dic[key] = _round_up(sensitivity_dic[key], sen_roundingbase)

        # Change status to be in line with sensitivity... Sensitivity has precedence
        if sensitivity_dic[key] > 0 and (status_dic[key] == "V" or status_dic[key] == "X"):
            status_dic[key] = "S"
            in_status_dic[key] = "S"
            logger.debug(_("Sensitivity value indicates sensitive but status is 'V' or 'X'. Status has been set to 'S'. CellId = {}").format(key))
        elif sensitivity_dic[key] <= 0 and status_dic[key] == "S":
            status_dic[key] = "V"
            in_status_dic[key] = "V"
            logger.debug(_("Sensitivity value indicates non-sensitive but status is 'S'. Status has been set to 'V'. CellId = {}").format(key))

        if sensitivity_dic[key] > total_noise_dic[key]:
            sensitivity_dic[key] = total_noise_dic[key]
            logger.debug(_("Sensitivity value is greater than the cell total. Sensitivity has been set to cell total. CellId = {}").format(key))

    # Initialize net variation and ambiguity of each cell
    net_variation = {}
    ambiguity = {}
    complements = [] if outcomplement else None

    active_cell_ids = []
    for i in cell_id:
        # Initialize a value here for every cell, even those excluded from the LP
        # This ensures all Cells are included in the final output
        net_variation[i] = 0
        if status_dic[i] != "P":
            active_cell_ids.append(i)
            ambiguity[i] = sensitivity_dic[i]/constraint_scale

    # Go through the constraint values and set the coefficient of cells with Status 'P' to be zero (note we are using STATUS here not INstatus)
    for constraint_value in coef.values():
        for sets in constraint_value:
            if status_dic[sets[0]] == "P":
                sets[1] = 0

    # Suppress specific warning - verified with methodology that this warning is expected.
    warnings.filterwarnings("ignore", category=UserWarning, message="Overwriting previously set objective.")

    # Phase 1
    ret_code, net_variation, ambiguity, complements = perform_solver_phase(1, active_cell_ids, ambiguity, coef, complements, cost_var1,
                                                                           cost_function1, cost1, size_roundingbase, total_noise_dic, scale_cost,
                                                                           in_status_dic, sensitivity_dic, constraint_scale, ambiguity_tolerance,
                                                                           suppress_order, net_variation, logger, multiprocess=multiprocess)
    if ret_code is SuppressReturnCode.SOLVER_FAIL:
        # There was a solver issue, return the outputs immediately using what should be the outputs from the last successful iteration
        return (ret_code, form_outcell(incell, net_variation, in_status_dic), form_outcomplements(complements))

    logger.info(generate_suppress_report(form_outcell(incell, net_variation, in_status_dic), by_group_info, 1))

    end_time = gconfid.get_time()
    logger.info(_("\nPhase 1 Time:                             {}\n").format(str(end_time - start_time)[:-4]))

    if cost_function2 is not None:
        # Phase 2
        start_time = gconfid.get_time()

        for i in active_cell_ids.copy():
            if net_variation[i] == 0 and status_dic[i] != "X":
                status_dic[i] = "P" # Cell is publishable after Phase 1
                active_cell_ids.remove(i)
            else:
                status_dic[i] = "S" # Cell is suppressed after Phase 1, as sensitive or complement, or was marked as 'X' by the user
                ambiguity[i] = sensitivity_dic[i]/constraint_scale
                net_variation[i] = 0 # only set this here for Sensitive cells, unlike phase 1 where we init ALL cells with 0

        # Go through the constraint values and set the coefficient of cells with Status 'P' to be zero (note we are using STATUS here not INstatus)
        for constraint_value in coef.values():
            for sets in constraint_value:
                if status_dic[sets[0]] == "P":
                    sets[1] = 0

        ret_code, net_variation, ambiguity, complements = perform_solver_phase(2, active_cell_ids, ambiguity, coef, complements, cost_var2, cost_function2,
                                                                               cost2, size_roundingbase, total_noise_dic, scale_cost, in_status_dic,
                                                                               sensitivity_dic, constraint_scale, ambiguity_tolerance, suppress_order,
                                                                               net_variation, logger, multiprocess=multiprocess)

        if ret_code is SuppressReturnCode.SOLVER_FAIL:
            # There was a solver issue, return the outputs immediately using what should be the outputs from the last successful iteration
            return (ret_code, form_outcell(incell, net_variation, in_status_dic), form_outcomplements(complements))

        end_time = gconfid.get_time()
        logger.info(_("\nPhase 2 Time:                             {}\n").format(str(end_time - start_time)[:-4]))

    return (SuppressReturnCode.SUCCESS, form_outcell(incell, net_variation, in_status_dic), form_outcomplements(complements))

def setup_problem(phase_num: int, active_cell_ids: list[float], constraint_scale: int, coef: dict, cost_var: dict,
                  cost_function: str, cost_num: dict, size_roundingbase: float | None, total_noise_dic: dict,
                  in_status_dic: dict, base_solver: pulp.LpSolver | None = None, lang: SupportedLanguage | None = None) -> None:
    """Set up the PuLP variables, the problem to solve and the cost values for a phase.

    Saves the problem, x and y variable arrays and the cost values for each cell to the _global_state.

    :param phase_num: The current phase of solving, either 1 or 2
    :type phase_num: int
    :param active_cell_ids: List of IDs for the active cells in the Linear Programming problem
    :type active_cell_ids: list[float]
    :param constraint_scale: Used for scaling various values
    :type constraint_scale: int
    :param coef: ConstraintIDs mapped to CellIDs and coefficients
    :type coef: dict
    :param cost_var: Specifies the name of the cost variable to be used. It must be a numeric variable present in the incell data set.
        This variable must have only positive values, and missing values are not allowed. When this parameter is provided, the variable
        specified is used to calculate the cost function and coefficients, instead of TOTAL
    :type cost_var: dict
    :param cost_function: Specifies the name of the cost function to be used. Possible values are DIGITS, INFORMATION, SCALEDINFORMATION,
        CONSTANT, SIZE, ROUNDEDSIZE, INVERSE, SCALEDINVERSE.
    :type cost_function: str
    :param cost_num: Mapping of `cost_var`s for each CellID
    :type cost_num: dict
    :param size_roundingbase: Rounding base used for the cost function ROUNDEDSIZE
    :type size_roundingbase: float | None
    :param total_noise_dic: Mapping of TotalNoise values for each CellID
    :type total_noise_dic: dict
    :param in_status_dic: Mapping of original Status values for each CellID
    :type in_status_dic: dict
    :param base_solver: If None, the global solver will be the same instance as the current pulp.LpSolverDefault.
        Otherwise a new instance of base_solver is created with the same parameters and options set, except for logPath
        or logfile which will receive a new value based on the current process pid.
    :type base_solver: pulp.LpSolver | None
    :param lang: The language of the parent process if calling this in a newly spawned process, defaults to None
    :type lang: SupportedLanguage | None
    """
    # Make sure we copy the language settings from the parent process as this value is overwritten at this point
    if lang is not None:
        # Make sure we don't also call _reload_all_procs as we know we won't need those here so it's just a waste of cycles
        gconfid.set_language(lang, reload_procs=False)

    if cost_var is not None:
        cost = _cost_coefficients(active_cell_ids, cost_function, cost_num.copy(), size_roundingbase)
    else:
        cost = _cost_coefficients(active_cell_ids, cost_function, total_noise_dic.copy(), size_roundingbase)
    # We specifically want to use INstatus here, not the status dict that gets updated
    cost = _cost_coef_zero(in_status_dic, active_cell_ids, cost)

    prob = pulp.LpProblem(f"Suppress{'2' if phase_num == 2 else ''}", pulp.const.LpMinimize) #noqa: PLR2004

    # Generate LpVariable dic
    x = pulp.LpVariable.dict("x", active_cell_ids, 0.0, None, cat=pulp.const.LpContinuous)
    y = pulp.LpVariable.dict("y", active_cell_ids, 0.0, None, cat=pulp.const.LpContinuous)

    # Initialize the cell-specific bounds of all of our LPVariables
    for i in active_cell_ids:
        x[i].upBound = total_noise_dic[i]/constraint_scale
        y[i].upBound = total_noise_dic[i]/constraint_scale

    # Constraint
    for conid, item in coef.items():
        conlist = []
        for index in range(len(item)):
            i = item[index][0]
            coefficient = item[index][1]
            if coefficient != 0:
                conlist.append(coefficient*(x[i] - y[i]))
        prob += pulp.LpConstraint(pulp.lpSum(conlist), 0, "conu"+str(conid), 0)

    # In order to support multi-processing we need to recreate a solver on each process
    # rather than share the same instance of the default.
    # When a new child process is spawned in Windows, the entire import chain is re-imported, thus the pulp.LpSolverDefault
    # is reset, wiping the custom_solver we set earlier. Therefore it must be passed in (base_solver) to retain the reference we use to create a new instance.
    _global_state["solver"] = pulp.LpSolverDefault if base_solver is None else clone_solver(base_solver, unique_log_suffix=str(os.getpid()))
    _global_state["prob"] = prob
    _global_state["x"] = x
    _global_state["y"] = y
    _global_state["cost"] = cost

def solve_cell_parallel(cells_to_solve: list[float], phase_num: int, active_cell_ids: list, cost_function: str,
                        total_noise_dic: dict, scale_cost: str | None, sensitivity_dic: dict, constraint_scale: int,
                        ) -> tuple[SuppressReturnCode, list[tuple], dict[int, list[str]]]:
    # It's not a good idea to send logs to the same logger in parallel so we'll collect the messages and report them when complete
    log_msgs = {ERROR: [], DEBUG: [], INFO: []}

    solver = _global_state["solver"]
    prob = _global_state["prob"]
    x = _global_state["x"]
    y = _global_state["y"]
    cost = _global_state["cost"]

    if cost_function in ("INFORMATION", "SIZE"):
        if scale_cost == "MEAN":
            cost = _cost_coefficients_mean(cost, active_cell_ids)
        elif scale_cost == "SCALE":
            cost = _cost_coefficients_scale(cost, active_cell_ids)

    #Objective function
    prob += pulp.lpSum([cost[i] * (x[i]+ y[i]) for i in active_cell_ids])

    results = {}
    for curr_cell in cells_to_solve:
        # Initialize the LP Problem in puLP
        log_msgs[INFO].append(_("Phase {:<9} CurrentCell: {:<9}").format(phase_num, curr_cell))

        # Bounds are set within
        solver_status = _suppression_solver(prob, x, y, sensitivity_dic, total_noise_dic, curr_cell, constraint_scale, solver=solver)

        if solver_status != "Optimal":
            # This current iteration's updates to our return objects have not been made if we're here, so we can just return the original objects
            # instead of the copies made for the previous iteration, like above
            log_msgs[ERROR].append(_("Error occurred in solving the LP: Non-Optimal status returned by solver for cell {}.").format(curr_cell))
            # Instead of raising an exception we now return valid outputs from the previous successful iteration on solver failure
            return (SuppressReturnCode.SOLVER_FAIL, results, log_msgs)

        log_msgs[DEBUG].append(_("Current cell lower bound {}").format(x[curr_cell].lowBound))
        log_msgs[DEBUG].append(_("x value {}").format(x[curr_cell].varValue))
        log_msgs[DEBUG].append(_("y value {}").format(y[curr_cell].varValue))
        log_msgs[DEBUG].append(_("Current cell ABS x minus y: {}").format(abs(x[curr_cell].varValue - y[curr_cell].varValue)))

        # Add the tuple of values we need to compute ambiguity and net_variation (and complements?)
        results[curr_cell] = (x[curr_cell].varValue, y[curr_cell].varValue)

    # Grab log filepath from the solver and modify filename on-disk then when this task runs next,
    # even if in the same process, this log file won't be overwritten.
    log_path = get_log_path(solver)
    if log_path:
        log_path = Path(log_path)
        # If this log file is in the multiprocess temp folder, we need to rename, otherwise we're good
        if log_path.parent.stem == _MULTIPROCESS_LOG_DIR:
            log_path.rename(f"{log_path.with_suffix('')}_{uuid.uuid4()}{log_path.suffix}")

    return SuppressReturnCode.SUCCESS, results, log_msgs

def solve_cell_sequential(phase_num: int, active_cell_ids: list, ambiguity: dict, complements: list | None,
                         cost_function: str, total_noise_dic: dict, scale_cost: str | None, sensitivity_dic: dict,
                         constraint_scale: int, ambiguity_tolerance: float, suppress_order: int, net_variation: dict,
                         logger: Logger) -> tuple[SuppressReturnCode, dict, dict, list, str]:
    solver = _global_state["solver"]
    prob = _global_state["prob"]
    x = _global_state["x"]
    y = _global_state["y"]
    cost = _global_state["cost"]

    # Get the number of sensitive cells to treat
    cells_to_treat = _count_cells_to_treat(ambiguity)
    # Track the last sensitive cell that was processed
    prev_sen_cell = None
    # Select the first sensitive cell to run (Cell with Highest ambiguity)
    current_sen_cell = _select_first_current_cell_amb(active_cell_ids, ambiguity)
    if(current_sen_cell is None):
        # This shouldn't generally be reached as we pre-check incell to avoid this. So if
        # we actually make it here there must have been an unexpected error
        msg = _("Selecting the cell with the highest ambiguity value failed.")
        logger.error(msg)
        raise gconfid.exceptions.GConfidSuppressNoSensitiveValuesError(msg)

    iteration = 0
    while cells_to_treat > 0:
        if cost_function in ("INFORMATION", "SIZE"):
            if scale_cost == "MEAN":
                cost = _cost_coefficients_mean(cost, active_cell_ids)
            elif scale_cost == "SCALE":
                cost = _cost_coefficients_scale(cost, active_cell_ids)

        #Objective function
        prob += pulp.lpSum([cost[i] * (x[i]+ y[i]) for i in active_cell_ids])
        # Initialize the LP Problem in puLP
        logger.info(_("Phase {:<9} iteration: {:<9} CurrentCell: {:<9} cells_to_treat: {:<9}").format(phase_num, iteration, current_sen_cell, cells_to_treat))

        solver_status = _suppression_solver(prob, x, y, sensitivity_dic, total_noise_dic, current_sen_cell, constraint_scale, prev_sen_cell, solver=solver)

        if solver_status == "Optimal":
            # We must keep a copy (can use shallow as dict and list elements are immutable) of the previous iteration's results in case the
            # updated ambiguity dict indicates the same number of cells to treat as the previous iteration, in which case we want to return
            # the last successful iteration's results which would at that point be updated by _update_ambiguity
            net_variation_prev = net_variation.copy()
            ambiguity_prev = ambiguity.copy()
            net_variation, ambiguity, complement_ids = _update_ambiguity(x, y, active_cell_ids, net_variation, sensitivity_dic, ambiguity,
                                                                         constraint_scale, ambiguity_tolerance, current_sen_cell)

            # We must perform this check prior to updating the complements list, lest we accidentally update complements with invalid data
            new_cells_to_treat = _count_cells_to_treat(ambiguity)
            if cells_to_treat == new_cells_to_treat:
                logger.error(_("Error occurred in solving the LP: Optimal status was returned but cells_to_treat count was unchanged from last iteration."))
                # Because our outputs have been updated by the output from an unsuccessful solver iteration, we need to return the previous
                # iteration's versions instead, except complements which is not updated yet
                return (SuppressReturnCode.SOLVER_FAIL, net_variation_prev, ambiguity_prev, complements)
            cells_to_treat = new_cells_to_treat

            # (Phase, "ProtectionNumber", CurrentCellId, Complement cell ID)
            if complements is not None:
                complements.extend([(phase_num, iteration, current_sen_cell, complement_id) for complement_id in complement_ids])
        else:
            # This current iteration's updates to our return objects have not been made if we're here, so we can just return the original objects
            # instead of the copies made for the previous iteration, like above
            logger.error(_("Error occurred in solving the LP: Non-Optimal status returned by solver."))
            # Instead of raising an exception we now return valid outputs from the previous successful iteration on solver failure
            return (SuppressReturnCode.SOLVER_FAIL, net_variation, ambiguity, complements)

        logger.debug(_("Current cell lower bound {}").format(x[current_sen_cell].lowBound))
        logger.debug(_("x value {}").format(x[current_sen_cell].varValue))
        logger.debug(_("y value {}").format(y[current_sen_cell].varValue))
        logger.debug(_("Current cell ABS x minus y: {}").format(abs(x[current_sen_cell].varValue - y[current_sen_cell].varValue)))
        logger.debug(_("Current cell net variation {}").format(net_variation[current_sen_cell]))
        logger.debug(_("Current cell ambiguity {}").format(ambiguity[current_sen_cell]))

        if cells_to_treat > 0:
            # Before we update current, save the previous
            prev_sen_cell = current_sen_cell

            cost = _cost_coef_zero_complements(active_cell_ids, net_variation, cost)
            if suppress_order == 0:
                current_sen_cell = _select_next_current_cell_suppress(active_cell_ids, ambiguity, net_variation)
            elif suppress_order == 1:
                current_sen_cell = _select_first_current_cell_amb(active_cell_ids, ambiguity)
            else:
                current_sen_cell = _select_first_current_cell_sens(active_cell_ids, ambiguity, sensitivity_dic)

            if(current_sen_cell is None):
                # This shouldn't generally be reached as we pre-check incell to avoid this. So if
                # we actually make it here there must have been an unexpected error
                msg = _("The suppress_order {} used was not able to find the next cell to protect.").format(suppress_order)
                logger.error(msg)
                raise gconfid.exceptions.GConfidSuppressNoSensitiveValuesError(msg)

        iteration += 1

    return SuppressReturnCode.SUCCESS, net_variation, ambiguity, complements

def perform_solver_phase(phase_num: int, active_cell_ids: list, ambiguity: dict, coef: dict, complements: list | None, cost_var: dict,
                         cost_function: str, cost_num: dict, size_roundingbase: float | None, total_noise_dic: dict, scale_cost: str | None,
                         in_status_dic: dict, sensitivity_dic: dict, constraint_scale: int, ambiguity_tolerance: float, suppress_order: int,
                         net_variation: dict, logger: Logger, multiprocess: bool = False,
                         ) -> tuple[SuppressReturnCode, dict[int, int], dict[int, int], list[tuple] | None]:
    """Perform a phase of the solver, running as many iterations as applicable until a solution is found.

    If a solver issue occurs during an iteration and the full phase is incomplete, a SuppressReturnCode of SOLVER_FAIL is returned and the other outputs
    are given with the values from the last successful iteration.

    :param phase_num: The current phase of solving, either 1 or 2
    :type phase_num: int
    :param active_cell_ids: List of IDs for the active cells in the Linear Programming problem
    :type active_cell_ids: list
    :param ambiguity: Mapping of ambiguity values for each CellID
    :type ambiguity: dict
    :param complements: The list of complements to add newly found complements to, if provided
    :type complements: list | None
    :param cost_function: Specifies the name of the cost function to be used. Possible values are DIGITS, INFORMATION, SCALEDINFORMATION,
        CONSTANT, SIZE, ROUNDEDSIZE, INVERSE, SCALEDINVERSE.
    :type cost_function: str
    :param total_noise_dic: Mapping of TotalNoise values for each CellID
    :type total_noise_dic: dict
    :param scale_cost: Specifies the method used to reduce the cost function coefficients. Possible values are NONE, MEAN and SCALE
    :type scale_cost: str | None
    :param sensitivity_dic: Mapping of sensitivity values for each CellID
    :type sensitivity_dic: dict
    :param constraint_scale: Used for scaling various values
    :type constraint_scale: int
    :param ambiguity_tolerance: Tolerance when considering ambiguity values equal
    :type ambiguity_tolerance: float
    :param suppress_order: How to choose the next cell, options include 0=Highest Ambiguity Changed, 1=Highest Ambiguity, 2=Highest Sensitivity
    :type suppress_order: int
    :param net_variation: Mapping of NetVariation values for each CellID
    :type net_variation: dict
    :param logger: Instance of logger to use to record information during execution
    :type logger: Logger
    :raises gconfid.exceptions.GConfidSuppressNoSensitiveValuesError: if no sensitive values can be found to suppress
    :return: A return code indicating success of failure of the solver process, the `net_variation` and `ambiguity` dictionaries
        and the complements list (if applicable) with any required updates made to their entries based on the solver process.
    :rtype: tuple[SuppressReturnCode, dict[int, int], dict[int, int], list[tuple] | None]
    """
    # Multiprocessing is currently not usable for the user-facing suppress wrapper.
    # It is left here for future development but the current parallel method invokes the solver more than
    # the sequential method so it is much slower and thus should not be used for any production cases.
    num_workers = multiprocessing.cpu_count() - 1 if multiprocess else 1
    if num_workers > 1 and phase_num == 1:
        # Divide the active_cell_ids into num_workers sub-lists of roughly equal length.
        # Alternatively we could divide active_cell_ids into sub-lists of length num_workers
        # and then sequentially process each sub-list, parallelizing each cell in a sub-list.
        # However this would necessarily mean recreating the solver/problem/vars for each individual cell
        # while still needing to sequentially process each sub-list in turn anyways.
        quot, rem = divmod(len(active_cell_ids), num_workers)
        cell_distr = [active_cell_ids[i*quot + min(i, rem):(i+1)*quot + min(i+1, rem)] for i in range(num_workers)]

        # We will retrieve a dict of all cellids processed mapped to a tuple of their x and y values as solved
        results = {}
        retcode = None
        # Parallelize each subset
        with ProcessPoolExecutor(max_workers=num_workers, initializer=setup_problem,
                                 initargs=(phase_num, active_cell_ids, constraint_scale, coef, cost_var, cost_function,
                                           cost_num, size_roundingbase, total_noise_dic, in_status_dic, pulp.LpSolverDefault, gconfid.nls.get_language()),
                                 ) as executor:
            # Each process will solve for a single cell
            futures = [executor.submit(solve_cell_parallel, cell_lst, phase_num, active_cell_ids, cost_function, total_noise_dic,
                                       scale_cost, sensitivity_dic, constraint_scale) for cell_lst in cell_distr]

            for future in as_completed(futures):
                # Add the returned messages to the log first
                for log_type, logs in future.result()[2].items():
                    for log in logs:
                        if log_type is ERROR:
                            logger.error(log)
                        elif log_type is INFO:
                            logger.info(log)
                        elif log_type is DEBUG:
                            logger.debug(log)

                if future.result()[0] is SuppressReturnCode.SOLVER_FAIL:
                    msg = _("Non-optimal result returned by solver. Returning partially suppressed data...")
                    logger.error(msg)
                    retcode = SuppressReturnCode.SOLVER_FAIL
                elif retcode is None: # only take the first succesful return code
                    retcode = future.result()[0]

                # Update the results dict
                results = results | future.result()[1]

        # If there is a log path set on the base solver (that was cloned once per-process)
        # we need to combine the log files they created into a single one
        combine_solver_logs(pulp.LpSolverDefault)

        # We don't consider compliments here so we can just discard the returned list
        net_variation, ambiguity, _complement_ids = _update_ambiguity(None, None, active_cell_ids, net_variation, sensitivity_dic, ambiguity,
                                                                      constraint_scale, ambiguity_tolerance, None, results)
        # If running in parallel we cannot produce these
        complements = None
    else:
        # Setup our _global_state
        setup_problem(phase_num, active_cell_ids, constraint_scale, coef, cost_var, cost_function, cost_num,
                      size_roundingbase, total_noise_dic, in_status_dic, base_solver=None)
        retcode, net_variation, ambiguity, complements = solve_cell_sequential(phase_num, active_cell_ids, ambiguity, complements, cost_function,
                                                                               total_noise_dic, scale_cost, sensitivity_dic, constraint_scale,
                                                                               ambiguity_tolerance, suppress_order, net_variation, logger)

    return (retcode, net_variation, ambiguity, complements)

def form_outcell(incell: pa.Table, net_variation: dict, instatus: dict) -> pa.Table:
    """Create the outcell file based on incell and the net_variations computed by suppress.

    :param incell: The input cell data to suppress
    :type incell: pa.Table
    :param net_variation: The computed net_variation values
    :type net_variation: dict
    :param instatus: The original, unmodified status values from incell
    :type instatus: dict
    :return: The outcell Table
    :rtype: pa.Table
    """
    out_status = {
        key: ("X" if val > 0 or instatus[key] in ("S", "X") else "P") for key, val in net_variation.items()
    }

    status_tbl = pa.Table.from_arrays(
        [
            pa.array(list(out_status.keys()), type=pa.float64()),
            pa.array(list(out_status.values()), type=pa.string()),
        ],
        names=["CellId", "OutStatus"],
    )

    netvariation_tbl = pa.Table.from_arrays(
        [
            pa.array(list(net_variation.keys()), type=pa.float64()),
            pa.array(list(net_variation.values()), type=pa.float64()),
        ],
        names=["CellId", "NetVariation"],
    )

    return status_tbl.join(netvariation_tbl, keys="CellId").join(incell, keys="CellId")

def form_outcomplements(complements: list[tuple] | None) -> pa.Table | None:
    """Form the outcomplements dataset if data is given, else return None.

    :param complements: The list of complement tuples to form outcomplements with
    :type complements: list[tuple] | None
    :return: The PyArrow Table equivalent of the complements list or None if complements is None
    :rtype: pa.Table | None
    """
    if complements is None:
        return None

    # Unzip the list of tuples into column arrays
    phases, protection_numbers, cell_ids, complement_ids = zip(*complements, strict=True)
    return pa.Table.from_arrays(
        [
            pa.array(phases, type=pa.int64()),
            pa.array(protection_numbers, type=pa.int64()),
            pa.array(cell_ids, type=pa.float64()),
            pa.array(complement_ids, type=pa.float64()),
        ],
        names=["Phase", "ProtectionNumber", "CellId", "ComplementId"],
    )

def _standardize_cost_function(function_name: str) -> str:
    """Standardize/validation the name of the cost function.

    Note that we no longer support short forms (for example INF instead of information).
    New cost functions have been added in version 2 of G-Confid.

    :param function_name: The function name to standardize
    :type function_name: str
    :raises ValueError: if the given `function_name` is not valid
    :return: `function_name` in its standard form
    :rtype: str
    """
    if function_name is None:
        return function_name

    std_function_name = function_name.upper()
    cost_function_list = ["DIGITS", "INFORMATION", "SCALEDINFORMATION", "CONSTANT", "SIZE",
                          "ROUNDEDSIZE", "INVERSE", "SCALEDINVERSE"]

    if std_function_name not in cost_function_list:
        msg = _("The provided cost function is not valid: {}").format(std_function_name)
        raise ValueError(msg)
    return std_function_name

def verify_need_suppress(incell: pa.Table, logger: Logger) -> pa.Table | None:
    """Verify if any cells need suppression.

    :param incell: Input data file to verify
    :type incell: pa.Table
    :param logger: Instance of logger to use to record information during execution
    :type logger: Logger
    :return: None if suppress needs to be run, otherwise the outcell dataframe formatted based on the
        state of incell.
    :rtype: pa.Table | None
    """
    # Total # of records
    num_obs = incell.num_rows
    incell_sens = incell.column("Sensitivity")
    #noqa: ERA001 - Total # of sensitive (i.e. Sensitivity > 0) records
    sens_gt0 = pc.greater(incell_sens, 0)
    num_sensitive = pc.sum(sens_gt0).as_py()
    #noqa: ERA001 - Total # of non-sensitive (i.e. Sensitivity <= 0) records
    num_non_sensitive = pc.sum(pc.less_equal(incell_sens, 0)).as_py()
    #noqa: ERA001 - Total # of sensitive OR Status 'X' or 'P' records
    status_upper = pc.utf8_upper(incell.column("Status"))
    status_in_xp = pc.is_in(status_upper, value_set=pa.array(["X", "P"]))
    num_sens_x_p = pc.sum(pc.or_(sens_gt0, status_in_xp)).as_py()

    outcell = None
    if(num_sensitive == num_obs):
        outcell = incell.set_column(incell.num_columns, "OutStatus", pa.array(["X"] * incell.num_rows))
        msg = _("The input cells data set contains only sensitive cells. No suppression will be performed.")
    elif(num_non_sensitive == num_obs):
        # If none of the loc conditions are met, OutStatus isn't created. So we add it now with null values
        status_upper = pc.utf8_upper(incell.column("Status"))
        # Start with all nulls
        outstatus = pa.nulls(incell.num_rows)
        # Fill "P" where Status ∈ {V, P, S}
        outstatus = pc.if_else(pc.is_in(status_upper, value_set=pa.array(["V", "P", "S"])), pa.scalar("P"), outstatus)
        # Fill "X" where Status == "X"
        outstatus = pc.if_else(pc.equal(status_upper, "X"), pa.scalar("X"), outstatus)
        # Add the column
        outcell = incell.append_column("OutStatus", outstatus)
        msg = _("The input cells data set does not contain any sensitive cells. No suppression will be performed.")
    elif(num_sens_x_p == num_obs):
        # If none of the loc conditions are met, OutStatus isn't created. So we add it now with null values
        status_upper = pc.utf8_upper(incell.column("Status"))
        # Start with all nulls
        outstatus = pa.nulls(incell.num_rows)
        # Fill "P" where Status == "P"
        outstatus = pc.if_else(pc.equal(status_upper, "P"), pa.scalar("P"), outstatus)
        # Fill "X" where Status ∈ {S, V, X}
        outstatus = pc.if_else(pc.is_in(status_upper, value_set=pa.array(["S", "V", "X"])), pa.scalar("X"), outstatus)
        # Add the column
        outcell = incell.append_column("OutStatus", outstatus)
        msg = _("The input cells data set contains only sensitive, suppressed or published cells. "
                "No further suppression will be performed.")

    # If a problem was found, wipe NetVariation and log the issue
    # No throwing exceptions, we are forming a valid output and we just want to skip the actual suppress
    if outcell is not None:
        # Set NetVariation to 0 for all rows
        outcell = outcell.set_column(outcell.num_columns, "NetVariation", pa.array([0] * outcell.num_rows, type=pa.float64()))
        logger.warning(msg)

    return outcell

def validate_inputs(incell: pa.Table, inconstraint: pa.Table, logger: Logger) -> None:
    """Perform additional input data validation that can't be done in the validation step, primarily cross-file validations.

    :param incell: Input data file to validate
    :type incell: pa.Table
    :param inconstraint: Input constraint file to validate
    :type inconstraint: pa.Table
    :param logger: Instance of logger to use to record information during execution
    :type logger: logging.Logger
    """
    incell_cellid = incell.column("CellId")
    incon_cellid = inconstraint.column("CellId")

    # Find CellId in incell that are not in inconstraint
    mask = pc.invert(pc.is_in(incell_cellid, value_set=incon_cellid))
    incell_cellid_not_in_incon = incell.filter(mask)
    if incell_cellid_not_in_incon.num_rows > 0:
        msg = _("Some cells are present in the input cells data set but not in the input constraints data set: \n{}").format(
            incell_cellid_not_in_incon.to_pandas())
        logger.error(msg)
        raise gconfid.exceptions.GConfidInputDatasetValidationError(msg)

    # Find CellId in incell that are not in incell
    mask = pc.invert(pc.is_in(incon_cellid, value_set=incell_cellid))
    incon_cellid_not_in_incell = inconstraint.filter(mask)
    if incon_cellid_not_in_incell.num_rows > 0:
        msg = _("Some cells are present in the input constraints data set but not in the input cells data set: \n{}").format(
            incon_cellid_not_in_incell.to_pandas())
        logger.error(msg)
        raise gconfid.exceptions.GConfidInputDatasetValidationError(msg)

    constraint_ids = inconstraint.column("ConstraintId")
    unique_ids = pc.unique(constraint_ids).to_pylist()

    # When there is only 1 sensitive cell in a constraint and all other cells in the constraint are
    # forced to be published, the sensitive cell cannot be protected. Issue an error message and stop.
    # Iterate over our "groups"
    for cid in unique_ids:
        # Filter inconstraints to get the CellIds we want out of incell (i.e. our "group")
        incon_mask = pc.equal(constraint_ids, cid)
        incon_group = inconstraint.filter(incon_mask)

        # Filter incell rows whose CellId is in this group
        mask_incell = pc.is_in(incell_cellid, value_set=incon_group.column("CellId"))
        incell_group = incell.filter(mask_incell)

        # Count Published ("P") cells
        status_upper = incell_group.column("Status")
        is_p = pc.equal(pc.utf8_upper(status_upper), "P")
        num_p = pc.sum(is_p).as_py()

        # Sensitive cells: Status == "S" OR Sensitivity > 0
        is_s = pc.equal(status_upper, "S")
        sens_gt0 = pc.greater(incell_group.column("Sensitivity"), 0)
        sens_mask = pc.or_(is_s, sens_gt0)
        sens_cell = incell_group.filter(sens_mask)
        num_sens = sens_cell.num_rows

        # Compare counts
        if num_p == incell_group.num_rows - 1 and num_sens == 1:
            cellid = sens_cell.column("CellId")[0].as_py()
            msg = _("A unique sensitive cell detected in a row or column with all other cells forced to be published: "
                    "ConstraintId = {}, CellId = {}" ) .format(cid, cellid)
            logger.error(msg)
            raise gconfid.exceptions.GConfidInputDatasetValidationError(msg)

def generate_suppress_report(outcell: pa.Table, by_group: str | None = None, phase: int | None = None) -> str:
    """Generate and return the formatted report of the results for a run of suppress.

    :param outcell: The output cell table to base the report on
    :type outcell: pa.Table
    :param by_group: The by-group for which this report applies to, defaults to None
    :type by_group: str | None, optional
    :param phase: Determines the correct title to print based on the phase the report is generated for, defaults to None
    :type phase: int | None, optional
    :return: The report formatted as a table to be output to the log
    :rtype: str
    """
    outcell_netvar = outcell.column("NetVariation")
    outcell_sens = outcell.column("Sensitivity")
    type_upper = pc.utf8_upper(outcell.column("Type"))
    status_upper = pc.utf8_upper(outcell.column("Status"))

    # Published cells
    mask_net_le0 = pc.less_equal(outcell_netvar, 0)
    mask_status_xs = pc.is_in(status_upper, value_set=pa.array(["X", "S"]))
    agr_pub = outcell.filter(pc.and_(mask_net_le0, pc.invert(mask_status_xs)))
    agr_pub_n = pc.count(agr_pub.column("TotalNoise")).as_py()
    agr_pub_total = pc.sum(agr_pub.column("TotalNoise")).as_py()
    agr_pub_total = 0 if agr_pub_total is None else agr_pub_total

    # Suppressed by user cells
    mask_status_x = pc.equal(status_upper, "X")
    mask_sens_le0 = pc.less_equal(outcell_sens, 0)
    mask_type_c = pc.equal(type_upper, "C")
    agr_sup_by_user_mask = pc.and_(pc.and_(mask_status_x, mask_sens_le0), mask_type_c)
    agr_sup_by_user = outcell.filter(agr_sup_by_user_mask)
    agr_sup_by_user_n = pc.count(agr_sup_by_user.column("TotalNoise")).as_py()
    agr_sup_by_user_total = pc.sum(agr_sup_by_user.column("TotalNoise")).as_py()
    agr_sup_by_user_total = 0 if agr_sup_by_user_total is None else agr_sup_by_user_total

    # Complements cells
    mask_net_gt0 = pc.greater(outcell_netvar, 0)
    mask_status_notx = pc.not_equal(status_upper, "X")
    agr_comp_mask = pc.and_(pc.and_(pc.and_(mask_net_gt0, mask_sens_le0), mask_type_c), mask_status_notx)
    agr_comp = outcell.filter(agr_comp_mask)
    agr_comp_n = pc.count(agr_comp.column("TotalNoise")).as_py()
    agr_comp_total = pc.sum(agr_comp.column("TotalNoise")).as_py()
    agr_comp_total = 0 if agr_comp_total is None else agr_comp_total

    # Sensitive cells
    mask_sens_gt0 = pc.greater(outcell_sens, 0)
    sens_cells_mask = pc.and_(pc.and_(pc.or_(mask_net_gt0, mask_status_xs), mask_sens_gt0), mask_type_c)
    sens_cells = outcell.filter(sens_cells_mask)
    sens_cells_n = pc.count(sens_cells.column("TotalNoise")).as_py()
    sens_cells_total = pc.sum(sens_cells.column("TotalNoise")).as_py()
    sens_cells_total = 0 if sens_cells_total is None else sens_cells_total

    # Sensitive aggregates
    mask_type_a = pc.equal(type_upper, "A")
    agr_sens = outcell.filter(mask_type_a)
    agr_sens_n = pc.count(agr_sens.column("TotalNoise")).as_py()
    agr_sens_total = pc.sum(agr_sens.column("TotalNoise")).as_py()
    agr_sens_total = 0 if agr_sens_total is None else agr_sens_total

    # Total of suppress cells
    agr_tot_sup = outcell.filter(pc.or_(pc.or_(sens_cells_mask, agr_comp_mask), agr_sup_by_user_mask))
    agr_tot_sup_n = pc.count(agr_tot_sup.column("TotalNoise")).as_py()
    agr_tot_sup_total = pc.sum(agr_tot_sup.column("TotalNoise")).as_py()
    agr_tot_sup_total = 0 if agr_tot_sup_total is None else agr_tot_sup_total

    # Total cells
    agr_tot = outcell.filter(mask_type_c)
    agr_tot_n = pc.count(agr_tot.column("TotalNoise")).as_py()
    agr_tot_total = pc.sum(agr_tot.column("TotalNoise")).as_py()
    agr_tot_total = 0 if agr_tot_total is None else agr_tot_total

    # If no cells are found for a particular condition, the _n and _total vars will contain 0

    # Build report
    report = "\n" + "*"*100
    report += "\n"
    if phase is None:
        report += _("Summary of the entire suppression process:")
    elif phase == 1:
        report += _("Summary of the suppression process phase 1:")
    elif phase == 2: #noqa: PLR2004
        report += _("Summary of the suppression process phase 2:")
    else:
        report += _("Summary of the entire suppression process (after phase 2):")

    report += "\n"
    if by_group and phase in (1, 2):
        report += "By Group: " + by_group + "\n"

    report += "                             " + "{:<10}".format(_("Number")) + "  " + "{:<10}".format(_("Value")) + "  "
    report += _("Percent of total number of cells\n")
    report +=  f"{'='*27}  {'='*10}  {'='*10}  {'='*32}\n"

    report += "{:<27}".format(_("All suppressed cells")) + f"  {agr_tot_sup_n:<10.2f}  {agr_tot_sup_total:<10.2f}  {agr_tot_sup_n/agr_tot_n*100:<10.2f}\n"
    report += "{:<27}".format(_("Suppressed sensitive cells")) + f"  {sens_cells_n:<10.2f}  {sens_cells_total:<10.2f}  {sens_cells_n/agr_tot_n*100:<10.2f}\n"
    report += "{:<27}".format(_("Suppressed complements")) + f"  {agr_comp_n:<10.2f}  {agr_comp_total:<10.2f}  {agr_comp_n/agr_tot_n*100:<10.2f}\n"
    report += "{:<27}".format(_("Cells suppressed by user"))
    report += f"  {agr_sup_by_user_n:<10.2f}  {agr_sup_by_user_total:<10.2f}  {agr_sup_by_user_n/agr_tot_n*100:<10.2f}\n"
    report += "{:<27}".format(_("Suppressed aggregates")) + f"  {agr_sens_n:<10.2f}  {agr_sens_total:<10.2f}  {agr_sens_n/agr_tot_n*100:<10.2f}\n"
    report += "{:<27}".format(_("Published cells")) + f"  {agr_pub_n:<10.2f}  {agr_pub_total:<10.2f}  {agr_pub_n/agr_tot_n*100:<10.2f}\n"

    report += "*"*100
    report += "\n"
    return report

def _suppress(incell: pa.Table, inconstraint: pa.Table, outcomplement: bool = False, cost_function1: str = "Size",
              cost_function2: str | None = None, cost_var1: str | None = None, cost_var2: str | None = None, scale_cost: str | None = None,
              size_roundingbase: int | None = None, sen_roundingbase: int | None = None, total_roundingbase: int | None = None,
              constraint_scale: int = 2, suppress_order: int = 0, ambiguity_tolerance: float = 0.00001,
              by: str | None = None, multiprocess: bool = False, skip_validation: bool = False, logger: Logger | None = None,
              ) -> tuple[SuppressReturnCode, pa.Table, pa.Table | None]:
    """Create a suppression pattern to protect sensitive cells.

    Identifies cells to be suppressed in a table, in addition to the sensitive cells, in order to prevent
    confidential data disclosure. Aggregated data is processed through a linear programming solver
    using the constraints generated by the SENSITIVITY procedure.

    :param incell: Dataset with the following columns (CellID, Total, TotalNoise, Sensitivity, Status, Type, and any by-variables).
    :type incell: pa.Table
    :param inconstraint: dataset with the following columns (ConstraintID, CellId, Coefficient and any by-variables).
    :type inconstraint: pa.Table
    :param outcomplement: Should an outcomplement dataset be generated?
    :type outcomplement: bool
    :param cost_function1: Specifies the name of the cost function to be used in phase 1.
        Possible values are DIGITS, INFORMATION, SCALEDINFORMATION, CONSTANT, SIZE,
        ROUNDEDSIZE, INVERSE, SCALEDINVERSE. Defaults to "SIZE"
    :type cost_function1: str, optional
    :param cost_function2: Specifies the name of the cost function to be used in phase 2.
        Possible values are DIGITS, INFORMATION, SCALEDINFORMATION, CONSTANT, SIZE,
        ROUNDEDSIZE, INVERSE, SCALEDINVERSE. Defaults to "SIZE", defaults to None
    :type cost_function2: str | None, optional
    :param cost_var1: Specifies the name of the cost variable to be used in phase 1. It must be a numeric variable
        present in the incell data set. This variable must have only positive values, and missing values are not.
        allowed. When this parameter is provided, the variable specified is used to calculate the cost function.
        coefficients, instead of TOTAL, defaults to None
    :type cost_var1: str | None, optional
    :param cost_var2: Specifies the name of the cost variable to be used in phase 2. It must be a numeric variable
        present in the incell data set. This variable must have only positive values, and missing values are not.
        allowed. When this parameter is provided, the variable specified is used to calculate the cost function.
        coefficients, instead of TOTAL, defaults to None, defaults to None
    :type cost_var2: str | None, optional
    :param scale_cost: Specifies the method used to reduce the cost function coefficients. Possible values are NONE,
        MEAN and SCALE, defaults to None.
    :type scale_cost: str | None, optional
    :param size_roundingbase: Rounding base used for the cost function ROUNDEDSIZE, defaults to None
    :type size_roundingbase: int, optional
    :param sen_roundingbase: Rounding base used to round the sensitivity value, defaults to None
    :type sen_roundingbase: int, optional
    :param total_roundingbase: Rounding base used to round the total value, defaults to None
    :type total_roundingbase: int, optional
    :param constraint_scale: Used for scaling various values, defaults to 2
    :type constraint_scale: int, optional
    :param suppress_order: How to choose the next cell, options include 0=Highest Ambiguity Changed, 1=Highest Ambiguity, 2=Highest Sensitivity, defaults to 0
    :type suppress_order: int, optional
    :param ambiguity_tolerance: Tolerance when considering ambiguity values equal, defaults to 0.00001
    :type ambiguity_tolerance: float, optional
    :param by: Specifies one or more variable names used to create BY groups. The process will be applied to each BY group separately.
        If no by-variables are specified, the process will be applied to the entire input data set., defaults to None
    :type by: str, optional
    :param multiprocess: Enables the use of multiple processors to distribute solver work, defaults to False
    :type multiprocess: bool, optional
    :param skip_validation: Should the validation of input files be skipped?, defaults to False
    :type skip_validation: bool, optional
    :param logger: Instance of logger to use to record information during execution
    :type Logger: Logging.logger, An instance of the Logger class likely created by a higher-level structure, defaults to None

    :return: One or two of four datasets, OutSuppress and OutComplement or OutSuppress_failed and OutComplement_failed

        For OutSuppress, in addition to the fields contained in the cell dataset, this file contains the following two new fields:
        1) OUTSTATUS: Indicates the status of the cell, as determined by the macro. Valid values are 'P' for published
                cells or 'X' for suppressed cell.
        2) NETVARIATION: Is the net variation in absolute value of the cell, required to protect sensitive cells, as
                calculated by the macro. A sensitive cell always has a non-zero net variation. A non-zero net
                variation for a non-sensitive cell indicates that this cell has been selected as a complement to a sensitive
                cell. A published cell has a zero net variation.

        For Outcomplement, the dataset is only created if the outcomplement parameter requests it. If not None is returned.
    :rtype: tuple[SuppressReturnCode, pa.Table, pa.Table | None]
    """
    start_time = gconfid.get_time()

    if(not logger):
        logger = gconfid.logging.init_proc_level("Suppress", trace_level=gconfid.log_level.DEBUG)
    logger.info(gconfid.get_execution_header())

    cost_function1 = _standardize_cost_function(cost_function1)
    cost_function2 = _standardize_cost_function(cost_function2)

    if(not skip_validation):
        validate_inputs(incell, inconstraint, logger)

    out_complement = pa.Table.from_arrays([])

    if by is not None:
        out_tbl = pa.Table.from_arrays([])
        by_list = by.split()
        # Dropping dupes via conversion to pandas is faster than the below pure-PyArrow method for anything less than 1m records,
        # however pandas .iterrows() is way slower than batching with pyarrow for larger datasets. So combine them.
        # We need to make sure we drop the induced index from the conversion, otherwise it will be included in the by_group processing
        by_group_pat = pa.Table.from_pandas(incell.select(by_list).to_pandas(split_blocks = True).drop_duplicates().reset_index(drop=True))
        for batch in by_group_pat.to_batches():
            d = batch.to_pydict()
            for items in zip(*d.values(), strict=True):
                by_group = dict(zip(d.keys(), items, strict=True))
                by_start_time = gconfid.get_time()
                formated_by_group_info = ", ".join([f"{index}={value}" for index, value in by_group.items()])
                logger.info("="*100)
                logger.info(_("Processing by Group: '{}'").format(formated_by_group_info))
                logger.info("="*100)

                # Form the mask by combining equality checks on by_group fields
                mask_incell = None
                mask_incon = None
                for col, val in by_group.items():
                    expr_incell = pc.equal(incell[col], pa.scalar(val))
                    mask_incell = expr_incell if mask_incell is None else pc.and_(mask_incell, expr_incell)
                    expr_incon = pc.equal(inconstraint[col], pa.scalar(val))
                    mask_incon = expr_incon if mask_incon is None else pc.and_(mask_incon, expr_incon)
                cell_by = incell.filter(mask_incell)
                con_by = inconstraint.filter(mask_incon)

                out_by = verify_need_suppress(cell_by, logger)
                out_complement_by = pa.Table.from_arrays([])
                if(out_by is None):
                    ret_code, out_by, out_complement_by = _suppress_main(cell_by, con_by, logger, cost_function1, cost_function2, cost_var1, cost_var2,
                                                                         scale_cost, size_roundingbase, sen_roundingbase, total_roundingbase,
                                                                         constraint_scale, suppress_order, ambiguity_tolerance, outcomplement,
                                                                         formated_by_group_info, multiprocess=multiprocess)
                    if ret_code is SuppressReturnCode.SOLVER_FAIL:
                        logger.error(_("Error occurred during solver execution for by_group: '{}'.").format(formated_by_group_info))
                        # The solver failed during an iteration, form outputs and return immediately
                        return (ret_code, pa.concat_tables([out_tbl, out_by]),
                                pa.concat_tables([out_complement, out_complement_by]) if out_complement_by else out_complement)
                    if out_by.num_rows > 0 and cost_function2:
                        logger.info(generate_suppress_report(out_by, formated_by_group_info, 2))

                end_time = gconfid.get_time()
                duration_stamp = str(end_time - by_start_time)[:-4]

                type_upper = pc.utf8_upper(out_by.column("Type"))
                # We form OutStatus column so we shouldn't need to upper case
                mask_outstatus_x = pc.equal(out_by.column("OutStatus"), "X")
                mask_type_c = pc.equal(type_upper, "C")
                out_by_filtered = out_by.filter(pc.and_(mask_outstatus_x, mask_type_c))

                sup_total_val = pc.sum(out_by_filtered.column("TotalNoise")).as_py()
                sup_total_val = 0 if sup_total_val is None else sup_total_val
                logger.info(_("Number of cell suppressed in by group: {}").format(out_by_filtered.num_rows))
                logger.info(_("Value amount suppressed in by group: {}").format(sup_total_val))
                logger.info(_("Duration: {}").format(duration_stamp))

                if out_tbl.num_rows == 0 and out_tbl.num_columns == 0:
                    out_tbl = out_by
                else:
                    out_tbl = pa.concat_tables([out_tbl, out_by])

                if out_complement_by is not None:
                    if out_complement.num_rows == 0 and out_complement.num_columns == 0:
                        out_complement = out_complement_by
                    else:
                        out_complement = pa.concat_tables([out_complement, out_complement_by])

            if out_tbl.num_rows > 0:
                logger.info(generate_suppress_report(out_tbl))
    else:
        # Verify if the incell data needs suppression
        out_tbl = verify_need_suppress(incell, logger)
        if(out_tbl is None):
            ret_code, out_tbl, out_complement = _suppress_main(incell, inconstraint, logger, cost_function1, cost_function2, cost_var1, cost_var2,
                                                    scale_cost, size_roundingbase, sen_roundingbase, total_roundingbase, constraint_scale,
                                                    suppress_order, ambiguity_tolerance, outcomplement, multiprocess=multiprocess)
            if ret_code is SuppressReturnCode.SOLVER_FAIL:
                logger.error(_("Error occurred during solver execution."))
                # The solver failed during an iteration, form outputs and return immediately
                return (ret_code, out_tbl, out_complement)
        if out_tbl.num_rows > 0:
            if cost_function2:
                # Use phase=0 to represent a quartenary condition over 1,2 and None
                logger.info(generate_suppress_report(out_tbl, phase=0))
            else:
                logger.info(generate_suppress_report(out_tbl))

    end_time = gconfid.get_time()
    duration_stamp = str(end_time - start_time)[:-4]

    logger.info("="*100)

    type_upper = pc.utf8_upper(out_tbl.column("Type"))
    # We form OutStatus column so we shouldn't need to upper case
    mask_outstatus_x = pc.equal(out_tbl.column("OutStatus"), "X")
    mask_type_c = pc.equal(type_upper, "C")
    out_df_filtered = out_tbl.filter(pc.and_(mask_outstatus_x, mask_type_c))

    sup_total_val = pc.sum(out_df_filtered.column("TotalNoise")).as_py()
    sup_total_val = 0 if sup_total_val is None else sup_total_val
    logger.info(_("Total number of cell suppressed: {}").format(out_df_filtered.num_rows))
    logger.info(_("Total value amount suppressed: {}").format(sup_total_val))
    logger.info(_("Total duration: {}").format(duration_stamp))
    logger.info(gconfid.get_execution_footer("suppress"))

    # still can't decide if outcomplement should be returned as empty or None if not requested
    return (SuppressReturnCode.SUCCESS, out_tbl, out_complement)
