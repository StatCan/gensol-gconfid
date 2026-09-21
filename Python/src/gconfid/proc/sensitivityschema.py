"""Contains the data models for the expected input data tables used by the Sensitivity module."""
import pandera.pandas as pr

from gconfid.util.validation import IndataModel, map_fields_to_pandera


class SensitivityInDataModel(IndataModel):
    """The data model for the InData table used by the Sensitivity module."""

    @classmethod
    def to_schema(cls, by_vars: str | None, dimension: str | None, char_fields: list[str] | None, num_fields: list[str] | None) -> pr.DataFrameSchema:
        """Create `pandera.DataFrameSchema` from the `SensitivityInDataModel`.

        :param by_vars: The by variable string containing the desired by groups seperated by spaces.
        :type by_vars: str | None
        :param by_vars: The dimension variable string containing the desired dimension fields seperated by spaces.
        :type by_vars: str | None
        :param char_fields: Names of fields that should be of character (`str`) type.
        :type char_fields: list[str] | None
        :param num_fields: Names of fields that should be of numeric (`int | float`) type.
        :type num_fields: list[str] | None
        :return: The `SensitivityInDataModel` in the form of a `pandera.DataFrameSchema`
        :rtype: pr.DataFrameSchema
        """
        # We shouldn't need to create a deep copy here as the IndataModel superclass already makes us a copy
        schema = super().to_schema(by_vars)

        # It's possible our user-supplied vars point to required columns already on the schema, so check first
        schema_columns = [col.upper() for col in schema.columns.keys()]

        field_dict = {}
        if(dimension and dimension.upper() not in schema_columns):
            field_dict.update(map_fields_to_pandera(dimension, dtype=str))
        if(char_fields):
            field_dict.update(map_fields_to_pandera([field for field in char_fields if field.upper() not in schema_columns], dtype=str))
        if(num_fields):
            field_dict.update(map_fields_to_pandera([field for field in num_fields if field.upper() not in schema_columns], dtype=int))

        if(field_dict):
            return schema.add_columns(field_dict)
        return schema
