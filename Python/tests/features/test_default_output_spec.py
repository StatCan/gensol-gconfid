"""Basic test of `gconfid.set_default_output_spec()`

Ensure that the default output spec is as expected, and that modifications are effective.
"""
import gconfid
import gconfid.testing
import pandas as pd
import pyarrow as pa
import pytest

# Remember that both the global and proc-level defaults are set on the class itself,
# so they will retain the last set value between each test function here when ran in sequence

@pytest.mark.m_auto_pass
def test_sensitiv_default_output_spec_a(indata):
    """Check for correct default output type"""

    test_call = gconfid.testing.sensitiv(
        indata=indata,
        outlargest=True,
        hierarchy="0 1 2; 0 1 2 3;",
        s_rule="pq .15",
        code_range=";1 101 201 301: 2 102 202 302: 3 103 203 303;",
        unit_id="Key",
        var="income",
        dimension="Prov Naics",
        by="QuestionNumber",
    )
    assert isinstance(test_call.call.outcell, pa.Table)

@pytest.mark.m_auto_pass
def test_sensitiv_default_output_spec_b(indata):
    """Try modifying global default"""
    orig = gconfid.get_default_output_spec()
    try:
        gconfid.set_default_output_spec("pandas")
        test_call = gconfid.testing.sensitiv(
            indata=indata,
            outlargest=True,
            hierarchy="0 1 2; 0 1 2 3;",
            s_rule="pq .15",
            code_range=";1 101 201 301: 2 102 202 302: 3 103 203 303;",
            unit_id="Key",
            var="income",
            dimension="Prov Naics",
            by="QuestionNumber",
        )
        assert isinstance(test_call.call.outcell, pd.DataFrame)
    finally:
        # Make sure we reset the default output spec just in case
        gconfid.set_default_output_spec(orig)

@pytest.mark.m_auto_pass
def test_sensitiv_default_output_spec_c(indata):
    """Try modifying proc-level default"""
    orig = gconfid.get_default_output_spec()
    try:
        gconfid.sensitiv.set_default_output_format("pyarrow")
        test_call = gconfid.testing.sensitiv(
            indata=indata,
            outlargest=True,
            hierarchy="0 1 2; 0 1 2 3;",
            s_rule="pq .15",
            code_range=";1 101 201 301: 2 102 202 302: 3 103 203 303;",
            unit_id="Key",
            var="income",
            dimension="Prov Naics",
            by="QuestionNumber",
        )
        assert isinstance(test_call.call.outcell, pa.Table)
    finally:
        # Make sure we reset the default output spec just in case
        gconfid.set_default_output_spec(orig)

@pytest.mark.m_auto_pass
def test_sensitiv_default_output_spec_d():
    """Try setting to invalid specification"""
    try:
        gconfid.set_default_output_spec("fake spec")
    except ValueError:
        pass
    else:
        raise AssertionError("Exception should occur when invalid spec provided")


@pytest.fixture
def indata():
    return gconfid.testing.PAT_from_string("""
        n s n s s 
        QuestionNumber	Key	Income	Prov	Naics
        1	111	101	1	101
        1	211	101	2	101
        1	121	202	1	202
        1	131	103	1	103
        1	132	203	1	203
        1	133	303	1	303
        2	111	101	1	101
        2	211	101	2	101
        2	121	202	1	202
        2	131	103	1	103
        2	132	203	1	203
        2	133	303	1	303
        3	111	101	1	101
        3	211	101	2	101
        3	121	202	1	202
        3	131	103	1	103
        3	132	203	1	203
        3	133	303	1	303
    """)

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()