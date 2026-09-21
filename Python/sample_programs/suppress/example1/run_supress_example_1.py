"""Example taken from the G-Confid user guide."""

from pathlib import Path

import gconfid

# Try setting the default output type by uncommenting the
# following line and commenting the `outsuppress=` line below

# The default output can be pandas or pyarrow
gconfid.Suppression.set_default_output_format('pandas')

# The language can be en or fr
gconfid.set_language(gconfid.SupportedLanguage.en)

test_folder = Path(__file__).resolve().parent
input_folder = test_folder / "inputdata"

result=gconfid.Suppression(
    incell=input_folder / "in_cell.sas7bdat",
    inconstraint=input_folder / "in_constraint.sas7bdat",
    cost_function1="Size",
    cost_function2="Information",
    by="QuestionNumber",
)

print(result.outsuppress)
