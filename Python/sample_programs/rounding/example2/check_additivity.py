"""Check that a total is additive based on the constraints."""

import pandas as pd

import gconfid


def check_additivity(
        incell : pd.DataFrame,
        additive_con : pd.DataFrame,
        variable_to_check: str = "RoundedTotal",
        check_coverage : int = 1,
        tolerance : float = 0.00001) -> float:
    """Check the additivity of a data table based on the constraints provided.

    :param incell: The data table with columns CellId and the variable to check.
    :type incell: pd.DataFrame
    :param additive_con: The constraints to use.
    :type additive_con: pd.DataFrame
    :param variable_to_check: The numeric variable to be checked, defaults to "RoundedTotal"
    :type variable_to_check: str, optional
    :param check_coverage: Whether or not to check if all cells are covered by the constraints, defaults to 1, use 0 only providing a subset of the constraints.
    :type check_coverage: int, optional
    :param tolerance: Tolerance used to account for floating point precision, defaults to 0.00001
    :type tolerance: float, optional
    :raises gconfid.exceptions.ProcessingError: raised when the coverage check fails.
    :return: Returns the running total used to determine if a table is additive, 0 indicates the table is additive (within the tolerance)
    :rtype: float
    """
    running_total = 0.0
    for _index, row in incell.iterrows():
        constraints = additive_con[additive_con["CellId"] == row["CellId"]]
        if check_coverage > 0 and len(constraints) == 0:
            msg = "{} is not involved in any constraints. The constraints do not cover all table cells.".format(row["CellId"])
            raise gconfid.exceptions.ProcessingError(msg)
        for coefficient in constraints["Coefficient"]:
            running_total = running_total + row[variable_to_check]*coefficient

    if abs(running_total) < tolerance:
        running_total = 0

    return running_total
