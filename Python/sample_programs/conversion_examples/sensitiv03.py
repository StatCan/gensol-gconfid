import pyarrow as pa

import gconfid

# create a schema for the indata dataset
indata_schema = pa.schema([
            ("ident", pa.string()),
            ("reg", pa.string()),
            ("cla", pa.string()),
            ("WaiverFlag", pa.int64()),
            ("val", pa.int64()),
            ("proxyvar", pa.float64()),
    ])

# create table using schema and lists of values for each column
indata = pa.table(
    schema=indata_schema,
    data=[
        ["SA064", "SA021", "SA021", "SA059", "SA033", "SA033", "SA076", "SA082", "SA114", "SA128", "SA177", "SA105", "SA105", "SA161", "SA143"],
        ["R1", "R1", "R1", "R1", "R1", "R1", "R1", "R1", "R2", "R2", "R2", "R2", "R2", "R2", "R2"],
        ["C1", "C1", "C2", "C2", "C2", "C3", "C3", "C3", "C1", "C2", "C2", "C2", "C3", "C3", "C3"],
        [0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0],
        [90, 160, 210, 20, 120, 110, 30, 200, 120, 90, 50, 80, 40, 30, 10],
        [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.4, 0.4, 0.4, 0.4],
    ],
)

sensitivity_call = gconfid.sensitiv(
    indata=indata,
    outlargest=True,
    hierarchy="""Tot_Reg R1 R2;
                Tot_Cla C1 C2 C3;""",
    s_rule="pq 0.2",
    proxy_ratio=0.2,
    proxy_diag=True,
    accept_negative=True,
    unit_id="ident",
    var="val",
    dimension="reg cla",
    proxy_size="proxyvar",
    p_waiver="WaiverFlag",
)
