"""Test integration between multiple procs."""
import pytest

import gconfid.testing
import gconfid
import numpy as np
import pandas as pd
from pathlib import Path


@pytest.mark.m_known_to_fail
def test_integration_01():
    expected_outputs = Path(__file__).parent / "expected"

    data = []
    np.random.seed(7005)
    province_map = {1: "10", 2: "11", 3: "12", 4: "13", 5: "24", 6: "35", 7: "46",
                    8: "47", 9: "48", 10: "59",
    }
    id_counter = 0
    for i in range(1, 11):
        for j in range(1, 3):
            for k in range(1, 6):
                id_counter += 1
                entid = str(id_counter).zfill(3)
                province = province_map[i]
                industry = "A" if j == 1 else "B"
                value = round(((np.random.rand() + 0.25) ** 7) * 1000) + 1
                data.append([entid, province, industry, value])
    df = pd.DataFrame(data, columns=["Entid", "Province", "Industry", "Value"])

    hierarchy_ex2 = """TOT_INDUSTRY A B;
                    TOT_PROVINCE 10 11 12 13 24 35 46 47 48 59;"""

    sens_res = gconfid.sensitiv(
        indata=df,
        outcell = "pandas",
        outconstraint= "pandas",
        s_rule="pq 0.1",
        hierarchy=hierarchy_ex2,
        unit_id="Entid",
        var="Value",
        dimension="Industry Province", # Two dimension variable
    )

    sup_res = gconfid.Suppression(
        incell=sens_res.outcell,
        inconstraint=sens_res.outconstraint,
        cost_function2 = "information",
        outcomplement = "pandas",
        #expected_outsuppress=expected_outputs / "outsuppress_aka_outpattern.sas7bdat",
        #expected_outcomplement=expected_outputs / "outcomplement.sas7bdat",
        #drop_columns=["ProtectionNumber"], # SAS indexes iterations on 1, Python on 0 so just ignore
        #round_data=13,
    )

    gconfid.testing.Auditing(
        # Inputs
        incell = sup_res.outsuppress,
        inconstraint = sens_res.outconstraint,
        lb_factor = 0.5,
        ub_factor = 1.5,
        report_level = 1,
        use_shuttle=False, # Shuttle technically implemented in SAS but bounds are not used
        # Outputs
        expected_outaudit = expected_outputs / "extended_audit_outcell.sas7bdat",
        round_data = 12,
        drop_columns=["UBound", "LBound", "TotalLB", "TotalUB"],
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()