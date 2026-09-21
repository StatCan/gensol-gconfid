import gconfid
from gconfid.testing import PAT_from_string

in_data = PAT_from_string("""
n s n s s
QuestionNumber Key Income Prov Naics
1 111 101 1 101
1 211 101 2 101
1 121 202 1 202
1 131 103 1 103
1 132 203 1 203
1 133 303 1 303
2 111 101 1 101
2 211 101 2 101
2 121 202 1 202
2 131 103 1 103
2 132 203 1 203
2 133 303 1 303
3 111 101 1 101
3 211 101 2 101
3 121 202 1 202
3 131 103 1 103
3 132 203 1 203
3 133 303 1 303
""")

sensitivity_result = gconfid.Sensitivity(
    indata=in_data,
    outlargest=True,
    unit_id="Key",
    var = "Income",
    dimension="Prov Naics",
    by="QuestionNumber",
    hierarchy="0 1 2; 0 1 2 3;", 
    s_rule="pq .15",
    code_range=";1 101 201 301: 2 102 202 302: 3 103 203 303;")

suppress_result = gconfid.Suppression(
    inconstraint=sensitivity_result.outconstraint,
    incell=sensitivity_result.outcell,
    cost_function1="DIGITS",
    cost_function2="INFORMATION",
    by="QuestionNumber",
    scale_cost=None,
    trace=gconfid.log_level.INFO)

audit_result= gconfid.Auditing(
    incell=suppress_result.outsuppress,
    inconstraint=sensitivity_result.outconstraint,
    lb_factor=0.5,
    ub_factor=1.5,
    by="QuestionNumber",
    trace=gconfid.log_level.INFO,
    report_level=2)

print(max(audit_result.outaudit["ProblemIndicator"]))
