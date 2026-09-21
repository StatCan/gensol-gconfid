# G-Confid Release Notes

## 2026-September-21 (Version `2.0.2`)

* Update jansonn submodule version to 2.15.0

## 2026-July-17 (Version `2.0.1`)

- Remove upper limit on pyarrow version
- Add testing of Python 3.14
- Fix: Sensitivity now properly identifies the by-group that is being shown in the log
- All instances of the G-Confid version number have been normalized to remove the additional 0 padding
- Updated package dependencies:
  - Pandas >= 2.2.0 - an issue with version 2.1.4 of Pandas has been discovered that causes errors in G-Confid
- Explicitly set dtypes on a number of output fields for each proc to avoid an issue when combining results from by-groups

## 2026-April-30 (Version `2.00.000`)

- Default PuLP solver updated to the [HiGHS](https://highs.dev/) solver (See [PuLP docs](https://coin-or.github.io/pulp/technical/solvers.html#pulp.apis.HiGHS))
  - requires the Python package `highspy`
- Added `highspy>=1.13.0` as a required dependency
- Enabled multithreading on default solver
- Audit:
  - Added identification and use of binary sets to improve solver efficiency
  - Added multiprocessing to distribute solving work over available processors
    - Toggled via new attribute `multiprocess`, False by default
  - `use_shuttle` is now False by default.
  - Improve summary report
- Added support for Arrow IPC (`.arrow`) files
- Outputs are now saved if a solver fails to produce a valid solution
  - During either phase, if a solver returns a non-optimal status or the cell_to_treat count is unchanged between 2 iterations, an error is logged, further solver iterations are not performed and the standard outputs are formed using the results from the last succesful iteration and placed in 2 new output tables: `outsuppress_failed` and `outcomplements_failed` (only if `outcomplements` is requested)
    - For now, only available as Pandas DataFrames
- Pre-execute input validation has been added
  - can be skipped using the `skip_validation` parameter
- `custom_solver` parameter has been added to Suppress, Audit and Opt_Round to supply a custom solver to use in place of the PULP default
- Suppress parameter `p1_roundingbase` renamed to `size_roundingbase`
- `by_variable` parameters were renamed to `by` to match the Sensitivitity module
- remove `no_*` and `reject_*` flag-type parameters
  - see [migration guide](/docs/EN/sas_migration_guide.md) for list of parameters and changes
