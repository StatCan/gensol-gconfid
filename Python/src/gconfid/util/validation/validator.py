"""Contains basic Validation framework including the base Validator protocol and ValidationResult class."""
from typing import Protocol
from warnings import WarningMessage

import pandas as pd
import pandera.pandas as pr


class Validator(Protocol):
    """An interface class for a single type of validation to perform on a dataset."""

    def validate(self, df: pd.DataFrame) -> tuple[pd.DataFrame, "ValidationResult"]:
        """Validate dataset df against a set of rules."""
        ...

    def normalize(self, df: pd.DataFrame) -> pd.DataFrame:
        """Modify the casing of dataset df's columns to fit their expected format."""
        ...

class ValidationResult:
    """Holds the result of a successful or failed validation execution."""

    _is_valid: bool
    _error_msg: str
    _warnings: list[WarningMessage]

    def __init__(self, is_valid: bool, warnings: list[WarningMessage], error_msg: str, e: pr.errors.SchemaError):
        """Create a `ValidationResult` instance.

        :param is_valid: Was the validation successful or not
        :type is_valid: bool
        :param error_msg: The error message stating the cause of failure, if applicable.
        :type error_msg: str
        """
        self._is_valid = is_valid
        self._warnings = warnings
        self._error_msg = error_msg
        self.exception = e

    @classmethod
    def valid(cls, warnings: list[WarningMessage] | None = None) -> "ValidationResult":
        """Return a `ValidationResult` object for a successful validation.

        :param warnings: A list of the WarningMessages received from the validation, defaults to None
        :type warnings: list[WarningMessage] | None, optional
        :return: The `ValidationResult` object indicating success and therefore no error message.
        :rtype: ValidationResult
        """
        return cls(True, warnings, None, None)

    @classmethod
    def invalid(cls, error_msg: str, e: pr.errors.SchemaError, warnings: list[WarningMessage] | None = None) -> "ValidationResult":
        """Return a `ValidationResult` object for a failed validation.

        :param error_msg: The reason for the validation failing.
        :type error_msg: str
        :param e: The exception object that was thrown.
        :type e: pa.errors.SchemaError
        :param warnings: A list of the WarningMessages received from the validation, defaults to None
        :type warnings: list[WarningMessage] | None, optional
        :return: The `ValidationResult` object indicating failure and the reason for failure.
        :rtype: ValidationResult
        """
        return cls(False, warnings, error_msg, e)

    @property
    def is_valid(self) -> bool:
        """Was the validation successful or not.

        :return: True if the validation succeeded, False otherwise.
        :rtype: bool
        """
        return self._is_valid

    @property
    def error_msg(self) -> str | None:
        """The message stating the reason for validation failure.

        :return: A message string indicating the reason the validation failed.
        :rtype: str
        """
        return self._error_msg

    @property
    def warnings(self) -> list[WarningMessage]:
        """The list of WarningMessages that were received from the validation.

        :return: A list of WarningMessages that were received from the validation.
        :rtype: list[WarningMessage]
        """
        return self._warnings or []
