# G-Confid Release Notes

## 2026-XXXX-YY (Version `2.0.2b1`)

* Update jansonn version to 2.15.0

## 2026-July-17 (Version `2.0.1`)

* Production release

## 2026-July-15 (Version `2.0.1b2`)

* Remove upper limit on pyarrow version
* Add testing of Python 3.14
* Fix: Sensitivity now properly identifies the by-group that is being shown in the log

## 2026-May-13 (Version `2.0.1b1`)

- All instances of the G-Confid version number have been normalized to remove the additional 0 padding
- Updated package dependencies:
  - PyArrow < 25 - new versions of PyArrow (22-24) are now available and tested with G-Confid
  - Pandas >= 2.2.0 - an issue with version 2.1.4 of Pandas has been discovered that causes errors in G-Confid
- Explicitly set dtypes on a number of output fields for each proc to avoid an issue when combining results from by-groups

## 2026-April-30 (Version `2.00.000`)

- Production release
- Fix small bug in Audit report displayed values

## 2026-April-28 (Version `2.00.000b16`)

- Suppress: Fixed a crash occuring if all cells in incell are marked as sensitive
- Audit:
  - Fixed a bug in binary set creation causing incorrect min and max values to be output in certain circumstances
  - Improve summary report
  - Improve setting of "ProblemIndicator" field after solving

## 2026-April-09 (Version `2.00.000b15`)

- Default PuLP solver updated to the [HiGHS](https://highs.dev/) solver (See [PuLP docs](https://coin-or.github.io/pulp/technical/solvers.html#pulp.apis.HiGHS))
  - requires the Python package `highspy`
- Added `highspy>=1.13.0` as a required dependency
- Enabled multithreading on default solver
- Suppress: Temporarily disabled `multiprocess` parameter, needs further development
- Audit:
  - With `multiprocess=True` - a `custom_solver` with a logfile or logPath set will now properly produce a cumulative log of all individual process logs at that location
  - Improved logs to report progress during solving

## 2026-March-23 (Version `2.00.000b14`)

- Suppress: Added multiprocessing to distribute solving work over available processors
  - Toggled via new attribute `multiprocess`, False by default

## 2026-March-17 (Version `2.00.000b13`)

- Audit: Added identification and use of binary sets to improve solver efficiency
- Audit: Added multiprocessing to distribute solving work over available processors
  - Toggled via new attribute `multiprocess`, False by default
- Audit: `use_shuttle` is now False by default.

## 2026-February-20 (Version `2.00.000b12`)

- Audit function updated to more closely replicate original SAS behaviour
- Updated for working support of Pandas 3.0
  - Updated upper bound of Pandas version dependency to 4 in pyproject.toml
- Updated build scripts to work with Visual Studio 2022
- Converted python-native procedures to use PyArrow Tables instead of Pandas Datasets
- Updated default output type to PyArrow
- Pandas dependency updated to add support for version 3
- Dropped support for Python 3.10
- Improve efficiency of Suppress bound calculations
- OptRound now properly allows you to leave one or both of the "CellUB" and "CellLB" fields empty
- Added support for Arrow IPC (`.arrow`) files

## 2025-October-23 (Version `2.00.000b11`)

- Minor documentation updates and internal cleanup preparing for public dissemination

## 2025-October-14 (Version `2.00.000b10`)

- G-Confid is now tested for use with Python 3.13
- Default output types can now be set individually on every proc (including sensitivity) or globally for every proc, as shown in the user guide
- The `split_blocks` option of PyArrow's `to_pandas()` method is now set to `False` by default
  - Pandas outputs from Sensitivity no longer require you to apply `.copy()` to disable read-only
- Corrected suppress report to properly count the number of complement cells
- Fixed bug with the improper setting of upper and lower bounds on solver fields
- PyArrow dependency updated to allow version 21.0.0

## 2025-August-18 (Version `2.00.000b9`)

- Outputs are now saved if a solver fails to produce a valid solution
  - During either phase, if a solver returns a non-optimal status or the cell_to_treat count is unchanged between 2 iterations, an error is logged, further solver iterations are not performed and the standard outputs are formed using the results from the last succesful iteration and placed in 2 new output tables: `outsuppress_failed` and `outcomplements_failed` (only if `outcomplements` is requested)
    - For now, only available as Pandas DataFrames

## 2025-July-17 (Version `2.00.000b8`)

- Implemented OutComplement dataset
  - Added new parameter to suppression procedure `outcomplement`
    - Like `outsuppress`, accepts a path to a desired output location
    - Additionally accepts a boolean if you do not wish to save to disk but just want the DataFrame generated
    - defaults to `False` as the dataset can get quite large
- Implements `generate_suppress_report` to log report of suppressed cells after each stage of the suppress procedure
- Fixed bug with validation leading to user-supplied field names not being included in validation schemas

## 2025-June-30 (Version `2.00.000b7`)

- Pre-execute input validation has been added
  - can be skipped using the `skip_validation` parameter
- Fully implements all `suppress_order` conditions in Suppress
- `VerifyNeedRunSuppressCells` macro has been implemented
  - Skips processing and produces a valid Outsuppress if incell contains only sensitive cells, no sensitive cells or only sensitive and cells with Status 'X' or 'P'
- `MINRESP` and `MINRESPW` are now ignored during sensitivity calculation for a cell if all records within that cell have a waiver given
- Corrects a bug causing OptRound to produce an outround file with improperly matched CellIds

## 2025-April-03 (Version `2.00.000b5`)

- `report_level` now properly logs the Audit report when set even if logging is set to INFO level
- `custom_solver` parameter has been added to Suppress, Audit and Opt_Round to supply a custom solver to use in place of the PULP default

## 2025-February-28 (Version `2.00.000b4`)

- Suppress parameter `p1_roundingbase` renamed to `size_roundingbase`
- The Shuttle algorithm is available for Audit and can be used with the `use_shuttle` parameter

## 2025-February-24 (Version `2.00.000b3`)

- new wrapper class created `gconfid.Auditing()`, `gconfid.Opt_Rounding()`
- Alias `gconfid.sensitivity()` renamed to `gconfid.Sensitivity()`
- PULP's the default solver is now used so that the user can change the default solver or options before calling G-Confid
- Input datasets are more case insensitive, G-Confid will try to adjust casing to match the expected names
- `by_variable` parameters were renamed to `by` to match the Sensitivitity module
- Improvements to documentation and examples

## 2025-January-31 (Version `2.00.000b2`)

- new `capture_text` attribute available on `gconfid.Suppression()` and `gconfid.sensitivity()`
  - when `capture=True` specified, this new attribute will hold a copy of the console output
- new suppress wrapper `gconfid.Suppression()`
  - also new test wrapper `gconfid.testing.Suppression()`
- initial draft of migration documentation
  - see [migration guide](/docs/EN/sas_migration_guide.md) for details on sensitivity procedure
  - see [migration tutorial](/docs/EN/sas_migration_tutorial.md) for a walkthrough of calling sensitivity
- remove `no_*` and `reject_*` flag-type parameters
  - see [migration guide](/docs/EN/sas_migration_guide.md) for list of parameters and changes
