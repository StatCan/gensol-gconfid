"""Test of G-Confid Suppress function."""

from pathlib import Path

import pandas as pd
import pytest

import gconfid.exceptions
import gconfid.io_util
import gconfid.testing
from gconfid import log_level
from gconfid.io_util import type_converters as tc


@pytest.fixture
def indata():
    return tc.DF_from_sas_file(Path(__file__).parent / "inputdata/in_cell.sas7bdat")

@pytest.fixture
def inconstraint():
    return tc.DF_from_sas_file(Path(__file__).parent / "inputdata/in_constraint.sas7bdat")

@pytest.mark.m_auto_pass
def test_tutorial_by_vars(indata, inconstraint) -> None:
    """Test using the example from the user guide."""
    gconfid.testing.Suppression(
        incell=indata,
        inconstraint=inconstraint,
        cost_function1="Size",
        cost_function2="Information",
        by="QuestionNumber",

        expected_outsuppress="./expected/out_suppressed_data.sas7bdat",
        round_data=12,
    )

@pytest.mark.m_auto_pass
def test_tutorial_by_vars_custom_solver(indata, inconstraint) -> None:
    """Test using the example from the user guide but using a custom solver."""
    import pulp

    log_path = Path(__file__).resolve().parent / "cbc.log"
    my_solver = pulp.PULP_CBC_CMD(mip=False, logPath=log_path)

    gconfid.testing.Suppression(
        incell=indata,
        inconstraint=inconstraint,
        cost_function1="Size",
        cost_function2="Information",
        by="QuestionNumber",
        custom_solver=my_solver,

        expected_outsuppress="./expected/out_suppressed_data.sas7bdat",
        round_data=12,
    )

    try:
        assert log_path.exists(), "No log file from custom solver was found"
    finally:
        # Cleanup the log file
        log_path.unlink(missing_ok=True)

@pytest.mark.m_auto_pass
def test_tutorial_no_by_vars(indata, inconstraint) -> None:
    """Test using the example from the user guide, except with no by-variables."""
    gconfid.testing.Suppression(
        incell=indata,
        inconstraint=inconstraint,
        cost_function1="Size",
        cost_function2="Information",

        expected_outsuppress="./expected/out_suppressed_data.sas7bdat",
        round_data=12,
    )

@pytest.mark.m_known_to_fail
def test_tutorial_no_by_vars_multi(indata, inconstraint) -> None:
    """Test using the example from the user guide, except with no by-variables and multiprocess=True."""
    gconfid.testing.Suppression(
        incell=indata,
        inconstraint=inconstraint,
        cost_function1="Size",
        cost_function2="Information",
        multiprocess = True,

        expected_outsuppress="./expected/out_suppressed_data.sas7bdat",
        round_data=12,
    )

def run_tutorial_cost(cost_function1, expected_outsuppress):
    """Run a suppress test using a given `cost_function1`."""
    gconfid.testing.Suppression(
        incell="./inputdata/ex1_outcell.sas7bdat",
        inconstraint="./inputdata/ex1_outconstraint.sas7bdat",
        cost_function1=cost_function1,

        expected_outsuppress=expected_outsuppress,
        round_data=12,
    )


@pytest.mark.m_known_to_fail
def test_tutorial_constant_cost() -> None:
    """Test using the example from the user guide, testing CONSTANT cost function."""
    run_tutorial_cost(
        cost_function1="CONSTANT",
        expected_outsuppress="./expected/outpattern_constant.sas7bdat",
    )

@pytest.mark.m_auto_pass
def test_tutorial_digits_cost() -> None:
    """Test using the example from the user guide, testing DIGITS cost function."""
    run_tutorial_cost(
        cost_function1="DIGITS",
        expected_outsuppress="./expected/outpattern_digits.sas7bdat",
    )

@pytest.mark.m_auto_pass
def test_tutorial_inverse_cost() -> None:
    """Test using the example from the user guide, testing INVERSE cost function."""
    run_tutorial_cost(
        cost_function1="INVERSE",
        expected_outsuppress="./expected/outpattern_inversecost.sas7bdat",
    )

@pytest.mark.m_auto_pass
def test_tutorial_scaled_information_cost() -> None:
    """Test using the example from the user guide, testing scaled informatio cost function."""
    run_tutorial_cost(
        cost_function1="SCALEDINFORMATION",
        expected_outsuppress="./expected/outpattern_scaledinformationcost.sas7bdat",
    )

@pytest.mark.m_auto_pass
def test_tutorial_scaled_inverse_cost() -> None:
    """Test using the example from the user guide, testing scaled inverse cost function."""
    run_tutorial_cost(
        cost_function1="SCALEDINVERSE",
        expected_outsuppress="./expected/outpattern_scaledinversecost.sas7bdat",
    )

@pytest.mark.m_auto_pass
def test_infeasible() -> None:
    """Test using the example from the user guide, some cells are forced to be published, leading to infeasiblity."""
    #with pytest.raises(gconfid.exceptions.GConfidInfeasibleProblemError) as e_info:
    gconfid.testing.Suppression(
        incell="./inputdata/infeasible_outcell.sas7bdat",
        inconstraint="./inputdata/infeasible_outconstraint.sas7bdat",
        expected_outsuppress_failed="./expected/outsuppress_failed.parquet",
        cost_function1="SIZE",
        skip_validation=True, # Validation results in catching the error earlier, tested elsewhere
        round_data=12,
        msg_list_contains="Error occurred in solving the LP",
    )

@pytest.mark.m_auto_pass
def test_user_defined_cost1() -> None:
    """Test using the example from the user guide, testing user defined and specified cost function for phase 1."""
    gconfid.testing.Suppression(
        incell="./inputdata/ex2_outcell.sas7bdat",
        inconstraint="./inputdata/ex2_outconstraint.sas7bdat",
        cost_function1="SIZE",

        expected_outsuppress="./expected/ex2_outpattern.sas7bdat",
        round_data=12,
    )

@pytest.mark.m_auto_pass
def test_user_defined_cost1_and_cost2(inconstraint) -> None:
    """Test using the example from the user guide, testing user defined and specified cost function for phase 1 and phase 2."""
    gconfid.testing.Suppression(
        incell="./inputdata/in_cell2.sas7bdat",
        inconstraint=inconstraint,
        cost_function1="SIZE",
        cost_function2="Information",
        cost_var1="cost1",
        cost_var2="cost2",

        expected_outsuppress="./expected/outpattern_user_cost.sas7bdat",
        round_data=12,
    )

@pytest.mark.m_auto_pass
def test_larger_log_level() -> None:
    """Test using the example from the user guide, testing scaled inverse cost function."""
    gconfid.testing.Suppression(
        incell="./inputdata/ex2_outcell.sas7bdat",
        inconstraint="./inputdata/ex2_outconstraint.sas7bdat",
        cost_function1="SIZE",
        trace=log_level.DEBUG,

        msg_list_contains_exact=[
            "Current cell lower bound 0.4",
            "x value 0.4",
            "y value 0",
            "Current cell ABS x minus y: 0.4",
            "Current cell net variation 0.4",
            "Current cell ambiguity 0",
        ],
        round_data=12,
    )

@pytest.mark.m_auto_pass
def test_no_sensitive_cells(indata, inconstraint) -> None:
    """Test using the example from the user guide modified to produce no sensitive cells."""
    # Creating sensitivity values of 0
    indata["Sensitivity"] = 0.0

    # incell Status column is all "S" or "V", therefore the whole column becomes "P"
    out_cell_df = indata.copy()
    out_cell_df["OutStatus"] = "P"
    # Pandas 3.0 properly coerces column to str type when we assign the value above
    # but the produced dataset has the column still typed as object, so just set it manually for the test
    out_cell_df["OutStatus"] = out_cell_df["OutStatus"].astype("object")
    out_cell_df["NetVariation"] = 0.0

    gconfid.testing.Suppression(
        incell=indata,
        inconstraint=inconstraint,
        cost_function1="Size",
        cost_function2="Information",

        expected_outsuppress=out_cell_df,
        round_data=12,
    )

@pytest.mark.m_auto_pass
def test_include_skipped_cellids(indata, inconstraint) -> None:
    """Test suppress includes all CellIDs in the outsuppress, even those skipped by the LP due to a status of 'P'."""
    # a row with CellId 4.0 should be included in the output
    indata.loc[3, "Status"] = "P"
    gconfid.testing.Suppression(
        incell=indata,
        inconstraint=inconstraint,
        cost_function1="Size",
        cost_function2="Information",
        by="QuestionNumber",

        expected_outsuppress="./expected/outsuppress_include_skipped.parquet",
        round_data=12,
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()
