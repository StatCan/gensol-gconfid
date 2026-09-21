"""Example taken from the methodology prototype (Example 2)."""

from pathlib import Path
import gconfid
import gconfid._log as lg

test_folder = Path(__file__).resolve().parent
input_folder = test_folder / "inputdata"

f = gconfid.Auditing(
    incell= input_folder / "outpattern.sas7bdat",
    inconstraint = input_folder / "outconstraint_ex2.sas7bdat",
    # specify an existing output folder to output to file directly, 
    # instead of as an object with default output type from gconfid.Auditing
    #outaudit = test_folder / "out" / "outaudit.csv",
    report_level = 1,
    trace=lg.log_levels.DEBUG,
)

# If outaudit not given above, prints the DataFrame
# If outaudit given as a filepath above, prints the filepath
print(f.outaudit)
