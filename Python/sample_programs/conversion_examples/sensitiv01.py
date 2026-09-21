import gconfid.testing
import gconfid

indata= gconfid.testing.PAT_from_string("""
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

gconfid.sensitiv(
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
