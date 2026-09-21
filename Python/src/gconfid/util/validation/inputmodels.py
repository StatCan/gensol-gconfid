"""Input data file common data models and helper methods."""
import copy

import numpy as np
import pandera.pandas as pr
from pandera.typing import Series

from gconfid.nls import _

##
#region   COMMON PANDERA CHECK METHODS
##

# We coerce to float and then check for int to allow strings like "1.0" which are not coercible directly to int.
# As well, if we coerce to int then a float like 2.5 will be coerced to 2 and then this check will pass, despite
# the value being truncated. So coerce to float then check that it has no decimals
def field_is_int(cls, series: Series[object]) -> Series[bool]:
    """Check that the values in `series` do not have any digits in the decimal place."""
    return np.floor(series) == np.ceil(series)

cell_id_int = pr.check("CellId", error=_("CellId must be a positive integer or able to be coerced to one."))(field_is_int)

#endregion

##
#region PANDERA BASE MODELS
##

class IndataModel(pr.DataFrameModel):
    """Base model of an input data file. Any common fields or constraints should go here."""

    @classmethod
    def to_schema(cls, by_vars: str | None = None) -> pr.DataFrameSchema:
        """Create a `pandera.DataFrameSchema` from the `pandera.DataFrameModel`.

        :param by_vars: The by variable string containing the by vars for this module separated by spaces.
        :type by_vars: str
        :return: The DataFrameModel as converted to a DataFrameSchema.
        :rtype: pr.DataFrameSchema
        """
        # We need to create a deep copy here, as Pandera caches the last schema created for this model
        # on the current thread. If we don't, when pytest runs tests in sequence, any mutable attributes
        # that are modified on one schema are modifying the cached version and therefore are present
        # every subsuquent time you get the schema.
        schema = copy.deepcopy(super().to_schema())

        if(by_vars):
            # It's possible our user-supplied vars point to required columns already on the schema, so check first
            schema_columns = [col.upper() for col in schema.columns.keys()]
            new_cols = [field for field in by_vars.split() if field.upper() not in schema_columns]
            # add_columns creates a deep copy of the schema, so don't run it if we don't have to
            if(new_cols):
                schema = schema.add_columns(map_fields_to_pandera(new_cols))

        return schema

    class Config:
        """Custom configuration for all Inconstraint models."""

        # We should probably set the pandas option to explicitly disallow this in our init, but put the check here too anyways
        unique_column_names = True

class IncellModel(IndataModel):
    """Base model of an input cell data file. Any common fields or constraints should go here."""

    CellId: float = pr.Field(coerce=True, gt=0, unique=True)

    check_cellid = cell_id_int

class InconstraintModel(IndataModel):
    """Base model of an input cell constraint data file. Any common fields or constraints should go here."""

    CellId: float = pr.Field(coerce=True, gt=0)
    ConstraintId: float = pr.Field(coerce=True, gt=0)
    Coefficient: int = pr.Field(isin=[1,-1], coerce=True, nullable=False)

    check_cellid = cell_id_int
    check_constraintid = pr.check("ConstraintId", error=_("Invalid value for 'ConstraintId' (expecting a positive integer)."))(field_is_int)

    class Config:
        """Custom configuration for all Inconstraint models."""

        # ConstraintId and CellId form a composite key
        unique=["ConstraintId", "CellId"]

#endregion

def map_fields_to_pandera(fields: str | list[str] | None, dtype: type | None = None) -> dict[str, pr.Column]:
    """Form a mapping of field names in a space-separated string to generic, nullable, required pandera column elements.

    :param fields: The string containg the space-separated list of field names to map.
    :type fields: str | list[str] | None
    :return: A mapping of each name in `fields` to a generic, nullable Pandera.Column if `dtype` is None.
        If `dtype` is str the field is also str. If `dtype` is int or float the field is float with coerce=True.
        If `fields` is empty, returns an empty dict.
    :rtype: dict[str, pr.Column]
    """
    by_var_fields = {}

    if(fields):
        if(isinstance(fields, str)):
            fields = fields.split()
        for field in fields:
            if(field):
                if(dtype in [int, float]):
                    by_var_fields[field] = pr.Column(dtype=float, coerce=True, nullable=True)
                elif(dtype is str):
                    by_var_fields[field] = pr.Column(dtype=str, nullable=True)
                else:
                    by_var_fields[field] = pr.Column(nullable=True)
    return by_var_fields
