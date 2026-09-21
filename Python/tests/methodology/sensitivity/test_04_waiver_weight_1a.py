"""The following tests are based on the file `SENSITIVITY_04_Waivers_Weights_1A.sas`
"""
import pytest
import gconfid.testing

def run_test_04_1a_f1(indata, s_rule, abb, **kwargs):
    hiwt1="""
    0000 1000 2000 3000 4000:
    1000 1101 1102:
    2000 2100 2200:
    2100 2101 2102 2103 2104:
    2200 2201 2202 2203 2204:
    3000 3100 3200 3300 3400:
    3100 3101 3102 3103 3104 3105 3106 3107 3108:
    3200 3201 3202 3203 3204 3205 3206 3207 3208:
    3300 3301 3302 3303 3304 3305 3306 3307 3308:
    3400 3401 3402 3403 3404 3405 3406 3407 3408:
    4000 4100 4200 4300 4400 4500 4600 4700 4800:
    4100 4101 4102 4103 4104 4105 4106 4107 4108 4109 4110 4111 4112 4113 4114 4115 4116:
    4200 4201 4202 4203 4204 4205 4206 4207 4208 4209 4210 4211 4212 4213 4214 4215 4216:
    4300 4301 4302 4303 4304 4305 4306 4307 4308 4309 4310 4311 4312 4313 4314 4315 4316:
    4400 4401 4402 4403 4404 4405 4406 4407 4408 4409 4410 4411 4412 4413 4414 4415 4416:
    4500 4501 4502 4503 4504 4505 4506 4507 4508 4509 4510 4511 4512 4513 4514 4515 4516:
    4600 4601 4602 4603 4604 4605 4606 4607 4608 4609 4610 4611 4612 4613 4614 4615 4616:
    4700 4701 4702 4703 4704 4705 4706 4707 4708 4709 4710 4711 4712 4713 4714 4715 4716:
    4800 4801 4802 4803 4804 4805 4806 4807 4808 4809 4810 4811 4812 4813 4814 4815 4816;"""

    gconfid.testing.sensitiv(
        indata=indata,
        outconstraint=True,
        outcell=True,
        outlargest=True,
        hierarchy=hiwt1,
        s_rule=s_rule,
        unit_id="entid",
        var="value",
        dimension="dimvar",
        waiver="WaiverFlag",

        **kwargs,

        msg_list_contains=[
            "1. Number of valid observations                                                            626",
            "b. # of valid observs from non-anonymous respondents                                    626",
            "c. # of valid observs with negative data for respondents                                  0",
            "2. Number of valid observations with zero value                                              0",
            "3. Number of invalid observations                                                            0",
            "4. # of observs invalid because dimension code set not in hierarchy                          0",
        ],
        expected_outcell=f"_1a_outcell_{abb}.sas7bdat",
        expected_outconstraint=f"_1a_outcon_{abb}.sas7bdat",
        expected_outlargest=f"_1a_outlargest_{abb}.sas7bdat",
    )

def run_test_04_1a_f2(indata, s_rule, abb, **kwargs):
    run_test_04_1a_f1(
        indata,
        s_rule,
        abb=f"{abb}_1",
        weight_prot_level="LINEAR",
        weight="poids",
        drop_columns="WeightedNbRespondents",
        **kwargs,
    )

@pytest.mark.m_auto_pass
def test_04_ex1_1(indata_04_1a):
    """Ex1_1 : Règle nk 1 70"""
    run_test_04_1a_f1(
        indata=indata_04_1a,
        s_rule="nk 1 70",
        abb="nk170",
    )

@pytest.mark.m_auto_pass
def test_04_ex1_2(indata_04_1a):
    """Ex1_2 : Règle nk 1 70 POIDS=1"""
    run_test_04_1a_f2(
        indata=indata_04_1a,
        s_rule="nk 1 70",
        abb="nk170",
    )

@pytest.mark.m_auto_pass
def test_04_ex2_1(indata_04_1a):
    """Ex2_1 : Règle nk 2 88"""
    run_test_04_1a_f1(
        indata=indata_04_1a,
        s_rule="nk 2 88",
        abb="nk288",
    )

@pytest.mark.m_auto_pass
def test_04_ex2_2(indata_04_1a):
    """Ex2_2 : Règle nk 2 88 POIDS=1"""
    run_test_04_1a_f2(
        indata=indata_04_1a,
        s_rule="nk 2 88",
        abb="nk288",
    )

@pytest.mark.m_auto_pass
def test_04_ex3_1(indata_04_1a):
    """Ex3_1 : Règle pq 20"""
    run_test_04_1a_f1(
        indata=indata_04_1a,
        s_rule="pq .20",
        abb="pq02",
    )

@pytest.mark.m_auto_pass
def test_04_ex3_2(indata_04_1a):
    """Ex3_2 : Règle pq 20 POIDS=1"""
    run_test_04_1a_f2(
        indata=indata_04_1a,
        s_rule="pq .20",
        abb="pq02",
    )

@pytest.mark.m_auto_pass
def test_04_ex4_1(indata_04_1a):
    """Ex4_1 : Règle nk 3 96"""
    run_test_04_1a_f1(
        indata=indata_04_1a,
        s_rule="nk 3 96",
        abb="nk396",
    )

@pytest.mark.m_auto_pass
def test_04_ex4_2(indata_04_1a):
    """Ex4_2 : Règle nk 3 96 POIDS=1"""
    run_test_04_1a_f2(
        indata=indata_04_1a,
        s_rule="nk 3 96",
        abb="nk396",
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()