"""Contains the data models for the expected input data tables used by the Audit module."""
import pandera.pandas as pr
from pandera.typing import Series

from gconfid.nls import _
from gconfid.util.validation import IncellModel, InconstraintModel


class AuditIncellModel(IncellModel):
    """The data model for the Incell table used by the Audit module."""

    TotalNoise: float
    Sensitivity: float
    OutStatus: str = pr.Field(nullable=False)

    @pr.check("OutStatus", error=_("'OutStatus' field must be one of ['P','X']."))
    @classmethod
    def outstatus_allowed_vals(cls, series: Series[str]) -> Series[bool]:
        """Check that OutStatus is 'P' (for published) or 'X' (for suppressed)."""
        return series.str.upper().isin(["P", "X"])

class AuditInconstraintModel(InconstraintModel):
    """The data model for the Inconstraints table used by the Audit module."""
