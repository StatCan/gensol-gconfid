"""Example taken from the methodology prototype guide."""

from pathlib import Path

import gconfid

# Get the directory of this script
test_folder = Path(__file__).resolve().parent
input_folder = test_folder / "inputdata"
output_folder = test_folder / "out"

if not (Path.exists(output_folder)):
    Path.mkdir(output_folder)

f = gconfid.OptRounding(
    incell = input_folder / "cell.sas7bdat",
    additive_con = input_folder / "cellconstraints.sas7bdat",
    additive_bound = input_folder / "constraints.sas7bdat",
    # specify an existing output folder to output to file directly,
    # instead of as an object with default output type from gconfid.Rounding
    #outround = test_folder / "out" / "roundedTable.csv",
    base = 5,
    trace=gconfid.log_level.DEBUG,
)

# If outround not given above, prints the DataFrame
# If outround given as a filepath above, prints the filepath
print(f.outround)
