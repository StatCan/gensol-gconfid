"""The following tests are based on the file `SENSITIVITY_05_Waivers_Weights_2B.sas`
"""
import pytest
import gconfid.testing

def run_test_05_f1(indata, foo, exp_suffix, **kwargs):
    gconfid.testing.sensitiv(
        indata=indata,
        outconstraint=True,
        outcell=True,
        outlargest=True,
        hierarchy="""ZAB A B;
                     ZSTU S T U;""",
        s_rule="pq 0.2",
        unit_id="entid",
        var="value",
        dimension="grA grB",
        p_waiver=f"wafl_{foo}",

        **kwargs,

        msg_list_contains=[
            "1. Number of valid observations                                                             25",
            "b. # of valid observs from non-anonymous respondents                                     25",
            "c. # of valid observs with negative data for respondents                                  0",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=f"_2b_outcell_{exp_suffix}.sas7bdat",
        expected_outconstraint=f"_2b_outcon_{exp_suffix}.sas7bdat",
        expected_outlargest=f"_2b_outlar_{exp_suffix}.sas7bdat",
    )

def run_test_05_f2(indata, foo, exp_suffix, **kwargs):
    run_test_05_f1(
        indata,
        foo,
        exp_suffix,
        weight_prot_level="LINEAR",
        weight="poids",
        drop_columns="WeightedNbRespondents",
        **kwargs,
    )

@pytest.mark.m_auto_pass
def test_05_ex1_1(indata_05):
    """Ex1_1 : Waiver flag 1"""
    run_test_05_f1(
        indata=indata_05,
        foo="1",
        exp_suffix="1",
    )

@pytest.mark.m_auto_pass
def test_05_ex1_2(indata_05):
    """Ex1_2 : Waiver flag 1 POIDS=1"""
    run_test_05_f2(
        indata=indata_05,
        foo="1",
        exp_suffix="1",
    )

@pytest.mark.m_known_to_fail
@pytest.mark.m_fails_missing_vs_blank  # sas dataset has MISSING, python "" for grA, grB
@pytest.mark.m_auto_pass
def test_05_ex2_1(indata_05):
    """Ex2_1 : Waiver flag 2"""
    run_test_05_f1(
        indata=indata_05,
        foo="2",
        exp_suffix="2",
    )

@pytest.mark.m_known_to_fail
@pytest.mark.m_fails_missing_vs_blank  # sas dataset has MISSING, python "" for grA, grB
@pytest.mark.m_auto_pass
def test_05_ex2_2(indata_05):
    """Ex2_2 : Waiver flag 2 POIDS=1"""
    run_test_05_f2(
        indata=indata_05,
        foo="2",
        exp_suffix="2",
    )

@pytest.mark.m_known_to_fail
@pytest.mark.m_fails_missing_vs_blank  # sas dataset has MISSING, python "" for grA, grB
@pytest.mark.m_auto_pass
def test_05_ex3_1(indata_05):
    """Ex3_1 : Waiver flag 3"""
    run_test_05_f1(
        indata=indata_05,
        foo="3",
        exp_suffix="3",
    )

@pytest.mark.m_known_to_fail
@pytest.mark.m_fails_missing_vs_blank  # sas dataset has MISSING, python "" for grA, grB
@pytest.mark.m_auto_pass
def test_05_ex3_2(indata_05):
    """Ex3_2 : Waiver flag 3 POIDS=1"""
    run_test_05_f2(
        indata=indata_05,
        foo="3",
        exp_suffix="3",
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()