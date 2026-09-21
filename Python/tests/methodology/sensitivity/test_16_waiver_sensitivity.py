"""The following tests were provided to test a new feature when all responses in a cell have waivers.
"""
import pytest
import gconfid.testing

def run_test_16_waiver_sensitivity(indata, s_rule, **kwargs):
    gconfid.testing.sensitiv(
        indata=indata,
        outconstraint=True,
        outcell=True,
        outlargest=True,
        hierarchy="TOT_INDUSTRY A B; TOT_REGION 1 2 3;",
        s_rule=s_rule,
        unit_id="Entid",
        var="value",
        dimension="Industry Region",
        p_waiver="wafl_4",

        **kwargs,
    )

def run_test_16_1a(indata, s_rule, **kwargs):
    run_test_16_waiver_sensitivity(
        indata,
        s_rule,
        **kwargs,
    )

@pytest.mark.m_auto_pass
def test_16_ex1(indata_16):
    """Tests a new feature: if # of responses < minresp but all responses have a True waiver value, do not mark the cell as sensitive."""
    run_test_16_1a(
        indata=indata_16,
        s_rule="pq 0.15",
        min_resp = 6,
        msg_list_contains=[
            "Internal cells                                  6                    5                83.33",
            "Marginal cells                                  6                    0                 0.00",
            "All cells                                      12                    5                41.67",
            "Aggregates                                      1                    0                 0.00",
            "Total                                          13                    5                38.46",
        ],
    )

@pytest.mark.m_auto_pass
def test_16_ex2(indata_16):
    """Control for new feature: no minresp given and all have waiver, so expect success and no sensitive values."""
    run_test_16_1a(
        indata=indata_16,
        s_rule="pq 0.15",
        msg_list_contains=[
            "Internal cells                                  6                    0               0.00",
            "Marginal cells                                  6                    0               0.00",
            "All cells                                      12                    0               0.00",
            "Total                                          12                    0               0.00",
        ],
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()
