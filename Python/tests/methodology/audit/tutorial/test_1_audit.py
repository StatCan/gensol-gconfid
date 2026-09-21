"""Test of G-Confid Audit function."""
from pathlib import Path

import pytest

import gconfid.io_util
import gconfid.testing


@pytest.mark.m_auto_pass
def test_tutorial_no_by() -> None:
    """Test using the example from the user guide, except with no by-variables.

    The shuttle algorithm in being used. Since G-Confid 1.07.003 didn't actually used the bounds calculated by the shuttle algorithm,
    the expected value of the bounds is being updated manually to the expected value.
    """
    # We expect new bounds created by shuttle algorithm
    test_folder = Path(__file__).resolve().parent
    expected_outaudit = gconfid.io_util.DF_from_sas_file(test_folder / "expected/outaudit.sas7bdat")
    index = (expected_outaudit["CellId"] == 6.0)
    expected_outaudit.loc[index, "LBound"] = 2584.5
    expected_outaudit.loc[index, "UBound"] = 2887.5

    gconfid.testing.Auditing(
        # Inputs
        incell = "./inputdata/suppresseddata.sas7bdat",
        inconstraint = "./inputdata/inconstraints.sas7bdat",
        lb_factor = 0.5,
        ub_factor = 1.5,
        report_level = 1,
        use_shuttle=True,
        # Outputs
        expected_outaudit = expected_outaudit,
        round_data = 12,
    )

@pytest.mark.m_auto_pass
def test_tutorial_no_by_no_shuttle() -> None:
    """Test example from the user guide with no by-variables and no shuttle algorithm."""
    gconfid.testing.Auditing(
        # Inputs
        incell = "./inputdata/suppresseddata.sas7bdat",
        inconstraint = "./inputdata/inconstraints.sas7bdat",
        lb_factor = 0.5,
        ub_factor = 1.5,
        report_level = 1,
        use_shuttle=False,
        # Outputs
        expected_outaudit = "./expected/outaudit.sas7bdat",
        round_data = 12,
    )

@pytest.mark.m_auto_pass
def test_tutorial_no_by_no_shuttle_multiprocess() -> None:
    """Test example from the user guide with no by-variables and no shuttle algorithm that uses the multiprocess feature."""
    gconfid.testing.Auditing(
        # Inputs
        incell = "./inputdata/suppresseddata.sas7bdat",
        inconstraint = "./inputdata/inconstraints.sas7bdat",
        lb_factor = 0.5,
        ub_factor = 1.5,
        report_level = 1,
        use_shuttle=False,
        multiprocess=True,
        # Outputs
        expected_outaudit = "./expected/outaudit.sas7bdat",
        round_data = 12,
    )

@pytest.mark.m_auto_pass
def test_tutorial_no_by_no_shuttle_custom_solver() -> None:
    """Test using the example from the user guide but using no by-variables and a custom solver.

    :param multiprocess: Run the Audit procedure in multiprocess mode or not, defaults to False
    :type multiprocess: bool, optional
    """
    import pulp

    log_path = Path(__file__).resolve().parent / "cbc.log"
    log_path.unlink(missing_ok=True)
    my_solver = pulp.PULP_CBC_CMD(mip=False, logPath=log_path)

    gconfid.testing.Auditing(
        # Inputs
        incell = "./inputdata/suppresseddata.sas7bdat",
        inconstraint = "./inputdata/inconstraints.sas7bdat",
        lb_factor = 0.5,
        ub_factor = 1.5,
        report_level = 1,
        use_shuttle=False,
        custom_solver=my_solver,
        # Outputs
        expected_outaudit = "./expected/outaudit.sas7bdat",
        round_data = 12,
    )

    try:
        assert log_path.exists(), "No log file from custom solver was found"
    finally:
        # Cleanup the log file
        log_path.unlink(missing_ok=True)

@pytest.mark.m_auto_pass
def test_tutorial_no_by_no_shuttle_custom_solver_multi():
    """Test using the example from the user guide but using no by-variables and a custom solver."""
    import pulp

    log_path = Path(__file__).resolve().parent / "cbc.log"
    log_path.unlink(missing_ok=True)
    my_solver = pulp.PULP_CBC_CMD(mip=False, logPath=log_path)

    gconfid.testing.Auditing(
        # Inputs
        incell = "./inputdata/suppresseddata.sas7bdat",
        inconstraint = "./inputdata/inconstraints.sas7bdat",
        lb_factor = 0.5,
        ub_factor = 1.5,
        report_level = 1,
        use_shuttle=False,
        custom_solver=my_solver,
        multiprocess=True,
        # Outputs
        expected_outaudit = "./expected/outaudit.sas7bdat",
        round_data = 12,
    )

    try:
        assert log_path.exists(), "No log file from custom solver was found"
    finally:
        # Cleanup the log file
        log_path.unlink(missing_ok=True)

@pytest.mark.m_auto_pass
def test_tutorial_by_var() -> None:
    """Test using the example from the user guide.

    The upper and lower bounds are being dropped from the output when comparing with the expected output
    as the shuttle algorithm is being used here, but the bounds calculated by the shuttle algorithm were
    not actually used in G-Confid 1.07.003.
    """
    gconfid.testing.Auditing(
        # Inputs
        incell = "./inputdata/suppresseddata_byvar.sas7bdat",
        inconstraint = "./inputdata/inconstraints_byvar.sas7bdat",
        lb_factor = 0.5,
        ub_factor = 1.5,
        report_level = 1,
        by = "QuestionNumber",
        use_shuttle=True,
        # Outputs
        expected_outaudit = "./expected/outaudit_byvar.sas7bdat",
        round_data = 12,
        drop_columns=["UBound", "LBound"],
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()
