# Tutorial

This tutorial will show the [Python equivalent](/Python/sample_programs/conversion_examples/sensitiv03.py) of the [SAS language Proc Sensitivity sample program 3](/Python/sample_programs/conversion_examples/sensitiv03.sas).  
Numerous additional SAS and Python equivalents are also available in the [`conversion_examples`](/Python/sample_programs/conversion_examples/) folder.  

The example program shows how to

- create a synthetic table
- sort the table
- specify
  - parameters
  - input table
  - output table options
- access results

It will discuss some relevant differences between SAS and Python as well.  

## SAS Language example

```sas
%let num=4;
data test&num;
    INPUT ident$ reg$ cla$ WaiverFlag val proxyvar; 
    CARDS;
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
RUN;

PROC SENSITIVITY 
    DATA=test&num 
    OUTCELL=outcell&num 
    OUTCONSTRAINT=outconstraint&num 
    OUTLARGEST=outlargest&num
    HIERARCHY="Tot_Reg R1 R2; Tot_Cla C1 C2 C3;" 
    SRULE="pq 0.2"
    proxyratio=0.2
    proxydiag
    AcceptNegative
    ;
    ID ident; 
    VAR val;
    DIMENSION reg cla; 
    PROXYSIZE proxyvar; 
    PWAIVER WaiverFlag;
RUN;
```

## Python language equivalent

```python
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
```

## Line by line explanation

### Import packages

In Python, *packages* must be *imported* into a session before they can be used.  

```python
import pyarrow as pa

import gconfid
```

The [`pyarrow`](https://pypi.org/project/pyarrow/) package is used for creating and manipulating tables.  Note that the *alias* `pa` is used for the `pyarrow` package.  

### Create synthetic data

We create the same synthetic table that was created in SAS.  

A *Pyarrow Schema* object is created and assigned to the variable `indata_schema`.  

```python
# create a schema for the indata dataset
indata_schema = pa.schema([
            ("ident", pa.string()),
            ("reg", pa.string()),
            ("cla", pa.string()),
            ("WaiverFlag", pa.int64()),
            ("val", pa.int64()),
            ("proxyvar", pa.float64()),
    ])
```

- this is used to define the name and datatype of each column in a table
- Pyarrow offers an [extensive set of datatypes](https://arrow.apache.org/docs/python/api/datatypes.html#factory-functions)
- for more details, see [`pyarrow.schema` documentation](https://arrow.apache.org/docs/python/generated/pyarrow.schema.html)

A *Pyarrow Table* object is created and stored in the `indata` variable.  

```python
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
```

- the order of columns in the schema corresponds to the order of lists of data
- for more details, see [`pyarrow.table` documentation](https://arrow.apache.org/docs/python/generated/pyarrow.table.html)
- unlike SAS, where tables are typically stored as files, Pyarrow Tables are typically stored in memory as "*objects*", hence the need to assign to the `indata` variable

#### Python Concepts

> - in Python, variables are often *objects*, containing not only data but also *methods*
> - methods are often useful for getting information about, or performing operations on, the data
> - the `indata` variable is a *Pyarrow Table* (`pyarrow.Table`) object, and accordingly has its own [`sort_by()` method](https://arrow.apache.org/docs/python/generated/pyarrow.Table.html#pyarrow.Table.sort_by)
>   - documentation on other methods available for Pyarrow Tables can be found [here](https://arrow.apache.org/docs/python/generated/pyarrow.Table.html#pyarrow.Table).

### Running The Sensitivity Procedure

```python
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
```

Calling `gconfid.sensitiv()` results in the sensitivity procedure executing and an object being assigned to the `sensitivity_call` variable.  The object can be used to access output tables.  

- note that all parameters and tables are specified as comma separated key-value pairs
- they can appear in any order
- `indata=indata` specifies that the `indata` table (recently sorted) should be provided as the "indata" table

### Procedure Execution

During procedure execution, the text written to the console by the procedure should be nearly identical to the SAS log from the equivalent example.  

### Accessing output tables

After execution completes, output table options are processed.  The default option was specified above, so the output tables are available as pyarrow tables.  

They are stored in the `sensitivity_call` object, access them using `sensitivity_call.outcell` and `sensitivity_call.outlargest`.  From here they can be handled by users like any other pyarrow table, for example:

- written to file
- manipulated (sorted, merged, etc.)
- used as input for another procedure (`incell=sensitivity_call.outcell`)

### Other input and output options

To provide a consistent means of reading/writing files and converting between table formats, while maintaining the highest possible floating-point precision, support for various input and output table formats has been implemented.  

Please consult the user guide for information on [supported formats](user_guide.md#supported-formats)

The following code will demonstrate the use of files for input and output tables by modifying the above example.  

```python
gconfid.sensitiv(
    indata=r"C:\temp\in_data.feather",
    outcell=r"C:\temp\outcell.parquet",
    outlargest=r"C:\temp\outlargest.feather",
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
```

- note that there is no need to assign the object returned by `gconfid.sensitiv()` to a variable because output data is written to disk
