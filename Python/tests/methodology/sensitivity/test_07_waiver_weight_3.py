"""The following tests are based on the file `SENSITIVITY_07_Waivers_Weights_3.sas`
"""
import pytest
import gconfid.testing

def run_test_07_f1(indata, foo, exp_suffix, **kwargs):
    gconfid.testing.sensitiv(
        indata=indata,
        outconstraint=True,
        outcell=True,
        hierarchy="""BZZ BXX BYY:
       BXX BX1 BX2:
       BXX B1X B2X:
       BX1 B11 B21:
       BX2 B12 B22:
       B1X B11 B12:
       B2X B21 B22;
       TR R1 R2;
       TC C1 C2;""",
        s_rule="pq .2",
        unit_id="EntID",
        var="value",
        dimension="b r c",
        p_waiver=f"{foo}",


        msg_list_contains=[
            "1. Number of valid observations                                                             80",
            "b. # of valid observs from non-anonymous respondents                                     80",
            "c. # of valid observs with negative data for respondents                                  0",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=kwargs.pop('expected_outcell', f"_w3_outcell_{exp_suffix}.sas7bdat"),
        expected_outconstraint=kwargs.pop('expected_outconstraint', f"_w3_outcon_{exp_suffix}.sas7bdat"),
        **kwargs,
    )

def run_test_07_f2(indata, foo, exp_suffix, **kwargs):
    run_test_07_f1(
        indata,
        foo,
        exp_suffix,
        weight_prot_level="LINEAR",
        weight="poids",
        drop_columns="WeightedNbRespondents",
        expected_outcell=kwargs.pop('expected_outcell', f"_w3_outcell_{exp_suffix}_v2.sas7bdat"),
        expected_outconstraint=kwargs.pop('expected_outconstraint', f"_w3_outcon_{exp_suffix}_v2.sas7bdat"),
        **kwargs,
    )

@pytest.mark.m_auto_pass
def test_07_ex1_1(indata_07):
    """Ex 1_1 : Scenario 0000"""
    run_test_07_f1(
        indata=indata_07,
        foo="sc0000",
        exp_suffix="sc0000",
    )

@pytest.mark.m_auto_pass
def test_07_ex1_2(indata_07):
    """Ex 1_2 : Scenario 0000 - POIDS=1"""
    run_test_07_f2(
        indata=indata_07,
        foo="sc0000",
        exp_suffix="sc0000",
    )

@pytest.mark.m_auto_pass
def test_07_ex2_1(indata_07):
    """Ex 2_1 : Scenario 1000"""
    run_test_07_f1(
        indata=indata_07,
        foo="sc1000",
        exp_suffix="sc1000",
    )

@pytest.mark.m_auto_pass
def test_07_ex2_2(indata_07):
    """Ex 2_1 : Scenario 1000"""
    run_test_07_f2(
        indata=indata_07,
        foo="sc1000",
        exp_suffix="sc1000",
    )

@pytest.mark.m_auto_pass
def test_07_ex3_1(indata_07):
    """Ex 3_1 : Scenario 1001"""
    run_test_07_f1(
        indata=indata_07,
        foo="sc1001",
        exp_suffix="sc1001",
    )

@pytest.mark.m_auto_pass
def test_07_ex3_2(indata_07):
    """Ex 3_2 : Scenario 1001 - POIDS=1"""
    run_test_07_f2(
        indata=indata_07,
        foo="sc1001",
        exp_suffix="sc1001",
    )

@pytest.mark.m_auto_pass
def test_07_ex4_1(indata_07):
    """Ex 4_1 : Scenario 1100"""
    run_test_07_f1(
        indata=indata_07,
        foo="sc1100",
        exp_suffix="sc1100",
    )

@pytest.mark.m_auto_pass
def test_07_ex4_2(indata_07):
    """Ex 4_2 : Scenario 1100 - POIDS=1"""
    run_test_07_f2(
        indata=indata_07,
        foo="sc1100",
        exp_suffix="sc1100",
    )

@pytest.mark.m_auto_pass
def test_07_ex5_1(indata_07):
    """Ex 5_1 : Scenario 1101"""
    run_test_07_f1(
        indata=indata_07,
        foo="sc1101",
        exp_suffix="sc1101",
    )

@pytest.mark.m_auto_pass
def test_07_ex5_2(indata_07):
    """Ex 5_2 : Scenario 1101 - POIDS=1"""
    run_test_07_f2(
        indata=indata_07,
        foo="sc1101",
        exp_suffix="sc1101",
    )

@pytest.mark.m_auto_pass
def test_07_ex6_1(indata_07):
    """Ex 6_1 : Scenario 1110"""
    run_test_07_f1(
        indata=indata_07,
        foo="sc1110",
        exp_suffix="sc1110",
    )

@pytest.mark.m_auto_pass
def test_07_ex6_2(indata_07):
    """Ex 6_2 : Scenario 1110 - POIDS=1"""
    run_test_07_f2(
        indata=indata_07,
        foo="sc1110",
        exp_suffix="sc1110",
    )

@pytest.mark.m_auto_pass
def test_07_ex7_1(indata_07):
    """Ex 7_1 : Scenario 1111"""
    run_test_07_f1(
        indata=indata_07,
        foo="sc1111",
        exp_suffix="sc1111",
    )

@pytest.mark.m_auto_pass
def test_07_ex7_2(indata_07):
    """Ex 7_2 : Scenario 1111 - POIDS=1"""
    run_test_07_f2(
        indata=indata_07,
        foo="sc1111",
        exp_suffix="sc1111",
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()