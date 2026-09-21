"""Example taken from the G-Confid user guide."""

from pathlib import Path

import gconfid
import gconfid._log as lg

test_folder = Path(__file__).resolve().parent
input_folder = test_folder / "inputdata"
output_folder = test_folder / "out"

if not (Path.exists(output_folder)):
    Path.mkdir(output_folder)

f = gconfid.Auditing(
    incell= input_folder / "suppresseddata.sas7bdat",
    inconstraint = input_folder / "inconstraints.sas7bdat",
    # specify an existing output folder to output to file directly,
    # instead of as an object with default output type from gconfid.Auditing
    outaudit = output_folder / "outaudit.csv",
    report_level = 1,
    trace=lg.log_levels.DEBUG,
    by = "QuestionNumber",
)

# If outaudit not given above, prints the DataFrame
# If outaudit given as a filepath above, prints the filepath
print(f.outaudit)
