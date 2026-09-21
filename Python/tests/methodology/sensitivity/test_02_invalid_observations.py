"""The following tests are based on the file `SENSITIVITY_02_Invalid_observations.sas`
"""
import pytest
import gconfid.testing

@pytest.mark.m_auto_pass
def test_02_a(indata_02, outcell_02_a, outconstraint_02_a):
    """Negative missing or zero values in `var`"""
    gconfid.testing.sensitiv(
        indata=indata_02,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TOT A B C;",
        unit_id="entid",
        var="value",
        dimension="dimvar",

        expected_error_count=0,
        expected_warning_count=3,
        msg_list_contains_exact=[
            "WARNING: There were 1 observations dropped from the indata data set because the variable value is",
            "WARNING: These observations will not be used for sensitivity calculation",
        ],
        msg_list_contains=[
            "1. Number of valid observations                                                              7",
            "b. # of valid observs from non-anonymous respondents                                      7",
            "2. Number of valid observations with zero value                                              1",
            "3. Number of invalid observations                                                            2",
            "a. # of observs invalid due to missing data for respondents                               1",
            "b. # of observs invalid due to negative data for respondents                              1",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=outcell_02_a,
        expected_outconstraint=outconstraint_02_a
    )

@pytest.mark.m_auto_pass
def test_02_b(indata_02, outcell_02_a, outconstraint_02_a):
    """Negative missing or zero values in `var` with `accept_negative=False`
    should be equivalent (devrait être équivalent)
    """
    gconfid.testing.sensitiv(
        indata=indata_02,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TOT A B C;",
        unit_id="entid",
        var="value",
        dimension="dimvar",
        accept_negative=False,

        expected_error_count=0,
        expected_warning_count=3,
        msg_list_contains_exact=[
            "WARNING: There were 1 observations dropped from the indata data set because the variable value is",
            "WARNING: These observations will not be used for sensitivity calculation",
        ],
        msg_list_contains=[
            "1. Number of valid observations                                                              7",
            "b. # of valid observs from non-anonymous respondents                                      7",
            "2. Number of valid observations with zero value                                              1",
            "3. Number of invalid observations                                                            2",
            "a. # of observs invalid due to missing data for respondents                               1",
            "b. # of observs invalid due to negative data for respondents                              1",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=outcell_02_a,
        expected_outconstraint=outconstraint_02_a
    )

@pytest.mark.m_auto_pass
def test_02_c(indata_02, outcell_02_c, outconstraint_02_c):
    """Negative missing or zero values in `var` with `accept_negative`
        (Cette fois les négatifs devraient être acceptés mais pas les positifs)
    """
    gconfid.testing.sensitiv(
        indata=indata_02,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TOT A B C;",
        unit_id="entid",
        var="value",
        dimension="dimvar",
        accept_negative=True,

        expected_error_count=0,
        expected_warning_count=2,
        msg_list_contains_exact=[
            "WARNING: There were 1 observations dropped from the indata data set because the variable value is",
            "NOTE: There were 1 observations read from the indata data with negative values in variable value",
            "WARNING: These observations will not be used for sensitivity calculation",
        ],
        msg_list_contains=[
            "1. Number of valid observations                                                              8",
            "b. # of valid observs from non-anonymous respondents                                      8",
            "c. # of valid observs with negative data for respondents                                  1",
            "2. Number of valid observations with zero value                                              1",
            "3. Number of invalid observations                                                            1",
            "a. # of observs invalid due to missing data for respondents                               1",
            "b. # of observs invalid due to negative data for respondents                              0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=outcell_02_c,
        expected_outconstraint=outconstraint_02_c
    )

@pytest.mark.m_auto_pass
def test_02_d(indata_02_d, outcell_02_d, outconstraint_02_d):
    """Missing or invalid code in `dimension`"""
    gconfid.testing.sensitiv(
        indata=indata_02_d,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TOT A B C;",
        unit_id="entid",
        var="value",
        dimension="dimvar",

        expected_error_count=0,
        expected_warning_count=5,
        msg_list_contains_exact=[
            "WARNING: Reading microdata: Microdata file contains a code that is either a parent in the",
            "WARNING: There were 1 observations dropped from indata data set because one of the dimension",
            "WARNING: These observations will not be used for sensitivity calculation",
            "WARNING: Reading microdata: A code is missing for dimension dimvar. The observation is dropped",
        ],
        msg_list_contains=[
            "1. Number of valid observations                                                              7",
            "b. # of valid observs from non-anonymous respondents                                      7",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            2",
            "c. # of observs invalid due to missing code for dimension                                 1",
            "4. # of observs invalid because dimension code set not in hierarchy                          1",
        ],
        expected_outcell=outcell_02_d,
        expected_outconstraint=outconstraint_02_d
    )

@pytest.mark.m_needs_update  # outcell now contains `WeightedNbRespondents`
@pytest.mark.m_auto_pass
def test_02_e(indata_02_e, outcell_02_e, outconstraint_02_e):
    """Negative missing or zero values in `weight`"""
    gconfid.testing.sensitiv(
        indata=indata_02_e,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TOT A B C;",
        unit_id="entid",
        var="value",
        dimension="dimvar",
        weight="weight",

        expected_error_count=0,
        expected_warning_count=2,
        msg_list_contains_exact=[
            "WARNING: There were 1 observations read from the indata data set with missing values in variable",
            "WARNING: There were 2 observations read from the indata data with negative or zeroes in variable",
        ],
        msg_list_contains=[
            "1. Number of valid observations                                                              9",
            "b. # of valid observs from non-anonymous respondents                                      9",
            "e. # of valid observs with zero or negative data for weight variable                      2",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            1",
            "f. # of observs invalid due to missing data for weight variable                           1",
        ],
        drop_columns="WeightedNbRespondents",  # need to update control file to include this variable
        expected_outcell=outcell_02_e,
        expected_outconstraint=outconstraint_02_e
    )

@pytest.mark.m_auto_pass
def test_02_f(indata_02_f, outcell_02_f, outconstraint_02_f):
    """Negative missing or zero values in `shadow`"""
    gconfid.testing.sensitiv(
        indata=indata_02_f,
        outcell=True,
        outconstraint=True,
        s_rule="pq 0.2",
        hierarchy="TOT A B C;",
        unit_id="entid",
        var="value",
        dimension="dimvar",
        shadow="shadow",

        expected_error_count=0,
        expected_warning_count=2,
        msg_list_contains_exact=[
            "WARNING: There were 1 observations dropped from the indata data set because the variable shadow is",
            "WARNING: These observations will not be used for sensitivity calculation",
        ],
        msg_list_contains=[
            "Total number of observations                                                                 9",
            "1. Number of valid observations                                                              8",
            "b. # of valid observs from non-anonymous respondents                                      8",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            1",
            "e. # of observs invalid due to missing data for shadow variable                           1",
        ],
        expected_outcell=outcell_02_f,
        expected_outconstraint=outconstraint_02_f
    )


# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()