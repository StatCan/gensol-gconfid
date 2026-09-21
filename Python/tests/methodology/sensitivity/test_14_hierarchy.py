"""The following tests are based on the file `SENSITIVITY_14_Hierarchy.sas`
"""
import pytest
import gconfid.testing

@pytest.mark.m_auto_pass
def test_14_a(indata_14):
    """Règle NK160 Flag 01"""
    hierarchy_ex1="""
        TOTAL EAST WEST: 
        TOTAL LOCKED COAST: 
        EAST 1 2 3: 
        EAST E_TEAM E_NOTEAM: 
        WEST 4 5: 
        WEST W_TEAM W_NOTEAM: 
        LOCKED 2 3 4: 
        COAST 1 5: 
        E_TEAM 24 35: 
        E_NOTEAM 10 11 12 13: 
        W_TEAM 46 48 59: 
        W_NOTEAM 47: 
        1 10 11 12 13: 
        2 24: 
        3 35: 
        4 46 47 48: 
        5 59; 
        IND_TOT A B;"""

    gconfid.testing.sensitiv(
        indata=indata_14,
        outcell=True,
        outconstraint=True,
        hierarchy=hierarchy_ex1,
        s_rule="pq 0.2",
        unit_id="EntID",
        var="value",
        dimension="Province Industry",

        expected_outcell="_hier_outcell_ex1.sas7bdat",
        expected_outconstraint="_hier_outconstraint_ex1.sas7bdat",
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()