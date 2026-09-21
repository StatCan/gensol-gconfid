"""Util module collects generally useful modules such as the proc wrapper superclass and the collection for standardizing dataset field names."""

from gconfid.util.solver_utils import _MULTIPROCESS_LOG_DIR, add_solver_threads, clone_solver, combine_solver_logs, get_log_path


def add_col_names(columns: list[str|None]) -> None:
    """Add each member of `columns` to the std_col_names dictionary as the standard name.

    :param columns: The list of column names to add to the dictionary
    :type columns: list[str | None]
    """
    for col in columns:
        if(col):
            std_col_names[col.upper()] = col

def get_std_col_name(column_name: str) -> str:
    """Return the standard version of `column_name`. If one does not exist `column_name` is returned as-is.

    :param column_name: The column name to fetch the standard name of
    :type column_name: str
    :return: The standardized column name
    :rtype: str
    """
    if(column_name.upper() in std_col_names):
        return std_col_names[column_name.upper()]
    return column_name

# Mapping of upper-case (for matching with arbitrarily cased DF field names) to standard case
std_col_names: dict[str, str] = {}
add_col_names(["CellId",
               "Coefficient",
               "ConstraintId",
               "NetVariation",
               "OutStatus",
               "Sensitivity",
               "TotalNoise",
               "Type"])

__all__ = [
    "_MULTIPROCESS_LOG_DIR",
    "add_solver_threads",
    "clone_solver",
    "combine_solver_logs",
    "get_log_path",
]
