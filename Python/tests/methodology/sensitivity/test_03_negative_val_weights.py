"""The following tests are based on the file `SENSITIVITY_03_Negative_val_Weights_EDCDS.sas`
"""
import pytest
import gconfid.testing

@pytest.mark.m_auto_pass
def test_03_ex1_a(indata_03_a, outcell_03_a, outconstraint_03_a):
    """Ex1 : ACCEPTNEGATIVE ADDITIVENOISE"""
    gconfid.testing.sensitiv(
        indata=indata_03_a,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TOT A B C;",
        unit_id="entid",
        accept_negative=True,
        #additive_noise=True,  # NOTE: this was disabled in the original test case too
        var="value",
        dimension="dimvar",

        msg_list_contains=[
            "1. Number of valid observations                                                              9",
            "b. # of valid observs from non-anonymous respondents                                      9",
            "c. # of valid observs with negative data for respondents                                  3",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=outcell_03_a,
        expected_outconstraint=outconstraint_03_a
    )

@pytest.mark.m_auto_pass
def test_03_ex1_b(indata_03_a, outcell_03_b, outconstraint_03_a):
    """Ex1 : ACCEPTNEGATIVE NOADDITIVENOISE"""
    gconfid.testing.sensitiv(
        indata=indata_03_a,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TOT A B C;",
        unit_id="entid",
        accept_negative=True,
        additive_noise=False,
        var="value",
        dimension="dimvar",

        msg_list_contains=[
            "1. Number of valid observations                                                              9",
            "b. # of valid observs from non-anonymous respondents                                      9",
            "c. # of valid observs with negative data for respondents                                  3",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=outcell_03_b,
        expected_outconstraint=outconstraint_03_a
    )

@pytest.mark.m_auto_pass
def test_03_ex2_1(indata_03_c, outcell_03_c, outconstraint_03_c):
    """Ex2_1 : PROXY proxyratio PQ"""
    gconfid.testing.sensitiv(
        indata=indata_03_c,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="ALLREV REV_A REV_B REV_C;",
        accept_negative=True,
        proxy_ratio=0.4,
        unit_id="respid",
        var="datavalue",
        dimension="VARIABLE",
        proxy_size="proxy_datavalue",

        msg_list_contains=[
            "1. Number of valid observations                                                              9",
            "b. # of valid observs from non-anonymous respondents                                      9",
            "c. # of valid observs with negative data for respondents                                  2",
            "d. # of valid observs with negative or missing data for proxy variable                    6",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=outcell_03_c,
        expected_outconstraint=outconstraint_03_c
    )

@pytest.mark.m_auto_pass
def test_03_ex2_2a(indata_03_d, outcell_03_d, outconstraint_03_d):
    """Ex2_2 : PROXY proxypercentile VS proxyratio PQ A"""
    gconfid.testing.sensitiv(
        indata=indata_03_d,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="ALLREV REV_A REV_B;",
        accept_negative=True,
        proxy_percentile=0.4,
        unit_id="respid",
        var="datavalue",
        dimension="VARIABLE",
        proxy_size="proxy_datavalue",

        msg_list_contains_exact="ProxyRatio (calculated from ProxyPercentile =    0.40000) =    0.08000",
        msg_list_contains=[
            "1. Number of valid observations                                                             20",
            "b. # of valid observs from non-anonymous respondents                                     20",
            "c. # of valid observs with negative data for respondents                                  0",
            "d. # of valid observs with negative or missing data for proxy variable                    0",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=outcell_03_d,
        expected_outconstraint=outconstraint_03_d,
        round_data=14,
    )

@pytest.mark.m_auto_pass
def test_03_ex2_2b(indata_03_d, outcell_03_d, outconstraint_03_d):
    """Ex2_2 : PROXY proxypercentile VS proxyratio PQ B"""
    gconfid.testing.sensitiv(
        indata=indata_03_d,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="ALLREV REV_A REV_B;",
        accept_negative=True,
        proxy_ratio=0.08,
        unit_id="respid",
        var="datavalue",
        dimension="VARIABLE",
        proxy_size="proxy_datavalue",

        msg_list_contains=[
            "1. Number of valid observations                                                             20",
            "b. # of valid observs from non-anonymous respondents                                     20",
            "c. # of valid observs with negative data for respondents                                  0",
            "d. # of valid observs with negative or missing data for proxy variable                    0",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=outcell_03_d,
        expected_outconstraint=outconstraint_03_d,
        round_data=14,
    )

@pytest.mark.m_auto_pass
def test_03_ex2_3(indata_03_f, outcell_03_f, outconstraint_03_a):
    """Ex2_3 : PROXY proxyratio NK"""
    gconfid.testing.sensitiv(
        indata=indata_03_f,
        outcell=True,
        outconstraint=True,
        s_rule="NK 2 80",
        hierarchy="ALLREV REV_A REV_B REV_C;",
        accept_negative=True,
        proxy_ratio=0.4,
        unit_id="respid",
        var="datavalue",
        dimension="VARIABLE",
        proxy_size="proxy_datavalue",

        msg_list_contains=[
            "1. Number of valid observations                                                              9",
            "b. # of valid observs from non-anonymous respondents                                      9",
            "c. # of valid observs with negative data for respondents                                  2",
            "d. # of valid observs with negative or missing data for proxy variable                    6",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=outcell_03_f,
        expected_outconstraint=outconstraint_03_a,
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
def test_03_ex3_1(indata_03_g, outcell_03_g, outconstraint_03_g):
    """Ex3_1 : WEIGHT LINEAR PQ"""
    gconfid.testing.sensitiv(
        indata=indata_03_g,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TOT D1 D2 D3 D4;",
        weight_prot_level="LINEAR",
        unit_id="entid",
        var="datavalue",
        dimension="VARIABLE",
        weight="estimweight",

        msg_list_contains=[
            "1. Number of valid observations                                                             12",
            "b. # of valid observs from non-anonymous respondents                                     12",
            "c. # of valid observs with negative data for respondents                                  0",
            "d. # of valid observs with negative or missing data for proxy variable                    0",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
        expected_outcell=outcell_03_g,
        expected_outconstraint=outconstraint_03_g,
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
def test_03_ex3_2(indata_03_h, outcell_03_h, outconstraint_03_h):
    """Ex3_2 : WEIGHT STEP PQ"""
    gconfid.testing.sensitiv(
        indata=indata_03_h,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TOT D3 D4 D5 D6 D7 D8;",
        weight_prot_level="STEP",
        unit_id="entid",
        var="datavalue",
        dimension="VARIABLE",
        weight="estimweight",

        msg_list_contains=[
            "1. Number of valid observations                                                             10",
            "b. # of valid observs from non-anonymous respondents                                     10",
            "c. # of valid observs with negative data for respondents                                  0",
            "d. # of valid observs with negative or missing data for proxy variable                    0",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
        expected_outcell=outcell_03_h,
        expected_outconstraint=outconstraint_03_h,
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
def test_03_ex3_3(indata_03_i, outcell_03_i, outconstraint_03_i):
    """Ex3_3 : WEIGHT lt 1 et 1 RESP par CAT LINEAR PQ"""
    gconfid.testing.sensitiv(
        indata=indata_03_i,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TOT E1 E2 E3 E4 E5 E6 E7 E8;",
        weight_prot_level="LINEAR",
        unit_id="entid",
        var="datavalue",
        dimension="VARIABLE",
        weight="estimweight",

        expected_warning_count=1,
        msg_list_contains_exact="WARNING: There were 5 observations read from the indata data with negative or zeroes in variable",
        msg_list_contains=[
            "1. Number of valid observations                                                             12",
            "b. # of valid observs from non-anonymous respondents                                     12",
            "c. # of valid observs with negative data for respondents                                  0",
            "d. # of valid observs with negative or missing data for proxy variable                    0",
            "e. # of valid observs with zero or negative data for weight variable                      5",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
        expected_outcell=outcell_03_i,
        expected_outconstraint=outconstraint_03_i,
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
def test_03_ex3_4(indata_03_j, outcell_03_j, outconstraint_03_j):
    """Ex3_4 : WEIGHT lt 1 et 2 RESP par CAT LINEAR PQ"""
    gconfid.testing.sensitiv(
        indata=indata_03_j,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TF F1 F2 F3 F4 F5;",
        weight_prot_level="LINEAR",
        unit_id="entid",
        var="datavalue",
        dimension="VARIABLE",
        weight="estimweight",

        expected_warning_count=1,
        msg_list_contains_exact="WARNING: There were 1 observations read from the indata data with negative or zeroes in variable",
        msg_list_contains=[
            "1. Number of valid observations                                                             10",
            "b. # of valid observs from non-anonymous respondents                                     10",
            "c. # of valid observs with negative data for respondents                                  0",
            "d. # of valid observs with negative or missing data for proxy variable                    0",
            "e. # of valid observs with zero or negative data for weight variable                      1",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
        expected_outcell=outcell_03_j,
        expected_outconstraint=outconstraint_03_j,
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
def test_03_ex3_5(indata_03_k, outcell_03_k, outconstraint_03_k):
    """Ex3_5 : WEIGHT lt 1 et 1 RESP par CAT with 3 OBS LINEAR PQ"""
    gconfid.testing.sensitiv(
        indata=indata_03_k,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TG G1 G2 G3 G4 G5;",
        weight_prot_level="LINEAR",
        unit_id="entid",
        var="datavalue",
        dimension="VARIABLE",
        weight="estimweight",

        expected_warning_count=1,
        msg_list_contains_exact="WARNING: There were 2 observations read from the indata data with negative or zeroes in variable",
        msg_list_contains=[
            "1. Number of valid observations                                                             15",
            "b. # of valid observs from non-anonymous respondents                                     15",
            "c. # of valid observs with negative data for respondents                                  0",
            "d. # of valid observs with negative or missing data for proxy variable                    0",
            "e. # of valid observs with zero or negative data for weight variable                      2",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
        expected_outcell=outcell_03_k,
        expected_outconstraint=outconstraint_03_k,
        round_data=13,
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
def test_03_ex3_6a(indata_03_l, outcell_03_l, outconstraint_03_k):
    """Ex3_6 : MINRESPW A"""
    gconfid.testing.sensitiv(
        indata=indata_03_l,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TG G1 G2 G3 G4 G5;",
        weight_prot_level="LINEAR",
        min_resp_w=5,
        unit_id="entid",
        var="datavalue",
        dimension="VARIABLE",
        weight="estimweight",

        expected_warning_count=1,
        msg_list_contains_exact="WARNING: There were 1 observations read from the indata data with negative or zeroes in variable",
        msg_list_contains=[
            "1. Number of valid observations                                                             14",
            "b. # of valid observs from non-anonymous respondents                                     14",
            "c. # of valid observs with negative data for respondents                                  0",
            "d. # of valid observs with negative or missing data for proxy variable                    0",
            "e. # of valid observs with zero or negative data for weight variable                      1",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
        expected_outcell=outcell_03_l,
        expected_outconstraint=outconstraint_03_k,
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
def test_03_ex3_6b(indata_03_l, outcell_03_m, outconstraint_03_k):
    """Ex3_6 : MINRESPW B"""
    gconfid.testing.sensitiv(
        indata=indata_03_l,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TG G1 G2 G3 G4 G5;",
        weight_prot_level="LINEAR",
        unit_id="entid",
        var="datavalue",
        dimension="VARIABLE",
        weight="estimweight",

        expected_warning_count=1,
        msg_list_contains_exact="WARNING: There were 1 observations read from the indata data with negative or zeroes in variable",
        msg_list_contains=[
            "1. Number of valid observations                                                             14",
            "b. # of valid observs from non-anonymous respondents                                     14",
            "c. # of valid observs with negative data for respondents                                  0",
            "d. # of valid observs with negative or missing data for proxy variable                    0",
            "e. # of valid observs with zero or negative data for weight variable                      1",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
        expected_outcell=outcell_03_m,
        expected_outconstraint=outconstraint_03_k,
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
def test_03_ex3_7(indata_03_n, outcell_03_m, outconstraint_03_k):
    """Ex3_7 : VARIOUS WEIGHTS GRAPH STEP PQ"""

    # generate hierarchy string: "TOT SP1 SP2 ... SP500 SN1 SN2 ... SN500;"
    hier="TOT"
    for i in range (1,501):
        hier = f"{hier} SP{i}"
    for i in range (1,501):
        hier = f"{hier} SN{i}"
    
    hier=f"{hier};"

    gconfid.testing.sensitiv(
        indata=indata_03_n,  #"edcds_ex3_7.sas7bdat",
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy=hier,
        weight_prot_level="STEP",
        unit_id="entid",
        var="datavalue",
        dimension="VARIABLE",
        weight="estimweight",

        expected_warning_count=1,
        msg_list_contains_exact="WARNING: There were 500 observations read from the indata data with negative or zeroes in variable",
        msg_list_contains=[
            "1. Number of valid observations                                                           1000",
            "b. # of valid observs from non-anonymous respondents                                   1000",
            "c. # of valid observs with negative data for respondents                                  0",
            "d. # of valid observs with negative or missing data for proxy variable                    0",
            "e. # of valid observs with zero or negative data for weight variable                    500",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        drop_columns=["WeightedNbRespondents", "Poids"],  # need to update control file to include this variable
        expected_outcell="edcds_ex3_7_outcell_exp.sas7bdat",
        expected_outconstraint="edcds_ex3_7_outcon_exp.sas7bdat",
        round_data=12,
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
def test_03_ex3_8(indata_03_o, outcell_03_o, outconstraint_03_o):
    """Ex3_8 : WEIGHT lt 1 LINEAR NK"""
    gconfid.testing.sensitiv(
        indata=indata_03_o,
        outcell=True,
        outconstraint=True,
        s_rule="NK 2 80",
        hierarchy="TOT E01 E02 E03 E04 E05 E06 E07 E08 E09 E10 E11 E12 E13 E14 E15 E16 E17 E18 E19 E20 E21 E22 E23 E24;",
        weight_prot_level="LINEAR",
        unit_id="entid",
        var="datavalue",
        dimension="VARIABLE",
        weight="estimweight",

        expected_warning_count=1,
        msg_list_contains_exact="WARNING: There were 9 observations read from the indata data with negative or zeroes in variable",
        msg_list_contains=[
            "1. Number of valid observations                                                             39",
            "b. # of valid observs from non-anonymous respondents                                     39",
            "c. # of valid observs with negative data for respondents                                  0",
            "d. # of valid observs with negative or missing data for proxy variable                    0",
            "e. # of valid observs with zero or negative data for weight variable                      9",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
        expected_outcell=outcell_03_o,
        expected_outconstraint=outconstraint_03_o,
    )

def run_test_ex4(exp_outcell, exp_outcon, **kwargs):
    gconfid.testing.sensitiv(
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.15",
        hierarchy="TOT DT ET FT WEST: DT D1 D2 D3: ET E1 E2 E3: FT F1 F2 F3: WEST BC AB SK MB;",
        unit_id="entid",
        var="datavalue",
        dimension="VARIABLE",
        **kwargs,

        msg_list_contains_exact="NOTE: There were 2 observations read from the indata data with negative values in variable",
        msg_list_contains=[
            "1. Number of valid observations                                                             35",
            "b. # of valid observs from non-anonymous respondents                                     35",
            "c. # of valid observs with negative data for respondents                                  2",
            "d. # of valid observs with negative or missing data for proxy variable                    0",
            "e. # of valid observs with zero or negative data for weight variable                      0",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=exp_outcell,
        expected_outconstraint=exp_outcon,
    )

@pytest.mark.m_auto_pass
def test_03_ex4_1(indata_03_p, outcell_03_p, outconstraint_03_p):
    """Ex4_1 : AGGREGATES default"""
    run_test_ex4(
        indata=indata_03_p,

        accept_negative=True,

        exp_outcell=outcell_03_p,
        exp_outcon=outconstraint_03_p,
        round_data=13,
    )

@pytest.mark.m_auto_pass
def test_03_ex4_2(indata_03_p, outcell_03_q, outconstraint_03_q):
    """Ex4_2 : AGGREGATES noadditivenoise"""
    run_test_ex4(
        indata=indata_03_p,

        accept_negative=True,
        additive_noise=False,

        exp_outcell=outcell_03_q,
        exp_outcon=outconstraint_03_q,
        round_data=13,
    )

@pytest.mark.m_auto_pass
def test_03_ex4_3(indata_03_p, outcell_03_r, outconstraint_03_r):
    """Ex4_3 : AGGREGATES pwaiver"""
    run_test_ex4(
        indata=indata_03_p,

        accept_negative=True,
        p_waiver="waiverflag",

        exp_outcell=outcell_03_r,
        exp_outcon=outconstraint_03_r,
        round_data=13,
    )

@pytest.mark.m_auto_pass
def test_03_ex4_4(indata_03_p, outcell_03_s, outconstraint_03_s):
    """Ex4_4 : AGGREGATES noadditivenoise pwaiver"""
    run_test_ex4(
        indata=indata_03_p,

        accept_negative=True,
        additive_noise=False,
        p_waiver="waiverflag",

        exp_outcell=outcell_03_s,
        exp_outcon=outconstraint_03_s,
        round_data=13,
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
def test_03_ex4_5(indata_03_p, outcell_03_t, outconstraint_03_t):
    """Ex4_5 : AGGREGATES weight LINEAR"""
    run_test_ex4(
        indata=indata_03_p,

        accept_negative=True,
        weight_prot_level="LINEAR",
        weight="estimweight",

        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
        exp_outcell=outcell_03_t,
        exp_outcon=outconstraint_03_t,
        round_data=11,
    )

@pytest.mark.m_needs_update
@pytest.mark.m_auto_pass
def test_03_ex4_6(indata_03_p, outcell_03_u, outconstraint_03_u):
    """Ex4_6 : AGGREGATES weight STEP"""
    run_test_ex4(
        indata=indata_03_p,

        accept_negative=True,
        weight_prot_level="STEP",
        weight="estimweight",

        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
        exp_outcell=outcell_03_u,
        exp_outcon=outconstraint_03_u,
        round_data=12,
    )


# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()