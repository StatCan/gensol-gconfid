"""Ensure that custom logger works as expected
When a logger is specified, it should receive messages from Python and C.  
"""
import gconfid.testing
import gconfid
import pytest

@pytest.mark.m_auto_pass
def test_sensitiv_logger_a(indata):
    """run procedure with custom logger and validate log contents"""

    temp_log_filename="test_custom_logger_a.log"

    import logging
    my_logger = logging.getLogger(__name__)
    my_logger.setLevel(logging.INFO)
    handler = logging.FileHandler(temp_log_filename, encoding='utf-8', mode='w')
    my_logger.addHandler(handler)

    gconfid.testing.sensitiv(
        logger=my_logger,
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

    handler.close()
    with open(temp_log_filename) as log_file:
        log_contents = log_file.read()
    import os
    os.remove(temp_log_filename)
    
    assert 'NOTE: PROCEDURE SENSITIVITY' in log_contents, "log file missing C log contents"
    assert 'Loading input datasets' in log_contents, "log file missing Python log contents"


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