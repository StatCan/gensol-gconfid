"""The following tests are based on the file `SENSITIVITY_12_Waivers_Weights_NK.sas`
"""
import pytest
import gconfid.testing

def run_test_12_f1(indata, regle, nom, flag):
    gconfid.testing.sensitiv(
        indata=indata,
        outcell=True,
        outlargest=True,
        hierarchy="""ZABC A B C;
              ZSTUV S T U V;""",
        s_rule=regle,
        unit_id="EntID",
        var="value",
        dimension="grA grB",
        p_waiver=f"wafl_{flag}",

        expected_outcell=f"_w_nk_outcell_{nom}_{flag}_lin.sas7bdat",
    )

    # clear all captured output so far
    gconfid.testing.sensitiv(
        indata=indata,
        outcell=True,
        outlargest=True,
        hierarchy="""ZABC A B C;
              ZSTUV S T U V;""",
        s_rule=regle,
        unit_id="EntID",
        var="value",
        dimension="grA grB",
        p_waiver=f"wafl_{flag}",
        weight_prot_level="LINEAR",
        weight="poids",

        drop_columns="WeightedNbRespondents",
        expected_outcell=f"_w_nk_outcell_{nom}_{flag}_ptn.sas7bdat",
    )

@pytest.mark.m_auto_pass
def test_12_a(indata_12):
    """Règle NK160 Flag 01"""
    run_test_12_f1(
        indata=indata_12,
        regle="nk 1 60",
        nom="nk160",
        flag="01",
    )

@pytest.mark.m_auto_pass
def test_12_b(indata_12):
    """Règle NK280 Flag 01"""
    run_test_12_f1(
        indata=indata_12,
        regle="nk 2 80",
        nom="nk280",
        flag="01",
    )

@pytest.mark.m_auto_pass
def test_12_c(indata_12):
    """Règle NK160 Flag 11"""
    run_test_12_f1(
        indata=indata_12,
        regle="nk 1 60",
        nom="nk160",
        flag="11",
    )

@pytest.mark.m_auto_pass
def test_12_d(indata_12):
    """Règle NK280 Flag 11"""
    run_test_12_f1(
        indata=indata_12,
        regle="nk 2 80",
        nom="nk280",
        flag="11",
    )

@pytest.mark.m_auto_pass
def test_12_e(indata_12):
    """Règle NK160 Flag 10"""
    run_test_12_f1(
        indata=indata_12,
        regle="nk 1 60",
        nom="nk160",
        flag="10",
    )

@pytest.mark.m_auto_pass
def test_12_f(indata_12):
    """Règle NK280 Flag 10"""
    run_test_12_f1(
        indata=indata_12,
        regle="nk 2 80",
        nom="nk280",
        flag="10",
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()