# Overview

G-Confid is a Statistics Canada generalized system that offers a methodology designed to prevent the release of confidential data. It is comprised of four modules, The `Sensitivity` module is used to determine the sensitivity of table cells (and combinations of cells). The `Suppression` module is used to determine a suppression pattern, or set of complementary cells, necessary to protect the sensitive cells (or combinations of cells). The third component, the `Auditing` module, is used to verify the validity of a suppression pattern. In addition to these three modules, the `OptimizedRounding` module provides rounding which is both controlled and additive. 

This project contains the Python version of G-Confid that has been adapted from the exisiting SAS version. G-Confid is tested under Python versions 3.10, 3.11, 3.12 and 3.13.

## Table of contents

- [Sensitivity](#sensitivity)
  - [Parameters](#sensitivity-parameters)
  - [Input Tables](#sensitivity-input-tables)
  - [Output Tables](#sensitivity-output-tables)
- [Suppression](#suppression)
  - [Parameters](#suppression-parameters)
  - [Input Tables](#suppression-input-tables)
  - [Output Tables](#suppression-output-tables)
- [Auditing](#auditing)
  - [Parameters](#auditing-parameters)
  - [Input Tables](#auditing-input-tables)
  - [Output Tables](#auditing-output-tables)
- [Optimized Rounding](#optimized-rounding)
  - [Parameters](#optimized-rounding-parameters)
  - [Input Tables](#optimized-rounding-input-tables)
  - [Output Tables](#optimized-rounding-output-tables)
- [Technical guide](#technical-guide)
  - [Variable names on input tables](#variable-names-on-input-tables)
  - [G-Confid log](#g-confid-log)
    - [Setting The Log Language](#setting-the-log-language)
      - [Python Log Verbosity (`trace=`)](#python-log-verbosity-trace)
      - [Suppressing and Troubleshooting Log Messages (`capture=`)](#suppressing-and-troubleshooting-log-messages-capture)
      - [Use Your Own Logger (`logger=`)](#use-your-own-logger-logger)
  - [Input and output table specification](#input-and-output-table-specification)
    - [Supported formats](#supported-formats)
    - [Specifying input tables](#specifying-input-tables)
    - [Specifying output tables](#specifying-output-tables)
    - [Customize Default Output Specification](#customize-default-output-specification)
  - [Solver Configuration](#solver-configuration)
  - [Other](#other)
    - [Escape Characters and File Paths](#escape-characters-and-file-paths)
    - [Errors and Exceptions](#errors-and-exceptions)
    - [Working with SAS Files in Python](#working-with-sas-files-in-python)
    - [Performance Considerations](#performance-considerations)
    - [Multiprocessing](#multiprocessing)

## Sensitivity

This module process a micro data file by aggregating the data into table cells, according to user specifications. It then calculates the sensitivity of the table cells.

### Sensitivity Parameters

| Parameter       | Python type  | Description                 |
| ----------------| -------------| --------------------------- |
|indata|[Input dataset](#input-and-output-table-specification)|Specify the input micro data file. Mandatory.|
|outconstraint|[Output dataset](#input-and-output-table-specification)|Contains the constraints. Mandatory.|
|outcell|[Output dataset](#input-and-output-table-specification)|Contains the sensitive cells. Mandatory.|
|outlargest|[Output dataset](#input-and-output-table-specification)|Contains information about the largest contributors to a cell. Optional|
|outpairs|[Output dataset](#input-and-output-table-specification)|Contains the most sensitive observation pairs in each cell. Optional|
|outtargets|[Output dataset](#input-and-output-table-specification)|Contains information the most sensitive combination of observations in each cell.|
|unit_id|str|Identify the name of the variable which is the key of the input data set.|
|by|str|Calculate sensitivity for each BY group.|
|var|str|Identify the name of the variable containing the respondent data.|
|shadow|str|Identify the name of the shadow variable.|
|dimension|str|Identify the name(s) of the variable(s) that represent the hierarchy dimensions.|
|waiver|str|Identify the name of the variable containing the full waiver flag.|
|p_waiver|str|Identify the name of the variable containing the partial waiver flag.|
|proxy_size|str|Identify the name of the auxiliary size variable used in the calculation.|
|weight|str|Identify the variable that represents the estimation weight.|
|s_rule|str|Specify the sensitivity rule to use.|
|hierarchy|str|Specify the hierarchy used for each dimension.|
|code_range|str|Specify the interval of codes associated with the lowest level of the hierarchy.|
|m|float|M represents the maximum number of non-sensitive cells that the user permits G-Confid to include in a set of sensitive cells that G-Confid combined to form an aggregate. This number is optional and has a default of 5. M must be >=0 and <= 10.|
|x|float|Specify the X parameter to adjust the number of cell combinations. `x` represents the minimum relative difference between the total of a non-sensitive cell and the net change in the sensitivity of the aggregate (if that cell were included in the aggregate). If the relative difference is below `x` then the non-sensitive cell is not included in the aggregate. Optional, must be >= 0, default = 0.|
|y|float|Specify the Y parameter to adjust the number of cell combinations. `y` represents the minimum ratio of the total of a non-sensitive cell to the sensitivity of the aggregate without this cell (for this cell to be included in the aggregate).  If the ratio is below `y` then the non-sensitive cell is not included in the aggregate. Optional, default = 0|
|z|float|Specify the Z parameter to adjust the number of cell combinations. `z` represents the minimum ratio of the value of the largest contributor to an aggregate to the total of a non-sensitive cell (before including that cell in the aggregate).  If the ratio is below `z` then the non- sensitive cell is not included in the aggregate. Optional, must be <= 100, default = 0|
|tolerance|float|Specify the upper bound value below which the sensitivity is deemed too small (below this value, the sensitivity is set to 0). For example, if tolerance=0.05, any value of the sensitivity between 0 and 0.05 will be set to zero. This number is optional and has a default of 0.01. Tolerance must be >= 0 and <= 1000.|
|min_resp|int|Specify the minimum number of respondents with a non-zero value in a cell.|
|proxy_ratio|float|This option specifies when and how to incorporate the `proxy_size` variable when calculating sensitivity. When specified, it will be used to determine the threshold at which observations are overridden by the proxy variable. It is optional and its values must be between 0.0 and 1.0. `accept_negative=True` and `proxy_size` must both be specified when using this option.|
|proxy_percentile||Specify the percentage of observations overridden by the proxy variable.|
|weight_prot_level|str|Specifies the level of desired protection in the presence of weights (only applies when the WEIGHT statement is specified). Optional. Valid values are "LINEAR", "STEP" and "EXACT". Default value is "LINEAR".|
|min_resp_w|float|Specify the weighted minimum number of respondents with a non-zero value in a cell.|
|debug_file_prefix||Specify a debug prefix.|
|debug_work_dir_path||Specify a debug folder.|
|verbose|bool|Prints additional information to the log.|
|timer|bool|Adds time information to the log.|
|print_codes|bool|Specify that hierarchies and ranges be printed to the log or not.|
|limit_warnings|bool|Specify that the printing of the warnings generated while reading the micro data be limited or not|
|additive_noise|bool|Determine how the system calculates sensitivity variables at the marginal (or aggregate) cell level.|
|proxy_diag|bool|Determine whether or not to produce a diagnostic report related to the use of a proxy variable.|
|weight_diag|bool|Determine whether or not to produce a diagnostic report related to the use of a weight variable.|
|accept_negative|bool|Include negative values when calculating sensitivity.  This option is required when proxy_size is specified.|
|presort|bool|Setting `presort = True` automatically sorts any specified input tables. Users may disable this feature by specifying `presort = False`.|
|skip_validation|bool|Disables the pre-process validation of expected fields on your input tables. Defaults to False.|
|trace|bool or int|See [the trace option](user_guide.md#python-log-verbosity-trace)|
|capture|bool|See [suppressing and troubleshooting log messages](user_guide.md#suppressing-and-troubleshooting-log-messages-capture)|
|logger|logging.Logger|See [using your own logger](#use-your-own-logger-logger)|

### Sensitivity Input Tables

#### indata

Specifies the input data set containing the respondent micro data. The variables associated with the `dimension` and `unit_id` statements must be character variables.  All other variables must be numeric, except for the `by` variables which can be of either type.

### Sensitivity Output Tables

#### outcell

| Variable | Content |
| -------- | --------|
|CellId|The cell identifier generated by G-Confid. This ID is used to link to the `outconstaint` and `outlargest` datasets.|
|NbAnonym|Number of respondents that contributed to the total of the cell but did not have a value in the `unit_id` variable.|
|AnonymTotalVar|The total value of the anonymous respondents for the cell. G-Confid considers as anonymous respondents any observations on the input micro data file with a missing value for the `unit_id` variable.|
|NbRespondents|Number of respondents that contributed to the total of the cell (excluding the anonymous). This number corresponds to the number of distinct respondents.|
|WeightedNbRespondents|The weighted number of respondents. This variable will exist if a `weight` variable was specified.|
|TotalVar|Total cell value|
|Sensitivity|The sensitivity of the cell. When the `waiver` or `p_waiver` statement is specified, it will contain the cell sensitivity adjusted for waivers.|
|Status|The status of the cell: If the cell is sensitive: `S`. If the cell is not sensitive: `V`.|
|Type|The type of the cell. If the cell is a table cell:  `C`.  If the cell is an aggregate:  `A`.|
|TotalNoise|The total amount of protection that can be provided by a cell against residual disclosure. This variable is required by `SUPPRESSION`. In the absence of negative values or weights, this value is equal to Total.|
|TotalProxySize|Total cell value for the auxiliary variable specified by PROXYSIZE. This variable is only calculated when the PROXYSIZE statement is specified.|
|SensitivityBeforeWaivers|When `waiver` or `p_waiver` is specified, this variable will contain the cell sensitivity before the adjustment for waivers.|
|FavCost|When `waiver` or `p_waiver` is specified, this variable will be added and will contain the cost calculation (FavCost) related to the use of waivers.|
|ShadowTotal|If a `shadow` variable is specified, this variable will be added and will contain the total value of the shadow variable.|
|{by variables}|Variables specified in the `by` parameter will be included on the dataset, if any|

#### outconstraint

| Variable | Content |
| -------- | --------|
|ConstraintId|Identification of a constraint (linear equation).  Integer from 1 to N, where N is the number of constraints that G-Confid identifies.|
|CellId|The cell identifier generated by G-Confid.|
|Coefficient|Coefficient of a cell within a constraint.  If the value is = 1, the cell is on the left side of the equation.  If the value is = -1, the cell is a total.|
|{by variables}|Variables specified in the `by` parameter will be included on the dataset, if any|

#### outlargest

Specifies the output data set containing the information about the largest contributors of a cell.

| Variable | Content |
| -------- | --------|
|CellId|The cell identifier generated by G-Confid.|
|{unit_id}|The variable specified in the unit_id parameter is included on the dataset. Special values `Anon` and `Pool` are used to represent anonymous and the rest of the respondents|
|NbRespondents|Number of respondents that contributed to the total of the cell (excluding the anonymous respondents, but including respondents providing contributions of zero). This number corresponds to the number of distinct respondents.|
|TotalVar|The total value of the respondent data, anonymous ({unit_id}="Anon") or the rest of the respondents ({unit_id}="Pool").|
|TotalPercent|The proportion of the total cell value this respondent represents.|
|WaiverFlag|When `waiver` or `p_waiver` is specified, the WaiverFlag from the input dataset will be added.|
|ShadowTotal|If a shadow variable is specified, the total value of the shadow variable for the respondents, anonymous contributors (`Anon`) or the rest of the respondents (`Pool`).|
|ShadowPercent||
|{by variables}|Variables specified in the `by` parameter will be included on the dataset, if any|

#### outpairs

Specifies the output dataset containing information about the most sensitive observation pairs in each cell. This data set can only be generated when a `weight` is specified with `s_rule="nk"` and a `weight_prot_level` value other than "EXACT".

| Variable | Content |
| -------- | --------|
|CellId|The cell identifier generated by G-Confid.|
|TargetId|Identification of the target (FT1 or FT2), depending on which pair produced higher sensitivity. The observation identified as the target in the cell’s most sensitive target / attacker pair.|
|TargetPt|Precision Threshold (PT) value for the identified target.|
|AttackerId|The observation identified as the attacker in the cell’s most sensitive target / attacker pair. If there is only one respondent in a cell, `AttackerId` will be blank. |
|AttackerSn|Self-Noise (SN) value for the identified attacker. If there is only one respondent in a cell, `AttackerSn` will be blank.|
|RemainderCount|Number of remaining units (other than target/attacker).  If there are two or fewer respondents, `RemainderCount=0`.|
|RemainderN|The total noise (N) for remaining units. If there are two or fewer respondents, `RemainderN=0`.|

#### outtargets

Specifies the output dataset containing information about the most sensitive combination of observations in each cell. This option can only be generated when the `weight` is specified with `s_rule="nk"` and a `weight_prot_level` value other than "EXACT".

| Variable | Content |
| -------- | --------|
|CellId|The cell identifier generated by G-Confid.|
|Id|Includes the ID of the first n observations and the value “Remainder” for the last observation. Includes the ID for the n (as specified by the nk rule parameter) observations contained in the most sensitive combination of targets in the cell, and the value “Remainder” for all remaining observations. If the cell contains n or fewer observations, no “Remainder” is included.|
|PtnVariable|Specifies which PTN variable is included in the row: Precision Threshold (PT) for targets, and cumulative Noise (N) for the remaining units.|
|Value|The value of the variable specified in the corresponding PtnVariable column.|

### Suppression

This module identifies cells to be suppressed in a table, in addition to the sensitive cells, in order to prevent confidential data disclosure.  Aggregated data is processed through a linear programming solver using the constraints generated by the [Sensitivity](#sensitivity) module.

### Suppression Parameters

| Parameter       | Python type  | Description                 |
| ----------------| -------------| --------------------------- |
|incell|[Input dataset](#input-and-output-table-specification)|See [`incell`](#incell) for details.|
|inconstraint|[Input dataset](#input-and-output-table-specification)|See [`inconstraint`](#inconstraint) for details.|
|outsuppress|[Output dataset](#input-and-output-table-specification)|See [`outsuppress`](#outsuppress) for details.|
|outcomplement|[Output dataset](#input-and-output-table-specification)|See [`outcomplement`](#outcomplement) for details.|
|cost_function1|string|Specifies the name of the cost function to be used in phase 1 of the Suppression process. Default = "Size"|
|cost_function2|string|Specifies the name of the cost function to be used in phase 2 of the Suppression process. Optional|
|cost_var1|string|Specifies the name of the cost variable to be used in phase 1 of Suppression process. It must be a numeric variable present in the dataset used for `incell`.  This variable must have only positive values, and missing values are not allowed.  When this parameter is provided, the variable specified is used to calculate the cost function coefficients, instead of `TotalNoise`|
|cost_var2|string|Specifies the name of the cost variable to be used in phase 2 of Suppression process. It must be a numeric variable present in the dataset used for `incell`.  This variable must have only positive values, and missing values are not allowed.  When this parameter is provided, the variable specified is used to calculate the cost function coefficients, instead of `TotalNoise`|
|scale_cost|string|Specifies the method used to reduce the cost function coefficients. Possible values are None, `MEAN` and `SCALE`. See [Scale Cost] for more information(#scale-cost)|
|p1_roundingbase|int|This is the rounding base to use in conjunction with the cost function `ROUNDEDSIZE`|
|sen_roundingbase|int|This is the rounding based used to round sensitivity values. Optional, by default raw values are used.|
|total_roundingbase|int|This is the rounding based used to round `TotalNoise` values. Optional, by default raw values are used.|
|constraint_scale|int|Default = 2|
|suppress_order|int|Default = = 0|
|ambiguity_tolerance|float| Default = 0.00001|
|by|string||
|custom_solver|pulp.LpSolver|An optional custom pulp solver to use instead of the PuLP default.|
|skip_validation|bool|Disables the pre-process validation of expected fields on your input tables. Defaults to False.|
|trace|bool or int|See [the trace option](user_guide.md#python-log-verbosity-trace)|
|capture|bool|See [suppressing and troubleshooting log messages](user_guide.md#suppressing-and-troubleshooting-log-messages-capture)|
|logger|logging.Logger|See [using your own logger](#use-your-own-logger-logger)|
<!-- |multiprocess|bool|Enables the use of multiple processors to distribute the work of the solver. Defaults to False. See [Multiprocessing](user_guide.md#Multiprocessing). *NOTE: May be unstable. Does not work with any custom_solver that logs to disk*| -->

#### Cost Functions

The following cost functions are available

| Function Name | Description |
| ------------- | ----------- |
|CONSTANT|Treats all cells equally (Cost = 1 for all values)|
|SIZE|Uses the Cost Variable directly.|
|DIGITS| $\log_{10}(cost\_var+1)$|
|INFORMATION|$\frac{\log_{10}(cost\_var+1)}{cost\_var+1}$|
|INVERSE|Uses the reciprocal value of the cost variable.|
|ROUNDEDSIZE|Applied rounding to the cost variable.|
|SCALEDINFORMATION|$\text{Round}(\text{mean}(cost\_var) * \frac{\log(cost\_var + 1)}{cost\_var+ 1}, 3)$|
|SCALEDINVERSE|$\text{Round}(\text{mean}(cost\_var) * INVERSE(cost\_var), 3)$|



- *by default, the cost variable is `TotalNoise` (The cell value).

#### Scale Cost

The scale_cost parameter specifies the method used to reduce the cost function coefficients. Possible values are None, `MEAN` and `SCALE`.

When `scale_cost=None`, the cost function coefficients of the LP problem will be used without modifications.

When `scale_cost="SCALE"`, the cost function coefficients of the LP problem will be scaled according to the following algorithm:

$$
y = \frac{(b - a)(x - \text{min})}{\text{max} - \text{min}}
$$

where *b* and *a* are the upper and lower bounds of the scaling interval, *max* and *min* are the maximum and minimum values of the cost function coefficients, *x* is the value of the current coefficient being scaled and *y* is the value of the scaled coefficient.

When `scale_cost="MEAN"`, the cost function coefficients of the LP problem will be reduced according to the following algorithm:

$$
y = \frac{\sum_{i=1}^{n} x_i}{n}
$$

where *x* is the value of the coefficient, *y* is the value of the reduce coefficient and *n* is the number of

coefficients of the cost function.

This parameter is optional and the default value is None.

### Suppression Input Tables

#### incell

This dataset contains the actual table cells, and the sensitive aggregates. The following variables are required in this data set:

- CellID
- TotalNoise
- Sensitivity
- Status
- Type

For information on these variables, please see [`outcell` from the Sensitivity modules output tables](#outcell) for information about these columns.

Note that any by-variables or additional cost variables are required on this dataset.

#### inconstraint

The dataset containing the linear constraints coefficients. Please see [`outconstraint` from the Sensitivity modules output tables](#outconstraint) for information about this dataset.

### Suppression Output Tables

#### outsuppress

In addition to the fields contained in the `incell` dataset, this table contains the following two new fields:

| Variable | Description |
| -------- | ----------- |
| OutStatus | Indicates the status of the cell. `P` indicates publishable and `X` indicated cells to be suppressed. |
| NetVariation | Is the net variation in absolute value of the cell, required to protect sensitive cells. A sensitive cell always has a non-zero net variation. A non-zero net variation for a non-sensitive cell indicates that this cell has been selected as a complement to a sensitive cell. A published cell has a zero net variation. |

#### outcomplement

Optional dataset. If not indicated to be saved or created, no table will be created. <!-- As well, if `multiprocess=True` and you only provide a `cost_function1` but no `cost_function2` then no complements are able to be produced and therefore no table will be created. -->

| Variable | Description |
| -------- | ----------- |
| Phase | Indicates the phase of the suppression process, 1 or 2. |
| ProtectionNumber | Order in which the sensitive cells were protected |
| CellId | Unique cell identification of the sensitive cell for which the complements are listed. |
| ComplementId | Lists the CELLIDs of all the complements of the sensitive cell, for a given phase. |

#### outsuppress_failed and outcomplement_failed

A new feature in the Python version of G-Confid. Upon a failed solver iteration an error is logged but instead of discarding the data from the previous succesful iterations, it is now used to form the regular outputs but they are instead placed in these 2 new output tables. `outsuppress_failed` is always created if a solver fails however `outcomplement_failed` is only created if the `outcomplement` table was requested originally. These 2 tables are identical to their `outsuppress` and `outcomplement` counterparts in format, however they are only created upon solver failure and are otherwise not created.

## Auditing

Verifies the validity of a suppression pattern. In practice, auditing should be used when we decide to change something in the suppression pattern provided by Suppression module. Since G-Confid does not necessarily identify the best suppression pattern in terms of loss of information, it may be useful to make changes to the chosen suppression pattern in order to reduce the loss of information when the additional cells are suppressed. However, by making these changes, there is a risk of choosing a suppression pattern that no longer guarantees the confidentiality of the data. To check the validity of a modified suppression pattern, we can use the Auditing module.

### Auditing Parameters

| Parameter       | Python type  | Description                 |
| ----------------| -------------| --------------------------- |
|incell|[Input dataset](#input-and-output-table-specification)||
|inconstraint|[Input dataset](#input-and-output-table-specification)||
|outaudit|[Output dataset](#input-and-output-table-specification)||
|lb_factor|float|Numeric value for the lower bound factor, which is between 0 and 1 inclusive. Optional, default value = 0.5|
|ub_factor|float|Numeric value for the upper bound factor, which is between 1 and 10 inclusive. Optional, default value = 1.5|
|use_shuttle|bool|Use the shuttle algorithm to compute lower and upper bounds for your cells. Defaults to False.|
|report_level|int|Controls whether a report is generated at the end of processing. Specify 1 to generate the report, 0 to skip the report. Optional, default = 0.|
|by|str|A list of variable names separated by spaces, which are used to create by groups.|
|custom_solver|pulp.LpSolver|An optional custom pulp solver to use instead of the PuLP default.|
|multiprocess|bool|Enables the use of multiple processors to distribute the work of the solver. Defaults to False. See [Multiprocessing](user_guide.md#Multiprocessing). *NOTE: May be unstable. Does not work with any custom_solver that logs to disk*|
|skip_validation|bool|Disables the pre-process validation of expected fields on your input tables. Defaults to False.|
|trace||See [the trace option](user_guide.md#python-log-verbosity-trace)|
|capture||See [suppressing and troubleshooting log messages](user_guide.md#suppressing-and-troubleshooting-log-messages-capture)|
|logger|logging.Logger|See [using your own logger](#use-your-own-logger-logger)|

### Auditing Input Tables

#### `incell` for Auditing

This dataset will have the same structure as [outsuppress](#outsuppress) from the Suppression module.

#### `inconstraint` for Auditing

This dataset will have the same structure as [outconstraint](#outconstraint) from the Sensitivity module.

### Auditing Output Tables

#### outaudit

The output dataset for the Audit module, will have all the columns included on the `incell` input dataset plus the following columns:

| Variable | Description |
| -------- | ----------- |
|LBound |Lower bound value of the cell (lb_factor*TotalNoise) |
|LTolerance |Lower tolerance of the cell(Total–Sensitivity/2).|
|MinValue|Minimum value of the cell, as calculated by the LP solver (should be between LBound and TotalNoise) |
|MidPoint|Midpoint value of the cell ((MinValue+MaxValue)/2)|
|UTolerance|Upper tolerance of the cell(Total+Sensitivity/2) |
|MaxValue|Maximum value of the cell, as calculated by the LP solver (should be between TotalNoise and UBound) |
|UBound|Upper bound value of the cell (ub_factor*TotalNoise) |
|IntervalWidth|Interval width of the cell, in percentage (100*( MaxValue - MinValue)/ Total).|
|MaxMinusMin| The difference between the maximum value and the miniumum value.|
|ProblemIndicator|Problem indicator for the cell. For sensitive cells or sensitive aggregates, its value is 0 for `good protection`, 1 for `unachieved protection`, and 2 for `exact disclosure`. For complements, its value is 0 for `good protection`, and 2 for `exact disclosure`.|

## Optimized Rounding

This module is used to round multi-dimensional tables. Given an input dataset, the system tries to find a rounded table with the same structure that is as close as possible to the given table. The rounded values are additive and controlled; control is maintained by avoiding rounding more than one multiple of the base beyond the original value. Rounding is used to protect frequencies by creating uncertainty about the exact values.

### Optimized Rounding Parameters

| Parameter       | Python type  | Description                 |
| ----------------| -------------| --------------------------- |
|incell|[Input dataset](#input-and-output-table-specification)||
|additive_con|[Input dataset](#input-and-output-table-specification)||
|additive_bound|[Input dataset](#input-and-output-table-specification)||
|outround|[Output dataset](#input-and-output-table-specification)||
|base|int|The rounding base to be used.|
|custom_solver|pulp.LpSolver|An optional custom pulp solver to use instead of the PuLP default.|
|skip_validation|bool|Disables the pre-process validation of expected fields on your input tables. Defaults to False.|
|trace||See [the trace option](user_guide.md#python-log-verbosity-trace)|
|capture||See [suppressing and troubleshooting log messages](user_guide.md#suppressing-and-troubleshooting-log-messages-capture)|
|logger|logging.Logger|See [using your own logger](#use-your-own-logger-logger)|

### Optimized Rounding Input Tables

#### `incell` for Optimized Rounding

| Variable | Description |
| -------- | ----------- |
|CellId|Unique identifier of each cell. This key column must be unique for each row. It is mandatory and must contain real numbers.|
|Total|The value of the cell to be rounded. It is mandatory and must contain real numbers. If a Weight column is not provided, all Weights will default to 1.0. Cells with higher weights are more likely to be rounded to the nearest value that is multiple of the rounding base when the solution when additivity constraints are relaxed (constraint bounds are missing or non-zero)|
|Weight|The weight that is applied by the rounding algorithm. It is mandatory and must contain positive real numbers.|
|CellLB|The cell lower bound value, used to force a cell to be rounded up. For each row, the value of CellLB must be less than or equal to the value of CellUB (unless one has a missing value). This column is optional and if it is not provided, all values will default to missing.|
|CellUB|The cell upper bound value, used to force a cell to be rounded down. For each row, the value of CellUB must be less than or equal to the value of CellLB (unless one has a missing value). This column is optional and if it is not provided, all values will default to missing.|

#### additive_con

This dataset has the same structure as [outconstraint](#outconstraint) for the Sensitivity module.

#### additive_bound

| Variable | Description |
| -------- | ----------- |
|ConstraintId|Unique identifier for each constraint. Mandatory.|
|ConstraintLB|The constraint lower bound used to relax the additivity requirement of certain totals. For each row, the value of ConstraintLB must be less than or equal to the value of ConstraintUB. If the column ConstraintLB is not provided, a value of zero is used for the lower bound of each cell (Which indicates Total cannot be less than than the SUM). It is optional and can contain real numbers or missing values.|
|ConstraintUB|The constraint upper bound, used to relax the additivity requirement of certain totals. For each row, the value of ConstraintUB must be greater than or equal to the value of ConstraintLB. If the column ConstraintUB is not provided, a value of zero is used for the upper bound of all cells(Which indicates Total cannot be greater than the SUM). It is optional and can contain real numbers or missing values.|

### Optimized Rounding Output Tables

#### outround

| Variable | Description |
| -------- | ----------- |
|CellId|Unique identifier for the table cell.|
|UpperResidual|Upper_residual. Contains real numbers between 0 and 1. |
|PositiveShiftIndicator|Positive_shift_indicator. Contains 0 or 1. |
|PositiveBaseShift|Positive_base_shift. Contains positive integers.|
|PositiveShift|Positive_shift. Contains positive real numbers.|
|LowerResidual|Lower_residual. Contains real numbers between 0 and 1.|
|NegativeShiftIndicator|Negative_shift_indicator. Contains 0 or 1. |
|NegativeBaseShift|Negative_base_shift. Contains positive integers. |
|NegativeShift|Negative_shift. Contains positive real numbers.|
|ResidualIndicator|Contains 0 or 1.|
|Shift|The difference between the rounded and unrounded value.|
|AbsoluteShift|The absolute value of the difference between the round and unrounded value. Contains positive real numbers.|
|CellLowerBound|Contains real numbers or missing values.|
|CellUpperBound|Contains real numbers or missing values.|
|Weight|Contains positive real numbers.|
|Total|The original total value.|
|RoundedTotal|The total with rounding applied to it.|

## Technical guide

### Executing the G-Confid modules

To execute a GConfid procedure in a Python script, we first import the G-Confid package alongside any other packages we plan on using:

```python
import gconfid
import pandas as pandas
```

When executing a G-Confid module, we create a new object with a name of our choosing (in this case, "my_sens_result"):

```python
my_sens_result = gconfid.sensitivity(
    indata=indata,
    outcell="pandas,
    outconstraint="pandas,
    outlargest="pandas,
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

- Modules must be referenced using the G-Confid package name, i.e., `gconfid.sensitivity()`
- Parameters (e.g., `hierarchy`) and tables (e.g., `indata`) are specified as comma-separated key-value pairs, and can appear in any order

Outputs can be stored in memory or saved to disk (see [this section](#specifying-output-tables) for details). If stored in memory, they are stored in the user-created object (e.g., `my_sens_result`) created when the procedure was executed:

```python
print(my_sens_result.outlargest) # Print the outlargest table
sens_outlargest = my_sens_result.outlargest # Save outlargest as a new PyArrow Table called "sens_outlargest"
```

### Variable names on input tables

G-Confid parameters reference variables (i.e., columns) from input tables such as `indata`. Any variable referenced by a G-Confid module must consist of a single string without spaces or special characters except underscore (`_`). For example, `"first_name"` is an acceptable variable name while `"first name"` and `"first-name$"` are not. To ensure that input tables are compatible with G-Confid, users may need to modify the variable names. Additional constraints:

- Variable names cannot exceed 64 characters in length.
- Variable names must be unique within an individual input table. (The same variable can appear on multiple input tables without issue.)

Parameters that reference variables include `by`, `var`, `weight`, and `cost_var1`. They are simply referenced by their variable name, and lists of two or more variables are separated by a single space. For example:

- Single variable: `weight = "wgt"`
- Variable list: `by = "province industry"`

Note that variable names are not case-sensitive in G-Confid; if an input table has two or more columns that differ only in case, it is unclear which column will be used during processing. (Users are strongly encouraged to give distinct case-insensitive names to all columns on any input tables.)

### G-Confid log

Log messages are written to the terminal during execution by default.  These messages originate from 2 sources: `Python` (applicable to all modules) code and `C` code (applicable to the Sensitivity module only).  Messages can be displayed in English or French.  

#### Setting The Log Language

The `g-confid` package produces log messages in either English or French.  The package attempts to detect the language from its host environment during import.  Use the function `g-confid.set_language()` to set the language at runtime, specifying a member of the `gconfid.SupportedLanguage` enumeration (i.e. `.en` or `.fr`)

> **Example**: setting the language to French
>
> ```python
> import gconfid
> gconfid.set_language(gconfid.SupportedLanguage.fr)
> res = gconfid.Auditing(...)
> ```

#### Python Log Messages

Python handles logging using the standard [`logging`](https://docs.python.org/3/library/logging.html#) package.  All Python log messages have an associated [*log level*](https://docs.python.org/3/library/logging.html#logging-levels), such as *ERROR*, *WARNING*, *INFO*, and *DEBUG*.  Messages from Python are generally one line per message and are prefixed with a timestamp and level.

By default, only warning and error messages are displayed.  Use the `trace` parameter to change what log levels are displayed.  

##### Python Log Verbosity (`trace=`)

Use the `trace` parameter to control which log levels are printed by specifying one of the following log levels

- `gconfid.log_level.ERROR`
- `gconfid.log_level.WARNING`
- `gconfid.log_level.INFO`
- `gconfid.log_level.DEBUG`

Messages from the specified log level and higher levels will be printed, lower levels will not.  

For convenience, specifying `trace=True` enables all logging output.  

#### Suppressing and Troubleshooting Log Messages (`capture=`)

The `capture` parameter can be used to suppress all log output (using `capture=None`).  This option is disabled by default (`capture=False`).  

Specifying `capture=True` will cause log messages to be printed all at once at the end of procedure execution, instead of printed immediately throughout execution.  The difference may only be noticeable during long running procedure calls.  Using this option can improve performance in some cases, such as when processing a very large number of by groups.  

> *Jupyter Notebooks and Missing Log Messages*  
> When running the Sensitivity module using Jupyter Notebooks, log messages generated from C code may be missing, particular when running on Visual Studio Code on Windows.  
> To fix this issue, try using the option `capture=True`.  
>
> - alternatively [use your own logger](#use-your-own-logger-logger)

Due to how Jupyter notebooks manage Python's terminal output, messages from procedure C code may not be displayed.  To resolve this, specify `capture=True` in the procedure call when running in a Jupyter notebook, or .  

#### Use Your Own Logger (`logger=`)

Use the `logger` parameter to specify a [Logger](https://docs.python.org/3/library/logging.html#logger-objects) that you have created.  All messages from Python and C will be sent to that logger.  This allows customization of message prefixes, support for writing to file, etc.  

Note that procedure C messages are sent to the logger in a single *INFO* level Python log message.  

> ***Example***: writing logs to file  
>
> ```python
> import gconfid
> import logging
> my_logger = logging.getLogger(__name__)
> logging.basicConfig(filename='example.log', encoding='utf-8', level=logging.DEBUG)
> 
> sensitivity_call = gconfid.sensitivity(
>   logger=my_logger,
>    indata=indata,
>    outlargest=True,
> ...
> ```

### Input and output table specification

For both input and output tables, users can specify in-memory objects or files on disk. A number of different formats are supported for both types. Objects are associated with identifiers (e.g., `"pandas dataframe"`) while files are associated with extensions (e.g., `"filename.parquet"`); please see the table below for details. Note that some are recommended for testing purposes only, and not all formats are supported for outputs tables.

#### Supported formats

| Format                | Type   | Supported identifier(s) or extension(s)         | Notes                                                     |
| --------------------- | ------ | ----------------------------------------------- | --------------------------------------------------------- |
| PyArrow Table         | Object | `"pyarrow"`, `"table"`, `"pyarrow table"`       | Recommended format for in-memory objects.                 |
| Pandas DataFrame      | Object | `"pandas"`, `"dataframe"`, `"pandas dataframe"` |                                                           |
| Apache Parquet        | File   | `.parquet`, `.parq`                             | Minimal RAM usage, good performance with large tables.    |
| Apache Arrow IPC      | File   | `.arrow`                                        | Least RAM usage, good performance with large tables.      |
| Apache Feather        | File   | `.feather`                                      | Similar to Apache Arrow IPC      |
| SAS Dataset           | File   | `.sas7bdat`                                     | For testing purposes, input only; not recommended in production. |
| Comma Separated Value | File   | `.csv`                                          | For testing purposes only; not recommended in production.  |

For tips related to file paths in Python, see [Escape Characters and File Paths](#escape-characters-and-file-paths)

#### Specifying input tables

To input from an in-memory object, simply reference the object name directly from the procedure call. The procedure will automatically detect the type of object from amongst the supported types.

```python
    suppression_call = gconfid.suppression(
        incell=df, # where df is a Pandas DataFrame previously generated
        incontraint=table, # where table is a PyArrow Table previously generated
        ... # etc. (parameters, output tables)
        )
```

To specify an input from file, include either a relative or complete file path:

```python
    suppression_call = gconfid.suppression(
        incell="./input_data.parquet", # Parquet file with local reference
        incontraint=r"C:\temp\input_constraints.feather", # Feather file with Windows reference
        ... # etc. (parameters, output datasets)
        )
```

Users can mix both type of inputs as well:

```python
    suppression_call  = gconfid.suppression(
        incell="./input_data.parquet", # Parquet file with local reference
        incontraint=table, # where table is a PyArrow Table previously generated
        ... # etc. (parameters, output tables)
        )
```

#### Specifying output tables

The Banff procedures automatically create a number of output tables. Some of these are optional, and can be disabled by specifying `False`. (Specifying `False` for an optional output will prevent it from being produced at all, possibly reducing memory usage.  Specifying `False` for a mandatory output will result in an error.) The default format for output tables is an in-memory PyArrow Table. To produce the output in another in-memory format, specify its associated _identifier_ as a string. To write outputs to file, specify a file path with a supported _extension_.

See the [table of supported formats](#supported-formats) for a list of identifiers and extensions.

The following example includes both mandatory and optional outputs, saved as a mix of in-memory objects and files.

```python
    result = gconfid.sensitivity(
        indata=my_micro_data,
        outcell=True, # Mandatory output saved as a PyArrow Table (due to defaults)
        outconstraint="pandas", # Mandatory output saved as a Pandas DataFrame
        outlargest="./out_largest.parquet", # Optional output, saved as a parquet file
        outpairs=False, # Optional output disabled
        outtargets="pandas" # Optional output saved as a Pandas DataFrame
        ... # etc. (parameters, output tables)
        )
```

**NOTE: Outputs will automatically overwrite existing objects and files with the same name.**

#### Customize Default Output Specification

To determine the current global default output table format

```python
>>> gconfid.get_default_output_spec()
'pyarrow'
```

- this corresponds to `pyarrow.Table`

The default can be set to any *identifier* from the [table of Supported Formats](#supported-formats)

> **Example**: Switch default output format to `pandas.DataFrame`
>
> ```python
> gconfid.set_default_output_spec('pandas')
> ```

For proc-specific default output formats, the same function can be called on the individual proc modules

> **Example**: Switch default output format for sensitivity *only* to `pandas.DataFrame`
>
> ```python
> gconfid.sensitiv.set_default_output_format('pandas')
> ```

The more granular setting takes precedence for format conflicts. If a specific table on a proc is requested in a certain format, proc-level and global defaults are ignored for that table. Similarly, if a proc-specific default output format is specified, it will override the global default output format for that proc only, otherwise the global value is used.

#### Accessing output tables

For objects saved in memory, access them using the object member naming the output tables:

```python
    result = gconfid.sensitivity(
        indata=my_micro_data,
        outcell=True,
        outconstraint=True, 
        outlargest="pandas",
        ... # etc.
        )
    print(result.outcell) # Print outcell to the terminal
    my_table = result.outconstraint # Save outconstraint as a new object called my_table
```

*Note: because `outcell`,`outconstraint` are mandatory outputs, they would still be accessible as `result.outcell` and `result.outconstraint` even if not specified using the `True` statements.*  

### Solver Configuration

The Suppression, Auditing and Optimized Rounding modules make use of a linear problem solver. The PULP package is used as a generic interface to various solvers, both open-source and commercial. G-Confid will call PULP's default solver `pulp.LpSolverDefault` which we pre-configure to the [`HiGHS`](https://highs.dev/) solver (See the [PuLP documentation](https://coin-or.github.io/pulp/technical/solvers.html#pulp.apis.HiGHS)). This requires the `highspy` package be installed, which is a dependency of the G-Confid package and therefore should be installed alongside G-Confid if using pip or other package managers. If you somehow have G-Confid installed without access to the `highspy` package, the `PULP_CBC_CMD` solver, which is included with PULP, will be used instead. Alternatively, you may instead choose to configure your own custom solver, which you can then specify to use instead of the default when calling your desired module.

```python
import pulp

# This prints the list of available solvers
solver_list = pulp.listSolvers(onlyAvailable=True)
print(solver_list)

# SCIP_PY is another open-source solver that could be used, if it is installed in your environment
my_solver = pulp.SCIP_PY(mip=False, msg = True, logPath="c:/temp/scip.log", timeLimit=600)

res = gconfid.suppression(
    incell=df,
    incontraint=table,
    custom_solver=my_solver, # Provide the solver we want to use instead of the default
    ... # etc. (parameters, output tables)
  )
```

The options available will vary from solver to solver. Solver options are documented [here](https://coin-or.github.io/pulp/technical/solvers.html).

#### SAS with PULP

As of PULP version 3, SAS 9.4 and SAS Viya are now available solvers. Therefore, G-Confid is able to leverage SAS, if a user has SAS installed on their system or access to a SAS server.

When using the SAS94 solver, the `saspy` package is used and therefore must be installed and configured. Once it is configured, the SAS94 solver should be available in PULP.

Configuring saspy involves creating a `sascfg_personal.py` file on the Python search path. If SAS is installed locally in a Windows environment, the file contents could be follows:

```python
SAS_config_names=['default']
default = {
    'provider': 'sas.iomprovider',
    'encoding': 'windows-1252'}
```

With `saspy` installed and configured, the default solver for PULP can be set to be SAS94:

```python
import pulp

solver = pulp.apis.SAS94(msg=True)
pulp.LpSolverDefault = solver
```

For more information on setting up saspy, please see their [documentation](https://sassoftware.github.io/saspy/install.html). SAS Viya can also be used, it relies on the `swat` library.

### Other

#### Input Validation

By default, when executing a G-Confid module your input data tables will have the capitalization of their column names normalized to the expected casing and their columns and content validated. A standard schema is used for all mandatory columns on each input file while the field names that you provide as parameters for each module are added to the standard schema dynamically. Therefore, the casing of the field names passed as parameters (i.e. `By`-variables) must be the same as they appear in the data file they are expected to be found on. Validation and normalization can be turned off by passing `skip_validation=True` to the respective module call.

#### Escape Characters and File Paths

On Windows, the backslash character (`\`) is typically used to separate folders and files in a file path.  

- Example `"C:\users\stc_user\documents\dataset.csv"`

In Python however, the character `\` is an *"escape character"* and is treated specially.  Providing a file path using the example above may cause a runtime error.  To disable this special treatment, use a "*raw string*" by adding the `r` prefix:

- `r"C:\users\stc_user\documents\dataset.csv"`

Alternatively,

- double backslash: `C:\\users\\stc_user\\documents\\dataset.csv`
- forward backslash: `C:/users/stc_user/documents/dataset.csv`

#### Errors and Exceptions

Python generally handles runtime errors by "raising an exception".  This has been adopted by the `gconfid` package.  Whenever an error occurs, an exception is raised.  This could occur while the package is loading or preprocessing input data, running a procedure, or writing output data.  

Generally exceptions will contain a helpful error message.  Exceptions are often "chained" to provide additional context to the exception.  

#### Working with SAS Files in Python

The G-Confid package provides a few useful functions for reading SAS files into memory or converting to another format.  

To use these functions your program must `import gconfid`

|Function|Description|
|--|--|
|`gconfid.io_util.SAS_file_to_arrowipc_file(file_path, destination)`|Reads *SAS dataset* at `file_path` and writes it to *Arrow IPC* file at `destination`|
|`gconfid.io_util.SAS_file_to_feather_file(file_path, destination)`|Reads *SAS dataset* at `file_path` and writes it to *feather* file at `destination`|
|`gconfid.io_util.SAS_file_to_parquet_file(file_path, destination)`|Reads *SAS dataset* at `file_path` and writes it to *parquet* file at `destination`|
|`gconfid.io_util.DF_from_sas_file(file_path)`|Reads *SAS dataset* at `file_path` and returns it as a *`pandas.DataFrame`*|
|`gconfid.io_util.PAT_from_sas_file(file_path)`|Reads *SAS dataset* at `file_path` and returns it as a *`pyarrow.Table`*|

#### Performance Considerations

The formats used for input and output datasets will affect performance.  

When there may not be sufficient RAM available (due to small RAM size or large datasets), datasets should be stored on disk.  The file format selected will have an effect on performance.  Apache Parquet (`.parquet`), Apache Arrow IPC (`.arrow`) and Apache Feather (`.feather`) file formats currently deliver the best performance when using files for input or output datasets.  

Feather should use the least amount of RAM, making it ideal for large datasets or execution environments with little RAM, it is the recommended format for temporary files.  Parquet is generally the smallest file size, however it still provides impressive read and write performance in multi-CPU environments and reasonably minimal RAM usage, it is recommended for medium-long term storage of data.  

Using the SAS dataset format for large input datasets may result in degraded performance, particularly in environments with little RAM.  This format is only recommended for use with small datasets (under a few hundred MB).  Using the SAS format is discouraged in general, with Apache Arrow formats (*parquet* and *feather*) being recommended instead.  

#### Multiprocessing

At the moment, Audit is the only procedure offering a `multiprocessing` option. It is `False` by default. If this parameter is set to `True`, G-Confid will attempt to use the logical processors available to it (`multiprocessing.cpu_count() - 1`) to parallelize the work of solving the Linear Programming problems it needs to solve. Using this option on Linux systems (such as the Zone) should not require any additional modifications to your existing scripts other than setting the new `multiprocess` parameter to `True`. To use this feature on **Windows** machines, other than the parameter setting, your script that you use to call G-Confid needs to be modified slightly by adding what's called a "guard" to the end of your file, as seen here:

<table>
<tr>
<th>Single process</th>
<th>Multiprocess</th>
</tr>
<tr>
<td>

```python
import gconfid

f = gconfid.Auditing(
    incell= "suppresseddata.sas7bdat",
    inconstraint = "inconstraints.sas7bdat",
    ...
)

print(f.outaudit)
```

</td>
<td>

```python
import gconfid

def run_audit():
    f = gconfid.Auditing(
        incell= "suppresseddata.sas7bdat",
        inconstraint = "inconstraints.sas7bdat",
        multiprocess = True
        ...
    )

    print(f.outaudit)

if __name__=="__main__":
    run_audit()
```

</td>
</tr>
</table>