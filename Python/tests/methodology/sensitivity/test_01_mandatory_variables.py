"""The following tests are based on the file `SENSITIVITY_01_Mandatory_variables.sas`
"""
import pytest
import gconfid.testing

@pytest.mark.m_auto_pass
def test_01_a(indata_01):
    """`unit_id` variable is mandatory, error should occur when not specified"""
    gconfid.testing.sensitiv(
        indata=indata_01,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TOT A B C;",
        #unit_id="entid",
        var="value",
        dimension="dimvar",

        rc_should_be_zero=False,
        expected_error_count=1,
        msg_list_contains_exact="ERROR: unit_id is mandatory"
    )

@pytest.mark.m_auto_pass
def test_01_b(indata_01):
    """`var` variable is mandatory, error should occur when not specified"""
    gconfid.testing.sensitiv(
        indata=indata_01,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TOT A B C;",
        unit_id="entid",
        #var="value",
        dimension="dimvar",

        rc_should_be_zero=False,
        expected_error_count=1,
        msg_list_contains_exact="ERROR: var is mandatory"
    )

@pytest.mark.m_auto_pass
def test_01_c(indata_01):
    """`dimension` variable is mandatory, error should occur when not specified"""
    gconfid.testing.sensitiv(
        indata=indata_01,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TOT A B C;",
        unit_id="entid",
        var="value",
        #dimension="dimvar",

        rc_should_be_zero=False,
        expected_error_count=2,
        msg_list_contains_exact=[
            "ERROR: The number of dimensions (1) in hierarchy statement does not match the number of variables (0) in the dimension statement",
            "ERROR: dimension is mandatory",
        ]
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()