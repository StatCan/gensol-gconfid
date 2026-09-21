"""Module file that imports all of the G-Confid functions."""

import multiprocessing
from datetime import datetime
from importlib.metadata import version
from importlib.util import find_spec

import pulp

import gconfid._log as logging
import gconfid.diagnostics
import gconfid.nls
from gconfid._log import log_levels as log_level  # expose log levels as `log_level`, refer to like `gconfid.log_level.INFO`
from gconfid.auditing.auditing import Auditing
from gconfid.io_util import (
    get_default_output_spec,
    get_OPT_split_blocks,
    set_default_output_spec,
    set_OPT_split_blocks,
)
from gconfid.nls import (
    SupportedLanguage,  # `SupportedLanguage` for user reference
    _,  # expose `_()` for local internal use
)
from gconfid.opt_rounding.opt_rounding import OptRounding
from gconfid.proc import (
    sensitiv,
)
from gconfid.suppression.suppression import Suppression

# get version from package metadata
__version__ = version("gconfid")

lg = logging  # alias for logging

# These initializations will need to be performed in the main process and any child processes that are spawned to parallelize work.

# Initialize Native Language Support (bilingual messages)
gconfid.nls.set_language()

# Set the global default output type
set_default_output_spec("pyarrow")
# Turn off the split_blocks option when converting PyArrow to Pandas
# With split_blocks=True users must call .copy() on any Pandas DF returned by Sensitivity in order to modify it
# as Sensitivity must convert from Pyarrow and using this option enforces zero-copy which results in a read-only DF.
# Other procs are written using Pandas natively so they don't face this issue nor will this option affect their outputs.
# Would be nice to instead have this set explicitly when write_outputs is called but there are too many layers to
# pass the option through for sensitivity, it makes more sense to just set it once
set_OPT_split_blocks(False)

# configure diagnostics: enable execution timer
gconfid.diagnostics.ExecTimer.enable_global()

# import and load user facing gconfid module functions
# These initializations we only want running when we first load our main, parent process
# Child processes that are spawned later to do some work we do not want these particular things re-initialized
if multiprocessing.current_process().name == "MainProcess":
    # Initialize top level logger which will have DEBUG trace level by default at the top-level
    log_lcl = lg.init_top_level(logger_name="gconfid")
    log_lcl.info(_("Importing gconfid package version {}").format(__version__)) # sac: should we log during import?
    log_lcl.info(_("G-Confid package imported"))

    # Currently sensitivity is only single-process so no need to reload binaries in child processes of other procs
    gconfid.proc.GConfidProcedure._load_all_procs(lang=gconfid.nls.get_language())  # noqa: SLF001

    # Update the default Pulp solver
    if find_spec("highspy") is not None:
        pulp.LpSolverDefault = pulp.HiGHS(msg=False)
        log_lcl.debug(_("Default solver set to {}").format(pulp.LpSolverDefault.name))
    else:
        log_lcl.info(_("highspy library is not installed. Pulp's default solver will be left as {}.").format(pulp.LpSolverDefault.name))

# Define method of modifying language (affects both Python and C)
def set_language(new_lang : SupportedLanguage | None= None, reload_procs: bool = True) -> None:
    """Set the language used for console log messages.

    For `new_lang`, specify a value from `gconfid.SupportedLanguage`.
    Use `reload_procs=False` if setting language from a sub-process where we know we won't need the reloaded c-proc(s).
    """
    gconfid.nls.set_language(lang=new_lang)
    if reload_procs:
        gconfid.proc.GConfidProcedure._reload_all_procs(lang=gconfid.nls.get_language())  # noqa: SLF001

# additional procedure aliases
Sensitivity = sensitiv

__all__ = [
    "Auditing",
    "OptRounding",
    "Sensitivity",
    "SupportedLanguage", # ?for user reference?
    "Suppression",
    "get_OPT_split_blocks",
    "get_default_output_spec",
    "log_level", # for user reference
    "sensitiv",
    "set_OPT_split_blocks",
    "set_default_output_spec",
    "set_language",
]

def get_time() -> datetime:
    """Return the current time.

    This was put in a function because it violates Ruff rule DTZ005 and there may be a better way to do this.
    """
    return datetime.now() # noqa: DTZ005

def get_execution_header() -> str:
    """Return an execution header that can be displayed in the log or console.

    :return: Formatted execution header
    :rtype: str
    """
    border_string = "="*100
    msg = "\n" + border_string + "\n"
    msg += _("G-Confid Version                : {}\n").format(gconfid.__version__)
    msg += _("Support Email                   : {}\n").format("statcan.gconfid-gconfid.statcan@statcan.gc.ca")
    msg += _("Start Time                      : {} ({})\n").format(get_time().strftime("%c"), get_time().astimezone().tzinfo)
    msg += border_string

    return msg

def get_execution_footer(proc_name: str) -> str:
    """Return an execution footer that can be displayed in the log or console.

    :param proc_name: The name of the process to be included in the footer
    :type proc_name: str
    :return: Formatted execution footer string
    :rtype: str
    """
    border_string = "="*100
    msg = border_string + "\n"
    msg += _("G-Confid {} completed: {} ({})\n").format(proc_name, get_time().strftime("%c"), get_time().astimezone().tzinfo)
    msg += border_string

    return msg
