#ifndef MESSAGEGCONFIDAPI_EN_H
#define MESSAGEGCONFIDAPI_EN_H

/**************************************************************/
/* M10000 series - Information messages                       */
/* - Severity is EIE_INFORMATION with EI_AddMessage()         */
/* - Other cases: word 'NOTE:' added at the beginning         */
/**************************************************************/
#define M10034 "Range for dimension %s"
#define M10035 "Code %-10s Lo   %10d Hi %10d"
#define M10036 "Code %-10s Data %-10s"
#define M10037 "Range is empty"
#define M10039 "SRule Alpha[%d][%d] = %f"
#define M10040 "Group %d"
//#define M10041 "This group refers to cells without data. The group will not be treated."
//#define M10042 "This group refers to a set of cells of which only one cell contains data. The group will not be treated."
//#define M10043 "This group is not sensitive."
//#define M10044 "This group is sensitive. Its ConstraintId is %d. Its CellId is %d."
//#define M10045 "Group Statistics"
//#define M10046 "This group refers to a set of cells of which only one cell is greater than zero. The group will not be treated."
/**************************************************************/
/* M20000 series - Warning messages                           */
/* - Severity is EIE_WARNING with EI_AddMessage()             */
/* - Other cases: word 'WARNING:' added at the beginning      */
/**************************************************************/
//#define M20028 "Microdata file contains a code that is not in the hierarchy/range for the dimension %s, code '%s'."
#define M20028 "Microdata file contains a code that is either a parent in the hierarchy or a code that is not in the hierarchy/range for the dimension %s, code '%s'. Only children codes will be processed"
#define M20032 "NK k (%g) parameter is lower than %1f."
#define M20033 "PQ parameter (%f). It should be between %.2f and %.2f inclusively."
#define M20034 "Range for dimension %s specifies a code '%s' which is not in the hierarchy for the same dimension."
#define M20035 "Range for dimension %s specifies a code '%s' which is not a final code in the hierarchy for the same dimension."
#define M20036 "The group %d contains less than two cells with valid data. It will not be treated."
#define M20037 "Rule %d has the same k as rule %d. 2 rules should not have the same k."
/**************************************************************/
/* M30000 series - Error messages                             */       
/* - Severity is EIE_ERROR with EI_AddMessage()               */
/* - Other cases: word 'ERROR:' added at the beginning        */
/**************************************************************/
#define M30003 "Too many errors. Stop processing."
#define M30007 "Looking for '%s' but found '%s' instead."
#define M30106 "Hierarchy (%d dimensions) and Range (%d dimensions) do not have the same number of dimensions."
#define M30107 "Code is repeated or range not disjoint while adding code '%d'."
#define M30108 "The decompositions %d and %d of code '%s' are not equivalent."
#define M30109 "Decomposition %d:"
#define M30111 "Codes '%s' and '%s' in decomposition %d of code '%s' have overlapping codes or ranges."
#define M30112 "The codes."
#define M30113 "Range code repeated in range. Code '%s' was found twice."
#define M30115 "Code '%s' has 2 identical decompositions."
#define M30116 "Code '%s' is in two branches."
#define M30117 "First branch."
#define M30118 "Second branch."
#define M30119 "Dimension %d is not valid."
#define M30120 "Code '%s' has two children codes with the same value '%s'."
#define M30121 "C2 does not need parameters."
#define M30122 "Lo %d is greater than Hi %d in range: Code '%s' Lo=%d Hi=%d"
#define M30123 "C2 is not supported."
#define M30126 "Sensitivity rule requires an argument."
#define M30127 "Invalid sensitivity rule type (%s)."
#define M30128 "Duffett is not supported."
#define M30129 "Duffett is not recommended. You should use another sensitivity rule."
#define M30130 "Invalid sensitivity rule (%s)."
#define M30131 "ARB needs %d to %d parameters."
#define M30132 "NK needs at least 2 parameters."
#define M30133 "NK needs a maximum of 3 pairs of parameters."
#define M30134 "NK needs an even number of parameters."
#define M30135 "PQ needs 1 parameter."
#define M30136 "Duffett does not need parameters."
#define M30137 "%s is not a known method"
#define M30138 "Invalid ARB parameter (%g). It must be between -1 and 1 inclusively."
#define M30139 "Invalid ARB parameters (%g %g). Parameters must be in descending order."

#define M30141 "Invalid NK parameter (%g). N must be an integer between %d and %d inclusively."
#define M30142 "Invalid NK parameter (%G). k must be a number strictly between 0 and 100."
#define M30143 "Invalid PQ parameter (%f). It must be strictly between 0 and 1."
#define M30144 "SRule Alpha %d %d %f is < -1"
#define M30145 "SRule Alpha %d %d %f is < SRule Alpha %d %d %f"
#define M30146 "Rule %d has the same n as rule %d. 2 rules cannot have the same n."

#define M30148 "Increment not preceded by a numeric code."
#define M30149 "Increment is not permitted when end code is not a number."
#define M30150 "Increment is not permitted when start code is not a number."
#define M30151 "Code is too long."
#define M30152 "Increment is too long."
#define M30153 "End code '%s' must be a number."
#define M30154 "Start code '%s' must be a number."
#define M30155 "Increment '%s' must be a number."
#define M30156 "Increment cannot be zero!"
#define M30157 "End code '%d' must be greater than start code '%d'."
#define M30158 "Child code '%s' is the same as the parent code '%s'."
#define M30159 "Child code '%s' is an ancestor of the parent '%s'."
#define M30160 "Too many codes in coordinate. Exactly %d codes are expected."
#define M30161 "Code '%s' not in hierarchy %s."
#define M30162 "Too few codes in coordinate. Exactly %d codes are expected."
#define M30163 "A grouping must have at least 2 cells."
#define M30164 "Unknown character in group."
#define M30165 "Two coordinates of group %d are related (coordinates %d and %d are related)."
#define M30166 "The hierarchy contains multiple roots while only one is permitted.\n"
#define M30167 "Multiple roots found: %s"

#define M30201 "Key '%s' not found in waiver_flags_slist\n" /* NO LONGER USED as of changeset 455679*/
#define M30202 "ERROR: Weight variable specified and weight protection level is not exact (so PTN sensitivity should be used) but sensitivity rule type is not compatible with PTN."
#define M30203 "error; value of \"number_of_respondents\" is zero in \"STC_SRuleSensitivity()\""
#define M30204 "error; invalid value '%c' for variable \"ptn_type1\" in \"STC_SRuleSensitivity()\""
#define M30205 "error: value of Cell->WhichNoise is neither 'n' nor 'N' in call to STC_WriteTargets()"
#define M30206 "heap property doesn't hold at i = %d, child_ndx = %d, and parent_ndx = %d ((WhichF=='T')?(LargestFX[child_ndx]->FT):(WhichF=='W')?(LargestFX[child_ndx]->FW):(LargestFX[child_ndx]->FS) = %f, and "
#define M30207 "(WhichF=='T')?(LargestFX[parent_ndx]->FT):(WhichF=='W')?(LargestFX[parent_ndx]->FW):(LargestFX[parent_ndx]->FS) = %f)"

/**************************************************************/
/* M40000 series - Statistics (reports)                       */
/**************************************************************/
/**************************************************************/
/* M00000 series - Miscellaneous                              */
/**************************************************************/
#define M00002 "Error at or before"
#define M00004 "Colon"
#define M00005 "Done"
#define M00006 "Error"
#define M00011 "SemiColon"
#define M00014 "Unknown character"
#define M00042 "Unknown token"
#define M00043 "Reading microdata" //also defined in MessageAPI_*.h
#define M00044 "Sensitivity rule parser"
#define M00045 "Hierarchy parser"
#define M00046 "Range parser"
#define M00047 "Group parser"
#define M00048 "Code"
#define M00049 "Increment"
#define M00050 "Quota reached... no more warning messages will be printed." //also defined in MessageAPI_*.h
#define M00051 "Hierarchy validation"
#define M00052 "Group validation"

#endif
