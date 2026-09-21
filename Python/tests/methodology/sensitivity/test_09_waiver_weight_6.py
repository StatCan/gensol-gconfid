"""The following tests are based on the file `SENSITIVITY_09_Waivers_Weights_6.sas`
"""
import pytest
import gconfid.testing

@pytest.mark.m_known_to_fail
@pytest.mark.m_fails_missing_vs_blank  # sas dataset has MISSING, python "" for grA, grB
@pytest.mark.m_auto_pass
def test_09_a(indata_09):
    """Sans les poids"""
    gconfid.testing.sensitiv(
        indata=indata_09,
        outconstraint=True,
        outcell=True,
        hierarchy="Allgrp A B C D; All2 Q R;",
        s_rule="pq .2",
        unit_id="EntID",
        var="value",
        dimension="gr1 gr2",
        p_waiver=f"wafl",


        msg_list_contains=[
            "1. Number of valid observations                                                             12",
            "b. # of valid observs from non-anonymous respondents                                     12",
            "c. # of valid observs with negative data for respondents                                  0",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell="_w6_outcell_6.sas7bdat",
        expected_outconstraint="_w6_outcon_6.sas7bdat",
    )

@pytest.mark.m_known_to_fail
@pytest.mark.m_fails_missing_vs_blank  # sas dataset has MISSING, python "" for grA, grB
@pytest.mark.m_auto_pass
def test_09_b(indata_09):
    """Avec les poids de 1"""
    gconfid.testing.sensitiv(
        indata=indata_09,
        outconstraint=True,
        outcell=True,
        hierarchy="Allgrp A B C D; All2 Q R;",
        s_rule="pq .2",
        unit_id="EntID",
        var="value",
        dimension="gr1 gr2",
        p_waiver=f"wafl",
        weight_prot_level="LINEAR",
        weight="poids",


        msg_list_contains=[
            "1. Number of valid observations                                                             12",
            "b. # of valid observs from non-anonymous respondents                                     12",
            "c. # of valid observs with negative data for respondents                                  0",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        drop_columns="WeightedNbRespondents",
        expected_outcell="_w6_outcell_6_v2.sas7bdat",
        expected_outconstraint="_w6_outcon_6_v2.sas7bdat",
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()