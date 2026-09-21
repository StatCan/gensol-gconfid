"""GConfid proc module provides general GConfidProcedure class and the module for the sensitivity procedure."""
# import for internal use
from gconfid.proc.gconfid_proc import (
    GConfidProcedure,
)

# expose each procedure class under aliases
from gconfid.proc.proc_sensitiv import ProcSensitiv as sensitiv  # noqa: N813

# expose methods for setting cross procedure default values
get_default = GConfidProcedure.get_default
set_default = GConfidProcedure.set_default

__all__ = [
    "GConfidProcedure",
    "get_default",
    "sensitiv",
    "set_default",
]
