"""Define G-Confid Custom Exceptions and shared custom exceptions."""

from gconfid._common.src.exceptions import (
    IOUtilError,
    NormalizationError,
    ProcedureCError,
    ProcedureError,
    ProcedureInputError,
    ProcedureIOError,
    ProcedureOutputError,
    ProcedureValidationError,
    ProcessingError,
    TypeConverterError,
)


class GConfidInfeasibleProblemError(Exception):
    """Exception raised when the underlying linear programming problem cannot be solved."""

class GConfidInputDatasetValidationError(Exception):
    """Exception raised when an input dataset fails a validation/normalization check."""

class GConfidSuppressNoSensitiveValuesError(Exception):
    """Exception raised when an input dataset's ambiguity or sensitivity values are all less than or equal to 0."""

class GConfidSuppressSolverNonOptimalError(Exception):
    """Exception raised when the solver returns a non-optimal return code and we can't otherwise deal with it."""

__all__ = [
    "GConfidInfeasibleProblemError",
    "GConfidInputDatasetValidationError",
    "GConfidSuppressNoSensitiveValuesError",
    "GConfidSuppressSolverNonOptimalError",
    "IOUtilError",
    "NormalizationError",
    "ProcedureCError",
    "ProcedureError",
    "ProcedureIOError",
    "ProcedureInputError",
    "ProcedureOutputError",
    "ProcedureValidationError",
    "ProcessingError",
    "TypeConverterError",
]
