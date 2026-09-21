# noqa: D100
import pandas as pd

from gconfid.nls import _
from gconfid.util.validation.validator import ValidationResult, Validator


class ValidationContext:
    """Holds a validation strategy and applies it to a dataset to produce a `ValidationResult`."""

    _strategy: Validator
    _validation_result: ValidationResult

    @property
    def strategy(self) -> Validator | None:
        """The desired `Validator` to validate datasets with.

        :return: The validator that is currently set as the active strategy.
        :rtype: Validator | None
        """
        return self._strategy
    @strategy.setter
    def strategy(self, value: Validator):
        self._strategy = value
        # Invalidate the result since our strategy changed
        self._validation_result = None

    @property
    def validation_result(self) -> ValidationResult | None:
        """The resultant `ValidationResult` from a call to `execute_validate()`.

        :return: The result of the last call to `execute_validation()`, None if validation has
            not been performed yet or if the strategy has changed since the last validation.
        :rtype: ValidationResult | None
        """
        return self._validation_result

    def __init__(self):
        """Create an instance of `ValidationContext`."""
        self.strategy = None
        self._validation_result = None

    def execute_validate(self, df: pd.DataFrame) -> pd.DataFrame:
        """Validate `df` against the selected `strategy`, store the result and return the validated DataFrame.

        :param df: The dataset to validate.
        :type df: pd.DataFrame
        :return: The validated dataset with any type coercions applied.
        :rtype: pd.DataFrame
        """
        if isinstance(df, pd.DataFrame):
            res, self._validation_result = self.strategy.validate(df)
            return res

        _msg = _("Validation can only be performed on input datasets of type pandas.DataFrame.")
        raise TypeError(_msg)

    def execute_normalize(self, df: pd.DataFrame) -> pd.DataFrame:
        """Normalize `df` to force column casing to respect the specification in the selected `strategy`.

        :param df: The dataset to normalize.
        :type df: pd.DataFrame
        :return: The dataset with updated columns if necessary.
        :rtype: pd.DataFrame
        """
        if isinstance(df, pd.DataFrame):
            return self.strategy.normalize(df)

        _msg = _("Normalization can only be performed on input datasets of type pandas.DataFrame.")
        raise TypeError(_msg)
