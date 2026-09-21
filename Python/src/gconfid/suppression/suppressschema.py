"""Contains the data models for the expected input data tables used by the Suppress module."""
import pandas as pd
import pandera.pandas as pr
from pandera.typing import Series

from gconfid.nls import _
from gconfid.util.validation import IncellModel, InconstraintModel


class SuppressIncellModel(IncellModel):
    """The data model for the Incell table used by the Sensitivity module."""

    TotalNoise: float = pr.Field(gt=0, nullable=False)
    Sensitivity: float
    Status: str
    Type: str

#region WARNINGS
    @pr.dataframe_check(raise_warning=True, error=_("Some cells have a status of 'V' or 'X' and have a positive sensitivity. "
                                                    "Those cells will be treated as sensitive."))
    @classmethod
    def warn_variable_status_sensitive(cls, df: pd.DataFrame) -> Series[bool]:
        """Check that Cell Status isn't variable or X while Sensitivity > 0."""
        return ~(df["Status"].str.upper().isin(["V", "X"]) & df["Sensitivity"].gt(0))

    @pr.dataframe_check(raise_warning=True, error=_("Some cells have a status of 'S' and have a non positive sensitivity. "
                                                    "Those cells will be treated as non-sensitive."))
    @classmethod
    def warn_variable_status_sens_conflict(cls, df: pd.DataFrame) -> Series[bool]:
        """Check that Cell Status isn't sensitive while Sensitivity <= 0."""
        return ~(df["Status"].str.upper().isin(["S"]) & df["Sensitivity"].le(0))

    # We already print a Warning for this when the replacement is performed, should we pre-print here too?
    @pr.dataframe_check(raise_warning=True, error=_("Sensitivity is greater than TotalNoise, will be considered equal to TotalNoise."))
    @classmethod
    def warn_sens_gt_total(cls, df: pd.DataFrame) -> Series[bool]:
        """Check that Sensitivity is less than or equal to TotalNoise."""
        return ~(df["Sensitivity"].gt(df["TotalNoise"]))
#endregion
#region ERRORS
    @pr.dataframe_check(error=_("'Status' is 'P' for a sensitive cell."))
    @classmethod
    def check_publish_sensitive(cls, df: pd.DataFrame) -> Series[bool]:
        """Check that sensitive cells haven't been marked to publish."""
        return ~(df["Status"].str.upper().isin(["P"]) & df["Sensitivity"].gt(0))

    @pr.check("Status", error=_("'Status' field must be one of ['P','S','X','V']."))
    @classmethod
    def status_allowed_vals(cls, series: Series[str]) -> Series[bool]:
        """Check that Status is 'P' (published), 'S' (sensitive), 'X' (suppressed) or 'V' (other)."""
        return series.str.upper().isin(["P", "S", "X", "V"])

    @pr.check("Type", error=_("'Type' field must be one of ['C','A']."))
    @classmethod
    def type_allowed_vals(cls, series: Series[str]) -> Series[bool]:
        """Check that Type is 'C' (table cell) or 'A' (aggregate)."""
        return series.str.upper().isin(["C", "A"])
#endregion

    @classmethod
    def to_schema(cls, by_vars: str | None, cost_var1: str | None, cost_var2: str | None) -> pr.DataFrameSchema:
        """Create a `pandera.DataFrameSchema` from the `pandera.DataFrameModel`.

        :param by_vars: The by variable string containing the by vars for this module separated by spaces.
        :type by_vars: str | None
        :param cost_var1: The cost_var1 field name parameter.
        :type cost_var1: str | None
        :param cost_var2: The cost_var1 field name parameter.
        :type cost_var2: str | None
        :return: The DataFrameModel as converted to a DataFrameSchema.
        :rtype: pr.DataFrameSchema
        """
        # We shouldn't need to create a deep copy here as the IndataModel superclass already makes us a copy
        schema = super().to_schema(by_vars)

        # It's possible our user-supplied vars point to required columns already on the schema, so check first
        schema_columns = [col.upper() for col in schema.columns.keys()]

        #cost_var columns are typed to float w/coerce=True for a general way of checking if numeric type
        for cvar in [cost_var1, cost_var2]:
            if(cvar and cvar.upper() not in schema_columns):
                schema = schema.add_columns({cvar: pr.Column(dtype=float, coerce=True)})

                # Cvar must not be negative or missing IF Status = V or S AND Sensitivity <= 0
                # See Suppress/Parms.sas:ValidateCVar() for SAS error check
                def cvar_check(df: pd.DataFrame, costvar=cvar) -> Series[bool]:
                    return ~(df["Status"].str.upper().isin(["V", "S"]) & df["Sensitivity"].le(0) & (df[costvar].lt(0) | df[costvar].isna()))

                schema.checks.append(pr.Check(cvar_check, error=_("Negative or missing value for cost variable {} detected in the input cells data set.",
                                                                  ).format(cvar)))

        return schema

class SuppressInconstraintModel(InconstraintModel):
    """The data model for the Inconstraint table used by the Sensitivity module."""

    @pr.check("Coefficient", groupby="ConstraintId", error=_("Missing or invalid Coefficient values"))
    @classmethod
    def coefficient_check(cls, grouped_value: dict[str, Series[float]]) -> bool:
        """Check that the Coefficient has the correct values in each ConstraintId group."""
        # Iterate over each group
        for conid, coeffs in grouped_value.items():
            # Get the count of each coefficient value
            counts = coeffs.value_counts()
            # Each group must contain at least one +1 and exactly one -1
            if(1.0 not in counts):
                raise pr.errors.SchemaError(cls.to_schema(), data=grouped_value, message=_("A constraint must have at least one coefficient equal to 1. "
                                                                                           "ConstraintId = {}").format(conid))
            elif(-1.0 not in counts): #noqa: RET506
                raise pr.errors.SchemaError(cls.to_schema(), data=grouped_value, message=_("A constraint has no -1 coefficient. Exactly one such "
                                                                                           "coefficient is expected. ConstraintId = {}").format(conid))
            elif(counts.loc[-1.0] > 1):
                raise pr.errors.SchemaError(cls.to_schema(), data=grouped_value, message=_("A constraint has more than one -1 coefficient. Exactly one "
                                                                                           "such coefficient is expected. ConstraintId = {}").format(conid))
        return True
