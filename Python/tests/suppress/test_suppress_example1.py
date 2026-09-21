"""Test suppression using example 1."""
import pytest

import gconfid.testing


@pytest.mark.m_auto_pass
def test_suppress_01():
    gconfid.testing.Suppression(
        incell="in_cell_example1.sas7bdat",
        inconstraint="in_constraint_example1.sas7bdat",
        cost_function1="Size",
        cost_function2="Information",
        by="QuestionNumber",

        expected_outsuppress="out_suppressed_data_example1.sas7bdat",
        round_data=13,
        msg_list_contains_exact=[
            "Total number of cell suppressed: 21",
            "Total value amount suppressed: 5463.0",
        ],
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()
