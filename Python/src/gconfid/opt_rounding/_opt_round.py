"""Created on Mon Oct  3 09:46:23 2022.

@author: Haoluan Chen
Disclaimer: This is a prototype for GCONFIDOPTROUND, users looking for officially supported software should use G-Confid.
"""

import math
from logging import Logger

import pandas as pd
import pulp
import pyarrow as pa
import pyarrow.compute as pc

import gconfid
from gconfid.nls import _

weight_default = 1.0
constraint_bounds_default = 0
cell_bounds_default = None

def _opt_round(incell: pa.Table, additive_con: pa.Table, additive_bound: pa.Table,
               base: int, logger: Logger | None = None) -> pa.Table:
    """Round data table based on rounding base and constraints.

    Find an optimal solution that is both additive and controlled (constraints are respected).

    :param incell: The data table to be rounded.
    :type incell: pa.Table
    :param additive_con: Data table containing constraints related to additivity.
    :type additive_con: pa.Table
    :param additive_bound: Data table containing bounds related to additivity constraints.
    :type additive_bound: pa.Table
    :param base: The rounding base, required
    :type base: int
    :param logger: The logger to use for logging the procedure information, defaults to None
    :type logger: Logger | None, optional
    :return: The outround dataset
    :rtype: pa.Table
    """
    start_time = gconfid.get_time()

    if(not logger):
        logger = gconfid.logging.init_proc_level("Opt_Round", trace_level=gconfid.log_level.DEBUG)
    logger.info(gconfid.get_execution_header())

    cell_id = incell["CellId"].to_pylist()
    # Divide Total by base and create a dict indexed on CellIDs
    total = dict(zip(cell_id, pc.divide(incell["Total"], base).to_pylist(), strict=True))

    if "Weight" in incell.column_names:
        weight = dict(zip(cell_id, incell["Weight"].to_pylist(), strict=True))
    else:
        weight = None
        logger.info(_("{} was not present on the input dataset incell, assuming all values are to be {}").format("Weight", str(weight_default)))

    p = {}
    q = {}
    if "CellUB" in incell.column_names:
        cell_con_ub_dic = dict(zip(cell_id, incell["CellUB"].to_pylist(), strict=True))
    else:
        cell_con_ub_dic = None
        logger.info(_("CellUB was not present on the input dataset incell, assuming all values are to be missing"))

    if "CellLB" in incell.column_names:
        cell_con_lb_dic = dict(zip(cell_id, incell["CellLB"].to_pylist(), strict=True))
    else:
        cell_con_lb_dic = None
        logger.info(_("CellLB was not present on the input dataset incell, assuming all values are to be missing"))

    cell_total_dic = dict(zip(cell_id, incell["Total"].to_pylist(), strict=True))
    con_ci = dict(enumerate(additive_con["CellId"].to_pylist()))
    ni = dict(enumerate(additive_con["ConstraintId"].to_pylist()))
    a = dict(enumerate(additive_con["Coefficient"].to_pylist()))
    coef = {}

    #result dic
    ps = {}
    ns = {}
    ri = {}
    s = {}
    abs_shift = {}

    for key, val in ni.items():
        if val not in coef:
            coef[val] = [(con_ci[key], a[key])]
        else:
            coef[val].append((con_ci[key], a[key]))

    additive_con_id = additive_bound["ConstraintId"].to_pylist()

    if "ConstraintLB" in additive_bound.column_names:
        additive_con_l_dic = dict(zip(additive_con_id, additive_bound["ConstraintLB"].to_pylist(), strict=True))
    else:
        additive_con_l_dic = None
        logger.info(_("{} was not present on the input dataset {}, assuming all values are to be {}").format("ConstraintLB",
                                                                                                             "additive_bound",
                                                                                                             constraint_bounds_default))

    if "ConstraintUB" in additive_bound.column_names:
        additive_con_u_dic = dict(zip(additive_con_id, additive_bound["ConstraintUB"].to_pylist(), strict=True))
    else:
        additive_con_u_dic = None
        logger.info(_("{} was not present on the input dataset {}, assuming all values are to be {}").format("ConstraintUB",
                                                                                                             "additive_bound",
                                                                                                     constraint_bounds_default))

    # Initialize the LpProblem in puLP
    prob = pulp.LpProblem("Round", pulp.LpMinimize)
    # Generate LpVariable dic
    y = pulp.LpVariable.dict("y", cell_id, 0, None, cat = "Integer")
    a = pulp.LpVariable.dict("a", cell_id, 0, 1, "Binary")
    z = pulp.LpVariable.dict("z", cell_id, 0, None, "Integer")
    b = pulp.LpVariable.dict("b", cell_id, 0, 1, "Binary")
    for i in cell_id:
        p[i] = math.ceil(total[i]) - total[i]
        q[i] = total[i] - math.floor(total[i])

    # Objective
    if weight is None:
        prob += pulp.lpSum([weight_default*(y[i]+a[i]*p[i]+z[i]+b[i]*q[i]) for i in cell_id])
    else:
        prob += pulp.lpSum([weight[i]*(y[i]+a[i]*p[i]+z[i]+b[i]*q[i]) for i in cell_id])
    # Constraint
    for conid in additive_con_id:
        conlist = []
        for index in range(len(coef[conid])):
            i = coef[conid][index][0]
            coefficent = coef[conid][index][1]
            conlist.append(coefficent*(total[i] + y[i] + a[i]*p[i] - z[i] - b[i]*q[i]))
        if additive_con_u_dic is not None and pd.isna(additive_con_u_dic[conid]) is False:
            prob += pulp.LpConstraint(pulp.lpSum(conlist), -1, "conu"+str(conid), additive_con_u_dic[conid])
        elif additive_con_u_dic is None:
            prob += pulp.LpConstraint(pulp.lpSum(conlist), -1, "conu"+str(conid), constraint_bounds_default)
        if additive_con_l_dic is not None and pd.isna(additive_con_l_dic[conid]) is False:
            prob += pulp.LpConstraint(pulp.lpSum(conlist), 1, "conl"+str(conid), additive_con_l_dic[conid])
        elif additive_con_l_dic is None:
            prob += pulp.LpConstraint(pulp.lpSum(conlist), 1, "conl"+str(conid), constraint_bounds_default)

    # ai + bi = 1 Constraint
    for i in a:
        prob += pulp.LpConstraint(a[i] + b[i], 0, "add"+str(i), 1)

    # Cell level constraint
    for i in cell_id:
        if cell_con_ub_dic is not None and pd.isna(cell_con_ub_dic[i]) is False:
            prob += pulp.LpConstraint(total[i] + y[i] + a[i]*p[i] - z[i] - b[i]*q[i], -1, "cellu"+str(i), cell_con_ub_dic[i]/base)
        if cell_con_lb_dic is not None and pd.isna(cell_con_lb_dic[i]) is False:
            prob += pulp.LpConstraint(total[i] + y[i] + a[i]*p[i] - z[i] - b[i]*q[i], 1, "celll"+str(i), cell_con_lb_dic[i]/base)

    logger.info(_("Start solving LP..."))

    # If the user supplied a custom_solver to the wrapper, it will be set as the default already
    solver = pulp.LpSolverDefault

    prob.solve(solver)
    logger.info(_("Status: {}").format(pulp.LpStatus[prob.status]))

    # rounded value
    rounded = {}
    for i in cell_id:
        rounded[i] = (total[i] + y[i].varValue + a[i].varValue*p[i] - z[i].varValue - b[i].varValue*q[i])*base
        # extract solutions to four variables
        y[i] = y[i].varValue
        a[i] = a[i].varValue
        z[i] = z[i].varValue
        b[i] = b[i].varValue
        # prepare outputs
        p[i] = p[i]*base
        q[i] = q[i]*base
        total[i] = total[i]*base
        ps[i] = y[i] + a[i]*p[i]
        ns[i] = z[i] + b[i]*q[i]
        ri[i] = p[i] + q[i]
        s[i] = ps[i] - ns[i]
        abs_shift[i] = ps[i] + ns[i]

    # Calculate the objective value
    objective_value = 0
    for i in cell_id:
        weight_value = weight_default if weight is None else weight[i]
        objective_value += weight_value*abs(cell_total_dic[i] - rounded[i])
    logger.info(_("The objective value is: {}").format(objective_value))

    #PyArrow
    def get_arr(x, dtype=None):
        return pa.array([x[i] for i in cell_id], type=pa.float64() if dtype is None else dtype)
    result = pa.Table.from_pydict({
        "CellId": pa.array(cell_id),
        "UpperResidual": get_arr(p),
        "PositiveShiftIndicator": get_arr(a),
        "PositiveBaseShift": get_arr(y),
        "PositiveShift": get_arr(ps),
        "LowerResidual": get_arr(q),
        "NegativeShiftIndicator": get_arr(b),
        "NegativeBaseShift": get_arr(z),
        "NegativeShift": get_arr(ns),
        "ResidualIndicator": get_arr(ri),
        "Shift": get_arr(s),
        "AbsoluteShift": get_arr(abs_shift),
        "Total": get_arr(total),
        "RoundedTotal": get_arr(rounded),
        "CellLowerBound": get_arr(cell_con_lb_dic) if cell_con_lb_dic is not None else pa.array([None]*len(incell), type=pa.float64()),
        "CellUpperBound": get_arr(cell_con_ub_dic) if cell_con_ub_dic is not None else pa.array([None]*len(incell), type=pa.float64()),
        "Weight": get_arr(weight) if weight is not None else pa.array([weight_default]*len(incell), type=pa.float64()),
    })

    end_time = gconfid.get_time()
    duration_stamp = str(end_time - start_time)[:-4]
    logger.info(_("Total duration: {}").format(duration_stamp))
    logger.info(gconfid.get_execution_footer("opt_round"))
    return result
