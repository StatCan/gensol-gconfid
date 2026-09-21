import gconfid.testing
import pytest

@pytest.fixture
def sensitiv_indata_ug04():
    return gconfid.testing.PAT_from_string("""
        s s s n 
        ident reg cla WaiverFlag val proxyvar
        SA064 R1 C1 0 90 0.5
        SA021 R1 C1 1 160 0.5
        SA021 R1 C2 1 210 0.5
        SA059 R1 C2 0 20 0.5
        SA033 R1 C2 1 120 0.5
        SA033 R1 C3 0 110 0.5
        SA076 R1 C3 0 30 0.5
        SA082 R1 C3 1 200 0.5
        SA114 R2 C1 1 120 0.5
        SA128 R2 C2 0 90 0.5
        SA177 R2 C2 0 50 0.5
        SA105 R2 C2 0 80 0.4
        SA105 R2 C3 0 40 0.4
        SA161 R2 C3 0 30 0.4
        SA143 R2 C3 0 10 0.4
    """)

@pytest.fixture
def sensitiv_expected_outcell():
    return gconfid.testing.PAT_from_string("""
        n n n n n n s s n n n n s s
CellId,NbAnonym,AnonymTotalVar,NbRespondents,TotalVar,Sensitivity,Status,Type,TotalNoise,TotalProxySize,SensitivityBeforeWaivers,FavCost,reg,cla
1,0,0,2,250,18,S,C,250,1,32,250,R1,C1
2,0,0,3,350,-116,V,C,350,1.5,22,940,R1,C2
3,0,0,3,340,-8,V,C,340,1.5,10,940,R1,C3
4,0,0,1,120,0,V,C,120,0.5,24,420,R2,C1
5,0,0,3,220,-32,V,C,220,1.4,-32,220,R2,C2
6,0,0,3,80,-2,V,C,80,1.2000000000000002,-2,80,R2,C3
7,0,0,12,1360,-714,V,C,1360,7.100000000000001,-686,1360,Tot_Reg,Tot_Cla
8,0,0,6,940,-294,V,C,940,4,-266,940,R1,Tot_Cla
9,0,0,6,420,-156,V,C,420,3.1,-156,420,R2,Tot_Cla
10,0,0,3,370,-102,V,C,370,1.5,-58,370,Tot_Reg,C1
11,0,0,6,570,-252,V,C,570,2.9,-198,570,Tot_Reg,C2
12,0,0,6,420,-88,V,C,420,2.6999999999999997,-70,420,Tot_Reg,C3
    """,
    sep=',')

@pytest.fixture
def sensitiv_expected_outconstraint():
    return gconfid.testing.PAT_from_string("""
n
ConstraintId,CellId,Coefficient
1,8,1
1,9,1
1,7,-1
2,10,1
2,11,1
2,12,1
2,7,-1
3,1,1
3,4,1
3,10,-1
4,2,1
4,5,1
4,11,-1
5,3,1
5,6,1
5,12,-1
6,1,1
6,2,1
6,3,1
6,8,-1
7,4,1
7,5,1
7,6,1
7,9,-1
    """,
    sep=',')

@pytest.fixture
def sensitiv_expected_outlargest():
    return gconfid.testing.PAT_from_string("""
        n s n 
CellId,ident,NbRespondents,TotalVar,TotalPercent,WaiverFlag
1,SA021,1,160,64,1
1,SA064,1,90,36,0
1,pool,0,0,0,1
1,anon,0,0,0,0
2,SA021,1,210,60,1
2,SA033,1,120,34.285714285714285,1
2,SA059,1,20,5.714285714285714,0
2,pool,0,0,0,1
2,anon,0,0,0,0
3,SA082,1,200,58.8235294117647,1
3,SA033,1,110,32.35294117647059,0
3,SA076,1,30,8.823529411764707,0
3,pool,0,0,0,1
3,anon,0,0,0,0
4,SA114,1,120,100,1
4,pool,0,0,0,1
4,anon,0,0,0,0
5,SA128,1,90,40.90909090909091,0
5,SA105,1,80,36.36363636363637,0
5,SA177,1,50,22.727272727272727,0
5,pool,0,0,0,1
5,anon,0,0,0,0
6,SA105,1,40,50,0
6,SA161,1,30,37.5,0
6,SA143,1,10,12.5,0
6,pool,0,0,0,1
6,anon,0,0,0,0
7,SA021,2,370,27.205882352941178,1
7,SA033,2,230,16.91176470588235,0
7,SA082,1,200,14.705882352941176,1
7,SA114,1,120,8.823529411764707,1
7,SA105,2,120,8.823529411764707,0
7,pool,7,320,23.529411764705884,0
7,anon,0,0,0,0
8,SA021,2,370,39.361702127659576,1
8,SA033,2,230,24.46808510638298,0
8,SA082,1,200,21.27659574468085,1
8,SA064,1,90,9.574468085106384,0
8,SA076,1,30,3.1914893617021276,0
8,pool,1,20,2.127659574468085,0
8,anon,0,0,0,0
9,SA114,1,120,28.571428571428573,1
9,SA105,2,120,28.571428571428573,0
9,SA128,1,90,21.428571428571427,0
9,SA177,1,50,11.904761904761905,0
9,SA161,1,30,7.142857142857143,0
9,pool,1,10,2.380952380952381,0
9,anon,0,0,0,0
10,SA021,1,160,43.24324324324324,1
10,SA114,1,120,32.432432432432435,1
10,SA064,1,90,24.324324324324323,0
10,pool,0,0,0,1
10,anon,0,0,0,0
11,SA021,1,210,36.8421052631579,1
11,SA033,1,120,21.05263157894737,1
11,SA128,1,90,15.789473684210526,0
11,SA105,1,80,14.035087719298245,0
11,SA177,1,50,8.771929824561404,0
11,pool,1,20,3.508771929824561,0
11,anon,0,0,0,0
12,SA082,1,200,47.61904761904762,1
12,SA033,1,110,26.19047619047619,0
12,SA105,1,40,9.523809523809524,0
12,SA076,1,30,7.142857142857143,0
12,SA161,1,30,7.142857142857143,0
12,pool,1,10,2.380952380952381,0
12,anon,0,0,0,0
    """,
    sep=',')

def test_sensitiv_ug04(
        capfd,
        sensitiv_indata_ug04,
        sensitiv_expected_outcell,
        sensitiv_expected_outconstraint,
        sensitiv_expected_outlargest,
):
    gconfid.testing.sensitiv(
        indata=sensitiv_indata_ug04,
        outlargest=True,
        hierarchy="Tot_Reg R1 R2; Tot_Cla C1 C2 C3;",
        s_rule="pq 0.2",
        proxy_ratio=0.2,
        proxy_diag=True,
        accept_negative=True,
        unit_id="ident",
        var="val",
        dimension="reg cla",
        proxy_size="proxyvar",
        p_waiver="WaiverFlag",

        pytest_capture=capfd,
        expected_outcell=sensitiv_expected_outcell,
        expected_outconstraint=sensitiv_expected_outconstraint,
        expected_outlargest=sensitiv_expected_outlargest,
    )

# invoke pytest if this file is executed directly
if __name__ == "__main__":
    gconfid.testing.run_pytest()