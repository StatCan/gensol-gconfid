"""Testing of G-Confid opt_round function."""

from pathlib import Path

import pytest

import gconfid.io_util
import gconfid.testing

# After the conversion of the procs from using Pandas to PyArrow, the expected datasets
# largely still contain an index column consisting of 0-indexed consecutive integers.
# For comparing with the default pyarrow output table OR the standard format produced by
# requesting a pandas DF explicitly we need to drop the index column ("__index_level_0__")
# or produce alternate files for every test with the index column dropped. For the sake
# of efficiency I am simply dropping the column for now, to also allow the newer test
# versions to be easier to use on older versions of the software with less changes.

@pytest.mark.m_auto_pass
def test_example1() -> None:
    """Test using example 1 from the sample programs."""
    test_folder = Path(__file__).resolve().parent
    input_folder = test_folder / "inputdata"

    gconfid.testing.OptRounding(
        # Inputs
        incell = input_folder / "cell.sas7bdat",
        additive_con = input_folder / "cellconstraints.sas7bdat",
        additive_bound = input_folder / "constraints.sas7bdat",
        base = 5,
        # Testing params
        expected_outround = test_folder / "expected" / "out_rounded_data.parq",
        round_data = 12,
        drop_columns = ["NegativeShiftIndicator", "PositiveShiftIndicator", "__index_level_0__"],
    )

@pytest.mark.m_auto_pass
def test_example1_custom_solver() -> None:
    """Test using example 1 from the sample programs but using a custom solver."""
    test_folder = Path(__file__).resolve().parent
    input_folder = test_folder / "inputdata"

    import pulp

    log_path = test_folder / "cbc.log"
    my_solver = pulp.PULP_CBC_CMD(mip=False, logPath=log_path)

    gconfid.testing.OptRounding(
        # Inputs
        incell = input_folder / "cell.sas7bdat",
        additive_con = input_folder / "cellconstraints.sas7bdat",
        additive_bound = input_folder / "constraints.sas7bdat",
        base = 5,
        custom_solver=my_solver,
        # Testing params
        expected_outround = test_folder / "expected" / "out_rounded_data.parq",
        round_data = 12,
        drop_columns = ["NegativeShiftIndicator", "PositiveShiftIndicator", "__index_level_0__"],
    )

    try:
        assert log_path.exists(), "No log file from custom solver was found"
    finally:
        # Cleanup the log file
        log_path.unlink(missing_ok=True)

@pytest.mark.m_auto_pass
def test_example1_no_bounds() -> None:
    """Test using example 1 from the sample programs."""
    test_folder = Path(__file__).resolve().parent
    input_folder = test_folder / "inputdata"

    # Removing cell bounds and weight
    in_cell_df = gconfid.io_util.DF_from_sas_file(input_folder / "cell.sas7bdat")
    in_cell_df = in_cell_df.drop(["CellUB", "CellLB", "Weight"], axis=1)

    # Removing constraint bounds
    add_bounds_df = gconfid.io_util.DF_from_sas_file(input_folder / "constraints.sas7bdat")
    add_bounds_df = add_bounds_df.drop(["ConstraintUB", "ConstraintLB"], axis=1)

    gconfid.testing.OptRounding(
        # Inputs
        incell = in_cell_df,
        additive_con = input_folder / "cellconstraints.sas7bdat",
        additive_bound = add_bounds_df,
        base = 5,
        skip_validation=True, # Weight is a mandatory column, but we removed it, so we skip validation
        # Testing params
        expected_outround = test_folder / "expected" / "out_rounded_data.parq",
        round_data = 12,
        drop_columns = ["NegativeShiftIndicator", "PositiveShiftIndicator", "__index_level_0__"],
    )

def test_example1_celllowerbound() -> None:
    """Test using example 1 from the sample programs with cell lower bound."""
    test_folder = Path(__file__).resolve().parent
    input_folder = test_folder / "inputdata"

    gconfid.testing.OptRounding(
        # Inputs
        incell = input_folder / "cell_ex1_lowerbound.sas7bdat",
        additive_con = input_folder / "cellconstraints.sas7bdat",
        additive_bound = input_folder / "constraints.sas7bdat",
        base = 5,
        # Testing params
        expected_outround = test_folder / "expected" / "output_ex1_lowerbound.parq",
        round_data = 12,
        drop_columns = ["NegativeShiftIndicator", "PositiveShiftIndicator", "ResidualIndicator"],
    )

def test_example1_cellupperlowerbound() -> None:
    """Test using example 1 from the sample programs with cell lower bound."""
    test_folder = Path(__file__).resolve().parent
    input_folder = test_folder / "inputdata"

    gconfid.testing.OptRounding(
        # Inputs
        incell = input_folder / "cell_ex1_upperlowerbound.sas7bdat",
        additive_con = input_folder / "cellconstraints.sas7bdat",
        additive_bound = input_folder / "constraints.sas7bdat",
        base = 5,
        # Testing params
        expected_outround = test_folder / "expected" / "output_ex1_upperlowerbound.parq",
        round_data = 12,
        drop_columns = ["NegativeShiftIndicator", "PositiveShiftIndicator", "ResidualIndicator"],
    )

def test_example1_cell_weight() -> None:
    """Test using example 1 from the sample programs with cell lowerbound."""
    test_folder = Path(__file__).resolve().parent
    input_folder = test_folder / "inputdata"

    gconfid.testing.OptRounding(
        # Inputs
        incell = input_folder / "cell_ex1_weight.sas7bdat",
        additive_con = input_folder / "cellconstraints.sas7bdat",
        additive_bound = input_folder / "constraints.sas7bdat",
        base = 5,
        # Testing params
        msg_list_contains_exact=[
            "The objective value is: 60.0"
        ],
        round_data = 12,
        drop_columns = ["NegativeShiftIndicator", "PositiveShiftIndicator", "ResidualIndicator"],
    )

def test_example1_cell_weight2() -> None:
    """Test using example 1 from the sample programs with non-integer weights."""
    test_folder = Path(__file__).resolve().parent
    input_folder = test_folder / "inputdata"

    gconfid.testing.OptRounding(
        # Inputs
        incell = input_folder / "cell_ex1_weight2.sas7bdat",
        additive_con = input_folder / "cellconstraints.sas7bdat",
        additive_bound = input_folder / "constraints.sas7bdat",
        base = 5,
        # Testing params
        expected_outround = test_folder / "expected" / "output_ex1_weight2.parq",
        round_data = 12,
        drop_columns = ["NegativeShiftIndicator", "PositiveShiftIndicator", "ResidualIndicator"],
    )

def test_example2_cell_3d() -> None:
    """Test using example 1 from the sample programs with 3d table."""
    test_folder = Path(__file__).resolve().parent
    input_folder = test_folder / "inputdata"

    gconfid.testing.OptRounding(
        # Inputs
        incell = input_folder / "cell_ex2_3d.sas7bdat",
        additive_con = input_folder / "cellconstraints_ex2_3d.sas7bdat",
        additive_bound = input_folder / "constraints_ex2_3d.sas7bdat",
        base = 2,
        # Testing params
        msg_list_contains_exact=[
            "The objective value is: 9.333333333333332",
        ],
        round_data = 12,
        drop_columns = ["NegativeShiftIndicator", "PositiveShiftIndicator", "ResidualIndicator"],
    )

def test_example3_infeasible() -> None:
    """Test using example 1 from the sample programs with 3d table."""
    test_folder = Path(__file__).resolve().parent
    input_folder = test_folder / "inputdata"

    gconfid.testing.OptRounding(
        # Inputs
        incell = input_folder / "cell_ex3_infeasible.sas7bdat",
        additive_con = input_folder / "cellconstraints_ex3_infeasible.sas7bdat",
        additive_bound = input_folder / "constraints_ex3_infeasible.sas7bdat",
        base = 2,
        # Testing params
        msg_list_contains_exact=[
            "Status: Infeasible",
        ],
        round_data = 12,
        drop_columns = ["NegativeShiftIndicator", "PositiveShiftIndicator", "ResidualIndicator"],
    )


# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()
