"""Tests file validation using Suppress Example 1 as a base for input file contents."""
from pathlib import Path

import numpy as np
import pandera.pandas as pr
import pytest

import gconfid.testing
from gconfid.exceptions import GConfidInputDatasetValidationError
from gconfid.io_util import type_converters as tc


@pytest.fixture
def indata():
    return tc.DF_from_sas_file(Path(__file__).parent.parent / "methodology/suppress/tutorial/inputdata/in_cell.sas7bdat")

@pytest.fixture
def inconstraint():
    return tc.DF_from_sas_file(Path(__file__).parent.parent / "methodology/suppress/tutorial/inputdata/in_constraint.sas7bdat")

def run_suppress_test(indata, inconstraint, **kwargs):
    gconfid.testing.Suppression(
        incell=indata,
        inconstraint=inconstraint,
        cost_function1="Size",
        cost_function2="Information",
        by="QuestionNumber",
        round_data=12,
        **kwargs,
    )

def run_suppress_validation_test(indata, inconstraint, expected_error_msgs: list[str] | str, **kwargs):
    # Unfortunately GConfid does not use any kind of return codes, except for Sensitivity
    # Therefore a proc either succeeds or fails with an exception, so we can't use the test utils to check captured messages
    with pytest.raises((GConfidInputDatasetValidationError, GConfidInputDatasetValidationError)) as e_info:
        run_suppress_test(indata, inconstraint, **kwargs)

    if type(expected_error_msgs) is str:
        expected_error_msgs = [expected_error_msgs]
    err_msg = str(e_info.value.args[0])
    for msg in expected_error_msgs:
        assert msg in err_msg
    #assert all(msg in err_msg for msg in expected_error_msgs)

@pytest.mark.m_auto_pass
def test_suppress_validation_column_case(indata, inconstraint):
    """Ensure the columns are normalized prior to execution, allowing a field header in all-caps as long as named correctly."""
    indata = indata.rename(columns={"CellId": "CELLID"})
    run_suppress_test(indata, inconstraint)

@pytest.mark.m_auto_pass
def test_suppress_validation_str_cellid(indata, inconstraint):
    """Ensure CellId is allowed to be a string field as long as the values can be coerced to int."""
    indata["CellId"] = indata["CellId"].astype("string")
    inconstraint["CellId"] = inconstraint["CellId"].astype("string")
    run_suppress_test(indata, inconstraint)

@pytest.mark.m_auto_pass
def test_suppress_validation_cellid_non_numeric_incell(indata, inconstraint):
    """Ensure CellId column does not allow values that cannot be successfully coerced to type float64."""
    indata["CellId"] = indata["CellId"].astype("string")
    indata.loc[0, "CellId"] = "a"
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs=["incell", "Field 'CellId' could not be coerced to type float64"])

@pytest.mark.m_auto_pass
def test_suppress_validation_cellid_non_numeric_inconstraint(indata, inconstraint):
    """Ensure CellId column does not allow values that cannot be successfully coerced to type float64."""
    inconstraint["CellId"] = inconstraint["CellId"].astype("string")
    inconstraint.loc[0, "CellId"] = "a"
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs=["inconstraint", "Field 'CellId' could not be coerced to type float64"])

@pytest.mark.m_auto_pass
def test_suppress_validation_missing_cellid_incell(indata, inconstraint):
    """Ensure CellId column is present."""
    indata = indata.drop(["CellId"], axis=1)
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs=["incell", "Field 'CellId' was not found"])

@pytest.mark.m_auto_pass
def test_suppress_validation_missing_cellid_inconstraint(indata, inconstraint):
    """Ensure CellId column is present."""
    inconstraint = inconstraint.drop(["CellId"], axis=1)
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs=["inconstraint", "Field 'CellId' was not found"])

@pytest.mark.m_auto_pass
def test_suppress_validation_nan_cellid_incell(indata, inconstraint):
    """Ensure CellId has no missing/null values."""
    indata.loc[0, "CellId"] = np.nan
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs=["incell", "Field 'CellId' does not allow null values"])

@pytest.mark.m_auto_pass
def test_suppress_validation_nan_cellid_inconstraint(indata, inconstraint):
    """Ensure CellId has no missing/null values."""
    inconstraint.loc[0, "CellId"] = np.nan
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs=["inconstraint", "Field 'CellId' does not allow null values"])

@pytest.mark.m_auto_pass
def test_suppress_validation_negative_cellid_incell(indata, inconstraint):
    """Ensure CellId has no negative values."""
    indata.loc[0, "CellId"] = -1
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs=["incell", "Field 'CellId' failed check: greater_than(0)"])

@pytest.mark.m_auto_pass
def test_suppress_validation_cellid_non_unique_incell(indata, inconstraint):
    """Ensure CellId values are all unique."""
    indata["CellId"] = 1
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs=["incell", "Field 'CellId' should be unique but contains duplicate values"])

@pytest.mark.m_auto_pass
def test_suppress_validation_composite_non_unique_inconstraint(indata, inconstraint):
    """Ensure CellId values are all unique."""
    inconstraint["CellId"] = 1
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs=["inconstraint", "Fields ('ConstraintId', 'CellId') form a composite "
                                                                            "key and are therefore required to be unique"])

@pytest.mark.m_auto_pass
def test_suppress_validation_coefficient_not_all_negative(indata, inconstraint):
    """Ensure Coefficient column of inconstraint has at least 1 positive value."""
    inconstraint["Coefficient"] = -1
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs="Missing or invalid Coefficient values")

@pytest.mark.m_auto_pass
def test_suppress_validation_coefficient_not_all_positive(indata, inconstraint):
    """Ensure Coefficient column of inconstraint has at least 1 negative value."""
    inconstraint["Coefficient"] = 1
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs="Missing or invalid Coefficient values")

@pytest.mark.m_auto_pass
def test_suppress_validation_coefficient_too_many_negative(indata, inconstraint):
    """Ensure Coefficient column of inconstraint has no more than one negative 1."""
    inconstraint.loc[0, "Coefficient"] = -1
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs="Missing or invalid Coefficient values")

@pytest.mark.m_auto_pass
def test_suppress_validation_invalid_coefficient(indata, inconstraint):
    """Ensure Coefficient values are only 1 or -1."""
    inconstraint.loc[0, "Coefficient"] = 2
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs="Field 'Coefficient' failed check: isin([1, -1])")

@pytest.mark.m_auto_pass
def test_suppress_validation_constraintid_non_negative(indata, inconstraint):
    """Ensure ConstraintId column of inconstraint is not negative."""
    inconstraint.loc[0, "ConstraintId"] = -1
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs="'ConstraintId' failed check: greater_than(0)")

@pytest.mark.m_auto_pass
def test_suppress_validation_constraintid_int(indata, inconstraint):
    """Ensure ConstraintId column of inconstraint is an integer (or can be coerced to one)."""
    inconstraint.loc[0, "ConstraintId"] = 2.5
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs="Invalid value for 'ConstraintId' (expecting a positive integer).")

@pytest.mark.m_auto_pass
def test_suppress_validation_publish_sensitive(indata, inconstraint):
    """Ensure cells marked sensitive cannot also be marked to publish."""
    indata.loc[0, "Status"] = "P"
    indata.loc[0, "Sensitivity"] = 1
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs="'Status' is 'P' for a sensitive cell")

@pytest.mark.m_auto_pass
def test_suppress_validation_extra_cellids_incell(indata, inconstraint):
    """Ensure Incell does not have any cellids that Inconstraint does not have."""
    inconstraint = inconstraint[inconstraint["CellId"] != 1.0]
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs="Some cells are present in the input cells data set but "
                                 "not in the input constraints data set")

@pytest.mark.m_auto_pass
def test_suppress_validation_extra_cellids_inconstraint(indata, inconstraint):
    """Ensure Inconstraint does not have any cellids that Incell does not have."""
    indata = indata.drop(0)
    run_suppress_validation_test(indata, inconstraint, expected_error_msgs="Some cells are present in the input constraints data set but "
                                 "not in the input cells data set")

@pytest.mark.m_auto_pass
def test_suppress_validation_cvar_negative(indata, inconstraint):
    """Ensure costvar1 cannot be missing/negative if the status is 'S'/'V' and sensitivity <= 0."""
    indata.loc[0, "Status"] = "S"
    indata.loc[0, "Sensitivity"] = -1
    indata.loc[0, "TotalVar"] = -1
    run_suppress_validation_test(indata, inconstraint, cost_var1="TotalVar", expected_error_msgs="Negative or missing value for cost variable "
                                 "TotalVar detected in the input cells data set.")

@pytest.mark.m_auto_pass
def test_suppress_validation_warn_vx_positive_sens(indata, inconstraint):
    """Ensure a warning is raised when a Status of V or X is present in a cell with positive sensitivity."""
    indata.loc[0, "Status"] = "V"
    # Warnings are not catchable as they are already caught, logged and discarded by the application.
    # Therefore we can re-use the testing methods log search component via msg_list_contains
    run_suppress_test(indata, inconstraint, msg_list_contains="Some cells have a status of 'V' or 'X' and have a positive sensitivity. "
                      "Those cells will be treated as sensitive")

@pytest.mark.m_auto_pass
def test_suppress_validation_force_publish() -> None:
    """Test using the example from the user guide, some cells are forced to be published, leading to infeasiblity."""
    incell = tc.DF_from_sas_file(Path(__file__).parent.parent / "methodology/suppress/tutorial/inputdata/infeasible_outcell.sas7bdat")
    inconstraint = tc.DF_from_sas_file(Path(__file__).parent.parent / "methodology/suppress/tutorial/inputdata/infeasible_outconstraint.sas7bdat")

    # Unfortunately GConfid does not use any kind of return codes, except for Sensitivity
    # Therefore a proc either succeeds or fails with an exception, so we can't use the test utils to check captured messages
    with pytest.raises((GConfidInputDatasetValidationError, GConfidInputDatasetValidationError)) as e_info:
        gconfid.testing.Suppression(
            incell=incell,
            inconstraint=inconstraint,
            cost_function1="SIZE",
            round_data=12,
        )

    err_msg = str(e_info.value.args[0])
    assert ("A unique sensitive cell detected in a row or column with all other cells forced "
            "to be published: ConstraintId = 3.0, CellId = 1.0") in err_msg

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()
