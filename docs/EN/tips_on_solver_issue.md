# Tips and Tricks for Suppression

In practice, tables can be complex and filled with extreme values that make it difficult for the solver to find a suppression pattern. When you run into solver issues such as infeasibility, this section provides adjustments and tips on how to effectively use parameters built within G-Confid to find a suppression pattern.

Before making any adjustments, the first thing to do is check your inputs in the outcell table. There are three inputs used in the formulation of the complementary cell suppression problem that need to be checked: TotalNoise, Sensitivity, and the cost variable (if specified). Verify that there are no values that are unusually small or large given your dataset, especially tiny positive values that are very close to zero, such as $1e^{-12}$. When such tiny values exist due to numerical errors from previous steps or macro adjustments, you should review your process or set the values to zero. The ideal range of values for the solver is within six decimal places. However, in practice, values usually fall outside of this range.

If the error remains after ensuring all inputs are reasonable, here are a few things you can try:

1. Switch the solver
2. Scale the microdata
3. Set tolerances
4. Set the cost function

## Switch the solver
The default solver in the Python version of G-Confid is HiGHS. While it is fast, it can be unstable when dealing with a wide range of input values. We recommend trying the SCIP solver if you encounter problems with HiGHS. Although SCIP may take significantly longer to complete, it is more robust and stable for complex tables. Since this requires only one line of code and does not change the problem formulation, it should be your first troubleshooting step.

To use SCIP, you will need to install `pyscipopt` via `pip`. Once installed, `SCIP_CMD` should appear in the `solver_list`, as shown below:

```python
import pulp

# This prints the list of available solvers
solver_list = pulp.listSolvers(onlyAvailable=True)
print(solver_list)

# Using the default solver (PULP_CBC_CMD) but with some options changed
# my_solver = pulp.PULP_CBC_CMD(mip=False, logPath="c:/temp/cbc.log", timeLimit=600)

# SCIP_PY is another open-source solver that could be used, if it is installed in your environment
my_solver = pulp.SCIP_PY(msg = False, logPath="c:/temp/scip.log", timeLimit=600)

res = gconfid.suppression(
    incell=df,
    incontraint=table,
    custom_solver=my_solver, # Provide the solver we want to use instead of the default
    ... # etc. (parameters, output tables)
  )
```

## Scale the microdata/changing the unit

For most surveys, individual cells may be in dollars, while aggregated cells can reach billions of dollars. However, the ideal range for the solver is within six digits to the left or right of the decimal point. In practice, it is often impossible to keep all values within that range. Through testing, we have found that dividing the microdata values by 100,000 can help in cases with extremely large values. This scaling is equivalent to changing the table's units from dollars to hundreds of thousands of dollars.

By applying this scaling to the microdata, all variables involved in the suppression problem including sensitivity, cell totals, and the cost variable (unless specified otherwise) will shift into a more reasonable range. Additionally, while the optimal suppression solution remains unchanged by scaling, the values in the outsuppress table will be shifted. If you prefer to keep dollars as the unit, you will need to reverse the scaling applied at the beginning.

```python
microdata["Value"] = microdata["Value"]/100000
```

### Setting tolerances

G-Confid includes two tolerance parameters: Tolerance in Sensitive() and Ambiguity Tolerance in Suppression(). Both are useful for helping the solver find a solution.
The Tolerance parameter in Sensitive() specifies an upper bound; any sensitivity value below this threshold is deemed too small and is set to zero. For example, if tolerance=0.05, any sensitivity value between 0 and 0.05 will be treated as zero. This value must be between 0 and 1,000.
If you are working with dollar values and the smallest cell value in your table is one dollar, it makes sense to set the Tolerance parameter to 0.01. In this context, anything below one cent is insignificant. However, when working with other types of data, you should apply this parameter with caution, as it determines whether or not a cell is considered sensitive. You must select a threshold that ensures it is truly safe to ignore values below that limit.


``` python
gconfid.sensitiv(
    ...
    tolerance= 0.01
)
```
Ambiguity tolerance serves as a threshold for the solver's solution. The default value is 0.00001. It works as follows: if a cell has a sensitivity score of 19.3394939, the solver will attempt to find that exact amount of protection. However, if the solver lacks sufficient precision, it may produce a solution that provides only 19.33949 in protection. Since the difference (0.0000039) is below the default ambiguity tolerance of 0.00001, G-Confid will accept the solution.
In practice, problems arise when sensitivity values reach the billions. For example, if an aggregated cell has a sensitivity value of 19,394,858,323, an open-source solver might only provide 19,394,858,300 in protection due to precision limits. The default ambiguity tolerance would reject this solution, causing errors later in the process. In such cases, you can increase the ambiguity tolerance to 100, which instructs G-Confid to ignore discrepancies below that value.


```python
gconfid.Suppression(
    ...
    ambiguity_tolerance = 0.0001
)
```

## Setting cost function

As you may have noticed, it is important to avoid a wide range of inputs and extremely small positive values. If you successfully complete Phase 1 but encounter solver issues in Phase 2, this often indicates a problem with the cost function used in the second phase.

In the Python version of G-Confid, we provide a scaled information cost function for Phase 2. The standard Phase 2 information cost can produce values that are extremely small for certain cells. By using the scaled version, the cost is multiplied by the average cell total, shifting the values into a more ideal range for the solver.

```python
gconfid.Suppression(
    ...
    cost_function2="SCALEDINFORMATION"
)
```