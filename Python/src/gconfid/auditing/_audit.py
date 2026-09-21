"""G-Confid audit module."""

import heapq
import multiprocessing
import os
import uuid
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor, as_completed
from enum import Enum
from logging import Logger
from pathlib import Path

import pulp
import pyarrow as pa
import pyarrow.compute as pc

import gconfid
from gconfid.nls import SupportedLanguage, _
from gconfid.util.solver_utils import _MULTIPROCESS_LOG_DIR, clone_solver, combine_solver_logs, get_log_path


class ProtectionLevel(Enum):
    """Protection level code set."""

    PROTECTED = 0
    UNACHIEVED = 1
    EXACT_DISCLOSURE = 2

_global_state = {}

def _audit(incell: pa.Table,
           inconstraint: pa.Table,
           lb_factor: float = 0.5,
           ub_factor: float = 1.5,
           report_level: int = 0,
           use_shuttle: bool = False,
           by: str | None = None,
           multiprocess: bool = False,
           logger: Logger | None = None) -> pa.Table:
    """Entry-point to verify the validity of a suppression pattern.

    :param incell: The input data set containing the cell information.
    :type incell: pa.Table
    :param inconstraint: The input data set containing the linear constraints coefficients.
    :type inconstraint: pa.Table
    :param lb_factor: Lower bound factor, between 0 and 1 inclusively, defaults to 0.5
    :type lb_factor: float, optional
    :param ub_factor: Upper bound factor, between 1 and 10 inclusively, defaults to 1.5
    :type ub_factor: float, optional
    :param report_level: If anything but 0 the report is logged, otherwise it is skipped, defaults to 0
    :type report_level: int, optional
    :param use_shuttle: Whether the Shuttle algorithm will be used or not, defaults to False
    :type use_shuttle: bool, optional
    :param by: List of BY variables, separated by a space, used to create processing groups, defaults to None
    :type by: str | None, optional
    :param multiprocess: Enables the use of multiple processors to distribute solver work, defaults to False
    :type multiprocess: bool, optional
    :param logger: The logger instance to log information to
    :type logger: logging.Logger, defaults to None
    :return: The OutAudit dataset
    :rtype: pa.Table
    """
    if(not logger):
        logger = gconfid.logging.init_proc_level("Audit", trace_level=gconfid.log_level.DEBUG)
    logger.info(gconfid.get_execution_header())

    logger.debug(_("lb_factor .......................=   {}").format(lb_factor))
    logger.debug(_("ub_factor .......................=   {}").format(ub_factor))
    logger.debug(_("report_level ....................=   {}").format(report_level))

    if by is not None:
        logger.info(_("by ...............................=   {}").format(by))
        outaudit = pa.Table.from_arrays([])
        by_list = by.split()
        # Dropping dupes via conversion to pandas is faster than the below pure-PyArrow method for anything less than 1m records,
        # however pandas .iterrows() is way slower than batching with pyarrow for larger datasets. So combine them.
        # by_group_df = incell.select(by_list).group_by(by_list).aggregate([]) #noqa: ERA001 - illustrate alternate methods for the future
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

                out_by = _audit_main(cell_by, con_by, logger,
                                     lb_factor=lb_factor, ub_factor=ub_factor,
                                     use_shuttle=use_shuttle, multiprocess=multiprocess)

                end_time = gconfid.get_time()
                duration_stamp = str(end_time - by_start_time)[:-4]
                if outaudit.num_rows == 0 and outaudit.num_columns == 0:
                    outaudit = out_by
                else:
                    outaudit = pa.concat_tables([outaudit, out_by])
                logger.info(_("Duration: {}").format(duration_stamp))
    else:
        outaudit = _audit_main(incell, inconstraint, logger,
                               lb_factor=lb_factor, ub_factor=ub_factor,
                               use_shuttle=use_shuttle, multiprocess=multiprocess)

    if(report_level != 0):
        _log_audit_info(outaudit, logger)

    logger.info(gconfid.get_execution_footer("audit"))

    return outaudit

def verify_need_audit(incell: pa.Table, logger: Logger) -> pa.Table | None:
    """Verify if the input dataset actually needs to have an audit performed.

    :param incell: The input dataset containing the cell information
    :type incell: pa.Table
    :param logger: The logger instance to log information to
    :type logger: logging.Logger
    """
    num_cells = incell.num_rows
    num_suppressed = pc.sum(pc.equal(pc.utf8_upper(incell["OutStatus"]), "X")).as_py()
    num_publishable = pc.sum(pc.equal(pc.utf8_upper(incell["OutStatus"]), "P")).as_py()

    if num_suppressed == num_cells:
        logger.warning(_("All information is suppressed, no validation is required."))
    elif num_publishable == num_cells:
        logger.warning(_("All information is publishable, no validation is required."))
    else:
        # We need to run audit
        return None

    # Form a valid output since we'll be skipping the Audit procedure
    # Technically the SAS does not produce an outcell if all cells are publishable, but the
    # documentation of the function suggests they should, so we will here
    outcell = incell.set_column(
        incell.column_names.index("Sensitivity"),
        "MaxMinusMin", #rename
        pc.add(incell["Sensitivity"], 1),
    )

    new_cols = ["LBound", "LTolerance", "MinValue", "MidPoint", "UTolerance", "MaxValue", "UBound", "IntervalWidth"]
    nan_col = pa.array([float("nan")]*num_cells, type=pa.float64())
    for col in new_cols:
        outcell = outcell.append_column(col, nan_col)

    return outcell

def _audit_main(incell: pa.Table, inconstraint: pa.Table, logger: Logger, lb_factor: float = 0.5,
                ub_factor: float = 1.5, use_shuttle: bool = False, multiprocess: bool = False) -> pa.Table:
    """Perform the actual work of verifying the validity of a suppression pattern."""
    need_audit_outcell = verify_need_audit(incell, logger)
    if need_audit_outcell is not None:
        return need_audit_outcell

    if use_shuttle:
        # SAS refers to reltol, abstol and max_iter as "hidden" so it's assumed they aren't meant to be available for
        # the user to configure via the wrapper interface. However in practice they don't seem to be any different from
        # all the other variables so this could be changed in future.
        incell = _shuttle(incell=incell, inconstraint=inconstraint, lb_factor=lb_factor, ub_factor=ub_factor,
                          rel_tolerance=0.0000000001, abs_tolerance=0.0000000001, logger=logger, max_iter=1000)

    cellid_list = incell["CellId"].to_pylist()

    # Create these dicts here rather than having to do it once per-process
    # Upper case status values for comparisons
    outstatus_upper = pc.utf8_upper(incell["OutStatus"])
    outstatus_dict = dict(zip(cellid_list, outstatus_upper.to_pylist(), strict=True))
    total_noise_dict = dict(zip(cellid_list, incell["TotalNoise"].to_pylist(), strict=True))

    # Active cells in this context have an OutStatus of X
    active_cell_ids = set(incell.filter(pc.equal(outstatus_upper, "X"))["CellId"].to_pylist())

    # Use existence of columns instead of use_shuttle, in case no cols were created
    if("TotalLB" in incell.column_names):
        l_bound = dict(zip(cellid_list, incell["TotalLB"].to_pylist(), strict=True))
        u_bound = dict(zip(cellid_list, incell["TotalUB"].to_pylist(), strict=True))
    else:
        l_bound = {}
        u_bound = {}

    con_cellids = inconstraint["CellId"]
    con_ids = inconstraint["ConstraintId"]
    con_coefs = inconstraint["Coefficient"]
    coef = {} # ConstraintID : [(CellID, Coefficient)]
    for i in range(len(con_ids)):
        curr_cellid = con_cellids[i].as_py()
        # Make sure published cells have their coefficients set to 0
        curr_coef = con_coefs[i].as_py() if outstatus_dict[curr_cellid] != "P" else 0
        curr_conid = con_ids[i].as_py()

        if curr_conid not in coef:
            coef[curr_conid] = [[curr_cellid, curr_coef]]
        else:
            coef[curr_conid].append([curr_cellid, curr_coef])

    # We use 1 less core than available for safety
    num_workers = multiprocessing.cpu_count() - 1 if multiprocess else 1

    # Get and distribute the binary sets and non-binary cells amongst the number of available CPUs
    binary_sets_to_cells = get_binary_cells(inconstraint, active_cell_ids, logger)
    non_binary_cells = set(active_cell_ids) - set().union(*binary_sets_to_cells.values()) if binary_sets_to_cells else set()

    # Sensitivity should not be greater than total
    min_arr = pc.min_element_wise(incell["Sensitivity"].combine_chunks(), incell["TotalNoise"].combine_chunks())
    sensitivity_dict = dict(zip(incell["CellId"].to_pylist(), min_arr.to_pylist(), strict=True))

    results = []
    if num_workers == 1:
        # Run the function in a single process
        if non_binary_cells:
            binary_sets_to_cells[-1] = non_binary_cells
        setup_solver(incell, active_cell_ids, total_noise_dict, l_bound, u_bound,
                     lb_factor, ub_factor, coef)
        # Since we're single-process and pass a logger in, _log_msgs is not needed
        result, _log_msgs = solve_cell_sets(incell, binary_sets_to_cells, total_noise_dict, sensitivity_dict,
                                          lb_factor, ub_factor, l_bound, u_bound, logger=logger)
        results.append(result)
    else:
        batches = allocate_cellids(binary_sets_to_cells, num_workers, list(non_binary_cells))
        # Sharing LPProblems and LPVariables between processes is more complex and error-prone, so
        # just have each process build its own LP components
        with ProcessPoolExecutor(max_workers=num_workers, initializer=setup_solver,
                                 initargs=(incell, active_cell_ids, total_noise_dict, l_bound, u_bound,
                                           lb_factor, ub_factor, coef, pulp.LpSolverDefault, gconfid.nls.get_language()),
                                 ) as executor:
            # Each process should receive a dict containing an equal amount of binary sets
            futures = [executor.submit(solve_cell_sets, incell, biset_batch, total_noise_dict,
                                       sensitivity_dict, lb_factor, ub_factor, l_bound, u_bound)
                       for biset_batch in batches]

            for future in as_completed(futures):
                # Extend the outaudit list with the resulting tables
                results.append(future.result()[0])
                logger.info(_("Sub-process completed - printing logs:"))
                # Add the returned messages to the log
                logger.info(future.result()[1])

        # If there is a log path set on the base solver (that was cloned once per-process)
        # we need to combine the log files they created into a single one
        combine_solver_logs(pulp.LpSolverDefault)

    # combine outaudits from processes
    outaudit = pa.concat_tables(results)

    # If shuttle was used, clear the columns off prior to returning the result
    if("TotalLB" in incell.column_names):
        drop_cols = ["TotalLB","TotalUB"]
        # Drop from incell too, just in case
        incell = incell.drop_columns(drop_cols)
        outaudit = outaudit.drop_columns(drop_cols)

    return outaudit

def setup_solver(incell: pa.Table, active_cell_ids: list[float], total_noise_dict: dict[float, float], l_bound: dict[float, float], u_bound: dict[float, float],
                 lb_factor: float, ub_factor: float, coef: dict[float, tuple[float, int]], base_solver: pulp.LpSolver | None = None,
                 lang: SupportedLanguage | None = None) -> None:
    """Initialize the _global_state with the LP components necessary to solve a set of cells.

    :param incell: The input cell data
    :type incell: pa.Table
    :param active_cell_ids: The list of all cellids that are currently active and therefore part of constraints in the system
    :type active_cell_ids: list[float]
    :param total_noise_dict: Mapping of cellids to their respective TotalNoise values
    :type total_noise_dict: dict[float, float]
    :param l_bound: Mapping of cellids to their respective LowBound values
    :type l_bound: dict[float, float]
    :param u_bound: Mapping of cellids to their respective UpBound values
    :type u_bound: dict[float, float]
    :param lb_factor: The constant factor used to compute lower bounds
    :type lb_factor: float
    :param ub_factor: The constant factor used to compute upper bounds
    :type ub_factor: float
    :param coef: Mapping of cellids to their respective coefficient information from the constraints table
    :type coef: dict[float, tuple[float, int]]
    :param base_solver: If None, the global solver will be the same instance as the current pulp.LpSolverDefault.
        Otherwise a new instance of base_solver is created with the same parameters and options set, except for logPath
        or logfile which will receive a new value based on the current process pid.
    :type base_solver: pulp.LpSolver | None
    :param lang: The language of the parent process if calling this in a newly spawned process, defaults to None
    :type lang: gconfid.nls.SupportedLanguage | None
    """
    # Make sure we copy the language settings from the parent process as this value is overwritten at this point
    if lang is not None:
        # Make sure we don't also call _reload_all_procs as we know we won't need those here so it's just a waste of cycles
        gconfid.set_language(lang, reload_procs=False)

    min_prob = pulp.LpProblem("MinProb", pulp.LpMinimize)
    max_prob = pulp.LpProblem("MaxProb", pulp.LpMaximize)
    min_x: dict[float, pulp.LpVariable] = {}
    max_x: dict[float, pulp.LpVariable] = {}

    # Generate LpVariable dic
    for i in active_cell_ids:
        total_noise = total_noise_dict[i]

        # Suppressed constraint.
        if("TotalLB" in incell.column_names):
            low_bound = l_bound[i] - total_noise
            up_bound = u_bound[i] - total_noise
            lb_factor_val = low_bound / total_noise + 1
            ub_factor_val = up_bound / total_noise + 1
        else:
            low_bound = total_noise * (lb_factor - 1)
            up_bound = total_noise * (ub_factor - 1)
            lb_factor_val = lb_factor
            ub_factor_val = ub_factor

        min_x[i] = pulp.LpVariable(name="minx" + str(i), lowBound=low_bound, upBound=up_bound)
        # If bounds are symmetric, we don't need to solve for both max AND min,
        # just one and then use it to derive the other
        if ub_factor_val + lb_factor_val == 2: #noqa: PLR2004 - nature of this relationship is unchanging
            max_x[i] = None
        else:
            max_x[i] = pulp.LpVariable(name="maxx" + str(i), lowBound=low_bound, upBound=up_bound)

    # Additive constraint
    for conid, cellid_coef_tuples in coef.items():
        min_con_list = []
        max_con_list = []
        # Iterate over each (cellid, coef) tuple in the list
        for index in range(len(cellid_coef_tuples)):
            cellid = cellid_coef_tuples[index][0]
            coefficent = cellid_coef_tuples[index][1]

            # If coefficient == 0 the cell is Published and should not be added as a constraint
            if coefficent != 0:
                min_con_list.append(coefficent * min_x[cellid])
                # If we had symmetric bounds earlier for this cellid then skip adding this
                # constraint as we will be skipping the problem solving
                if max_x[cellid] is not None:
                    max_con_list.append(coefficent * max_x[cellid])
        min_prob += pulp.LpConstraint(pulp.lpSum(min_con_list), 0, "conu" + str(conid), 0)
        max_prob += pulp.LpConstraint(pulp.lpSum(max_con_list), 0, "conu" + str(conid), 0)

    # In order to support multi-processing we need to recreate a solver on each process
    # rather than share the same instance of the default.
    # When a new child process is spawned in Windows, the entire import chain is re-imported, thus the pulp.LpSolverDefault
    # is reset, wiping the custom_solver we set earlier. Therefore it must be passed in (base_solver) to retain the reference we use to create a new instance.
    _global_state["solver"] = pulp.LpSolverDefault if base_solver is None else clone_solver(base_solver, unique_log_suffix=str(os.getpid()))
    _global_state["min_prob"] = min_prob
    _global_state["max_prob"] = max_prob
    _global_state["min_x"] = min_x
    _global_state["max_x"] = max_x

def solve_cell_sets(incell: pa.Table, setid_to_cellids: dict[int, set[float]], total_noise_dict: dict[float, float], sensitivity_dict: dict[float, float],
                    lb_factor: float, ub_factor: float, l_bound: dict[float, float], u_bound: dict[float, float], tolerance: float = 1e-5,
                    logger: Logger | None = None) -> tuple[pa.Table, str]:
    """Process the cellIDs in `setid_to_cellids` and return a list of the tables containing their solved max/min values.

    :param incell: The input cell data
    :type incell: pa.Table
    :param setid_to_cellids: A mapping of arbitrary setids to the list of cellids in that set. If the setid is -1 it is
        not a binary set otherwise it is a binary set.
    :type setid_to_cellids: dict[int, set[float]]
    :param total_noise_dict: Map of incell cellids to its TotalNoise field
    :type total_noise_dict: dict[float, float]
    :param sensitivity_dict: Map of incell cellids to its Sensitivity field
    :type sensitivity_dict: dict[float, float]
    :param lb_factor: Lower bound factor, between 0 and 1 inclusively, defaults to 0.5
    :type lb_factor: float, optional
    :param ub_factor: Upper bound factor, between 1 and 10 inclusively, defaults to 1.5
    :type ub_factor: float, optional
    :param l_bound: Map of cellIDs to the cell's lower bound
    :type l_bound: dict[float, float]
    :param u_bound: Map of cellIDs to the cell's upper bound
    :type u_bound: dict[float, float]
    :param tolerance: Tolerance for determining protection levels based on solved max and min values, defaults to 1e-5
    :type tolerance: float, optional
    :param logger: The logger to log messages to if running in a single process. Passing a normal logger
        in if calling this function in a concurrent manner will result in unexpected behaviour, defaults to None
    :type logger: Logger | None, optional
    :return: The outaudit table with an entry for each key in `setid_to_cellids` and the log messages recorded
        during execution, if logger is None (for safety if running multiple processes).
    :rtype: tuple[pa.Table, str]
    """
    log_msg = ""

    start_time = gconfid.get_time()

    # If we're in multiprocess execution these should be unique per-process
    solver: pulp.LpSolver = _global_state["solver"]
    min_prob: pulp.LpProblem = _global_state["min_prob"]
    max_prob: pulp.LpProblem = _global_state["max_prob"]
    min_x: dict[float, pulp.LpVariable] = _global_state["min_x"]
    max_x: dict[float, pulp.LpVariable] = _global_state["max_x"]

    max_values = {}
    min_values = {}
    l_tolerance = {}
    u_tolerance = {}
    max_minus_min = {}
    mid_point = {}
    interval_width = {}
    problem_indicator = {}
    processed_binary_sets = []

    total_cells_to_solve = sum(len(v) for v in setid_to_cellids.values())
    solved_cells = set()
    def set_value(cellid, val, maxx = False, minx = False):
        if minx == maxx:
            raise ValueError

        if maxx:
            max_values[cellid] = val
            if cellid in min_values:
                solved_cells.add(cellid)
        elif minx:
            min_values[cellid] = val
            if cellid in max_values:
                solved_cells.add(cellid)

    # Helper function to check fellow binary cells in a set after computing one cell's max/min value
    def check_sibling(sib_val: float, sibling_id: float):
        """Process a fellow member of a binary set after a completed solve to quickly derive max/min value.

        For all elements of the objective cell's binary set for which that cell has a computed max/min:
          if the sibling's computed val is (-) and it does not have a min yet then that cell's min can be set.
          if val is (+) and it does not have a max set yet then the max for that cell can be set.

        :param sib_val: The result of a solver execution for a cell that was not set as the objective and is part
            of the binary set that contains the cell that was set as the objective.
        :type sib_val: float
        :param sibling_id: The cellID of the sibling.
        :type sibling_id: float
        """
        symmetric_bounds = (max_x[sibling_id] is None)
        if sib_val > 0 and sibling_id not in max_values:
            set_value(sibling_id, sib_val, maxx=True)
            if symmetric_bounds:
                set_value(sibling_id, -sib_val, minx=True)
        elif sib_val < 0 and sibling_id not in min_values:
            set_value(sibling_id, sib_val, minx=True)
            if symmetric_bounds:
                set_value(sibling_id, -sib_val, maxx=True)

    def check_cell_binary_set(curr_cellid: float, set_cells: set[float], minx: bool = False, maxx: bool = False):
        """Process all elements of a binary set after a completed solve to derive max/min values for each cell that was not set as the objective.

        :param curr_cellid: The cellID of the cell that was set as the objective of the solver
        :type curr_cellid: float
        :param set_cells: The set of cells that make up the binary set that `curr_cellid` is a part of
        :type set_cells: set[float]
        :param minx: True if the current solver objective is for min, otherwise False, defaults to False
        :type minx: bool, optional
        :param maxx: True if the current solver objective is for max, otherwise False, defaults to False
        :type maxx: bool, optional
        """
        for sibling in set_cells:
            if sibling != curr_cellid:
                if minx:
                    check_sibling(min_x[sibling].varValue, sibling)
                elif maxx:
                    if sibling in max_x:
                        check_sibling(max_x[sibling].varValue, sibling)
                    # If the sibling has no max_x variable it should have had a symmetric max
                    # value set above however we'll check again just in case
                    elif sibling not in max_values:
                        set_value(sibling, -min_values[sibling], maxx=True)

    def check_bound_equality_shortcut(curr_setid: int, curr_cellid: float, minx: bool = False, maxx: bool = False):
        """Reduce the need to solve for every cell by checking solved values against their bounds and also using the properties of a binary set, if applicable.

        :param curr_setid: The setid whose cellids are currently being solved for
        :type curr_setid: int
        :param curr_cellid: The cellid that was solved for
        :type curr_cellid: float
        :param minx: True if the current solver objective is for min, otherwise False, defaults to False
        :type minx: bool, optional
        :param maxx: True if the current solver objective is for max, otherwise False, defaults to False
        :type maxx: bool, optional
        :raises ValueError: if minx or max are both True or both False
        """
        if minx == maxx:
            raise ValueError
        # See if we can skip any further solves by checking other solved values on any cell in any set we were assigned

        # We don't want to run this on either any binary sets that have done both halves of the solver nor any
        # binary sets that have been processed once on the current solve, as we would have already processed any usable values
        # and any further processing would be redundant in this loop. We will however re-run these checks when the main set
        # being processed runs its 2nd solver, since we can only theoretically set half the max/min values based on one solve.
        processed_binary_sets_inner = [*processed_binary_sets, curr_setid]
        for inner_setid, cid_set in setid_to_cellids.items():
            inner_is_binary_set = (inner_setid != -1)
            if inner_is_binary_set and inner_setid in processed_binary_sets_inner:
                # No need to re-process the current binary set, though if we're in a non-binary set we want to check those
                continue
            for j in cid_set:
                if j != curr_cellid:
                    j_lpvar: pulp.LpVariable = min_x[j] if minx else max_x[j]
                    j_val = j_lpvar.varValue
                    # If any cells are currently at their lower/upper bound, we can take the value now
                    if j_val == j_lpvar.lowBound and j not in min_values:
                        set_value(j, j_val, minx=True)
                        # Symmetry
                        if max_x[j] is None:
                            set_value(j, -j_val, maxx=True)
                        if inner_is_binary_set:
                            check_cell_binary_set(j, cid_set, minx=minx, maxx=maxx)
                            # Once we've ran this inner loop once, there's really no need to re-check all the cells in this set
                            processed_binary_sets_inner.append(inner_setid)
                    elif j_val == j_lpvar.upBound and j not in max_values:
                        set_value(j, j_val, maxx=True)
                        # Symmetry
                        if max_x[j] is None:
                            set_value(j, -j_val, minx=True)
                        if inner_is_binary_set:
                            check_cell_binary_set(j, cid_set, minx=minx, maxx=maxx)
                            # Once we've ran this inner loop once, there's really no need to re-check all the cells in this set
                            processed_binary_sets_inner.append(inner_setid)

    # Objective function
    # Add min_values and Max Values to the dataframe
    binary_set_msg = False
    # Helpers for log messages
    left_msg_text = _("Left to solve")
    remain_msg_text = _("Remaining")
    for setid, set_cells in setid_to_cellids.items():
        is_binary_set = (setid != -1)
        if is_binary_set:
            if not binary_set_msg:
                binary_set_msg = True
                msg = _("Solving for max and min values of binary sets:")
                if logger:
                    logger.info(msg)
                else:
                    log_msg += msg + "\n"
        elif len(solved_cells) != total_cells_to_solve:
            msg = _("Solving for max and min values of cells not part of a binary set:")
            if logger:
                logger.info(msg)
            else:
                log_msg += msg + "\n"

        for i in set_cells:
            if i in solved_cells:
                continue

            # Update progress
            checkpoint = gconfid.get_time()
            left_to_solve = total_cells_to_solve - len(solved_cells)
            msg = _("Solving cell {:<7}  |  {:^{width2}}  |  {:^{width3}}  |  Solve duration: {:>10}").format(
                int(i), # cellID is a float but it should not ever have decimal places
                _("{} / {} {}").format(left_to_solve, total_cells_to_solve, left_msg_text),
                _("{:>5.2f}% {}").format((left_to_solve/total_cells_to_solve)*100, remain_msg_text),
                str(checkpoint - start_time)[:-4],
                width2 = len(str(total_cells_to_solve))*2 + 4 + len(left_msg_text), # dynamically size the 2nd and 3rd columns, considering language
                width3 = 8 + len(remain_msg_text),
            )
            if logger:
                logger.info(msg)
            else:
                log_msg += msg + "\n"

            # if a cell has no max variable created for it that is because it had symmetric bounds
            is_symmetric = (max_x[i] is None)

            # Only solve if we haven't computed a min yet
            if i not in min_values:
                min_prob.setObjective(min_x[i])
                min_prob.solve(solver)
                set_value(i, min_x[i].varValue, minx=True)

                # If we're in a binary set we can use the set properties to derive the other cells
                if is_binary_set:
                    check_cell_binary_set(i, set_cells, minx=True)

                # Check all other cells to see if they are at a max/min by comparing with their bounds
                check_bound_equality_shortcut(setid, i, minx=True)

            if i not in max_values:
                if not is_symmetric:
                    max_prob.setObjective(max_x[i])
                    max_prob.solve(solver)
                    set_value(i, max_x[i].varValue, maxx=True)

                    # If we're in a binary set we can use the set properties to derive the other cells
                    if is_binary_set:
                        check_cell_binary_set(i, set_cells, maxx=True)

                    # Check all other cells to see if they are at a max/min by comparing with their bounds
                    check_bound_equality_shortcut(setid, i, maxx=True)
                elif i not in max_values:
                    # The original bounds were symmetric, skip the solve and derive from min
                    set_value(i, -min_values[i], maxx=True)

        # This binary set has been processed, there is no need to run the inner loop on this set in the future.
        # since the first time processing the binary set every cell should have both a max and min set, necessarily.
        if is_binary_set:
            processed_binary_sets.append(setid)

    if logger:
        logger.info(_("Solving complete."))
    else:
        log_msg += _("Solving complete in sub-process.") + "\n"

    # Filter on all the cells that we processed
    outaudit = incell.filter(pc.is_in(incell["CellId"], value_set=pa.array(cid for cset in setid_to_cellids.values() for cid in cset)))

    l_bound_new = {}
    u_bound_new = {}
    max_values_new = {}
    min_values_new = {}
    for cell_set in setid_to_cellids.values():
        for i in cell_set:
            if("TotalLB" not in incell.column_names):
                l_bound_new[i] = total_noise_dict[i] * lb_factor
                u_bound_new[i] = total_noise_dict[i] * ub_factor
            else:
                l_bound_new[i] = l_bound[i]
                u_bound_new[i] = u_bound[i]

            max_values_new[i] = total_noise_dict[i] + max_values[i]
            min_values_new[i] = total_noise_dict[i] + min_values[i]

            mid_point[i] = (max_values_new[i] + min_values_new[i]) / 2
            max_minus_min[i] = max_values_new[i] - min_values_new[i]

            if total_noise_dict[i] != 0:
                interval_width[i] = max_minus_min[i] / total_noise_dict[i] * 100
            else:
                interval_width[i] = 0

            if sensitivity_dict[i] > 0:
                l_tolerance[i] = total_noise_dict[i] - sensitivity_dict[i] / 2
                u_tolerance[i] = total_noise_dict[i] + sensitivity_dict[i] / 2

                if max_minus_min[i] - sensitivity_dict[i] >= -tolerance:
                    problem_indicator[i] = ProtectionLevel.PROTECTED.value
                elif 0 < max_minus_min[i] < sensitivity_dict[i]:
                    problem_indicator[i] = ProtectionLevel.UNACHIEVED.value
                else:
                    problem_indicator[i] = ProtectionLevel.EXACT_DISCLOSURE.value
            else:
                l_tolerance[i] = total_noise_dict[i]
                u_tolerance[i] = total_noise_dict[i]

                if max_minus_min[i] != 0:
                    problem_indicator[i] = ProtectionLevel.PROTECTED.value
                else:
                    problem_indicator[i] = ProtectionLevel.EXACT_DISCLOSURE.value

    # Get the list of CellIds left in outaudit so we can use the ordering to add the new columns
    outaudit_cellids = outaudit["CellId"].to_pylist()
    def append_outaudit(data, name, dtype):
        return outaudit.append_column(name,
                                      pa.array([data[cid] for cid in outaudit_cellids], type=dtype),
                                      ) #noqa:B023 - not a risk here

    outaudit = append_outaudit(l_bound_new, "LBound", pa.float64())
    outaudit = append_outaudit(l_tolerance, "LTolerance", pa.float64())
    outaudit = append_outaudit(min_values_new, "MinValue", pa.float64())
    outaudit = append_outaudit(mid_point, "MidPoint", pa.float64())
    outaudit = append_outaudit(u_tolerance, "UTolerance", pa.float64())
    outaudit = append_outaudit(max_values_new, "MaxValue", pa.float64())
    outaudit = append_outaudit(u_bound_new, "UBound", pa.float64())
    outaudit = append_outaudit(interval_width, "IntervalWidth", pa.float64())
    outaudit = append_outaudit(max_minus_min, "MaxMinusMin", pa.float64())
    outaudit = outaudit.append_column("ProblemIndicator",
                                      pa.array([float(problem_indicator[cid]) for cid in outaudit_cellids], type=pa.float64()))

    # Grab log filepath from the solver and modify filename on-disk then when this task runs next,
    # even if in the same process, this log file won't be overwritten.
    log_path = get_log_path(solver)
    if log_path:
        log_path = Path(log_path)
        # If this log file is in the multiprocess temp folder, we need to rename, otherwise we're good
        if log_path.parent.stem == _MULTIPROCESS_LOG_DIR:
            log_path.rename(f"{log_path.with_suffix('')}_{uuid.uuid4()}{log_path.suffix}")

    return (outaudit, log_msg)

def _log_audit_info(outaudit: pa.Table, logger: Logger) -> None:
    """Log the string of Audit summary information.

    :param outaudit: Output dataset from a call to the audit function.
    :type outaudit: pa.Table
    :param logger: Instance of logger to log the info to
    :type logger: logging.Logger
    """
    def get_count(conds: list) -> int:
        """Combine a set of pyarrow compute conditions with AND then return the sum of true values.

        :param conds: List of pyarrow.compute conditions to combine
        :type conds: list
        :return: The sum of true values from the resulting mask
        :rtype: int
        """
        mask = conds[0]
        for c in conds[1:]:
            mask = pc.and_(mask, c)
        return pc.sum(mask).as_py()

    outaudit_active = outaudit.filter(pc.equal(outaudit["OutStatus"], "X"))

    n_sensitive_good = get_count([
        pc.equal(outaudit_active["Status"], "S"),
        pc.equal(outaudit_active["Type"], "C"),
        pc.equal(outaudit_active["ProblemIndicator"], ProtectionLevel.PROTECTED.value),
    ])
    n_sensitive_not_good = get_count([
        pc.equal(outaudit_active["Status"], "S"),
        pc.equal(outaudit_active["Type"], "C"),
        pc.equal(outaudit_active["ProblemIndicator"], ProtectionLevel.UNACHIEVED.value),
    ])
    n_sensitive_exact = get_count([
        pc.equal(outaudit_active["Status"], "S"),
        pc.equal(outaudit_active["Type"], "C"),
        pc.equal(outaudit_active["ProblemIndicator"], ProtectionLevel.EXACT_DISCLOSURE.value),
    ])
    n_sensitive_total = n_sensitive_good + n_sensitive_not_good + n_sensitive_exact


    n_complement_good = get_count([
        pc.invert(pc.is_in(outaudit_active["Status"], value_set=pa.array(["S", "X"]))),
        pc.equal(outaudit_active["ProblemIndicator"], ProtectionLevel.PROTECTED.value),
    ])
    n_complement_not_good = 0 # N/A
    n_complement_exact = get_count([
        pc.invert(pc.is_in(outaudit_active["Status"], value_set=pa.array(["S", "X"]))),
        pc.equal(outaudit_active["ProblemIndicator"], ProtectionLevel.EXACT_DISCLOSURE.value),
    ])
    n_complement_total = n_complement_good + n_complement_not_good + n_complement_exact


    n_user_suppress_comp_good = get_count([
        pc.equal(outaudit_active["Status"], "X"),
        pc.greater(outaudit_active["NetVariation"], 0),
        pc.equal(outaudit_active["ProblemIndicator"], ProtectionLevel.PROTECTED.value),
    ])
    n_user_suppress_comp_not_good = 0 # N/A
    n_user_suppress_comp_exact = get_count([
        pc.equal(outaudit_active["Status"], "X"),
        pc.greater(outaudit_active["NetVariation"], 0),
        pc.equal(outaudit_active["ProblemIndicator"], ProtectionLevel.EXACT_DISCLOSURE.value),
    ])
    n_user_suppress_comp_total = n_user_suppress_comp_good + n_user_suppress_comp_not_good + n_user_suppress_comp_exact


    n_user_suppress_not_comp_good = get_count([
        pc.equal(outaudit_active["Status"], "X"),
        pc.less_equal(outaudit_active["NetVariation"], 0),
        pc.equal(outaudit_active["ProblemIndicator"], ProtectionLevel.PROTECTED.value),
    ])
    n_user_suppress_not_comp_not_good = 0 # N/A
    n_user_suppress_not_comp_exact = get_count([
        pc.equal(outaudit_active["Status"], "X"),
        pc.less_equal(outaudit_active["NetVariation"], 0),
        pc.equal(outaudit_active["ProblemIndicator"], ProtectionLevel.EXACT_DISCLOSURE.value),
    ])
    n_user_suppress_not_comp_total = n_user_suppress_not_comp_good + n_user_suppress_not_comp_not_good + n_user_suppress_not_comp_exact


    n_sensitive_agg_good = get_count([
        pc.equal(outaudit_active["Type"], "A"),
        pc.equal(outaudit_active["ProblemIndicator"], ProtectionLevel.PROTECTED.value),
    ])
    n_sensitive_agg_not_good = get_count([
        pc.equal(outaudit_active["Type"], "A"),
        pc.equal(outaudit_active["ProblemIndicator"], ProtectionLevel.UNACHIEVED.value),
    ])
    n_sensitive_agg_exact = get_count([
        pc.equal(outaudit_active["Type"], "A"),
        pc.equal(outaudit_active["ProblemIndicator"], ProtectionLevel.EXACT_DISCLOSURE.value),
    ])
    n_sensitive_agg_total = n_sensitive_agg_good + n_sensitive_agg_not_good + n_sensitive_agg_exact

    logger.info("="*100)
    logger.info(_("Summary Report"))
    logger.info("="*100)

    TAB_WIDTH = 5
    base_string = " "*TAB_WIDTH + "{:<25} {}"
    good_prot_lbl = _("Good protection")
    no_prot_lbl = _("Protection not achieved")
    exact_lbl = _("Exact disclosure")
    total_lbl = _("Total")

    logger.info(_("Number of sensitive cells"))
    logger.info(base_string.format(good_prot_lbl, n_sensitive_good))
    logger.info(base_string.format(no_prot_lbl, n_sensitive_not_good))
    logger.info(base_string.format(exact_lbl, n_sensitive_exact))
    logger.info(base_string.format(total_lbl, n_sensitive_total))
    logger.info("-"*5)

    logger.info(_("Number of complements"))
    logger.info(base_string.format(good_prot_lbl, n_complement_good))
    logger.info(base_string.format(no_prot_lbl, n_complement_not_good))
    logger.info(base_string.format(exact_lbl, n_complement_exact))
    logger.info(base_string.format(total_lbl, n_complement_total))
    logger.info("-"*5)

    logger.info(_("Number of user suppressed cells (used as complements)"))
    logger.info(base_string.format(good_prot_lbl, n_user_suppress_comp_good))
    logger.info(base_string.format(no_prot_lbl, n_user_suppress_comp_not_good))
    logger.info(base_string.format(exact_lbl, n_user_suppress_comp_exact))
    logger.info(base_string.format(total_lbl, n_user_suppress_comp_total))
    logger.info("-"*5)

    logger.info(_("Number of user suppressed cells (not used as complements)"))
    logger.info(base_string.format(good_prot_lbl, n_user_suppress_not_comp_good))
    logger.info(base_string.format(no_prot_lbl, n_user_suppress_not_comp_not_good))
    logger.info(base_string.format(exact_lbl, n_user_suppress_not_comp_exact))
    logger.info(base_string.format(total_lbl, n_user_suppress_not_comp_total))
    logger.info("-"*5)

    logger.info(_("Number of sensitive aggregates"))
    logger.info(base_string.format(good_prot_lbl, n_sensitive_agg_good))
    logger.info(base_string.format(no_prot_lbl, n_sensitive_agg_not_good))
    logger.info(base_string.format(exact_lbl, n_sensitive_agg_exact))
    logger.info(base_string.format(total_lbl, n_sensitive_agg_total))
    logger.info("-"*5)

    total_good = n_sensitive_good + n_complement_good + n_user_suppress_comp_good + n_user_suppress_not_comp_good + n_sensitive_agg_good
    total_not_good = n_sensitive_not_good + n_complement_not_good + n_user_suppress_comp_not_good + n_user_suppress_not_comp_not_good + n_sensitive_agg_not_good
    total_exact = n_sensitive_exact + n_complement_exact + n_user_suppress_comp_exact + n_user_suppress_not_comp_exact + n_sensitive_agg_exact

    logger.info(_("Total"))
    logger.info(base_string.format(good_prot_lbl, total_good))
    logger.info(base_string.format(no_prot_lbl, total_not_good))
    logger.info(base_string.format(exact_lbl, total_exact))

def _shuttle(incell: pa.Table,
            inconstraint: pa.Table,
            lb_factor: float,
            ub_factor: float,
            rel_tolerance: float,
            abs_tolerance: float,
            logger: Logger,
            max_iter: int = 1) -> pa.Table:
    """Create lower and upper bounds for auditing a cell suppression pattern.

    :param incell: The input data set containing the cell information.
    :type incell: pa.Table
    :param inconstraint: The input data set containing the linear constraints coefficients.
    :type inconstraint: pa.Table
    :param lb_factor: Lower bound factor, between 0 and 1 inclusively, defaults to 0.5
    :type lb_factor: float, optional
    :param ub_factor: Upper bound factor, between 1 and 10 inclusively, defaults to 1.5
    :type ub_factor: float, optional
    :param rel_tolerance: The relative tolerance
    :type rel_tolerance: float
    :param abs_tolerance: The absolute tolerance
    :type abs_tolerance: float
    :param logger: The logger instance to log messages to
    :type logger: logging.Logger
    :param max_iter: The maximum number of iterations to calculate before selecting bounds, defaults to None
    :type max_iter: int | None, optional
    :return: The incell dataset adjusted to include the calculated bounds for each cell
    :rtype: pa.Table
    """
    logger.info(_("Calculating bounds with Shuttle"))

    start_time = gconfid.get_time()
    logger.info(_("Start time: {}").format(start_time))

    expr_notp = pc.field("OutStatus").is_null(nan_is_null=True) | (pc.utf8_upper(pc.field("OutStatus")) != "P")
    # These records require computed values for LB and UB
    incell_notp = incell.filter(expr_notp)
    # These records will have all LB and UB values set to 0
    incell_p = incell.filter(~expr_notp)

    # Compute the LB and UB values
    lb_col = pc.subtract(pc.multiply(incell_notp["TotalNoise"], lb_factor), incell_notp["TotalNoise"])
    ub_col = pc.subtract(pc.multiply(incell_notp["TotalNoise"], ub_factor), incell_notp["TotalNoise"])

    # Set the new columns on the end of the index
    incell_notp = incell_notp.add_column(incell_notp.num_columns, "LB", lb_col)
    incell_notp = incell_notp.add_column(incell_notp.num_columns, "UB", ub_col)
    incell_p = incell_p.add_column(incell_p.num_columns, "LB", pa.array([0]*len(incell_p), type=pa.float64()))
    incell_p = incell_p.add_column(incell_p.num_columns, "UB", pa.array([0]*len(incell_p), type=pa.float64()))

    # Form result
    cell_bounds = pa.concat_tables([incell_notp, incell_p], promote_options="permissive").select(["CellId", "TotalNoise", "LB", "UB"])

    # Print debug info table headers
    logger.debug(_("Iteration  Cells with Changed Bounds"))
    logger.debug(_("=========  ========================="))

    change = 1
    iterations = 1

    while change != 0 and iterations <= max_iter:
        cell_cons_bounds = inconstraint.select(["CellId", "ConstraintId", "Coefficient"]).join(
            cell_bounds.select(["CellId", "LB", "UB"]), "CellId", join_type="inner")

        cons_bounds = pa.TableGroupBy(cell_cons_bounds,
                                     ["ConstraintId", "Coefficient"],
                                     ).aggregate([("LB", "sum"), ("UB", "sum")])
        cons_bounds = cons_bounds.rename_columns({"LB_sum": "LB", "UB_sum": "UB"})

        cons_bounds_int = cons_bounds.filter(pc.field("Coefficient") == 1).select(["ConstraintId", "LB", "UB"])
        cons_bounds_mar = cons_bounds.filter(pc.field("Coefficient") == -1).select(["ConstraintId", "LB", "UB"])

        # Join all 3 tables
        cell_bounds_up = cell_cons_bounds.join(cons_bounds_int, "ConstraintId", right_suffix="_int", join_type="inner")
        cell_bounds_up = cell_bounds_up.join(cons_bounds_mar, "ConstraintId", right_suffix="_mar", join_type="inner")

        # Drop all records that DON'T have a matching constraintId in BOTH int and mar tables.
        # SQL/SAS does this implicitly during the aggregate operations, we need to do it explicitly
        # as pyarrow will otherwise be left with erroneous rows of data. Technically, partially complete
        # records will keep the constraintid in the final result set but the computed LBUp/UBUp value(s)
        # will be null. However, as these records are useless anyways, we can just filter them early
        cell_bounds_up = cell_bounds_up.filter(~(pc.field("LB_int").is_null() | pc.field("UB_int").is_null()) &
                                               ~(pc.field("LB_mar").is_null() | pc.field("UB_mar").is_null()))

        # If Coefficient == 1 then LB_mar - UB_int + UB else LB_int
        lbup = pc.if_else(
            pc.equal(cell_bounds_up["Coefficient"], 1),
            pc.add(pc.subtract(cell_bounds_up["LB_mar"], cell_bounds_up["UB_int"]), cell_bounds_up["UB"]),
            cell_bounds_up["LB_int"])
        # If Coefficient == 1 then UB_mar - LB_int + LB else UB_int
        ubup = pc.if_else(
            pc.equal(cell_bounds_up["Coefficient"], 1),
            pc.add(pc.subtract(cell_bounds_up["UB_mar"], cell_bounds_up["LB_int"]), cell_bounds_up["LB"]),
            cell_bounds_up["UB_int"])
        cell_bounds_up = cell_bounds_up.append_column("LBUp", lbup)
        cell_bounds_up = cell_bounds_up.append_column("UBUp", ubup)

        cell_bounds_up = pa.TableGroupBy(cell_bounds_up, ["CellId", "LB", "UB"],
                                         ).aggregate([("LBUp", "max"), ("UBUp", "min")])
        cell_bounds_up = cell_bounds_up.rename_columns({"LBUp_max": "LBUp", "UBUp_min": "UBUp"})
        cell_bounds_up = cell_bounds_up.select(["CellId", "LB", "UB", "LBUp", "UBUp"])

        # if LBUp - LB > (|LB|+1)*(reltol+abstol) then LBUp else LB
        lbup = pc.if_else(
            pc.greater(pc.subtract(cell_bounds_up["LBUp"], cell_bounds_up["LB"]),
                       pc.multiply(
                           pc.add(pc.abs(cell_bounds_up["LB"]), 1),
                           rel_tolerance+abs_tolerance),
                       ),
            cell_bounds_up["LBUp"],
            cell_bounds_up["LB"],
        )
        cell_bounds_up = cell_bounds_up.set_column(cell_bounds_up.column_names.index("LBUp"), "LBUp", lbup)

        # if UB - UBUp > (|UB|+1)*(reltol+abstol) then UBUp else UB
        ubup = pc.if_else(
            pc.greater(pc.subtract(cell_bounds_up["UB"], cell_bounds_up["UBUp"]),
                       pc.multiply(
                           pc.add(pc.abs(cell_bounds_up["UB"]), 1),
                           rel_tolerance+abs_tolerance),
                       ),
            cell_bounds_up["UBUp"],
            cell_bounds_up["UB"],
        )
        cell_bounds_up = cell_bounds_up.set_column(cell_bounds_up.column_names.index("UBUp"), "UBUp", ubup)

        # LBUp > UBUp?
        cond = pc.greater(cell_bounds_up["LBUp"], cell_bounds_up["UBUp"])

        lbup = pc.if_else(cond, cell_bounds_up["LB"], cell_bounds_up["LBUp"])
        cell_bounds_up = cell_bounds_up.set_column(cell_bounds_up.column_names.index("LBUp"), "LBUp", lbup)
        ubup = pc.if_else(cond, cell_bounds_up["UB"], cell_bounds_up["UBUp"])
        cell_bounds_up = cell_bounds_up.set_column(cell_bounds_up.column_names.index("UBUp"), "UBUp", ubup)

        # How many of the new computed LB/UB values are larger than where they started?
        # This assumes that pyarrow/numpy will always treat True = 1 as python itself does
        change = pc.add(
            pc.sum(pc.greater(cell_bounds_up["LBUp"], cell_bounds_up["LB"])),
            pc.sum(pc.greater(cell_bounds_up["UBUp"], cell_bounds_up["UB"])))

        cell_bounds = cell_bounds_up.select(["CellId", "LBUp", "UBUp"])
        cell_bounds = cell_bounds.rename_columns({"LBUp": "LB", "UBUp": "UB"})

        # Spacing is added to properly fit beneath table headers from debug printout before loop
        logger.debug(_("   {}                  {}").format(iterations, change))

        iterations += 1

    outcell = incell.join(cell_bounds, "CellId", join_type="inner")
    totallb = pc.add(outcell["TotalNoise"], outcell["LB"])
    totalub = pc.add(outcell["TotalNoise"], outcell["UB"])
    outcell = outcell.append_column("TotalLB", totallb)
    outcell = outcell.append_column("TotalUB", totalub)
    outcell = outcell.drop_columns(["LB", "UB"])

    logger.info(_("The Shuttle algorithm has completed"))

    end_time = gconfid.get_time()
    duration_stamp = str(end_time - start_time)[:-4]
    logger.info(_("Duration: {}").format(duration_stamp))

    # Make sure tables are all cleared from memory
    incell = None
    incell_p = None
    incell_notp = None
    inconstraint = None
    cell_bounds = None
    totallb = None
    totalub = None

    return outcell

def get_binary_cells(inconstraint: pa.Table, active_cells: list[float], logger: Logger) -> dict[int, set[float]]:
    """Determine the groups of binary cells that can be processed together.

    Binary cells are those found in constraints like X1+X2=cte; X2+X3=cte. Here X1, X2 and X3
    constitute a set of related binary cells. When any cell of such a set is at maximum or minimum,
    all the other cells of the set are necessarily at their maximum or minimum. This property is used
    to reduce processing time. The objective of the distribution below is to make sure all the cells
    of a given set of binary cells are allocated to the same processor.

    :param inconstraint: The dataset containing the constraint data
    :type inconstraint: pa.Table
    :param active_cells: The list of active cell IDs (i.e. status == 'X')
    :type active_cells: list[float]
    :param logger: The logger instance to log messages to
    :type logger: logging.Logger
    :return: A mapping of binarySetIDs to lists of their respective cellIDs
    :rtype: dict[int, set[float]]
    """
    start_time = gconfid.get_time()

    con_cid = list(zip(inconstraint["ConstraintId"].to_pylist(), inconstraint["CellId"].to_pylist(), strict=True))
    # Get unique constraint ids
    conids_unique = {cid for cid, _ in con_cid}

    # Get the active cells mapped to their constraint ids
    constraint_coef_nonzero = defaultdict(set)
    for conid, cellid in con_cid:
        if cellid in active_cells:
            constraint_coef_nonzero[conid].add(cellid)

    # Find the constraint ids that are mapped to sets of cellids that have exactly 2 elements
    binary_constraint_ids = {
        cid for cid in conids_unique
        if len(constraint_coef_nonzero[cid]) == 2 #noqa:PLR2004 - binary implies we are searching for 2
    }

    conids_unique = None

    # Analyse all cells involved in binary constraints, i.e. of type: x1 + x2 = cte.
    #     We will:
    #     - Determine the set of indexes of such constraints,
    #     - Determine the set of cells involved in such constraints,
    #     - Determine binary sets, and give a binary set ID to each. Cells in
    #       one binary set are related together, directly or indirectly. Example:
    #       if we have A+B=cte; B+C=cte; C+D=cte; E+F=cte; F+G=cte; we have two binary sets:
    #       Set1 = {A, B, C, D}; Set2 = {E, F, G}.
    #       What is interesting: when the solver is at optimum, if a binary cell is at optimum,
    #       then all related binary cells are also at optimum, due to the binary constraints.
    #       This property will be used in AUDIT to accelerate the determination
    #       of max and min of binary cells.

    # Get all CellIDs that are part of a binary set
    binary_cell_ids = set()
    for cid in binary_constraint_ids:
        bi_cell_ids = constraint_coef_nonzero[cid]
        binary_cell_ids.update(bi_cell_ids)

    # Determine the Id of the binary set for each binary cell: each binary
    # cell will have a number indicating to which binary set it belongs.
    # Binary set Id is a kind of group number to which the binary cell belongs

    binary_set_id = 0 # ID of the current binary set being created
    binary_set_ids = [] # List of all Binary Set IDs
    setid_to_cellids = {}  # {set_id:[cell_id,...]} - The cells that make up each binary set
    cellid_to_setid = {} # {cell_id:set_id} - The binary set ID for which each cellID belongs
    binary_con_treated = set() # if the cells of the binary constraint are categorized into a binary set yet

    for bcid in binary_cell_ids:
        # If this cell is not yet assigned to a binary set
        if bcid not in cellid_to_setid:
            # Create a new binary set
            binary_set_id += 1
            binary_set_ids.append(binary_set_id)

            # bcid is the first cell of the new group
            curr_binary_set_cell_ids = {bcid}

            # We need to continue to iterate to ensure we capture multiple overlapping sets one
            # after the other, rather than doing a single pass
            changed = True
            while changed:
                changed = False

                # Loop through all binary constraints to find cells belonging to this set
                for i in binary_constraint_ids:
                    # this constraint ID has not yet been processed to a set
                    if i not in binary_con_treated:
                        coef_cells = constraint_coef_nonzero[i]

                        # If this cell (i) has any cells in the current set as a coefficient
                        if coef_cells & curr_binary_set_cell_ids:
                            # How many cells are in this set to start
                            before = len(curr_binary_set_cell_ids)

                            # Add the coefficients to the binary set
                            curr_binary_set_cell_ids |= coef_cells
                            binary_con_treated.add(i)

                            # If the binary set grew in size, re-iterate to capture any missed coefficients
                            # that should now be part of the set because of the cells we added
                            if len(curr_binary_set_cell_ids) > before:
                                changed = True

            # Mark each cell in the current binary set
            for cell in curr_binary_set_cell_ids:
                cellid_to_setid[cell] = binary_set_id

            # Store the set
            setid_to_cellids[binary_set_id] = curr_binary_set_cell_ids

    # SAS contains a set of code to re-link any disjointed sets here, but the re-factor for
    # Python makes this step redundant.

    setid_to_cellids = {k:v for k,v in setid_to_cellids.items() if v}

    end_time = gconfid.get_time()
    logger.info(_("Computation of Binary Cells completed in: {}").format(str(end_time - start_time)[:-4]))
    logger.info(_("Number of binary constraints: {}").format(len(binary_constraint_ids)))
    logger.info(_("Number of binary cells: {}").format(len(binary_cell_ids)))
    logger.info(_("Number of binary sets: {}").format(len(setid_to_cellids)))
    logger.info("") # insert a newline here

    return setid_to_cellids

def allocate_cellids(input_dict: dict[int, set[float]], num_parts: int, extra: list[int] | None = None, extra_key: int = -1) -> list[dict[int, set[float]]]:
    """Split `input_dict` into up to `num_parts` dictionaries (no empty dicts), with key counts as equal as possible.

    If `extra` is provided, distribute it across the output dicts so that final loads are balanced, treating:
      - each set value in a dict as 1 element (regardless of its size),
      - each integer in `extra` as 1 element.

    The `extra` assigned to a chunk is stored under `extra_key` (default: -1).
    If `extra` is None or empty, no `extra_key` is added.
    If `num_parts` <= 0, raises ValueError.

    :param input_dict: The input dictionary to split.
    :type input_dict: dict[int, set[float]]
    :param num_parts: Maximum number of chunks to create (must be >= 1).
    :type num_parts: int
    :param extra: Optional set of ints to balance across chunks, defaults to None
    :type extra: list[int], optional
    :param extra_key: key used to store the per-chunk list of extra integers, defaults to -1
    :type extra_key: int, optional
    :return: each dict maps original keys (int) to sets[float], and may include `extra_key`: list[int].
    :rtype: list[dict[int, set[float]]]
    """
    if num_parts <= 0:
        raise ValueError

    total_keys = len(input_dict)
    if total_keys == 0:
        # No dict to divide but extra needs to be divided
        if extra and len(extra) > 0:
            # Create up to n chunks, don't want empty entries
            num_chunks = min(num_parts, len(extra))
            # Evenly allocate counts using heap strategy (or simple even split)
            allocations = _allocate_extra_counts([0] * num_chunks, len(extra))
            chunks = []
            idx = 0
            for take in allocations:
                sub = extra[idx:idx + take]
                idx += take
                # Only create non-empty chunks (allocations are guaranteed > 0)
                if take > 0:
                    chunks.append({extra_key: sub})
            return chunks

        # No keys and no extra -> no chunks
        return []

    # Number of chunks - at most n, but not more than the number of keys
    num_chunks = min(num_parts, total_keys)

    # Split keys as evenly as possible (stable by insertion order)
    items = list(input_dict.items())
    base = total_keys // num_chunks
    rem = total_keys % num_chunks

    chunks = []
    # Calculate how many sets will be added per-chunk
    sizes = [base + (1 if i < rem else 0) for i in range(num_chunks)]
    pos = 0
    # Add the binary sets to their respective chunk
    for size in sizes:
        chunk_items = items[pos:pos + size]
        pos += size
        chunks.append(dict(chunk_items))

    # Nothing else to distribute, we're done
    if not extra:
        return chunks

    # For the purposes of solving a Binary Cell set, we treat the amount of work required
    # as a single unit, since a solver only needs to be run for a max/min on a single cell
    # and the rest are derived. Therefore, for counting the current load per-chunk we count
    # one Binary Set of cellIDs as 1. Later, if we have anything to add from `extra`, we will
    # count each cellID as 1 towards the current load, since the solver will most likely need
    # to be run twice on each (i.e. 1 work unit).
    current_loads = [len(chunk) for chunk in chunks]

    # allocations will have the number of elements from `extra`` to add per-chunk
    allocations = _allocate_extra_counts(current_loads, len(extra))

    # Attach contiguous slices of `extra` to preserve order
    idx = 0
    for i, num_elms in enumerate(allocations):
        sub = extra[idx:idx + num_elms]
        idx += num_elms
        if num_elms > 0:
            chunks[i][extra_key] = set(sub)

    return chunks

def _allocate_extra_counts(current_loads: list[int], extra_count: int) -> list[int]:
    """Get the amount of `extra_count` elements that should be distributed to each chunk to create an even distribution.

    Uses a min-heap to always give the next item to the least-loaded chunk.

    :param current_loads: The current load of each chunk (1 set = 1 unit, 1 `extra` element = 1 unit)
    :type current_loads: list[int]
    :param extra_count: Number of `extra` elements to allocate across len(current_loads) chunks
    :type extra_count: int
    :return: Allocation of elements to each chunk
    :rtype: list[int]
    """
    num_chunks = len(current_loads)
    if num_chunks == 0 or extra_count <= 0:
        return [0] * num_chunks
    if num_chunks == 1:
        return [extra_count]

    # Min-heap of (chunk load, chunk index). We build final loads as we go.
    heap = [(load, i) for i, load in enumerate(current_loads)]
    heapq.heapify(heap)

    alloc = [0] * num_chunks
    for _i in range(extra_count):
        # fetch the most empty chunk (tie-break by index)
        load, i = heapq.heappop(heap)
        # Increase load count on heap and in final output
        alloc[i] += 1
        load += 1
        # Add the updated load count back to the heap
        heapq.heappush(heap, (load, i))

    return alloc
