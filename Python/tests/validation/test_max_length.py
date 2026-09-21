"""Tests of maximum variable length allowed by C code
C code accepts parameters in which users specify the names of variables which appear in one or more
input datasets.  It also accepts names of "codes".  The length of the names must be equal to or less than the maximum
allowed length, otherwise an error should occur.  

To implement these tests, a user guide example was used to create a function which
allows the caller to define the length of various variables used in the procedure.  The function then 
generates input and expected output datasets and parameters accordingly and runs the procedure via `gconfid.testing`.  
"""
import gconfid.testing
import pytest

# maximum allowed variable (column) name length by C code
var_name_max_len=64
code_max_len=100

def repeat_to_length(string_to_expand, length=None):
    """repeat a string until it reaches a certain length"""
    if length is None:
        length=len(string_to_expand)

    return (string_to_expand * (int(length/len(string_to_expand))+1))[:length]

def run_max_len_sensitiv(
        id_len=None,
        var_len=None,
        dim1_len=None,
        by_len=None,
        code_201_len=None,
        code_203_len=None,
        code_0_len=None,
        **kwargs,
):
    """Function for testing variable lengths in sensitivity procedure"""
    id_name = repeat_to_length("Key", id_len)
    var_name = repeat_to_length("income", var_len)
    dim1_name = repeat_to_length("Prov", dim1_len)
    by_name = repeat_to_length("QuestionNumber", by_len)
    _code_201_name = repeat_to_length("201", code_201_len)
    _code_203_name = repeat_to_length("203", code_203_len)
    _code_0_name = repeat_to_length("0", code_0_len)

    indata = gconfid.testing.PAT_from_string(f"""
        n s n s s 
        {by_name}	{id_name}	{var_name}	{dim1_name}	Naics
        1	111	101	1	101
        1	211	101	2	101
        1	121	202	1	202
        1	131	103	1	103
        1	132	{_code_203_name}	1	{_code_203_name}
        1	133	303	1	303
        2	111	101	1	101
        2	211	101	2	101
        2	121	202	1	202
        2	131	103	1	103
        2	132	{_code_203_name}	1	{_code_203_name}
        2	133	303	1	303
        3	111	101	1	101
        3	211	101	2	101
        3	121	202	1	202
        3	131	103	1	103
        3	132	{_code_203_name}	1	{_code_203_name}
        3	133	303	1	303
    """)

    outcell = gconfid.testing.PAT_from_string(f"""
        n n n n n n s s n s s n
        CellId	NbAnonym	AnonymTotalVar	NbRespondents	TotalVar	Sensitivity	Status	Type	TotalNoise	{dim1_name}	Naics	{by_name}
        1	0	0	1	101	    15.149999999999991	S	C	101	    1	1	1
        2	0	0	1	101	    15.149999999999991	S	C	101	    2	1	1
        3	0	0	1	202	    30.299999999999983	S	C	202	    1	2	1
        4	0	0	3	609	    -57.55	            V	C	609	    1	3	1
        5	0	0	6	1013	-461.55	            V	C	1013	{_code_0_name}	{_code_0_name}	1
        6	0	0	5	912	    -360.55	            V	C	912	    1	{_code_0_name}	1
        7	0	0	1	101	    15.149999999999991	S	C	101	    2	{_code_0_name}	1
        8	0	0	2	202	    15.149999999999991	S	C	202	    {_code_0_name}	1	1
        9	0	0	1	202	    30.299999999999983	S	C	202	    {_code_0_name}	2	1
        10	0	0	3	609	    -57.55	            V	C	609	    {_code_0_name}	3	1
        11	0	0	2	303	    30.299999999999983	S	A	303	    ''	''	1
        12	0	0	1	101	    15.149999999999991	S	C	101	    1	1	2
        13	0	0	1	101	    15.149999999999991	S	C	101	    2	1	2
        14	0	0	1	202	    30.299999999999983	S	C	202	    1	2	2
        15	0	0	3	609	    -57.55	            V	C	609	    1	3	2
        16	0	0	6	1013	-461.55	            V	C	1013	{_code_0_name}	{_code_0_name}	2
        17	0	0	5	912	    -360.55	            V	C	912	    1	{_code_0_name}	2
        18	0	0	1	101	    15.149999999999991	S	C	101	    2	{_code_0_name}	2
        19	0	0	2	202	    15.149999999999991	S	C	202	    {_code_0_name}	1	2
        20	0	0	1	202	    30.299999999999983	S	C	202	    {_code_0_name}	2	2
        21	0	0	3	609	    -57.55	            V	C	609	    {_code_0_name}	3	2
        22	0	0	2	303	    30.299999999999983	S	A	303	    ''	''	2
        23	0	0	1	101	    15.149999999999991	S	C	101	    1	1	3
        24	0	0	1	101	    15.149999999999991	S	C	101	    2	1	3
        25	0	0	1	202	    30.299999999999983	S	C	202	    1	2	3
        26	0	0	3	609	    -57.55	            V	C	609	    1	3	3
        27	0	0	6	1013	-461.55	            V	C	1013	{_code_0_name}	{_code_0_name}	3
        28	0	0	5	912	    -360.55	            V	C	912	    1	{_code_0_name}	3
        29	0	0	1	101	    15.149999999999991	S	C	101	    2	{_code_0_name}	3
        30	0	0	2	202	    15.149999999999991	S	C	202	    {_code_0_name}	1	3
        31	0	0	1	202	    30.299999999999983	S	C	202	    {_code_0_name}	2	3
        32	0	0	3	609	    -57.55	            V	C	609	    {_code_0_name}	3	3
        33	0	0	2	303	    30.299999999999983	S	A	303	    ''	''	3
    """)

    outconstraint = gconfid.testing.PAT_from_string(f"""
        n
        ConstraintId	CellId	Coefficient	{by_name}
        1	6	1	1
        1	7	1	1
        1	5	-1	1
        2	8	1	1
        2	9	1	1
        2	10	1	1
        2	5	-1	1
        3	1	1	1
        3	2	1	1
        3	8	-1	1
        4	3	1	1
        4	9	-1	1
        5	4	1	1
        5	10	-1	1
        6	1	1	1
        6	3	1	1
        6	4	1	1
        6	6	-1	1
        7	2	1	1
        7	7	-1	1
        8	1	1	1
        8	3	1	1
        8	11	-1	1
        9	17	1	2
        9	18	1	2
        9	16	-1	2
        10	19	1	2
        10	20	1	2
        10	21	1	2
        10	16	-1	2
        11	12	1	2
        11	13	1	2
        11	19	-1	2
        12	14	1	2
        12	20	-1	2
        13	15	1	2
        13	21	-1	2
        14	12	1	2
        14	14	1	2
        14	15	1	2
        14	17	-1	2
        15	13	1	2
        15	18	-1	2
        16	12	1	2
        16	14	1	2
        16	22	-1	2
        17	28	1	3
        17	29	1	3
        17	27	-1	3
        18	30	1	3
        18	31	1	3
        18	32	1	3
        18	27	-1	3
        19	23	1	3
        19	24	1	3
        19	30	-1	3
        20	25	1	3
        20	31	-1	3
        21	26	1	3
        21	32	-1	3
        22	23	1	3
        22	25	1	3
        22	26	1	3
        22	28	-1	3
        23	24	1	3
        23	29	-1	3
        24	23	1	3
        24	25	1	3
        24	33	-1	3
    """)

    outlargest = gconfid.testing.PAT_from_string(f"""
        n s n
        CellId	{id_name}	NbRespondents	TotalVar	TotalPercent	{by_name}
        1	111	1	101	100	1
        1	pool	0	0	0	1
        1	anon	0	0	0	1
        2	211	1	101	100	1
        2	pool	0	0	0	1
        2	anon	0	0	0	1
        3	121	1	202	100	1
        3	pool	0	0	0	1
        3	anon	0	0	0	1
        4	133	1	303	49.75369458128079	1
        4	132	1	{_code_203_name}	33.333333333333336	1
        4	131	1	103	16.912972085385878	1
        4	pool	0	0	0	1
        4	anon	0	0	0	1
        5	133	1	303	29.911154985192496	1
        5	132	1	{_code_203_name}	20.03948667324778	1
        5	121	1	202	19.94076999012833	1
        5	131	1	103	10.16781836130306	1
        5	111	1	101	9.970384995064165	1
        5	pool	1	101	9.970384995064165	1
        5	anon	0	0	0	1
        6	133	1	303	33.223684210526315	1
        6	132	1	{_code_203_name}	22.25877192982456	1
        6	121	1	202	22.149122807017545	1
        6	131	1	103	11.293859649122806	1
        6	111	1	101	11.074561403508772	1
        6	pool	0	0	0	1
        6	anon	0	0	0	1
        7	211	1	101	100	1
        7	pool	0	0	0	1
        7	anon	0	0	0	1
        8	111	1	101	50	1
        8	211	1	101	50	1
        8	pool	0	0	0	1
        8	anon	0	0	0	1
        9	121	1	202	100	1
        9	pool	0	0	0	1
        9	anon	0	0	0	1
        10	133	1	303	49.75369458128079	1
        10	132	1	{_code_203_name}	33.333333333333336	1
        10	131	1	103	16.912972085385878	1
        10	pool	0	0	0	1
        10	anon	0	0	0	1
        11	121	1	202	66.66666666666667	1
        11	111	1	101	33.333333333333336	1
        11	pool	0	0	0	1
        11	anon	0	0	0	1
        12	111	1	101	100	2
        12	pool	0	0	0	2
        12	anon	0	0	0	2
        13	211	1	101	100	2
        13	pool	0	0	0	2
        13	anon	0	0	0	2
        14	121	1	202	100	2
        14	pool	0	0	0	2
        14	anon	0	0	0	2
        15	133	1	303	49.75369458128079	2
        15	132	1	{_code_203_name}	33.333333333333336	2
        15	131	1	103	16.912972085385878	2
        15	pool	0	0	0	2
        15	anon	0	0	0	2
        16	133	1	303	29.911154985192496	2
        16	132	1	{_code_203_name}	20.03948667324778	2
        16	121	1	202	19.94076999012833	2
        16	131	1	103	10.16781836130306	2
        16	111	1	101	9.970384995064165	2
        16	pool	1	101	9.970384995064165	2
        16	anon	0	0	0	2
        17	133	1	303	33.223684210526315	2
        17	132	1	{_code_203_name}	22.25877192982456	2
        17	121	1	202	22.149122807017545	2
        17	131	1	103	11.293859649122806	2
        17	111	1	101	11.074561403508772	2
        17	pool	0	0	0	2
        17	anon	0	0	0	2
        18	211	1	101	100	2
        18	pool	0	0	0	2
        18	anon	0	0	0	2
        19	111	1	101	50	2
        19	211	1	101	50	2
        19	pool	0	0	0	2
        19	anon	0	0	0	2
        20	121	1	202	100	2
        20	pool	0	0	0	2
        20	anon	0	0	0	2
        21	133	1	303	49.75369458128079	2
        21	132	1	{_code_203_name}	33.333333333333336	2
        21	131	1	103	16.912972085385878	2
        21	pool	0	0	0	2
        21	anon	0	0	0	2
        22	121	1	202	66.66666666666667	2
        22	111	1	101	33.333333333333336	2
        22	pool	0	0	0	2
        22	anon	0	0	0	2
        23	111	1	101	100	3
        23	pool	0	0	0	3
        23	anon	0	0	0	3
        24	211	1	101	100	3
        24	pool	0	0	0	3
        24	anon	0	0	0	3
        25	121	1	202	100	3
        25	pool	0	0	0	3
        25	anon	0	0	0	3
        26	133	1	303	49.75369458128079	3
        26	132	1	{_code_203_name}	33.333333333333336	3
        26	131	1	103	16.912972085385878	3
        26	pool	0	0	0	3
        26	anon	0	0	0	3
        27	133	1	303	29.911154985192496	3
        27	132	1	{_code_203_name}	20.03948667324778	3
        27	121	1	202	19.94076999012833	3
        27	131	1	103	10.16781836130306	3
        27	111	1	101	9.970384995064165	3
        27	pool	1	101	9.970384995064165	3
        27	anon	0	0	0	3
        28	133	1	303	33.223684210526315	3
        28	132	1	{_code_203_name}	22.25877192982456	3
        28	121	1	202	22.149122807017545	3
        28	131	1	103	11.293859649122806	3
        28	111	1	101	11.074561403508772	3
        28	pool	0	0	0	3
        28	anon	0	0	0	3
        29	211	1	101	100	3
        29	pool	0	0	0	3
        29	anon	0	0	0	3
        30	111	1	101	50	3
        30	211	1	101	50	3
        30	pool	0	0	0	3
        30	anon	0	0	0	3
        31	121	1	202	100	3
        31	pool	0	0	0	3
        31	anon	0	0	0	3
        32	133	1	303	49.75369458128079	3
        32	132	1	{_code_203_name}	33.333333333333336	3
        32	131	1	103	16.912972085385878	3
        32	pool	0	0	0	3
        32	anon	0	0	0	3
        33	121	1	202	66.66666666666667	3
        33	111	1	101	33.333333333333336	3
        33	pool	0	0	0	3
        33	anon	0	0	0	3
    """)

    gconfid.testing.sensitiv(
        indata=indata,
        outlargest=True,
        hierarchy=f"{_code_0_name} 1 2; {_code_0_name} 1 2 3;",
        s_rule="pq .15",
        code_range=f";1 101 {_code_201_name} 301: 2 102 202 302: 3 103 {_code_203_name} 303;",
        unit_id=id_name,
        var=var_name,
        dimension=f"{dim1_name} Naics",
        by=by_name,
        #trace=True,
        expected_outcell=outcell,
        expected_outconstraint=outconstraint,
        expected_outlargest=outlargest,
        round_data=12,
        **kwargs,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv_typical():
    """typical valid variable lengths"""
    run_max_len_sensitiv()

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__code_range_max():
    """code(code_range): max valid variable lengths"""
    run_max_len_sensitiv(
        code_201_len=code_max_len,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__code_range_p1():
    """code(code_range): max +1 variable lengths"""
    run_max_len_sensitiv(
        code_201_len=code_max_len+1,
        rc_should_be_zero=False,
        msg_list_contains_exact=[
            "ERROR: Range parser: Code is too long.",
        ],
        expected_error_count=1,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__code_range_x2():
    """code(code_range): max *2 variable lengths"""
    run_max_len_sensitiv(
        code_201_len=code_max_len*2,
        rc_should_be_zero=False,
        msg_list_contains_exact=[
            "ERROR: Range parser: Code is too long.",
        ],
        expected_error_count=1,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__hierarchy_max():
    """code(hierarchy): max valid variable lengths"""
    run_max_len_sensitiv(
        code_0_len=code_max_len,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__hierarchy_p1():
    """code(hierarchy): max +1 variable lengths"""
    run_max_len_sensitiv(
        code_0_len=code_max_len+1,
        rc_should_be_zero=False,
        msg_list_contains_exact=[
            "ERROR: Hierarchy parser: Code is too long.",
        ],
        expected_error_count=2,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__hierarchy_x2():
    """code(hierarchy): max *2 variable lengths"""
    run_max_len_sensitiv(
        code_0_len=code_max_len*2,
        rc_should_be_zero=False,
        msg_list_contains_exact=[
            "ERROR: Hierarchy parser: Code is too long.",
        ],
        expected_error_count=2,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__unit_id_max():
    """varlist(unit_id): max valid variable lengths"""
    run_max_len_sensitiv(
        id_len=var_name_max_len,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__unit_id_p1():
    """varlist(unit_id): max +1 variable lengths"""
    run_max_len_sensitiv(
        id_len=var_name_max_len+1,
        rc_should_be_zero=False,
        msg_list_contains_exact=f"ERROR: variable name exceeds max length ({var_name_max_len}) in varlist 'unit_id'",
        expected_error_count=1,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__unit_id_x2():
    """varlist(unit_id): max *2 variable lengths"""
    run_max_len_sensitiv(
        id_len=var_name_max_len*2,
        rc_should_be_zero=False,
        msg_list_contains_exact=f"ERROR: variable name exceeds max length ({var_name_max_len}) in varlist 'unit_id'",
        expected_error_count=1,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__dim1_max():
    """varlist(dimension_1): max valid variable lengths"""
    run_max_len_sensitiv(
        dim1_len=var_name_max_len,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__dim1_p1():
    """varlist(dimension_1): max +1 variable lengths"""
    run_max_len_sensitiv(
        dim1_len=var_name_max_len+1,
        rc_should_be_zero=False,
        msg_list_contains_exact=f"ERROR: variable name exceeds max length ({var_name_max_len}) in varlist 'dimension'",
        expected_error_count=1,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__dim1_x2():
    """varlist(dimension_1): max *2 variable lengths"""
    run_max_len_sensitiv(
        dim1_len=var_name_max_len*2,
        rc_should_be_zero=False,
        msg_list_contains_exact=f"ERROR: variable name exceeds max length ({var_name_max_len}) in varlist 'dimension'",
        expected_error_count=1,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__var_max():
    """varlist(var): max valid variable lengths"""
    run_max_len_sensitiv(
        var_len=var_name_max_len,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__var_p1():
    """varlist(var): max +1 variable lengths"""
    run_max_len_sensitiv(
        var_len=var_name_max_len+1,
        rc_should_be_zero=False,
        msg_list_contains_exact=f"ERROR: variable name exceeds max length ({var_name_max_len}) in varlist 'var'",
        expected_error_count=1,
    )

@pytest.mark.m_auto_pass
def test_max_len_sensitiv__var_x2():
    """varlist(var): max *2 variable lengths"""
    run_max_len_sensitiv(
        var_len=var_name_max_len*2,
        rc_should_be_zero=False,
        msg_list_contains_exact=f"ERROR: variable name exceeds max length ({var_name_max_len}) in varlist 'var'",
        expected_error_count=1,
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()
