"""Contains the data models for the expected input data tables used by the Opt_Round module."""
from typing import Optional

import pandas as pd
import pandera.pandas as pr
from pandera.typing import Series

from gconfid.nls import _
from gconfid.util.validation import IncellModel, InconstraintModel


class OptRoundIncellModel(IncellModel):
    """The data model for the Incell table used by the Opt_Round module."""

    Total: float
    Weight: float = pr.Field(coerce=True, gt=0)
    CellLB: Optional[float] = pr.Field(nullable=True) #noqa: UP045, UP007 - this format is necessary for Pandera
    CellUB: Optional[float] = pr.Field(nullable=True) #noqa: UP045, UP007

    @pr.dataframe_check(error=_("'CellLB' values must be less than or equal to their respective 'CellUB' values."))
    @classmethod
    def check_bound_relativity(cls, df: pd.DataFrame) -> Series[bool]:
        """Check that CellLB values are less than or equal to their respective CellUB values."""
        cell_lb = df.get("CellLB", None)
        cell_ub = df.get("CellUB", None)
        # If a user specifies only one bound, no need to check for relativity
        return True if cell_ub is None or cell_lb is None else bound_relative_values(cell_lb, cell_ub)

class OptRoundAdditiveBoundModel(pr.DataFrameModel):
    """The data model for the AdditiveBound table used by the Opt_Round module."""

    #InConstraints
    ConstraintId: int = pr.Field(coerce=True, unique=True) # Key column
    ConstraintLB: Optional[float] = pr.Field(nullable=True) #noqa: UP045, UP007
    ConstraintUB: Optional[float] = pr.Field(nullable=True) #noqa: UP045, UP007

    @pr.dataframe_check(error=_("'ConstraintLB' values must be less than or equal to their respective 'ConstraintUB' values."))
    @classmethod
    def check_bound_relativity(cls, df: pd.DataFrame) -> Series[bool]:
        """Check that ConstraintLB values are less than or equal to their respective ConstraintUB values."""
        try:
            return bound_relative_values(df["ConstraintLB"], df["ConstraintUB"])
        except KeyError:
            # If the columns don't exist then we can skip this check
            return True

class OptRoundAdditiveConModel(InconstraintModel):
    """The data model for the AdditiveConstraints table used by the Opt_Round module."""

def bound_relative_values(low_bound: Series[float], up_bound: Series[float]) -> Series[bool]:
    """Check that lower bound values are less than or equal to their respective upper bound values."""
    # Replace all NaN values with 0 then compare the 2 series
    # Could cause an issue if upper bound is less than 0 and lower is NaN since the comparison
    # will fail when it should succeed?
    lb_masked = low_bound.mask(low_bound.isna(), 0)
    ub_masked = up_bound.mask(up_bound.isna(), 0)

    return lb_masked.le(ub_masked)
