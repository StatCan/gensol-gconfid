# G-Confid Package SAS Migration Guide

## In this guide

The target audience for this guide is users of SAS based G-Confid, version 1.07.003 or earlier, who are migrating to Python based G-Confid, version 2.0.0b4 or later.
It summarizes information on the changes made in version 2, including new names for parameters and tables, as well as examples of how SAS programs which call G-Confid can be converted into equivalent Python programs.

This document is not intended for *new* G-Confid users as it does not provide a comprehensive overview of G-Confid 2.x. Please refer to the [User Guide](user_guide.md#overview) for complete details on the use of each module, its parameters, and tables.

## Table of Contents

- [General Changes](#general-changes)
- [Sensitivity](#sensitivity)
- [Suppression](#suppression)
- [Auditing](#auditing)
- [Optimized Rounding](#optimized-rounding)

## General Changes

- Modules audit, suppress and opt_round make use of a linear problem solver. In the SAS version of G-Confid, `Proc OptModel` was used. In the Python version, a generic python package is used which interfaces with various solvers called `PuLP`. By default, we set `PuLP` to use the `HiGHS` solver, which is not included with PuLP but is included in the `highspy` package which should be installed along with G-Confid. PuLP can also use other open-source and commercial solvers. For more information about PuLP, see [solver configuration](user_guide.md#solver-configuration) and the PuLP [documentation](https://pypi.org/project/PuLP/).
- Logging: In the SAS version, information was written to the SAS log and the SAS log could be redirected to a file. In Python, loggers are used. G-Confid modules accept a logger as an optional input parameter. If no logger is provided, a logger will be created which writes to terminal.
- Table formats: In SAS, users passed the name of a SAS dataset to G-Confid, in Python, users can pass a Pandas DataFrame, Arrow table,  or a filename of a supported type (`.parq`, `.parquet`, `.feather`, `csv`, or `.sasbdat`). See [Supported formats](user_guide.md#supported-formats) for more information.
- Table Columns: In Python, variable names are typically case sensitive. On a Pandas data frame, variables `Revenue` and `revenue` and `REVENUE` can all coexist on the same data frame. Furthermore, variable names can have spaces and be longer than 32 characters. G-Confid now supports variable names up to 64 characters long, however, variable names with spaces are still not permitted. Although we have programmed in some flexibility, the casing of variable names should be consistent between parameters and data sets. G-Confid will attempt to standardize standard variables on the input files and the output files will have the standard names. The standardization does not permit multiple columns under the same name with different casing. We typically use Pascal casing for data set variable names. For example, `CELLID` will be converted to `CellId` and `outstatus` would be converted to `OutStatus`. Non standard variables, such as cost variables or variables used for by processing will not be modified and the user must respect the casing of the input data when specifying parameters.
- Exception handling: In Python, we rely on exception handling rather than a return code for error handling. See [G-Confid log](user_guide.md#g-confid-log) for more information.
- The following function are not available in Python as they were deemed to be unnecessary. If this is not the case, please notify the G-Confid support team.
  - Aggregate
  - ReportCells
  - GConfidRound: Only GConfidOptRound was ported to Python

## Sensitivity

The sensitivity procedure has been converted by taking the SAS-dependent G-Confid 1.07.003 procedure source code and modifying it to produce an open-source based procedure which is *"wrapped"* in a Python package. The underlying mathematical computations remain largely unaltered with one exception:
  - `MINRESP` and `MINRESPW` are now ignored during sensitivity calculation for a cell if all records within that cell have a waiver given  

Due to differences between SAS and Python, users must adapt *how* they specify parameters and tables; the sets of parameters and tables remains largely unchanged (although most [*parameter identifiers*](#table-of-sensitivity-parameters-and-types) and [*table identifiers*](#table-of-sensitivity-table-identifiers) have changed).

### Sensitivity Parameters

Many parameter names have changed to better follow common Python naming conventions.

The identifiers used in SAS programs correspond to the following identifiers and Python types:

### Table of Sensitivity Parameters and Types

|SAS Identifier|Python Identifier|Python Type|Note|
|--|--|--|--|
|`ACCEPTNEGATIVE`|`accept_negative`|`bool`||
|~~`REJECTNEGATIVE`~~|||use `accept_negative=False`|
|`ADDITIVENOISE`|`additive_noise`|`bool`||
|~~`NOADDITIVENOISE`~~|||use `additive_noise=False`|
|`BY`|`by`|`str`||
|`DBGFILEPREFIX`|`debug_file_prefix`|`str`||
|`DBGWORKDIRPATH`|`debug_work_dir_path`|`str`||
|`DIMENSION`|`dimension`|`str`||
|`HIERARCHY`|`hierarchy`|`str`||
|`ID`|`unit_id`|`str`||
|`LIMITWARNINGS`|`limit_warnings`|`bool`||
|~~`NOLIMITWARNINGS`~~|||use `limit_warnings=False`|
|`M`|`m`|`int` or `float`||
|`MINRESP`|`min_resp`|`int` or `float`||
|`MINRESPW`|`min_resp_w`|`int` or `float`||
|`PRINTCODES`|`print_codes`|`bool`||
|~~`NOPRINTCODES`~~|||use `print_codes=False`|
|`PROXYDIAG`|`proxy_diag`|`bool`||
|~~`NOPROXYDIAG`~~|||use `proxy_diag=False`|
|`PROXYPERCENTILE`|`proxy_percentile`|`int` or `float`||
|`PROXYRATIO`|`proxy_ratio`|`int` or `float`||
|`PROXYSIZE`|`proxy_size`|`str`||
|`PWAIVER`|`p_waiver`|`str`||
|`RANGE`|`code_range`|`str`||
|`SHADOW`|`shadow`|`str`||
|`SRULE`|`s_rule`|`str`||
|`TIMER`|`timer`|`bool`||
|`TOLERANCE`|`tolerance`|`int` or `float`||
|`VAR`|`var`|`str`||
|`VERBOSE`|`verbose`|`bool`||
|`WAIVER`|`waiver`|`str`||
|`WEIGHT`|`weight`|`str`||
|`WEIGHTDIAG`|`weight_diag`|`bool`||
|~~`NOWEIGHTDIAG`~~|||use `weight_diag=False`|
|`WEIGHTPROTLEVEL`|`weight_prot_level`|`str`||
|`X`|`x`|`int` or `float`||
|`Y`|`y`|`int` or `float`||
|`Z`|`z`|`int` or `float`||

### Example of Parameter Specification in Python

The following code demonstrates how to specify different *Python types* associated with some common parameters.

```python
gconfid.sensitiv(
    indata=indata,
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
)
```

> *Sample SAS Parameter Specification*
>
> ```sas
> PROC SENSITIVITY
>     DATA=test3
>     OUTCELL=outcell3
>     OUTCONSTRAINT=outconstraint3
>     OUTLARGEST=outlargest3
>     hierarchy="""Tot_Reg R1 R2;
>                 Tot_Cla C1 C2 C3;""",
>     SRULE="pq 0.2"
>     proxyratio=0.2
>     proxydiag
>     AcceptNegative
>     ;
>     ID ident;
>     VAR val;
>     DIMENSION reg cla;
>     PROXYSIZE proxyvar;
>     PWAIVER WaiverFlag;
> RUN;
> ```

|parameter|note|
|--|--|
|`unit_id`|A single variable name|
|`dimension`|a list of 0 or more space-separated variable names|
|`proxy_ratio`|excepts a number, see user guide for advice|
|`accept_negative`|bool: `True`-> enabled, `False`-> opposite behaviour (equal to SAS `REJECTNEGATIVE`)|
|`hierarchy`| wrap multi-line strings with triple quotes (`"""edit>=string"""`), can use single quotes for single line|

### New Sensitivity Options

New options include

- [`presort`](#presort-option)
- [`skip_validation`](#skip_validation-option)
- [`capture`](#capture-option)
- [`trace`](#trace-option)

#### `presort` Option

This option is available in the sensitivity procedure and enabled by default. For more information, see the sensitivity parameters in the [user guide](./user_guide.md#sensitivity-parameters)

#### `skip_validation` Option

This option is available in all procedures and disabled by default. For more information, see the sensitivity parameters in the [user guide](./user_guide.md#sensitivity-parameters)

#### `capture` Option

This option is available in all procedures and disabled by default. For more information, see the sensitivity parameters in the [user guide](./user_guide.md#sensitivity-parameters)

#### `trace` Option

This option is available in all procedures and controls console log verbosity. For more information, see the following section in the [user guide](user_guide.md/#python-log-verbosity-trace).

## Sensitivity Tables

Many table parameter names have changed to better follow common Python naming conventions.

The identifiers used in SAS programs correspond to the following identifiers in Python:

### Table of Sensitivity Table Identifiers

|SAS identifier|Python identifier|Note|
|--|--|--|
|`DATA`|`indata`||
|`OUTCELL`|`outcell`||
|`OUTCONSTRAINT`|`outconstraint`||
|`OUTLARGEST`|`outlargest`||
|`OUTPAIRS`|`outpairs`||
|`OUTTARGETS`|`outtargets`||

#### Table Changes

An overview of changes is provided in the following subsections, with links to detailed information.

## Suppression

In G-Confid 1.07.003, Suppress was a SAS macro. This SAS macro has been rewritten in Python.

### Suppression Parameters

### Table of Suppression Parameters and Types

|SAS Identifier|Python Identifier|Python Type|Note|
|--|--|--|--|
|InCell|incell|`pd.DataFrame` or `pa.Table` or `str`||
|Constraint|inconstraint|`pd.DataFrame` or `pa.Table` or `str`||
|OutCell|outsuppress|`str` or `None`||
|OutComplement|outcomplement|`str` or `Path` or `bool`|Provide either the path where the output will be saved or a bool indicating that a DataFrame should be created instead. Optional, defaults to False|
|CFunction1|cost_function1|str|New cost functions are available|
|CFunction2|cost_function2|str|New cost functions are available|
|CVar1|cost_var1|str||
|CVar2|cost_var2|str||
|ScaleCost|scale_cost|str||
|--|size_roundingbase|int|Used with the new ROUNDEDSIZE cost function.|
|--|sen_roundingbase|int|Optional parameter, used to round the sensitivity value upwards|
|--|total_roundingbase|int|Optional parameter, used to round the total noise value downwards|
|--|constraint_scale|int|This is divided by the sensitivity value to determine ambiguity, defaults to 2|
|--|suppress_order|int|How to choose the next cell, options include 0=Highest Ambiguity Changed, 1=Highest Ambiguity, 2=Highest Sensitivity, defaults to 0|
|AmbiguityTolerance|ambiguity_tolerance|float|Tolerance related to ambiguity, defaults to 0.00001|
|DebugInfo|trace|bool or int|The trace option is used to control the level of logging. See the [user guide](user_guide.md/#python-log-verbosity-trace).|
|ByVars or By|by|str||
|--|skip_validation|bool|New Option - Disables the pre-process validation of expected fields on your input tables. Defaults to False.|
|--|custom_solver|pulp.LpSolver|New Option - An optional custom pulp solver to use instead of the PuLP default.|
|--|logger|logging.Logger|See [using your own logger](user_guide.md#use-your-own-logger-logger) in the user guide.|
|--|capture|bool|See [Suppressing and Troubleshooting Log Messages (`capture=`)](user_guide.md#suppressing-and-troubleshooting-log-messages-capture)|
|SaveMps|--|--|See [Solver Configuration](user_guide.md#solver-configuration) in the user guide.|
|SolverOptions|--|--|See [Solver Configuration](user_guide.md#solver-configuration) in the user guide.|
|OptmodelOptions|--|--|See [Solver Configuration](user_guide.md#solver-configuration) in the user guide.|
|ScaleCostUpperBound||||
|RoundFactor||||
|RoundLowerBoundFactor||||
<!-- |--|multiprocess|bool|New option - enables the use of multiple processors to distribute the work of the solver. Defaults to False. See [Multiprocessing](user_guide.md#Multiprocessing) in the user guide.| -->

#### Example of Suppression Parameter Specification in Python

The following code demonstrates how to specify different *Python types* associated with some common parameters.

```python
import gconfid

result=gconfid.Suppression(
    incell="./outcell_sens.sas7bdat",
    inconstraint="./outconstraint.sas7bdat",
    outsuppress="./outcell_sprs.csv",
    cost_function1="digits",
    cost_function2="information",
    by="year",
)
```

> *Sample SAS Parameter Specification*
>
>```sas
>%SUPPRESS(
>   INCELL=outcell_sens,
>   CONSTRAINT=outconstraint,  
>   CFUNCTION1=digits,
>   CFUNCTION2=information,
>   BYVARS=year,
>   OUTCELL=outcell_sprs);
>```

## Auditing

In G-Confid 1.07.003, Audit was a SAS macro. This SAS macro has been rewritten in Python.

### Table of Auditing Parameters and Types

|SAS Identifier|Python Identifier|Python Type|Note|
|--|--|--|--|
|InCell|incell|`pd.DataFrame` or `pa.Table` or `str`||
|Constraint|inconstraint|`pd.DataFrame` or `pa.Table` or `str`||
|OutCell|outaudit|`str` or `None`||
|ByVars or By|by|||
|DebugInfo|trace|bool or int|Setting trace=gconfid.log_level.ERROR is essentially the same as DebugInfo=1 and gconfid.log_level.INFO is the same as DebugInfo=0. See the [user guide](user_guide.md/#python-log-verbosity-trace).|
|SolverOptions|--|--|See [Solver Configuration](user_guide.md#solver-configuration) in the user guide.|
|LBFactor|lb_factor|float||
|UBFactor|ub_factor|float||
|ReportLevel|report_level|int|report_level 2 is no longer available.|
|SasConnect|--|--|No longer applicable, this is a SAS specific parameter|
|AuditSensitiveCellsOnly|--|--|Was not carried over - to be investigated.|
|Tolerance|--|--|Was not carried over - to be investigated.|
|ParallelMode|--|--|No longer applicable, this is a SAS specific parameter|
|NumberOfNodes|--|--|No longer applicable, this is a SAS specific parameter|
|SASApplicationServer|--|--|No longer applicable, this is a SAS specific parameter|
|SasConnectThreshold |--|--|No longer applicable, this is a SAS specific parameter|
|LpFeasTol|--|--|This was declared obsolete in G-Confid 1.07.003|
|PrintProgress|--|--|This was declared obsolete in G-Confid 1.07.003|
|UseShuttle|use_shuttle|bool|Enables the shuttle algorithm to compute lower and upper bounds. This was a hidden parameter in G-Confid 1.07.003. Defaults to False.|
|RelTol|--|--|This was a hidden parameter in G-Confid 1.07.003|
|AbsTol|--|--|This was a hidden parameter in G-Confid 1.07.003|
|MaxIter|--|--|This was a hidden parameter in G-Confid 1.07.003|
|--|multiprocess|bool|New option - enables the use of multiple processors to distribute the work of the solver. Defaults to False. See [Multiprocessing](user_guide.md#Multiprocessing) in the user guide.|
|--|skip_validation|bool|New Option - Disables the pre-process validation of expected fields on your input tables. Defaults to False.|
|--|custom_solver|pulp.LpSolver|New Option - An optional custom pulp solver to use instead of the PuLP default.|
|--|logger|logging.Logger|See [using your own logger](user_guide.md#use-your-own-logger-logger) in the user guide.|
|--|capture|bool|See [Suppressing and Troubleshooting Log Messages (`capture=`)](user_guide.md#suppressing-and-troubleshooting-log-messages-capture)|

## Optimized Rounding

In G-Confid 1.07.003, OptRound was a SAS macro. This SAS macro has been rewritten in Python.

Note that in the SAS version of G-Confid there were two rounding macros `GConfidOptRound` and `GConfidRound`. Only `GConfidOptRound` was converted to Python as this method was thought to be more methodologically sound. `GConfidOptRound` looks to provide an optimal solution; one that is as close to the original table as possible while respecting the constraints. Constraints can be used to request a rounded table that is both additive and controlled. However, in certain cases, the problem may not be feasible. `GConfidRound` would provide a solution that was controlled but not additive, along with an additivity report. `GConfidOptRound` fails if no solution can be found and constraints need to be relaxed in order to get a solution.

### Table of Opt_Rounding Parameters and Types

|SAS Identifier|Python Identifier|Python Type|Note|
|--|--|--|--|
|twoLevelCatalogName|--|--|No longer applicable, this is a SAS specific parameter|
|Language|--|--|The language can be set for the g-confid package before calling a module with `gconfid.set_language = gconfid.SupportedLanguage.fr`|
|InCells|incell|`pd.DataFrame` or `pa.Table` or `str`| |
|InConstraints|additive_bound|`pd.DataFrame` or `pa.Table` or `str`| |
|InCellConstraints|additive_con|`pd.DataFrame` or `pa.Table` or `str`| |
|OutCells|outround|`pd.DataFrame` or `pa.Table` or `str`|The column names have been lengthened to be more readable.|
|OutConstraints|--|--|Was not carried over - to be investigated.|
|OutSummary|--|--|Was not carried over - to be investigated.|
|Base|base|int||
|DebugInfo|trace|bool or int|The trace option is used to control the level of logging. See the [user guide](user_guide.md/#python-log-verbosity-trace).|
|Objective|--|--|Was not carried over - to be investigated.|
|Solver|--|--|See [Solver Configuration](user_guide.md#solver-configuration) in the user guide.|
|OptModelLog|--|--|See [Solver Configuration](user_guide.md#solver-configuration) in the user guide.|
|OptModelStatus|--|--|Future option|
|--|skip_validation|bool|New Option - Disables the pre-process validation of expected fields on your input tables. Defaults to False.|
|--|custom_solver|pulp.LpSolver|New Option - An optional custom pulp solver to use instead of the PuLP default.|
|--|trace|int or bool or None|The default is None|
|--|logger|logging.Logger|See [using your own logger](user_guide.md#use-your-own-logger-logger) in the user guide.|
|--|capture|bool|See [Suppressing and Troubleshooting Log Messages (`capture=`)](user_guide.md#suppressing-and-troubleshooting-log-messages-capture)|

## Other Python Runtime Options

### Native Language Support

G-Confid produces a [log](user_guide.md#g-confid-log) which can output messages in either English or French. See [*setting the log language*](user_guide.md#setting-the-log-language) from the user guide for details.

### `capture` option

When running in Jupyter Notebooks, some log messages may be missing. Specifying `capture=True` in a procedure call to may fix the issue. See [suppressing and troubleshooting log messages](user_guide.md#suppressing-and-troubleshooting-log-messages-capture) from the user guide for details.

## Performance Considerations

Certain options and table formats can be expected to deliver optimal performance. See [Performance Considerations](user_guide.md#performance-considerations) for details.

## Errors and Exceptions

Error's are handled differently in SAS vs in Python, where they are called *exceptions*. See [Errors and Exceptions](user_guide.md#errors-and-exceptions) from the user guide for details.

## Utility Functions

### Working with SAS Files in Python

The G-Confid package provides a few useful functions for reading SAS files in Python. See [Working with SAS Files in Python](user_guide.md#working-with-sas-files-in-python) in the user guide for details.
