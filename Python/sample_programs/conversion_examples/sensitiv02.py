import gconfid
from gconfid.testing import PAT_from_string

test2 = PAT_from_string("""
s s s n n 
ident reg cla WaiverFlag val
SA064 R1 C1 0 90
SA021 R1 C1 1 160
SA021 R1 C2 1 210
SA059 R1 C2 0 20
SA033 R1 C2 1 120
SA033 R1 C3 0 110
SA076 R1 C3 0 30
SA082 R1 C3 1 200
SA114 R2 C1 1 120
SA128 R2 C2 0 90
SA177 R2 C2 0 50
SA105 R2 C2 0 80
SA105 R2 C3 0 40
SA161 R2 C3 0 30
SA143 R2 C3 0 10
""")

proc_call = gconfid.sensitivity(
    indata=test2,
    hierarchy="Tot_Reg R1 R2; Tot_Cla C1 C2 C3;",
    s_rule="pq 0.2",
    unit_id="ident",
    var="val",
    dimension="reg cla",
    p_waiver="WaiverFlag",
    trace=True,
)
print("foo")