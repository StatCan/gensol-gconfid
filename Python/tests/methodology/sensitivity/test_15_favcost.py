"""The following tests are based on the file `SENSITIVITY_15_FavCost.sas`
"""
import pytest
import gconfid.testing

@pytest.mark.m_auto_pass
def test_15_a(indata_15):
    """Additive Noise"""
    gconfid.testing.sensitiv(
        indata=indata_15,
        outcell=True,
        outconstraint=True,
        hierarchy="All A B C; All2 Q R;",
        s_rule="pq .2",
        accept_negative=True,
        unit_id="EntID",
        var="value",
        dimension="gr1 gr2",
        p_waiver="wafl",
        weight="poids",

        rc_should_be_zero=True,
        expected_warning_count=1,

        msg_list_contains_exact="WARNING: There were 3 observations read from the indata data with negative or zeroes in variable",
        msg_list_contains=[
            "1. Number of valid observations                                                             12",
            "b. # of valid observs from non-anonymous respondents                                     12",
            "c. # of valid observs with negative data for respondents                                  2",
            "e. # of valid observs with zero or negative data for weight variable                      3",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        drop_columns="WeightedNbRespondents",
        expected_outcell="_fav_outcell_1.sas7bdat",
        expected_outconstraint="_fav_outcon_1.sas7bdat",
    )

@pytest.mark.m_auto_pass
def test_15_b(indata_15):
    """No Additive Noise"""
    gconfid.testing.sensitiv(
        indata=indata_15,
        outcell=True,
        outconstraint=True,
        hierarchy="All A B C; All2 Q R;",
        s_rule="pq .2",
        accept_negative=True,
        additive_noise=False,
        unit_id="EntID",
        var="value",
        dimension="gr1 gr2",
        p_waiver="wafl",
        weight="poids",

        rc_should_be_zero=True,
        expected_warning_count=1,

        msg_list_contains_exact="WARNING: There were 3 observations read from the indata data with negative or zeroes in variable",
        msg_list_contains=[
            "1. Number of valid observations                                                             12",
            "b. # of valid observs from non-anonymous respondents                                     12",
            "c. # of valid observs with negative data for respondents                                  2",
            "e. # of valid observs with zero or negative data for weight variable                      3",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        drop_columns="WeightedNbRespondents",
        expected_outcell="_fav_outcell_2.sas7bdat",
        expected_outconstraint="_fav_outcon_2.sas7bdat",
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()