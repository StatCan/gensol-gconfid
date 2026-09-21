import pandas as pd
import pyarrow as pa

from ..nls import _


def validate_arg_type(arg_val, parm_name, allowed_types, skip_none=False, log=None):
    """Validate argument's type, log and raise `TypeError` if type invalid.

    Logs error message if `log` specified.
    To facilitate skipping validation of unspecified parameters,
    skips validation if `arg_val` is None and `skip_none` is True.
    """
    if skip_none and arg_val is None:
        return

    if not isinstance(arg_val, allowed_types):
        mesg = _("option '{}' must be type `{}`, but it is `{}`").format(
            parm_name,
            allowed_types,
            type(arg_val),
        )
        if log is not None:
            log.error(mesg)
        raise(TypeError(mesg))

def string_parm_is_empty(arg_val):
    """Whether string value is considered "empty".

    Returns `True` if empty, `False` otherwise
    """
    return len(arg_val.strip()) == 0

def get_duplicated_columns(dataset: pd.DataFrame | pa.Table | None) -> dict[str, list[str]]:
    """Get the column names of `dataset` that are found multiple times, if any, while ignoring case.

    :param dataset: The dataset to check for duplicate columns
    :type dataset: pd.DataFrame | pa.Table | None
    :return: A map of the upper case representation of the duplicated column name to a list of the
        actual column names. i.e. {"ABCD": ["abcd", "AbCd", "ABCD"]}
    :rtype: dict[str, list[str]]
    """
    if isinstance(dataset, pa.Table):
        cols = dataset.column_names
    elif isinstance(dataset, pd.DataFrame):
        cols = dataset.columns
    else:
        return {}

    dupe_map = {}
    for col in cols:
        upped = col.upper()
        if upped in dupe_map:
            dupe_map[upped].append(col)
        else:
            dupe_map[upped] = [col]

    return {key: val for key, val in dupe_map.items() if len(val) > 1}
