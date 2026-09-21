"""Contains the implementation of the Validator interface for use in the GConfid project."""
import warnings

import pandas as pd
import pandera.pandas as pa
from pandera.errors import SchemaErrorReason as Reason

from gconfid.nls import _
from gconfid.util.validation.validator import ValidationResult, Validator


class GConfidValidator(Validator):
    """A validation strategy for GConfid modules provides a Validator implementation to attach to a validation context."""

    _schema: pa.DataFrameSchema

    def __init__(self, schema: pa.DataFrameSchema):
        """Create a validation strategy that holds a specific schema to apply when applying operations.

        :param schema: The `pandera.DataFrameSchema` to validate and normalize against.
        :type schema: pr.DataFrameSchema
        """
        self._schema = schema
        super().__init__()

    def validate(self, df: pd.DataFrame) -> tuple[pd.DataFrame, "ValidationResult"]:
        """Validate the contents of `df` using the specified schema and normalize the dataset's columns to fit that schema, if possible.

        :param df: The dataset to validate.
        :type df: pd.DataFrame
        :return: The dataset with correct datatypes on coerced fields and a ValidationResult object containing the results of the
            validation. If the validation failed the original dataset is returned and the ValidationResult indicates the reason for failure.
        :rtype: pd.DataFrame, ValidationResult
        """
        try:
            with warnings.catch_warnings(record=True) as caught_warnings:
                warnings.simplefilter("always")
                res = self._schema.validate(df)
                return res, ValidationResult.valid(warnings=caught_warnings)
        except pa.errors.SchemaError as e:
            return df, ValidationResult.invalid(translate_error(e), e)
        except pa.errors.SchemaErrors as e:
            # Could improve the processing here but for now this will work
            msg = ""
            for error in e.schema_errors:
                msg += translate_error(error) + "\n"
            return df, ValidationResult.invalid(msg, e)

    def normalize(self, df: pd.DataFrame) -> pd.DataFrame:
        """Normalize the field names of dataset `df` using the specified schema.

        :param df: The dataset to normalize
        :type df: pd.DataFrame
        :return: A copy of dataset `df` with all field names normalized to their expected values
        :rtype: pd.DataFrame
        """
        # Preferably we'd do this in the validate() method on the Model we are using.
        # However, we need to convert our Pandera models to schemas before running
        # validation in order to dynamically add user-supplied special variables, like
        # by_vars, as columns on the Model/Schema prior to validation.
        # Unfortunately Pandera for some reason does not preserve the Model's overridden
        # validate() method after the Schema is created from the Model. Therefore we have to
        # do our normalization separately, we cannot do it in the validate method.
        std_col_names = {field.upper():field for field in
                         list(self._schema.columns.keys()) if field}

        def get_std_col_name(column_name: str):
            if(column_name.upper() in std_col_names):
                return std_col_names[column_name.upper()]
            return column_name

        return df.rename(columns=get_std_col_name)

# Each entry is indexed on a reason_code reported by Pandera, indicating the type of error encountered
_custom_messages = {
    # INVALID_TYPE - used by check_types decorator, we don't use at the moment
    Reason.DATATYPE_COERCION : _("Field '{}' could not be coerced to type {}"),
    Reason.COLUMN_NOT_IN_SCHEMA : _("Field '{}' was found on the input file but not expected."),
    # COLUMN_NOT_ORDERED - we don't require pre-sorting in any input files yet
    Reason.DUPLICATE_COLUMN_LABELS : _("Multiple fields with the label(s) {} were found but should only appear once."),
    Reason.COLUMN_NOT_IN_DATAFRAME : _("Field '{}' was not found in the data set."),
    # SCHEMA_COMPONENT_CHECK - can't seem to find when this would even get returned
    Reason.DATAFRAME_CHECK : _("Field '{}' failed check: {}"),
    Reason.CHECK_ERROR : _("Field '{}' failed check: {}"),
    # SCHEMA_COMPONENT_PARSER - we don't use parsers at the moment
    # DATAFRAME_PARSER - see above
    # PARSER_ERROR - ditto
    Reason.DUPLICATES : _("Fields {} form a composite key and are therefore required to be unique."),
    Reason.WRONG_FIELD_NAME : _("Field or index expected to have name '{}', found '{}' instead."),
    Reason.SERIES_CONTAINS_NULLS : _("Field '{}' does not allow null values."),
    Reason.SERIES_CONTAINS_DUPLICATES : _("Field '{}' should be unique but contains duplicate values."),
    Reason.WRONG_DATATYPE : _("Field '{}' expected type {} but instead got {}."),
    # NO_ERROR - Not for our use
    # ADD_MISSING_COLUMN_NO_DEFAULT - we don't add any missing columns, just report the error
    # INVALID_COLUMN_NAME - does not seem to fire, prefers COLUMN_NOT_IN_DATAFRAME
    # MISMATCH_INDEX - we do not validate any indices
}

def translate_error(err: pa.errors.SchemaError) -> str:
    """Convert Pandera errors to bilingual error messages.

    :param e: The Pandera error to translate
    :type e: pa.errors.SchemaError
    :return: A string message containing a description of the error that was translated via NLS and gettext.
    :rtype: str
    """
    custom_message = _custom_messages.get(err.reason_code)

    if custom_message:
        try:
            custom_message = err.reason_code.name + ": " + custom_message
            match err.reason_code:
                case Reason.DATATYPE_COERCION:
                    return custom_message.format(err.schema.name, str.split(err.check, "'")[1])
                case Reason.COLUMN_NOT_IN_SCHEMA | Reason.COLUMN_NOT_IN_DATAFRAME:
                    return custom_message.format(err.failure_cases)
                case Reason.DUPLICATE_COLUMN_LABELS:
                    return custom_message.format(err.failure_cases.values)
                case Reason.DUPLICATES:
                    # Only place the specific failing fields are stored is in the error message unfortunately
                    error_msg = err.args[0]
                    return custom_message.format(error_msg[error_msg.index("columns") + 9:error_msg.index("')'")+2])
                case Reason.WRONG_FIELD_NAME:
                    return custom_message.format(err.schema.name, err.failure_cases)
                case Reason.DATAFRAME_CHECK | Reason.CHECK_ERROR:
                    if(err.column_name):
                        return custom_message.format(err.column_name, err.check.error)
                    return _("DataFrame failed check: {}").format(err.check.error)
                case Reason.WRONG_DATATYPE:
                    return custom_message.format(err.column_name, str.split(err.check, "'")[1], err.failure_cases)
                case _:
                    return custom_message.format(err.column_name) if err.column_name else custom_message
        except Exception as e: #noqa: BLE001
            return str(e)
    return _("Unexpected issue found during input validation:")
