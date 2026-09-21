import pandas as pd
import gconfid
from gconfid.testing import PAT_from_string

in_data = PAT_from_string("""
s s s n
ID Industry Region Freq
01 1 A 35
02 1 B 3
03 1 C 5
04 2 A 6
05 2 B 3
06 2 C 9
""")

sens_result = gconfid.Sensitivity(
    indata=in_data,
    outconstraint="pandas",
    outcell="pandas",
    s_rule="arb -1",
    hierarchy="""
    TOT_INDUSTRY 1 2;
    TOT_REGION A B C;
    """,
    unit_id="ID",
    var="Freq",
    dimension="Industry Region")

in_bounds = pd.DataFrame(sens_result.outconstraint[sens_result.outconstraint["Coefficient"] == -1]["ConstraintId"])
in_bounds["ConstraintLB"] = 0
in_bounds["ConstraintUB"] = 0

in_data = sens_result.outcell[["CellId", "TotalNoise"]]
in_data = in_data.rename(columns={"TotalNoise": "Total"})
in_data["CellLB"] = pd.NA
in_data["CellUB"] = pd.NA
in_data.loc[in_data["CellId"] == 1, "CellLB"] = 40

round_result = gconfid.OptRounding(
    incell=in_data,
    additive_con=sens_result.outconstraint,
    additive_bound=in_bounds,
    base=5)

print(round_result.outround[["CellId", "RoundedTotal"]])
