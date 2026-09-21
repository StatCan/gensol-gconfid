#ifndef MESSAGEGCONFID_EN_H
#define MESSAGEGCONFID_EN_H

#include "GConfidIdentifiers.h"

/********************************************************************************************/
/* Introduced during redesign conversion                                                    */
/********************************************************************************************/
/* return code descriptions */
#define RC_DESC_SUCCESS "successful execution"
#define RC_DESC_FAIL_UNHANDLED "unexpected error"
#define RC_DESC_FAIL_INIT_IN_DATASET "failed to initialize input dataset"
#define RC_DESC_FAIL_READ_PARMS_LEGACY "failed to get parameters"
#define RC_DESC_FAIL_MISSING_DATASET "missing mandatory dataset"
#define RC_DESC_FAIL_VARLIST_NOT_FOUND "variable in varlist not found"
#define RC_DESC_FAIL_VARLIST_INVALID_COUNT "invalid number of variables in varlist"
#define RC_DESC_FAIL_VARLIST_SYSTEM_GENERATED "system generated varlist invalid"
#define RC_DESC_FAIL_SETUP_DATASET_IN "failed to setup input dataset"
#define RC_DESC_FAIL_SETUP_OTHER "unexpected setup error"
#define RC_DESC_FAIL_ALLOCATE_MEMORY "failed to allocate memory"
#define RC_DESC_FAIL_LPI_INIT "LPI initialization failed"
#define RC_DESC_FAIL_EDITS_PARSE "failed to parse edits"
#define RC_DESC_FAIL_VALIDATION_LEGACY "failed legacy validation"
#define RC_DESC_FAIL_VALIDATION_NEW "failed validation"
#define RC_DESC_FAIL_EDITS_OTHER "unexpected edits related error"
#define RC_DESC_FAIL_EDITS_CONSISTENCY "failed edits consistency check"
#define RC_DESC_FAIL_EDITS_REDUNDANCY "failed edits redundancy check"
#define RC_DESC_FAIL_NAME_TOO_LONG "variable name exceeds maximum length"
#define RC_DESC_FAIL_READ_GENERIC "unexpected error reading dataset"
#define RC_DESC_FAIL_WRONG_SORT_ORDER "sort order invalid"
#define RC_DESC_FAIL_READ_SYNC "failed to synchronize datasets"
#define RC_DESC_FAIL_READ_DUPLICATE_DATA "duplicate data detected"
#define RC_DESC_FAIL_WRITE_GENERIC "failed to write to output dataset"
#define RC_DESC_FAIL_PROCESSING_GENERIC "unexpected processing error"
#define RC_DESC_EIE_TRANSFORM_FAIL "transform failure"
#define RC_DESC_EIE_KDTREE_FAIL "KDTREE failure"
#define RC_DESC_EIE_MATCHFIELDS_FAIL "match fields error"

/* Generic */

#define MsgParmMandatory "%s is mandatory."
#define MsgParmNotSpecified "%s not specified."
#define MsgParmEqualDouble "%s = %.*f"
#define MsgParmEqualInteger "%s = %d"
#define MsgParmEqualString "%s = %s"
#define MsgParmEqualDoubleDefault "%s = %.*f (default)"
#define MsgParmEqualIntegerDefault "%s = %d (default)"
#define MsgFooterAllByGroup "The above message was for the total of all by-groups."
#define MsgHeaderForByGroupAbove_SAS_FREE "The above message was for the following " GPN_BY " group:" "\n" MSG_INDENT_NOTE
#define MsgDoubleNotInRange "%s must be between %f and %f inclusively."
#define MsgIntegerNotInRange "%s must be between %d and %d inclusively."
#define MsgNoObservationsInDataSet "No observations in %s data set."
#define MsgNoValidObservationsInDataSet "No valid observations in %s data set."
#define MsgParmWithDuplicateVariable "A variable is repeated in %s."
#define MsgVarNameInTwoStatementsExclusive "Variable %s is listed in %s and %s statements. These statements are mutually exclusive."
#define MsgStatementsExclusive "'%s' and '%s' statements cannot both be specified."
#define MsgBothOptionAndItsOpposite "%s and %s options were both specified. %s will be used by default."
#define Msg2OptionsButNoStatement "%s options (%s, %s) were specified while the %s statement was not specified. All %s related options will be ignored."
#define Msg3OptionsButNoStatement "%s options (%s, %s, %s) were specified while the %s statement was not specified. All %s related options will be ignored."
#define MsgProxypercAndProxyratioSpecified "Both %s and %s options were specified. %s will be ignored."

#define MsgNumberDroppedInDataSetMissingValueForVar "There were %d observations dropped from the %s data set because the variable %s is missing."
#define MsgNumberDroppedInDataSetNegativeValueForVar "There were %d observations dropped from the %s data set because the variable %s is negative."
#define MsgNumberReadInDataSetNegativeValueForVar "There were %d observations read from the %s data with negative values in variable %s."
#define MsgNumberReadInDataSetNonPositiveValueForVar "There were %d observations read from the %s data with negative or zeroes in variable %s."
/* #define MsgNumberReadInDataSetInvalidValueForWaiver "There were %d observations read from the %s data set in which the variable %s has an invalid value; the value 0 will be used." */
#define MsgNumberReadInDataSetMissingValueForVar "There were %d observations read from the %s data set with missing values in variable %s"
#define MsgNumberReadInDataSetNegativeOrMissingValueForVar "There were %d observations read from the %s data set with missing or negative values in variable %s"
/* #define MsgWeightSruleInvalid "Invalid combination of SRULE and WEIGHTPROTLEVEL specified. Srules \"duffet\" and \"arb\" are only compatible with WEIGHTPROTLEVEL=\"EXACT\" for PTN sensitivity." */
#define MsgWeightSruleInvalid "SRULE type must be \"PQ\", \"NK\", or \"C2\" when a WEIGHT variable and WEIGHTPROTLEVEL not equal to \"EXACT\" are specified (for PTN sensitivity)."
#define MsgSruleNkPtnTooManyRules "When using the NK rule with the WEIGHT statement and a WEIGHTPROTLEVEL other than EXACT, a maximum of 2 nk rules can be specified."
#define MsgInvalidOptionValue "Invalid value \"%s\" for option \"%s\"."
#define MsgInvalidParameterCombination "Invalid combination of parameters: a weight variable and a weight protection level other than \"EXACT\" has been specified along with waivers."
#define MsgParameterMustBeGreaterThan1 "Parameter \"%s\" = %f must be > 1.0."
#define MsgParameterMustBeGreaterThanX "The value for %s parameter should be a value greater than %1.0f."
#define MsgParam1RequiresParam2 "A value for the %s parameter was specified but no %s variable was specified."
#define MsgParameterRequired "Parameter %s must be specified when the %s statement is used."
#define MsgOuttargetsInvalid "OUTTARGETS dataset specified, but OUTTARGETS can only be generated when WEIGHT is specified, WEIGHTPROTLEVEL is not 'EXACT' and the SRULETYPE is NK."
#define MsgOutpairsInvalid "OUTPAIRS dataset specified, but OUTPAIRS can only be generated when WEIGHT is specified, WEIGHTPROTLEVEL is not 'EXACT' and the SRULETYPE is PQ."

/* Specific */

/* Sensitivity */

#define MsgNumberDroppedMissingDimension "There were %d observations dropped from %s data set because one of the %s variables is missing."
#define MsgNumberDroppedIllegalCharactersDimension "There were %d observations dropped from %s data set because one of the %s variables contains illegal characters."
#define MsgNumberDroppedDimensionNotInHierarchy "There were %d observations dropped from %s data set because one of the %s variables is not in the hierarchy."

#define MsgObsNotUsedForSensitivityCalculation "These observations will not be used for sensitivity calculation."

#define MsgValueMissing "%s value is missing. The observation is dropped."
#define MsgValueNegative "%s value is negative. The observation is dropped."
#define MsgCodeMissing "A code is missing for dimension %s. The observation is dropped."
#define MsgCodeHasIllegalCharacters "A code has illegal characters for dimension %s. The observation is dropped."

#define MsgNoMoreMessages "Quota reached... no more warning messages will be printed." //also defined in MessageAPI_*.h

#define MsgSensitivityChanged "The sensitivity was changed because it is larger than the total value of the cell."
#define MsgInternalCellsSensitivityChanged "  Internal cells sensitivity was changed %d times."
#define MsgMarginalCellsSensitivityChanged "  Marginal cells sensitivity was changed %d times."
#define MsgAggregatesSensitivityChanged "  Sensitive aggregates sensitivity was changed %d times."
#define MsgUserGroupsSensitivityChanged "  User groupings sensitivity was changed %d times."

#define MsgToleranceTooBigForYourOwnGood "It is not recommended to use a %s greater than %f. Some confidential cells may not be identified when the tolerance exceeds this value."
#define MsgNbHierarchiesNotEqualNbDimensions "The number of dimensions (%d) in %s statement does not match the number of variables (%d) in the %s statement."
#define MsgMinRespGreaterZero "The value for MINRESP parameter must be an integer greater than 0."
#define MsgMinRespOutsideCommonRange "The value for MINRESP parameter is outside of commonly used range 3-5."
#define MsgToleranceMinRespNotCompatible "TOLERANCE must be lower than 1.00 when MINRESP is active."
#define MsgMinRespTooBigChanged "The value for MINRESP parameter is too large. Its value has been replaced with %d."

#define MsgReadingMicrodata "Reading microdata" //also defined in MessageAPI_*.h

#define MsgMicrodataStatisticsHeader "Microdata Statistics"
#define MsgCellStatisticsHeader "Cell Statistics"
#define MsgNumberCalculated "Number calculated"
#define MsgNumberSensitive "Number sensitive"
#define MsgPercentSensitive "Percent sensitive"
#define MsgCellsZeroValue "All cells with zero value"
#define MsgCellsNonZeroValue "All cells with non-zero value"
#define MsgInternalCells "Internal cells"
#define MsgMarginalCells "Marginal cells"
#define MsgAllCells "All cells"
#define MsgAggregates "Aggregates"
#define MsgUserGroups "User Groups"
#define MsgTotal "Total"

#define MsgWaiversListKvPairAdded "added key value pair (\"%s\", %d) to list of waiver flags keys and list of waiver flags values"
#define MsgWaiversListKvPairChanged "changed key value pair (\"%s\", %d) in list of waiver flags keys and list of waiver flags values to (\"%s\", %d)"
#define MsgWaiversListNoChangeNeeded "input waiver flag key value pair (\"%s\", %d) consistent with key value pair already in list of waiver flags keys and list of waiver flags values; lists left unchanged"
#define MsgWaiversListDisplayStart "waiver flag key value pairs = ["
#define MsgWaiversKeyListFailed "Failed to add key to list of waiver flags keys"
#define MsgWaiversKeyValuePairFailed "Failed to add key value pair (\"%s\", %d) to list of waiver flags keys and list of waiver flags values"
#define MsgWaiversKeyValueListFailed "Failed to add value to list of waiver flags values"
#define MsgWaiversWaiverFlagNotConsistent "Contributor '%s' has a waiver flag for some but not all contributions. Please verify the consistency of the waiver flag and rerun.\n"
#define MsgWaiversWaiverFlagUnabletoUpdate "Unable to update waiver flags lists\n"
#define MsgWaiversReadDataMemoryError "memory error in call to \"ReadData()\" "
/* #define MsgWaiversNumberWaiverFlagNotPresentDataset "There were %d observations with the waiver flag variable that is not present in the input dataset. The value 0 will be used."  */
#define MsgWaiversNumberWaiverFlagMissingDataset "There were %d observations with a missing value in the waiver flag variable. The value 0 will be used (i.e. no waiver)."
#define MsgWaiversNumberWaiverFlagInvalidDataset "There were %d observations with an invalid value in the waiver flag variable. The value 0 will be used (i.e. no waiver)."
#define MsgInvalidValueInFunctionCall "Invalid value for \"%s\" in call to \"%s\"."

#define MsgProxyRatio "ProxyRatio (calculated from ProxyPercentile = %10.5f) = %10.5f"
#define MsgCellZeroContributions "cell with all contributions equal to zero encountered in \"generate_weight_and_proxy_diagnostics()\""
#define MsgCellInvalidContributions "cell with invalid value %d for \"TotalMixedSignStatus\" field encountered in \"generate_weight_and_proxy_diagnostics()\""
#define MsgInvalidWhichNoise "Error: invalid value for \"Cell->WhichNoise\" in call to \"WriteCellObs()\""

#define MsgMemoryErrorCalculateProxyRatio "error in \"CalculateProxyRatio()\"--unable to allocate memory for array of doubles \"Heap\""
#define MsgMemoryError "heap property doesn't hold at c = %d, i = %d, child_ndx = %d, && parent_ndx = %d (Heap[child_ndx] = %f, && Heap[parent_ndx] = %f)"

/* #define MsgTotalObs "Total number of observations"                   */
/* #define Msg1     "1. Number of valid observations"                   */
/* #define Msg1a    "   a. Number of anonymous respondents"             */
/* #define Msg1b    "   b. Number of non-anonymous respondents"         */
/* #define Msg1c    "   c. Negative data for respondents"               */
/* #define Msg1d    "   d. Negative or Missing data for proxy variable" */
/* #define Msg1e    "   e. Zero or negative data for weight variable"   */
/* #define Msg2     "2. Number of observations with zero value"         */
/* #define Msg3     "3. Number of invalid observations"                 */
/* #define Msg3a    "   a. Missing data for respondents"                */
/* #define Msg3b    "   b. Negative data for respondents"               */
/* #define Msg3c    "   c. Missing code for dimension"                  */
/* #define Msg3d    "   d. Invalid code for dimension"                  */
/* #define Msg3e    "   e. Missing data for shadow variable"            */
/* #define Msg3f    "   f. Missing data for weight variable"            */

#define MsgTotalObs "Total number of observations"
#define Msg1     "1. Number of valid observations"
#define Msg1a    "   a. # of valid observs from anonymous respondents"
#define Msg1b    "   b. # of valid observs from non-anonymous respondents"
#define Msg1c    "   c. # of valid observs with negative data for respondents"
#define Msg1d    "   d. # of valid observs with negative or missing data for proxy variable"
#define Msg1e    "   e. # of valid observs with zero or negative data for weight variable"
#define Msg1f    "   f. # of valid observs with invalid data for waiver variable"
#define Msg2     "2. Number of valid observations with zero value"
#define Msg3     "3. Number of invalid observations"
#define Msg3a    "   a. # of observs invalid due to missing data for respondents"
#define Msg3b    "   b. # of observs invalid due to negative data for respondents"
#define Msg3c    "   c. # of observs invalid due to missing code for dimension"
#define Msg3d    "   d. # of observs invalid due to invalid code for dimension"
#define Msg3e    "   e. # of observs invalid due to missing data for shadow variable"
#define Msg3f    "   f. # of observs invalid due to missing data for weight variable"
#define Msg4     "4. # of observs invalid because dimension code set not in hierarchy"

/* #define Msg3f    "   f. Negative or Missing data for proxy variable" */
/* #define Msg3g    "   g. Missing data for weight variable"            */
/* #define Msg3h    "   h. Zero or negative data for weight variable"   */

/* #define Msg4     "4. Number of observations excluded (not in hierarchy)" */

#define TableW1L1        "Table W1: Effect on sensitivity of using a weight variable"                 
#define TableW1LineShort "                                        ----------- ----------- -----------" 
#define TableW1L2        "                                        Sensitivity Sensitivity Sensitivity" 
#define TableW1L3        "                                          decreased   unchanged   increased" 
#define TableW1LineLong  "    ----------------------------------- ----------- ----------- -----------" 
#define TableW1L4        "    Number of internal cells           "     "%12d"      "%12d"      "%12d"
#define TableW1L5        "    Number of marginal cells           "     "%12d"      "%12d"      "%12d"
#define TableW1L6        "    Total number of published cells    "     "%12d"      "%12d"      "%12d"

#define TableW2L1        "Table W2: Effect on cell status counts of using a weight variable"           
#define TableW2LineShort "                                    ------------------- -------------------" 
#define TableW2L2        "                                             Unweighted            Weighted" 
#define TableW2L3        "                                            sensitivity         sensitivity" 
#define TableW2L4        "                                         Safe Sensitive      Safe Sensitive" 
#define TableW2LineLong  "    ------------------------------- --------- --------- --------- ---------" 
#define TableW2L5        "    Number of internal cells       "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableW2L6        "    Number of marginal cells       "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableW2L7        "    Total number of published cells"   "%10d"    "%10d"    "%10d"    "%10d"
#define TableW2L8        "    Number of aggregates           "   "%10d"    "%10d"    "%10d"    "%10d"

#define TableP1L1        "Table P1: Effect on sensitivity of using a proxy variable"                         
#define TableP1LineShort "                                               ----------- ----------- -----------"
#define TableP1L2        "                                               Sensitivity Sensitivity Sensitivity"
#define TableP1L3        "                                                 decreased   unchanged   increased"
#define TableP1LineLong  "    ------------------------------------------ ----------- ----------- -----------"
#define TableP1L4        "    Number of internal cells                  "     "%12d"      "%12d"      "%12d"
#define TableP1L5        "       Contains positive and negative data    "     "%12d"      "%12d"      "%12d"
#define TableP1L6        "       All contributions >=0 and some > 0     "     "%12d"      "%12d"      "%12d"
#define TableP1L7        "       All contributions = 0                  "     "%12d"      "%12d"      "%12d"
#define TableP1L8        "       All contributions <=0 and some < 0     "     "%12d"      "%12d"      "%12d"
#define TableP1L9        "    Number of marginal cells                  "     "%12d"      "%12d"      "%12d"
#define TableP1L10       "       Contains positive and negative data    "     "%12d"      "%12d"      "%12d"
#define TableP1L11       "       All contributions >=0 and some > 0     "     "%12d"      "%12d"      "%12d"
#define TableP1L12       "       All contributions = 0                  "     "%12d"      "%12d"      "%12d"
#define TableP1L13       "       All contributions <=0 and some < 0     "     "%12d"      "%12d"      "%12d"
#define TableP1L14       "    Total number of published cells           "     "%12d"      "%12d"      "%12d"
#define TableP1L15       "       Contains positive and negative data    "     "%12d"      "%12d"      "%12d"
#define TableP1L16       "       All contributions >=0 and some > 0     "     "%12d"      "%12d"      "%12d"
#define TableP1L17       "       All contributions = 0                  "     "%12d"      "%12d"      "%12d"
#define TableP1L18       "       All contributions <=0 and some < 0     "     "%12d"      "%12d"      "%12d"

#define TableP2L1         "Table P2: Effect on cell status counts of using a proxy variable"                  
#define TableP2LineShort  "                                           ------------------- -------------------"
#define TableP2L2         "                                                   Sensitivity         Sensitivity"
#define TableP2L3         "                                                       without                with"
#define TableP2L4         "                                                         proxy               proxy"
#define TableP2LineShort2 "                                           --------- --------- --------- ---------"
#define TableP2L5         "                                                Safe Sensitive      Safe Sensitive"
#define TableP2LineLong   "    -------------------------------------- --------- --------- --------- ---------"
#define TableP2L6         "    Number of internal cells              "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L7         "       Contains positive and negative data"   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L8         "       All contributions >=0 and some > 0 "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L9         "       All contributions = 0              "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L10        "       All contributions <=0 and some < 0 "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L11        "    Number of marginal cells              "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L12        "       Contains positive and negative data"   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L13        "       All contributions >=0 and some > 0 "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L14        "       All contributions = 0              "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L15        "       All contributions <=0 and some < 0 "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L16        "    Total number of published cells       "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L17        "       Contains positive and negative data"   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L18        "       All contributions >=0 and some > 0 "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L19        "       All contributions = 0              "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L20        "       All contributions <=0 and some < 0 "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L21        "    Number of aggregates                  "   "%10d"    "%10d"    "%10d"    "%10d"

#endif
