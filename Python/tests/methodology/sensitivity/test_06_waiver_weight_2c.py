"""The following tests are based on the file `SENSITIVITY_06_Waivers_Weights_2C.sas`
"""
import pytest
import gconfid.testing

def run_test_06_f1(indata, foo, exp_suffix, **kwargs):
    gconfid.testing.sensitiv(
        indata=indata,
        outconstraint=True,
        outcell=True,
        hierarchy="""ZABC A B C;
                     ZSTUV S T U V;""",
        s_rule="nk 1 60 2 80",
        unit_id="EntID",
        var="value",
        dimension="grA grB",
        p_waiver=f"wafl_{foo}",


        msg_list_contains=[
            "1. Number of valid observations                                                             24",
            "b. # of valid observs from non-anonymous respondents                                     24",
            "c. # of valid observs with negative data for respondents                                  0",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=kwargs.pop('expected_outcell', f"_2c_outcell_{exp_suffix}.sas7bdat"),
        expected_outconstraint=kwargs.pop('expected_outconstraint', f"_2c_outcon_{exp_suffix}.sas7bdat"),
        **kwargs,
    )

def run_test_06_f2(indata, foo, exp_suffix, **kwargs):
    run_test_06_f1(
        indata,
        foo,
        exp_suffix,
        weight_prot_level="LINEAR",
        weight="poids",
        drop_columns="WeightedNbRespondents",
        expected_outcell=kwargs.pop('expected_outcell', f"_2c_outcell_{exp_suffix}_v2.sas7bdat"),
        expected_outconstraint=kwargs.pop('expected_outconstraint', f"_2c_outcon_{exp_suffix}_v2.sas7bdat"),
        **kwargs,
    )

@pytest.mark.m_auto_pass
def test_06_ex1_1(indata_06):
    """Ex1_1 : Waiver flag 1"""
    run_test_06_f1(
        indata=indata_06,
        foo="01",
        exp_suffix="01",
    )

@pytest.mark.m_auto_pass
def test_06_ex1_2(indata_06):
    """Ex1_2 : Waiver flag 1 POIDS=1"""
    run_test_06_f2(
        indata=indata_06,
        foo="01",
        exp_suffix="01",
    )

@pytest.mark.m_auto_pass
def test_06_ex2_1(indata_06):
    """Ex2_1 : Waiver flag 2"""
    run_test_06_f1(
        indata=indata_06,
        foo="10",
        exp_suffix="10",
    )

@pytest.mark.m_auto_pass
def test_06_ex2_2(indata_06):
    """Ex2_2 : Waiver flag 2 POIDS=1"""
    run_test_06_f2(
        indata=indata_06,
        foo="10",
        exp_suffix="10",
    )

@pytest.mark.m_auto_pass
def test_06_ex3_1(indata_06):
    """Ex3_1 : Waiver flag 3"""
    run_test_06_f1(
        indata=indata_06,
        foo="11",
        exp_suffix="11",
    )

@pytest.mark.m_auto_pass
def test_06_ex3_2(indata_06):
    """Ex3_2 : Waiver flag 3 POIDS=1"""
    run_test_06_f2(
        indata=indata_06,
        foo="11",
        exp_suffix="11",
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()