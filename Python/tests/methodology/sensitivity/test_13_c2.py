"""The following tests are based on the file `SENSITIVITY_13_C2.sas`
"""
import pytest
import gconfid.testing

def run_test_13(
        indata,
        hierarchy,
        outcell=None,
        outconstraint=None,
        outlargest=None,
        **kwargs,
):
    """Helper functions for running c2 tests"""
    gconfid.testing.sensitiv(
        indata=indata,
        outcell=True,
        outconstraint=True,
        outlargest=True,
        s_rule="c2",
        hierarchy=hierarchy,
        **kwargs,
        unit_id="ENTID",
        var="datavalue",
        dimension="VARIABLE",

        expected_outcell=outcell,
        expected_outconstraint=outconstraint,
        expected_outlargest=outlargest,
    )

@pytest.mark.m_auto_pass
@pytest.mark.m_internal_build_only
def test_13_01a(indata_13_c2_01):
    """01a negative with noadditivenoise, ignoring waiver"""
    run_test_13(
        indata=indata_13_c2_01,
        hierarchy="G0 G1 G2 G3;",
        accept_negative=True,
        additive_noise=False,

        outcell=      "_c2_outcell_01a.sas7bdat",
        outconstraint= "_c2_outcon_01a.sas7bdat",
        outlargest="_c2_outlargest_01a.sas7bdat",
    )

@pytest.mark.m_external_build_only
def test_13_01a_external(indata_13_c2_01):
    """For external builds, C2 should fail with an error message"""
    run_test_13(
        indata=indata_13_c2_01,
        hierarchy="G0 G1 G2 G3;",
        accept_negative=True,
        additive_noise=False,
        expected_error_count=1,
        msg_list_contains_exact="ERROR: Sensitivity rule parser: C2 is not supported",
        rc_should_be_zero=False,
    )

@pytest.mark.m_auto_pass
@pytest.mark.m_internal_build_only
def test_13_01b(indata_13_c2_01):
    """01b negative with no additive noise and waiver"""
    run_test_13(
        indata=indata_13_c2_01,
        hierarchy="G0 G1 G2 G3;",
        accept_negative=True,
        additive_noise=False,
        p_waiver="waiverflag",

        outcell=      "_c2_outcell_01b.sas7bdat",
        outconstraint= "_c2_outcon_01b.sas7bdat",
        outlargest="_c2_outlargest_01b.sas7bdat",
    )

@pytest.mark.m_auto_pass
@pytest.mark.m_internal_build_only
def test_13_02a(indata_13_c2_02):
    """02a default proxypercentile=0.1"""
    run_test_13(
        indata=indata_13_c2_02,
        hierarchy="G0 G1 G2 G3;",
        accept_negative=True,
        proxy_size="proxyvar",

        outcell=      "_c2_outcell_02a.sas7bdat",
        outconstraint= "_c2_outcon_02a.sas7bdat",
        outlargest="_c2_outlargest_02a.sas7bdat",
    )

@pytest.mark.m_auto_pass
@pytest.mark.m_internal_build_only
def test_13_02b(indata_13_c2_02):
    """02b with proxyratio - same input file with renamed output files"""
    run_test_13(
        indata=indata_13_c2_02,
        hierarchy="G0 G1 G2 G3;",
        accept_negative=True,
        proxy_ratio=0.2,
        proxy_size="proxyvar",

        outcell=      "_c2_outcell_02b.sas7bdat",
        outconstraint= "_c2_outcon_02b.sas7bdat",
        outlargest="_c2_outlargest_02b.sas7bdat",
    )

@pytest.mark.m_auto_pass
@pytest.mark.m_internal_build_only
def test_13_03a(indata_13_c2_03):
    """03a negative with proxypercentile=0.3"""
    run_test_13(
        indata=indata_13_c2_03,
        hierarchy="D0 D1 D2 D3 D4;",
        accept_negative=True,
        proxy_percentile=0.3,
        proxy_size="proxyvar",

        outcell=      "_c2_outcell_03a.sas7bdat",
        outconstraint= "_c2_outcon_03a.sas7bdat",
        outlargest="_c2_outlargest_03a.sas7bdat",
    )

@pytest.mark.m_auto_pass
@pytest.mark.m_internal_build_only
def test_13_03b(indata_13_c2_03):
    """03b negative with proxypercentile=0.4"""
    run_test_13(
        indata=indata_13_c2_03,
        hierarchy="D0 D1 D2 D3 D4;",
        accept_negative=True,
        proxy_percentile=0.4,
        proxy_size="proxyvar",

        outcell=      "_c2_outcell_03b.sas7bdat",
        outconstraint= "_c2_outcon_03b.sas7bdat",
        outlargest="_c2_outlargest_03b.sas7bdat",
    )

@pytest.mark.m_auto_pass
@pytest.mark.m_internal_build_only
def test_13_03c(indata_13_c2_03):
    """03c negative with proxypercentile=0.5"""
    run_test_13(
        indata=indata_13_c2_03,
        hierarchy="D0 D1 D2 D3 D4;",
        accept_negative=True,
        proxy_percentile=0.5,
        proxy_size="proxyvar",

        outcell=      "_c2_outcell_03c.sas7bdat",
        outconstraint= "_c2_outcon_03c.sas7bdat",
        outlargest="_c2_outlargest_03c.sas7bdat",
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
@pytest.mark.m_internal_build_only
def test_13_04a(indata_13_c2_04):
    """04a weight with LINEAR"""
    run_test_13(
        indata=indata_13_c2_04,
        hierarchy="H0 H1 H2 H3 H4 H5;",
        weight_prot_level="LINEAR",
        weight="estimweight",

        outcell=      "_c2_outcell_04a.sas7bdat",
        outconstraint= "_c2_outcon_04a.sas7bdat",
        outlargest="_c2_outlargest_04a.sas7bdat",
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
@pytest.mark.m_internal_build_only
def test_13_04b(indata_13_c2_04):
    """04b weight with STEP"""
    run_test_13(
        indata=indata_13_c2_04,
        hierarchy="H0 H1 H2 H3 H4 H5;",
        weight_prot_level="STEP",
        weight="estimweight",

        outcell=      "_c2_outcell_04b.sas7bdat",
        outconstraint= "_c2_outcon_04b.sas7bdat",
        outlargest="_c2_outlargest_04b.sas7bdat",
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
@pytest.mark.m_internal_build_only
def test_13_05a(indata_13_c2_05):
    """5a negative values, NOADDITIVENOISE and STEP"""
    run_test_13(
        indata=indata_13_c2_05,
        hierarchy="G0 G1 G2 G3;",
        accept_negative=True,
        additive_noise=False,
        weight_prot_level="STEP",
        weight="estimweight",

        outcell=      "_c2_outcell_05a.sas7bdat",
        outconstraint= "_c2_outcon_05a.sas7bdat",
        outlargest="_c2_outlargest_05a.sas7bdat",
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
@pytest.mark.m_internal_build_only
def test_13_05b(indata_13_c2_05):
    """5b negative values, NOADDITIVENOISE and LINEAR"""
    run_test_13(
        indata=indata_13_c2_05,
        hierarchy="G0 G1 G2 G3;",
        accept_negative=True,
        additive_noise=False,
        weight_prot_level="LINEAR",
        weight="estimweight",

        outcell=      "_c2_outcell_05b.sas7bdat",
        outconstraint= "_c2_outcon_05b.sas7bdat",
        outlargest="_c2_outlargest_05b.sas7bdat",
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
@pytest.mark.m_internal_build_only
def test_13_05c(indata_13_c2_05):
    """5c negative values, NOADDITIVENOISE and LOW"""
    run_test_13(
        indata=indata_13_c2_05,
        hierarchy="G0 G1 G2 G3;",
        accept_negative=True,
        additive_noise=False,
        weight_prot_level="LOW",
        weight="estimweight",

        outcell=      "_c2_outcell_05c.sas7bdat",
        outconstraint= "_c2_outcon_05c.sas7bdat",
        outlargest="_c2_outlargest_05c.sas7bdat",
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
@pytest.mark.m_internal_build_only
def test_13_06a(indata_13_c2_06, outcell_13_c2_06a):
    """6 negative values with NOADDITIVENOISE, PROXYRATIO and WEIGHT"""
    gconfid.testing.sensitiv(
        indata=indata_13_c2_06,
        outcell=True,
        outconstraint=True,
        outlargest=True,
        s_rule="c2",
        hierarchy="D0 D1 D2 D3 D4 D5;",
        accept_negative=True,
        proxy_ratio=0.2,
        weight_prot_level="LINEAR",
        proxy_size="proxyvar",
        weight="estimweight",
        round_data=12,
        unit_id="entid", # dif from other methods, otherwise could use run_test_13
        var="datavalue",
        dimension="VARIABLE",

        expected_outcell=outcell_13_c2_06a, #"_c2_outcell_06.sas7bdat",
        expected_outconstraint="_c2_outcon_06.sas7bdat",
        expected_outlargest="_c2_outlargest_06.sas7bdat",
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
@pytest.mark.m_internal_build_only
def test_13_07a(indata_13_c2_07):
    """7 to check an issue with v.1.07.001"""
    run_test_13(
        indata=indata_13_c2_07,
        hierarchy="D0 D1 D2 D3 D4 D5 D6;",
        accept_negative=True,
        additive_noise=False,
        weight="estimweight",
        p_waiver="waiverflag",

        outcell=      "_c2_outcell_07.sas7bdat",
        outconstraint= "_c2_outcon_07.sas7bdat",
        outlargest="_c2_outlargest_07.sas7bdat",
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
    )


# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()