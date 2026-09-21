"""The following tests are based on the file `SENSITIVITY_08_Waivers_Weights_5.sas`
"""
import pytest
import gconfid.testing

def run_test_08_fa(indata, foo, **kwargs):
    gconfid.testing.sensitiv(
        indata=indata,
        outconstraint=True,
        outcell=True,
        hierarchy="Allgrp S T U;",
        s_rule="pq .2",
        unit_id="EntID",
        var="value",
        dimension="grp",
        p_waiver=f"wafl_{foo}",


        msg_list_contains=[
            "1. Number of valid observations                                                             12",
            "b. # of valid observs from non-anonymous respondents                                     12",
            "c. # of valid observs with negative data for respondents                                  0",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=kwargs.pop('expected_outcell', f"_w5_outcell_5a_{foo}.sas7bdat"),
        expected_outconstraint=kwargs.pop('expected_outconstraint', f"_w5_outcon_5a_{foo}.sas7bdat"),
        **kwargs,
    )

def run_test_08_fb(indata, foo, **kwargs):
    run_test_08_fa(
        indata,
        foo,
        code_range="S SA SB: T TA TB: U UA UB;",
        expected_outcell=kwargs.pop('expected_outcell', f"_w5_outcell_5b_{foo}.sas7bdat"),
        expected_outconstraint=kwargs.pop('expected_outconstraint', f"_w5_outcon_5b_{foo}.sas7bdat"),
        **kwargs,
    )

def run_test_08_fa1(indata, foo, **kwargs):
    run_test_08_fa(
        indata,
        foo,
        weight_prot_level="LINEAR",
        weight="poids",
        drop_columns="WeightedNbRespondents",
        expected_outcell=kwargs.pop('expected_outcell', f"_w5_outcell_5a_{foo}_v2.sas7bdat"),
        expected_outconstraint=kwargs.pop('expected_outconstraint', f"_w5_outcon_5a_{foo}_v2.sas7bdat"),
        **kwargs,
    )

def run_test_08_fb1(indata, foo, **kwargs):
    run_test_08_fa(
        indata,
        foo,
        code_range="S SA SB: T TA TB: U UA UB;",
        weight_prot_level="LINEAR",
        weight="poids",
        drop_columns="WeightedNbRespondents",
        expected_outcell=kwargs.pop('expected_outcell', f"_w5_outcell_5b_{foo}_v2.sas7bdat"),
        expected_outconstraint=kwargs.pop('expected_outconstraint', f"_w5_outcon_5b_{foo}_v2.sas7bdat"),
        **kwargs,
    )

@pytest.mark.m_auto_pass
def test_08_exa1_1(indata_08_a):
    """Ex A1_1 : Fichier A Flag 1"""
    run_test_08_fa(
        indata=indata_08_a,
        foo="1",
    )

@pytest.mark.m_auto_pass
def test_08_exa1_2(indata_08_a):
    """Ex A1_2 : Fichier A Flag 1 POIDS=1"""
    run_test_08_fa1(
        indata=indata_08_a,
        foo="1",
    )

@pytest.mark.m_known_to_fail
@pytest.mark.m_fails_missing_vs_blank  # sas dataset has MISSING, python "" for grA, grB
@pytest.mark.m_auto_pass
def test_08_exa2_1(indata_08_a):
    """Ex A2_1 : Fichier A Flag 2"""
    run_test_08_fa(
        indata=indata_08_a,
        foo="2",
    )

@pytest.mark.m_known_to_fail
@pytest.mark.m_fails_missing_vs_blank  # sas dataset has MISSING, python "" for grA, grB
@pytest.mark.m_auto_pass
def test_08_exa2_2(indata_08_a):
    """Ex A2_2 : Fichier A Flag 2 POIDS=1"""
    run_test_08_fa1(
        indata=indata_08_a,
        foo="2",
    )

@pytest.mark.m_known_to_fail
@pytest.mark.m_fails_missing_vs_blank  # sas dataset has MISSING, python "" for grA, grB
@pytest.mark.m_auto_pass
def test_08_exa3_1(indata_08_a):
    """Ex A3_1 : Fichier A Flag 3"""
    run_test_08_fa(
        indata=indata_08_a,
        foo="3",
    )

@pytest.mark.m_known_to_fail
@pytest.mark.m_fails_missing_vs_blank  # sas dataset has MISSING, python "" for grA, grB
@pytest.mark.m_auto_pass
def test_08_exa3_2(indata_08_a):
    """Ex A3_2 : Fichier A Flag 3 POIDS=1"""
    run_test_08_fa1(
        indata=indata_08_a,
        foo="3",
    )

@pytest.mark.m_auto_pass
def test_08_exb1_1(indata_08_b):
    """Ex B1_1 : Fichier B Flag 1"""
    run_test_08_fb(
        indata=indata_08_b,
        foo="1",
    )

@pytest.mark.m_auto_pass
def test_08_exb1_2(indata_08_b):
    """Ex B1_2 : Fichier B Flag 1 POIDS=1"""
    run_test_08_fb1(
        indata=indata_08_b,
        foo="1",
    )

@pytest.mark.m_known_to_fail
@pytest.mark.m_fails_missing_vs_blank  # sas dataset has MISSING, python "" for grA, grB
@pytest.mark.m_auto_pass
def test_08_exb2_1(indata_08_b):
    """Ex B2_1 : Fichier B Flag 2"""
    run_test_08_fb(
        indata=indata_08_b,
        foo="2",
    )

@pytest.mark.m_known_to_fail
@pytest.mark.m_fails_missing_vs_blank  # sas dataset has MISSING, python "" for grA, grB
@pytest.mark.m_auto_pass
def test_08_exb2_2(indata_08_b):
    """Ex B2_2 : Fichier B Flag 2 POIDS=1"""
    run_test_08_fb1(
        indata=indata_08_b,
        foo="2",
    )

@pytest.mark.m_known_to_fail
@pytest.mark.m_fails_missing_vs_blank  # sas dataset has MISSING, python "" for grA, grB
@pytest.mark.m_auto_pass
def test_08_exb3_1(indata_08_b):
    """Ex B3_1 : Fichier B Flag 3"""
    run_test_08_fb(
        indata=indata_08_b,
        foo="3",
    )

@pytest.mark.m_known_to_fail
@pytest.mark.m_fails_missing_vs_blank  # sas dataset has MISSING, python "" for grA, grB
@pytest.mark.m_auto_pass
def test_08_exb3_2(indata_08_b):
    """Ex B3_2 : Fichier B Flag 3 POIDS=1"""
    run_test_08_fb1(
        indata=indata_08_b,
        foo="3",
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()