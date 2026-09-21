"""Provides a set of useful methods for working with Pulp LpSolvers."""
import copy
import inspect
import shutil
from logging import Logger
from pathlib import Path

import pulp

from gconfid.nls import _

_DEFAULT_NUM_THREADS = 4
"""Default number of threads to allow a solver to use if adding a thread count using :data:`add_solver_threads`"""

_MULTIPROCESS_LOG_DIR = "temp_logs"
"""Directory name to create log files in for solvers cloned from a base solver with a log location.
Will be deleted once all logs are combined."""

def clone_solver(solver: pulp.LpSolver, unique_log_suffix: str | None = None) -> pulp.LpSolver:
    """Create a copy of `solver`.

    If `unique_log_suffix` is given, the logfile property or logPath option will be copied with the suffix appended
    to the filename and the :data:`_MULTIPROCESS_LOG_DIR` appended to the directory chain where the file will be saved.
    If it is not given the logfile or logPath will be copied as-is, if present.

    :param solver: The solver to duplicate
    :type solver: pulp.LpSolver
    :param unique_log_suffix: Suffix to append to the logPath or logfile copied from `solver` to ensure uniqueness, defaults to None
    :type unique_log_suffix: str, optional
    :return: A deep copy of `solver`
    :rtype: pulp.LpSolver
    """
    # Only some solvers have a .copy() implementation and this does not always copy over every attribute we want
    cls = solver.__class__
    sig = inspect.signature(cls.__init__)
    non_constructor_args = ("optionsDict", "options", "keepFiles", "path", "tmpDir")

    # Copy over named constructor arguments:
    kwargs = {}
    for name in sig.parameters.keys():
        if name not in non_constructor_args and name != "self" and hasattr(solver, name):
            attr = getattr(solver, name)
            if name == "logfile" and unique_log_suffix is not None:
                if isinstance(attr, str) and not attr.strip():
                    kwargs[name] = attr
                    continue
                # If this solver has a logfile property and we have a suffix to append, modify the path before copying
                logfile = Path(attr)
                logfile = logfile.parent / _MULTIPROCESS_LOG_DIR / logfile.name
                # If the temp directory already exists, delete it then remake
                if logfile.parent.is_dir():
                    shutil.rmtree(logfile.parent)
                logfile.parent.mkdir()
                kwargs[name] = f"{logfile.with_suffix('')}_{unique_log_suffix}{logfile.suffix}"
            else:
                kwargs[name] = attr

    new_solver = cls(**kwargs)

    # For arguments with different names or otherwise that can't be copied the same as above such as runtime state
    for arg in non_constructor_args:
        if hasattr(solver, arg):
            try: #noqa: SIM105
                attr = getattr(solver, arg)
                copied_data = copy.deepcopy(attr)
                # If we just copied optionsDict, update logPath if required
                if arg == "optionsDict" and isinstance(copied_data, dict) and unique_log_suffix is not None:
                    log_path = copied_data.get("logPath")
                    if isinstance(log_path, str) and not log_path.strip():
                        # log_path is a string but it's empty
                        continue
                    log_path = Path(log_path)
                    log_path = log_path.parent / _MULTIPROCESS_LOG_DIR / log_path.name
                    # If the temp directory already exists, delete it then remake
                    if log_path.parent.is_dir():
                        shutil.rmtree(log_path.parent)
                    log_path.parent.mkdir()
                    copied_data["logPath"] = f"{log_path.with_suffix('')}_{unique_log_suffix}{log_path.suffix}"
                setattr(new_solver, arg, copied_data)
            except Exception: #noqa: S110 BLE001
                # Some attributes may not be deepcopy-able; skip safely
                pass

    return new_solver

def add_solver_threads(solver: pulp.LpSolver, logger: Logger, num_threads: int = _DEFAULT_NUM_THREADS, overwrite: bool = False) -> bool:
    """Add a value for the `threads` option to `solver` if supported by `solver`.

    If `overwrite` is True the `threads` count will be overwritten, otherwise it will only be updated if
    the value is currently None.

    :param solver: The solver to update.
    :type solver: pulp.LpSolver
    :param logger: A logger instance
    :type logger: Logger
    :param num_threads: The number of threads to set `threads` to, defaults to :data:`_DEFAULT_NUM_THREADS`
    :type num_threads: int
    :param overwrite: Should `threads` be updated if it is already set?, defaults to False
    :type overwrite: bool, optional
    :return: True if the `threads` option exists on the solver, False otherwise.
    :rtype: bool
    """
    if hasattr(solver, "threads"):
        if solver.threads is None or overwrite:
            solver.threads = num_threads
            logger.debug(_("Solver threads set to {}").format(num_threads))
        return True
    if hasattr(solver, "optionsDict"):
        if "threads" in solver.optionsDict:
            if solver.optionsDict["threads"] is None or overwrite:
                solver.optionsDict["threads"] = num_threads
                logger.debug(_("Solver threads set to {}").format(num_threads))
            return True

        solver.optionsDict["threads"] = num_threads
        logger.debug(_("Solver threads set to {}").format(num_threads))
        return True

    return False

def combine_solver_logs(solver: pulp.LpSolver) -> None:
    """Combine all logs from the :data:`_MULTIPROCESS_LOG_DIR` sub-directory into a single log file, based on the logfile or logPath found in `solver`.

    The final log created is found at the path in the solver's logfile property or logPath option. The :data:`_MULTIPROCESS_LOG_DIR`
    sub-directory is deleted, along with all .log files within, once the main file is created.

    :param solver: The "base" solver holding the final log path in the logfile property or logPath option.
    :type solver: pulp.LpSolver
    :raises ValueError: If the :data:`_MULTIPROCESS_LOG_DIR` sub-directory does not exist at the same location
        as the `solver`'s logfile/logPath.
    """
    base_log_path = get_log_path(solver)

    if not base_log_path:
        return

    # This is the directory that should hold all the logs from each sub-process we need to combine
    base_log_path = Path(base_log_path)
    logs_to_combine_dir = base_log_path.parent / _MULTIPROCESS_LOG_DIR
    if not logs_to_combine_dir.is_dir():
        raise ValueError(_("Error while combining solver log files: sub-process log directory {} does not exist").format(logs_to_combine_dir))

    # Iterate over the logs in the temp log directory and write them to the main file
    with Path.open(base_log_path, "w") as outlog:
        for sub_log in logs_to_combine_dir.iterdir():
            if sub_log.suffix == ".log":
                with Path.open(sub_log, "r") as inlog:
                    outlog.write(inlog.read())
                    outlog.write("\n")

    # Delete the temp log directory
    shutil.rmtree(logs_to_combine_dir)

def get_log_path(solver: pulp.LpSolver) -> str | Path | None | bool:
    """Get the logfile property or logPath option from `solver`, False if it is not found.

    :param solver: The solver to get the logfile or logPath value from
    :type solver: pulp.LpSolver
    :return: The value of logfile or logPath, False if not found
    :rtype: str | Path | None | bool
    """
    # Each solver type should use one or the other method of storing the log path
    # is it possible for any of the types to use both?
    log_path = solver.optionsDict.get("logPath")

    # log_path is not a Path, not a str or is a str but it's empty
    if not isinstance(log_path, Path) and (not isinstance(log_path, str) or not log_path.strip()):
        # this solver does not use logPath or no log has been set, check logfile
        log_path = getattr(solver, "logfile", None)
        if not isinstance(log_path, Path) and (not isinstance(log_path, str) or not log_path.strip()):
            log_path = None

    return log_path
