#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EI_Message.h"
#include "MessageGConfidAPI.h"
#include "STC_Memory.h"
#include "STC_SRule.h"
#include "slist.h"
#include "util.h"


#define PQ_MAX_PARAMETERS (4)

#define MINRESP_SENSITIVITY (1.0)
#define MINRESPW_SENSITIVITY (1.0)


static EIT_RETURNCODE PtnSensitivity (
	STCT_CELL      * Cell,
	STCT_SRULE     * SRule,
	int              ProxySpecified,
	char             ptn_type1,
	char             ptn_type2,
	double         * MaxSensitivityPtr,
	int              UseWaivers,
	double           Cell__TotalFX,
	int              Cell__LargestFXNumberEntries,
	STCT_DATAITEM ** Cell__LargestFX,
	double           Cell__TotalSecondryFX,
	int              Cell__Largst2FXNumberEntries,
	STCT_DATAITEM ** Cell__Largst2FX);
static void InitAlpha (double Alpha[][STCM_MAXALPHA]);
static EIT_BOOLEAN IsC2Supported (void);
static EIT_BOOLEAN IsDuffettSupported (void);
static STCT_SRULE * SRuleAllocate (STCT_SRULE_TYPE Type, int g, int e[]);
static EIT_RETURNCODE SRuleValidate (STCT_SRULE * SRule);
static EIT_RETURNCODE ValidateNkParameters (tSList * List,
	double Alpha[STCM_MAXRULES][STCM_MAXALPHA], int NumberEntries[STCM_MAXRULES], int n);


/*------------------------------------------------------------------------------
Free the STCT_SRULE structure
------------------------------------------------------------------------------*/
void STC_SRuleFree (
	STCT_SRULE * SRule)
{
	STC_FreeMemory (SRule);
}
/*------------------------------------------------------------------------------
Parse a string that contain the sensitivity rule
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_SRuleParse (
	char * Sens,
	STCT_SRULE ** SRule)
{
	double Alpha[STCM_MAXRULES][STCM_MAXALPHA];
	double Beta;
	char * dString;
	int i, j, n;
	tSList * List;
	STCT_SRULE * lSRule;
	int NumberEntries[STCM_MAXRULES];
	EIT_RETURNCODE rc;
	STCT_SRULE_TYPE SRuleType;
	char SRuleTypeString[11];
	char * s;

	*SRule = NULL;

	SList_New (&List);
	if (List == NULL) return EIE_FAIL;

	s = strtok (Sens, " ");
	while (s != NULL) {
		if (SList_Add (s, List) == eSListFail)
			return EIE_FAIL;
		s = strtok (NULL, " ");
	}

	if (List->ne == 0) {
		EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30126);
		SList_Free (List);
		return EIE_FAIL;
	}

	if (strlen (SList_Entry (List, 0)) > strlen (STCM_SRULE_TYPE_DUFFETT_STRING)) {
		EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30130,
			SList_Entry (List, 0));
		SList_Free (List);
		return EIE_FAIL;
	}

	UTIL_StrUpper (SRuleTypeString, SList_Entry (List, 0));
	SList_Remove (List, 0);

	if (strcmp (SRuleTypeString, STCM_SRULE_TYPE_ARB_STRING) == 0) {
		SRuleType = STCE_SRULE_TYPE_ARB;
	}
	else if (strcmp (SRuleTypeString, STCM_SRULE_TYPE_C2_STRING) == 0) {
		if (!IsC2Supported ()) {
			EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30123);//C2 not supported
			SList_Free (List);
			return EIE_FAIL;
		}
		SRuleType = STCE_SRULE_TYPE_C2;
	}
	else if (strcmp (SRuleTypeString, STCM_SRULE_TYPE_DUFFETT_STRING) == 0) {
		if (!IsDuffettSupported ()) {
			EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30128);//Duffett not supported
			SList_Free (List);
			return EIE_FAIL;
		}
		EI_AddMessage (M00044, EIE_MESSAGESEVERITY_WARNING, M30129);//Duffett not recommended
		SRuleType = STCE_SRULE_TYPE_DUFFETT;
	}
	else if (strcmp (SRuleTypeString, STCM_SRULE_TYPE_NK_STRING) == 0) {
		SRuleType = STCE_SRULE_TYPE_NK;
	}
	/* else if (strcmp (SRuleTypeString, STCM_SRULE_TYPE_NKREQ_STRING) == 0) { */
	/* 	SRuleType = STCE_SRULE_TYPE_NKREQ;                                 */
	/* }                                                                       */
	else if (strcmp (SRuleTypeString, STCM_SRULE_TYPE_PQ_STRING) == 0) {
		SRuleType = STCE_SRULE_TYPE_PQ;
	}
	else {
		EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30130, SRuleTypeString);
		SList_Free (List);
		return EIE_FAIL;
	}

	switch (SRuleType) {
	case STCE_SRULE_TYPE_ARB:
		if (List->ne < 1 || List->ne > PQ_MAX_PARAMETERS) {
			EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30131, 1, PQ_MAX_PARAMETERS);
			SList_Free (List);
			return EIE_FAIL;
		}
		break;
#ifdef STC_C2_IS_SUPPORTED
	#include "internal_code/src/STC_SRule_h4.h"
#endif
#ifdef STC_DUFFETT_IS_SUPPORTED
	#include "internal_code/src/STC_SRule_h5.h"
#endif
	case STCE_SRULE_TYPE_NK:
		if (List->ne < 2) {
			EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30132);
			SList_Free (List);
			return EIE_FAIL;
		}
		if (List->ne > 6) {
			EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30133);
			SList_Free (List);
			return EIE_FAIL;
		}
		if ((List->ne) % 2) {
			EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30134);
			SList_Free (List);
			return EIE_FAIL;
		}
		break;
	/* case STCE_SRULE_TYPE_NKREQ:                                        */
	/*     if (List->ne < 2) {                                            */
	/*         EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30132); */
	/*         SList_Free (List);                                         */
	/*         return EIE_FAIL;                                           */
	/*     }                                                              */
	/*     if (List->ne > 6) {                                            */
	/*         EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30133); */
	/*         SList_Free (List);                                         */
	/*         return EIE_FAIL;                                           */
	/*     }                                                              */
	/*     if ((List->ne) % 2) {                                          */
	/*         EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30134); */
	/*         SList_Free (List);                                         */
	/*         return EIE_FAIL;                                           */
	/*     }                                                              */
	/*     break;                                                         */
	case STCE_SRULE_TYPE_PQ:
		if (List->ne != 1) {
			EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30135);
			SList_Free (List);
			return EIE_FAIL;
		}
		break;
	default:
		EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30137,
			SRuleTypeString);
		SList_Free (List);
		return EIE_FAIL;
	}

	InitAlpha (Alpha);
	switch (SRuleType) {
	case STCE_SRULE_TYPE_ARB:
		for (i = 0; i < List->ne; i++) {
			Beta = strtod (SList_Entry (List, i), &dString);
			if (Beta < -1.0 || Beta > 1.0) { //don't use STC_COMPARE() here
				EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR,
					M30138, Beta);
				SList_Free (List);
				return EIE_FAIL;
			}
			Alpha[0][i] = Beta + 1.0;
		}
		for (i = 0; i < List->ne-1; i++) {
			if (Alpha[0][i] < Alpha[0][i+1]) { //don't use STC_COMPARE() here
				EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR,
					M30139,
					Alpha[0][i]-1.0, Alpha[0][i+1]-1.0);
				SList_Free (List);
				return EIE_FAIL;
			}
		}
		NumberEntries[0] = List->ne;
		lSRule = SRuleAllocate (SRuleType, 1, NumberEntries);
		if (lSRule == NULL) return EIE_FAIL;
		for (i = 0; i < NumberEntries[0]; i++)
			lSRule->Alpha[0][i] = Alpha[0][i];
		break;
#ifdef STC_C2_IS_SUPPORTED
	#include "internal_code/src/STC_SRule_h1.h"
#endif
#ifdef STC_DUFFETT_IS_SUPPORTED
	#include "internal_code/src/STC_SRule_h2.h"
#endif
	case STCE_SRULE_TYPE_NK:
		n = 1;
		rc = ValidateNkParameters (List, Alpha, NumberEntries, n);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
		if (List->ne > 2) {
			n = 2;
			rc = ValidateNkParameters (List, Alpha, NumberEntries, n);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
		if (List->ne > 4) {
			n = 3;
			rc = ValidateNkParameters (List, Alpha, NumberEntries, n);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
		lSRule = SRuleAllocate (SRuleType, n, NumberEntries);
		if (lSRule == NULL)
			return EIE_FAIL;
		for (i = 0; i < n; i++) {
			for (j = 0; j < NumberEntries[i]; j++)
				lSRule->Alpha[i][j] = Alpha[i][j];
		}
		break;
	/* case STCE_SRULE_TYPE_NKREQ:                                        */
	/*     n = 1;                                                         */
	/*     rc = ValidateNkParameters (List, Alpha, NumberEntries, n);     */
	/*     if (rc != EIE_SUCCEED) return EIE_FAIL;                        */
	/*     if (List->ne > 2) {                                            */
	/*         n = 2;                                                     */
	/*         rc = ValidateNkParameters (List, Alpha, NumberEntries, n); */
	/*         if (rc != EIE_SUCCEED) return EIE_FAIL;                    */
	/*     }                                                              */
	/*     if (List->ne > 4) {                                            */
	/*         n = 3;                                                     */
	/*         rc = ValidateNkParameters (List, Alpha, NumberEntries, n); */
	/*         if (rc != EIE_SUCCEED) return EIE_FAIL;                    */
	/*     }                                                              */
	/*     lSRule = SRuleAllocate (SRuleType, n, NumberEntries);          */
	/*     if (lSRule == NULL)                                            */
	/*         return EIE_FAIL;                                           */
	/*     for (i = 0; i < n; i++) {                                      */
	/*          for (j = 0; j < NumberEntries[i]; j++)                    */
	/*              lSRule->Alpha[i][j] = Alpha[i][j];                    */
	/*     }                                                              */
	/*     break;                                                         */
	case STCE_SRULE_TYPE_PQ:
		Beta = strtod (SList_Entry (List, 0), &dString);
		if (Beta <= 0.0 || Beta >= 1.0) { //don't use STC_COMPARE() here
			EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30143, Beta);
			SList_Free (List);
			return EIE_FAIL;
		}
#define PQ_WARNING_LOWER_LIMIT 0.03
#define PQ_WARNING_UPPER_LIMIT 0.25
		if (Beta < PQ_WARNING_LOWER_LIMIT || Beta > PQ_WARNING_UPPER_LIMIT) { //don't use STC_COMPARE() here
			EI_AddMessage (M00044, EIE_MESSAGESEVERITY_WARNING, M20033, Beta,
				PQ_WARNING_LOWER_LIMIT, PQ_WARNING_UPPER_LIMIT);
		}
		Alpha[0][0] = Beta + 1.0;
		Alpha[0][1] = 1.0;
		NumberEntries[0] = 2;
		lSRule = SRuleAllocate (SRuleType, 1, NumberEntries);
		if (lSRule == NULL) return EIE_FAIL;
		for (i = 0; i < NumberEntries[0]; i++)
			lSRule->Alpha[0][i] = Alpha[0][i];
		break;
	}

	SList_Free (List);

	rc = SRuleValidate (lSRule);
	if (rc != EIE_SUCCEED) {
		STC_SRuleFree (lSRule);
		return EIE_FAIL;
	}

	*SRule = lSRule;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Print the STCT_SRULE structure
duffett has the SRule->NumberGroups set to 0... nothing is printed.
------------------------------------------------------------------------------*/
void STC_SRulePrint (
	STCT_SRULE * SRule)
{
	int i;
	int j;
	int n;

	if (SRule->Type == STCE_SRULE_TYPE_C2 || SRule->Type == STCE_SRULE_TYPE_DUFFETT)
		return;//confidentiel

	for (i = 0; i < SRule->NumberGroups; i++) {
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "SRule->NumberEntries[%d] = %d\n", i, SRule->NumberEntries[i]);
		if (SRule->Type == STCE_SRULE_TYPE_ARB)
			n = 4;//only show 4 for arbitrary
		else //pq or nk
			n = STCM_MAXALPHA;
		for (j = 0; j < n/*STCM_MAXALPHA*//*SRule->NumberEntries[i]*/; j++)
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, M10039 "\n",
				i, j, SRule->Alpha[i][j]-1.0);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	}
}


/*------------------------------------------------------------------------------
Calculate the sensitivity
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_SRuleSensitivity (
	STCT_SRULE * SRule,
	double entval[],
	int n,
	double TotalValue,
	EIT_BOOLEAN HasAnonymous,
	double AnonymousValue,
	int NonAnonymousPositiveNumberEntries,
	int entwaivers[],
	STCT_CELL * Cell,
	int waiver_flags_present,
	int WeightSpecified,
	int ProxySpecified,
	int UsePtnSensitivity,
	int UseAdditiveNoise,
	int NonPtnSensitivityVariable,
	char ptn_type1,
	char ptn_type2,
	double * Sensitivity,
	double * RawSensitivity)
{
	int i, j;
	double MaxSensitivity;
	EIT_RETURNCODE ReturnCode;
	double s1;
	int    k;
	int    is_anonymous;
	int    at_risk_index;
	int    intruder_index;
	double at_risk_value;
	double intruder_value;
	double largest_value;
	double second_largest_value;
	double MaxSensitivity_waivers;
	int    largest_value_index;
	int    second_largest_value_index;
	double Alphw                         [STCM_MAXRULES][STCM_MAXALPHA];
	double sensitivity_nowaivers_array   [STCM_MAXRULES];
	double sensitivity_waivers_array     [STCM_MAXRULES];
	double entval_for_sensitivity_waivers[2];
    char WhichNoise;
    double MaxSensitivity_ptn_before_waivers;
    int waiver_flag;
    STCT_DATAITEM * DataItemPtr;
    double CandidatePtFactor;
    double CandidatePtFactorCount;
    double ValuePtFactor1;
    double ProxyPtFactor1;
    double ValueEpl1;
    double ProxyEpl1;
    double TotalNoise;
    double FS_1st_or_2nd;
    double ValuePtFactor2;
    double ProxyPtFactor2;
    double ValueEpl2;
    double ProxyEpl2;

	/* Currently, if all records in the cell are waived, MINRESP and MINRESPW will still apply	*/
	/* and may indicate the cell as sensitive. However, in this case we actually want to		*/
	/* retain the lack of sensitivity. Therefore if any flags are present, determine here if	*/
	/* we need to skip the later MINRESP and MINRESPW overwrite. This should probably be done	*/
	/* below where we already iterate over the data, however to ensure this check is not		*/
	/* skipped by some coincidence of if-conditions, just do it here for now to ensure this is	*/
	/* always checked.																			*/
	int all_waived = 0;
	if (waiver_flags_present != 0) {
		for (j = 0; j < Cell->Data.NumberEntries; j++) {
			if (Cell->Data.Item[j]->waiver_flag == 0) {
				// We found a non-waived record
				all_waived = 0;
				break;
			}
			else all_waived = 1;
		}
	}

	for (j = 0; j < STCM_MAXRULES; j++) {
	    sensitivity_nowaivers_array[j] = 0.0;
	    sensitivity_waivers_array  [j] = 0.0;
	}
	entval_for_sensitivity_waivers[0] = 0.0;
	entval_for_sensitivity_waivers[1] = 0.0;

#ifdef STC_DUFFETT_IS_SUPPORTED
	#include "internal_code/src/STC_SRule_h3.h"
#endif
	if (SRule->Type != STCE_SRULE_TYPE_DUFFETT) {
		/* !Duffett */
		MaxSensitivity = -DBL_MAX;
		for (i = 0; i < SRule->NumberGroups; i++) {
			s1 = -TotalValue;
			for (j = 0; j < n && j < SRule->NumberEntries[i]; j++) {
				s1 += SRule->Alpha[i][j] * entval[j];
			}
			sensitivity_nowaivers_array[i] = s1;
			if (MaxSensitivity < s1) MaxSensitivity = s1;
		}
	}
	ReturnCode = EIE_SUCCEED;
	if (!UsePtnSensitivity && waiver_flags_present != 0) {
		for (i = 0; i < SRule->NumberGroups; i=i+1) {
			/*-find the largest contributor that hasn't given a waiver (it will be designated  */
			/* the at-risk contributor) and the two largest contributors (ignoring whether     */
			/* they've given a waiver); if the largest isn't the at-risk contributor, then it  */
			/* will be designated the intruder; if it is the at-risk contributor, then the     */
			/* second-largest will be designated the intruder:                                 */
			at_risk_value  = -DBL_MAX;
			at_risk_index  = -1;
			intruder_value = -DBL_MAX;
			intruder_index = -1;
			largest_value = -DBL_MAX;
			second_largest_value = -DBL_MAX;
			largest_value_index = -1;
			second_largest_value_index = -1;
			for (j = 0; j < Cell->Data.NumberEntries; j=j+1) {
				if (largest_value < SELECT_DI_VAL(Cell->Data.Item[j], WeightSpecified, ProxySpecified)) {
					second_largest_value = largest_value;
					second_largest_value_index = largest_value_index;
					largest_value = SELECT_DI_VAL(Cell->Data.Item[j], WeightSpecified, ProxySpecified);
					largest_value_index = j;
				}
				else if (second_largest_value < SELECT_DI_VAL(Cell->Data.Item[j], WeightSpecified, ProxySpecified)) {
					second_largest_value = SELECT_DI_VAL(Cell->Data.Item[j], WeightSpecified, ProxySpecified);
					second_largest_value_index = j;
				}
				is_anonymous = 1;
				for (k = 0; k < (int)strlen(Cell->Data.Item[k]->Key); k=k+1) {
					if (' '!= (Cell->Data.Item[k]->Key)[k]) {
						is_anonymous = 0;
						break;
					}
				}
				if (!is_anonymous) {
					if (waiver_flags_present == 2 || waiver_flags_present == 1) {
						if ((Cell->Data.Item[j]->waiver_flag == 0) && (at_risk_value < SELECT_DI_VAL(Cell->Data.Item[j], WeightSpecified, ProxySpecified))) {
							at_risk_value = SELECT_DI_VAL(Cell->Data.Item[j], WeightSpecified, ProxySpecified);
							at_risk_index = j;
						}
					}
				}
			}
			if ((Cell->Data.NumberEntries == 0) || (at_risk_value == 0.0)) {
				/*-at-risk contributor: */
				Alphw[i][0] = 0.0;
				entval_for_sensitivity_waivers[0] = (at_risk_index  == -1)?0.0:SELECT_DI_VAL(Cell->Data.Item[at_risk_index ], WeightSpecified, ProxySpecified);
				/*-intruder: */
				Alphw[i][1] = 0.0;
				entval_for_sensitivity_waivers[1] = (intruder_index == -1)?0.0:SELECT_DI_VAL(Cell->Data.Item[intruder_index], WeightSpecified, ProxySpecified);
			}
			else {
				if (at_risk_index  == -1) {
					Alphw[i][0] = 0.0;
					entval_for_sensitivity_waivers[0] = 0.0;
				}
				else {
					if (SELECT_DI_VAL(Cell->Data.Item[largest_value_index], WeightSpecified, ProxySpecified) == 0.0) {
						Alphw[i][0] = 0.0;
					}
					else {
						Alphw[i][0] = 1.0 + (sensitivity_nowaivers_array[i] +
							                    (TotalValue - (                                    SELECT_DI_VAL(Cell->Data.Item[       largest_value_index], WeightSpecified, ProxySpecified)  +
							                                ((Cell->Data.NumberEntries < 2)?0.0:SELECT_DI_VAL(Cell->Data.Item[second_largest_value_index], WeightSpecified, ProxySpecified))
							                                )
							                    )
							                ) / SELECT_DI_VAL(Cell->Data.Item[largest_value_index], WeightSpecified, ProxySpecified);
					}
					entval_for_sensitivity_waivers[0] = SELECT_DI_VAL(Cell->Data.Item[at_risk_index ], WeightSpecified, ProxySpecified);
				}
				/*-intruder: */
				intruder_index = -1;
				for (j = 0; j < Cell->Data.NumberEntries; j=j+1) {
					if (j != at_risk_index) {
						if (intruder_value < SELECT_DI_VAL(Cell->Data.Item[j], WeightSpecified, ProxySpecified)) {
							intruder_index = j;
							intruder_value = SELECT_DI_VAL(Cell->Data.Item[j], WeightSpecified, ProxySpecified);
						}
					}
				}
				if (intruder_index == -1) {
					Alphw[i][1] = 0.0;
					entval_for_sensitivity_waivers[1] = 0.0;
				}
				else {
					Alphw[i][1] = 1.0;
					entval_for_sensitivity_waivers[1] = SELECT_DI_VAL(Cell->Data.Item[intruder_index], WeightSpecified, ProxySpecified);
				}
			}
		}
		/*-note that if all of the contributors have given waivers, no at-risk contributor will  */
		/* be found, and therefore there the sensitivity will be guaranteed to be negative--     */
		/* maybe the algorithm should be changed so that if there is no at-risk contributor      */
		/* then no contributor is designated as an intruder (which will mean that the            */
		/* sensitivity will be zero)--ok, I've done that; I should check whether it's correct??? */
		/* } */
		MaxSensitivity_waivers = -DBL_MAX;
		for (i = 0; i < SRule->NumberGroups; i++) {
			s1 = -TotalValue;
			for (j = 0; j < 2; j=j+1) {
				s1 = s1 + Alphw[i][j] * entval_for_sensitivity_waivers[j];
			}
			sensitivity_waivers_array[i] = s1;
			if (MaxSensitivity_waivers < s1) MaxSensitivity_waivers = s1;
		}
		/*-assign sensitivity calculated taking waivers into account to "MaxSensitivity": */
		MaxSensitivity         = MaxSensitivity_waivers;
	}
    if (UsePtnSensitivity) {
        ReturnCode = PtnSensitivity(Cell, SRule, ProxySpecified, ptn_type1, ptn_type2, &MaxSensitivity, 0,
                                                                                                        Cell->TotalFT,
                                                                                                        Cell->LargestFTNumberEntries,
                                                                                                        Cell->LargestFT,
                                                                                                        Cell->TotalSecondryFT,
                                                                                                        Cell->Largst2FTNumberEntries,
                                                                                                        Cell->Largst2FT
                                   );
        if (ReturnCode == EIE_FAIL) {
            goto error_cleanup;
        }
        if (waiver_flags_present != 0) {
            /*-adjust ptn sensitivity for waivers: */
            /*-first, save the sensitivity before waivers: */
            MaxSensitivity_ptn_before_waivers = MaxSensitivity;
            /*-allowed values of Cell->WhichNoise: 'v'=ValueX, 'p'=ProxyX, 'V'=WeightedValueX, 'P'=WeightedProxyX, 'n'=ValueN, 'N'=ProxyN */
            WhichNoise = UsePtnSensitivity?(                   (ProxySpecified)?'N':'n')
                                          :((WeightSpecified)?((ProxySpecified)?'P':'V')
                                                             :((ProxySpecified)?'p':'v')
                                           );
            /*-calculate "TotalNoise": */
            TotalNoise = 0.0;
            if (Cell->AnonymousDataItem.NumberObservations > 0) {
                TotalNoise = TotalNoise + SELECT_DI_VAL2((&(Cell->AnonymousDataItem)), WhichNoise);
            }
            for (i = 0; i < Cell->Data.NumberEntries; i++) {
                TotalNoise = TotalNoise + SELECT_DI_VAL2(Cell->Data.Item[i], WhichNoise);
            }
            if (!EIM_DBL_EQ((TotalNoise), (SELECT_CE_VAL2(Cell, WhichNoise)))) {
                IO_PRINT_LINE("in STC_SRuleSensitivity(); (TotalNoise, SELECT_CE_VAL2(Cell, WhichNoise)) = (%f, %f)", TotalNoise, SELECT_CE_VAL2(Cell, WhichNoise));
                ReturnCode = EIE_FAIL;
                goto error_cleanup;
            }
            
            /*-next, calculate "ValuePtFactor1" and "ProxyPtFactor1": */
            if (ptn_type1 == 'n') {
                ValuePtFactor1 = DBL_MAX;
                ProxyPtFactor1 = DBL_MAX;
                CandidatePtFactorCount = 0;
                for (i = 0; i <= Cell->Data.NumberEntries; i++)
                {
                    if (i < Cell->Data.NumberEntries) {
                        DataItemPtr = Cell->Data.Item[i];
                    }
                    else { /* (i == Cell->Data.NumberEntries) */
                        if (Cell->AnonymousDataItem.NumberObservations == 0) {
                            continue;
                        }
                        DataItemPtr = &(Cell->AnonymousDataItem);
                    }
                    waiver_flag = DataItemPtr->waiver_flag;
                    if (ProxySpecified) {
                        if ((Cell->Data.NumberEntries + (Cell->AnonymousDataItem.NumberObservations != 0)) == 1) {
                            FS_1st_or_2nd = 0.0;
                        }
                        else {
                            FS_1st_or_2nd = (DataItemPtr == Cell->LargestFS[0])?(Cell->LargestFS[1]->FS        ):(Cell->LargestFS[0]->FS        );
                        }
                        ProxyEpl1 = MaxSensitivity_ptn_before_waivers + TotalNoise - (FS_1st_or_2nd + DataItemPtr->ProxyN);
                        if (DataItemPtr->ProxyPt > 0.0) {
                            CandidatePtFactorCount = CandidatePtFactorCount + 1;
                            CandidatePtFactor = ProxyEpl1 / DataItemPtr->ProxyPt;
                            ProxyPtFactor1 = (CandidatePtFactor <= ProxyPtFactor1)?CandidatePtFactor:ProxyPtFactor1;
                        }
                    }
                    else { /* !ProxySpecified */
                        if ((Cell->Data.NumberEntries + (Cell->AnonymousDataItem.NumberObservations != 0)) == 1) {
                            FS_1st_or_2nd = 0.0;
                        }
                        else {
                            FS_1st_or_2nd = (DataItemPtr == Cell->LargestFS[0])?(Cell->LargestFS[1]->FS        ):(Cell->LargestFS[0]->FS        );
                        }
                        ValueEpl1 = MaxSensitivity_ptn_before_waivers + TotalNoise - (FS_1st_or_2nd + DataItemPtr->ValueN);
                        if (DataItemPtr->ValuePt > 0.0) {
                            CandidatePtFactorCount = CandidatePtFactorCount + 1;
                            CandidatePtFactor = ValueEpl1 / DataItemPtr->ValuePt;
                            ValuePtFactor1 = (CandidatePtFactor <= ValuePtFactor1)?CandidatePtFactor:ValuePtFactor1;
                        }
                    }
                }
                if (CandidatePtFactorCount == 0) {
                    if (ProxySpecified) {
                        ProxyPtFactor1 = 1.0;
                    }
                    else { /* !ProxySpecified */
                        ValuePtFactor1 = 1.0;
                    }
                }
            }
            if (SRule->NumberGroups == 2) {
                /*-next, calculate "ValuePtFactor2" and "ProxyPtFactor2": */
                if (ptn_type2 == 'n') {
                    ValuePtFactor2 = DBL_MAX;
                    ProxyPtFactor2 = DBL_MAX;
                    CandidatePtFactorCount = 0;
                    for (i = 0; i <= Cell->Data.NumberEntries; i++)
                    {
                        if (i < Cell->Data.NumberEntries) {
                            DataItemPtr = Cell->Data.Item[i];
                        }
                        else { /* (i == Cell->Data.NumberEntries) */
                            if (Cell->AnonymousDataItem.NumberObservations == 0) {
                                continue;
                            }
                            DataItemPtr = &(Cell->AnonymousDataItem);
                        }
                        waiver_flag = DataItemPtr->waiver_flag;
                        if (ProxySpecified) {
                            if ((Cell->Data.NumberEntries + (Cell->AnonymousDataItem.NumberObservations != 0)) == 1) {
                                FS_1st_or_2nd = 0.0;
                            }
                            else {
                                FS_1st_or_2nd = (DataItemPtr == Cell->Largst2FS[0])?Cell->Largst2FS[1]->SecondryFS:Cell->Largst2FS[0]->SecondryFS;
                            }
                            ProxyEpl2 = MaxSensitivity_ptn_before_waivers + TotalNoise - (FS_1st_or_2nd + DataItemPtr->ProxyN);
                            if (DataItemPtr->SecondryProxyPt > 0.0) {
                                CandidatePtFactorCount = CandidatePtFactorCount + 1;
                                CandidatePtFactor = ProxyEpl2 / DataItemPtr->SecondryProxyPt;
                                ProxyPtFactor2 = (CandidatePtFactor <= ProxyPtFactor2)?CandidatePtFactor:ProxyPtFactor2;
                            }
                        }
                        else { /* !ProxySpecified */
                            if ((Cell->Data.NumberEntries + (Cell->AnonymousDataItem.NumberObservations != 0)) == 1) {
                                FS_1st_or_2nd = 0.0;
                            }
                            else {
                                FS_1st_or_2nd = (DataItemPtr == Cell->Largst2FS[0])?Cell->Largst2FS[1]->SecondryFS:Cell->Largst2FS[0]->SecondryFS;
                            }
                            ValueEpl2 = MaxSensitivity_ptn_before_waivers + TotalNoise - (FS_1st_or_2nd + DataItemPtr->ValueN);
                            if (DataItemPtr->SecondryValuePt > 0.0) {
                                CandidatePtFactorCount = CandidatePtFactorCount + 1;
                                CandidatePtFactor = ValueEpl2 / DataItemPtr->SecondryValuePt;
                                ValuePtFactor2 = (CandidatePtFactor <= ValuePtFactor2)?CandidatePtFactor:ValuePtFactor2;
                            }
                        }
                    }
                    if (CandidatePtFactorCount == 0) {
                        if (ProxySpecified) {
                            ProxyPtFactor2 = 1.0;
                        }
                        else { /* !ProxySpecified */
                            ValuePtFactor2 = 1.0;
                        }
                    }
                }
            }
            /*-next, calculate the value of "DataItemPtr->ValuePw"         and "DataItemPtr->ProxyPw"         */
            /* and                          "DataItemPtr->SecondryValuePw" and "DataItemPtr->SecondryProxyPw" */
            /* in each dataitem:                                                                                */
            for (i = 0; i <= Cell->Data.NumberEntries; i++)
            {
                if (i < Cell->Data.NumberEntries) {
                    DataItemPtr = Cell->Data.Item[i];
                }
                else { /* (i == Cell->Data.NumberEntries) */
                    if (Cell->AnonymousDataItem.NumberObservations == 0) {
                        continue;
                    }
                    DataItemPtr = &(Cell->AnonymousDataItem);
                }
                waiver_flag = DataItemPtr->waiver_flag;
                if (ptn_type1 == '1') {
                    if (ProxySpecified) {
                        DataItemPtr->ProxyPw = (waiver_flag == 0)?(DataItemPtr->ProxyPt):0;
                    }
                    else {
                        DataItemPtr->ValuePw = (waiver_flag == 0)?(DataItemPtr->ValuePt):0;
                    }
                }
                else { /* (ptn_type1 == 'n') */
                    if (ProxySpecified) {
                        DataItemPtr->ProxyPw = (waiver_flag == 0)?(ProxyPtFactor1 * DataItemPtr->ProxyPt):0;
                    }
                    else {
                        DataItemPtr->ValuePw = (waiver_flag == 0)?(ValuePtFactor1 * DataItemPtr->ValuePt):0;
                    }
                }
                if (SRule->NumberGroups == 2) {
                    if (ptn_type2 == '1') {
                        if (ProxySpecified) {
                            DataItemPtr->SecondryProxyPw = (waiver_flag == 0)?(DataItemPtr->SecondryProxyPt):0;
                        }
                        else {
                            DataItemPtr->SecondryValuePw = (waiver_flag == 0)?(DataItemPtr->SecondryValuePt):0;
                        }
                    }
                    else if (ptn_type2 == 'n') {
                        if (ProxySpecified) {
                            DataItemPtr->SecondryProxyPw = (waiver_flag == 0)?(ProxyPtFactor2 * DataItemPtr->SecondryProxyPt):0;
                        }
                        else {
                            DataItemPtr->SecondryValuePw = (waiver_flag == 0)?(ValuePtFactor2 * DataItemPtr->SecondryValuePt):0;
                        }
                    }
                }
            }
            
            /*-now calculate DataItemPtr->        FW in each dataitem and Cell->TotalFW          */
            /* and           DataItemPtr->SecondryFW in each dataitem and Cell->TotalSecondryFW: */
            Cell->TotalFW         = 0.0;
            Cell->TotalSecondryFW = 0.0;
            for (i = 0; i <= Cell->Data.NumberEntries; i++) {
                if (i < Cell->Data.NumberEntries) {
                    DataItemPtr = Cell->Data.Item[i];
                }
                else { /* (i == Cell->Data.NumberEntries) */
                    if (Cell->AnonymousDataItem.NumberObservations == 0) {
                        continue;
                    }
                    DataItemPtr = &(Cell->AnonymousDataItem);
                }
                waiver_flag = DataItemPtr->waiver_flag;
                if (ProxySpecified) {
                    DataItemPtr->FW = DataItemPtr->ProxyPw + DataItemPtr->ProxyN ;
                }
                else {
                    DataItemPtr->FW = DataItemPtr->ValuePw + DataItemPtr->ValueN ;
                }
                Cell->TotalFW   = Cell->TotalFW        + DataItemPtr->FW     ;
                if (SRule->NumberGroups == 2) {
                    if (ProxySpecified) {
                        DataItemPtr->SecondryFW = DataItemPtr->SecondryProxyPw + DataItemPtr->        ProxyN ;
                    }
                    else {
                        DataItemPtr->SecondryFW = DataItemPtr->SecondryValuePw + DataItemPtr->        ValueN ;
                    }
                    Cell->TotalSecondryFW   = Cell->TotalSecondryFW        + DataItemPtr->SecondryFW     ;
                }
            }
            
            /*-now calculate Cell->LargestFWNumberEntries and Cell->LargestFW  */
            /* and           Cell->Largst2FWNumberEntries and Cell->Largst2FW: */
            for (i = 0; i < STCM_MAXLARGESTS; i = i + 1) {
                Cell->LargestFW[i] = NULL;
            }
            Cell->LargestFWNumberEntries = 0;
            
            for (i = 0; i < STCM_MAXLARGESTS; i = i + 1) {
                Cell->Largst2FW[i] = NULL;
            }
            Cell->Largst2FWNumberEntries = 0;
            
            ReturnCode = FindLargestFX(Cell, Cell->LargestFW, &(Cell->LargestFWNumberEntries), 'W');
            if (ReturnCode == EIE_FAIL) {
                goto error_cleanup;
            }
            if (SRule->NumberGroups == 2) {
                ReturnCode = FindLargestFX(Cell, Cell->Largst2FW, &(Cell->Largst2FWNumberEntries), 'W');
                if (ReturnCode == EIE_FAIL) {
                    goto error_cleanup;
                }
            }
            /*-now recalculate MaxSensitivity (using the PTN_11 sensitivity rule): */
            /* ReturnCode = PtnSensitivity(Cell, SRule, ProxySpecified, '1', '1', &MaxSensitivity); */
            ReturnCode = PtnSensitivity(Cell, SRule, ProxySpecified, '1', '1', &MaxSensitivity, 1,
                                                                                                Cell->TotalFW,
                                                                                                Cell->LargestFWNumberEntries,
                                                                                                Cell->LargestFW,
                                                                                                Cell->TotalSecondryFW,
                                                                                                Cell->Largst2FWNumberEntries,
                                                                                                Cell->Largst2FW
                                       );
            if (ReturnCode == EIE_FAIL) {
                goto error_cleanup;
            }
        }
    }
	/*-output sensitivity BEFORE applying the adjustments for:                                           */
	/*    -the MINRESP rule (if it was specified by the user)                                            */
	/*    -the MINRESPW rule (if it was specified by the user)                                           */
	/*    -the rule that sensitivity must be <= the total value of the cell                              */
	/*    -the rule that sensitivity will be set to 0.0 if it is less than the value of SRule->Tolerance */
	/* --this is the value that will be used for sensitivity BEFORE waivers IF waivers have been         */
	/* specified by the user; if waivers haven't been specified then the value of sensitivity AFTER      */
	/* those adjustments have been applied will be used as the value of sensitivity                      */
	*RawSensitivity = MaxSensitivity;

	/* compute WeightedNbRespondents */
	if (SRule->Parms->Weight != 0) {
		Cell->WeightedNbResp = 0.0;
		for (int i = 0; i <= Cell->Data.NumberEntries; i++) {
			if (i < Cell->Data.NumberEntries) {
				DataItemPtr = Cell->Data.Item[i];
			}
			else { /* (i == Cell->Data.NumberEntries) => "anonymous" */
				if (Cell->AnonymousDataItem.NumberObservations == 0) {
					continue;
				}
				DataItemPtr = &(Cell->AnonymousDataItem);
			}
			if (DataItemPtr->AbsVar != 0.0) {
				Cell->WeightedNbResp += DataItemPtr->WAbsVar / DataItemPtr->AbsVar;
			}
			// else {Cell->WeightedNbResp += 0;} /* this line appears for completeness (as described in the specs) and readability 
		}
	}

	/* If every record of the Cell had a waiver use the calculated sensitivity and ignore the following MINRESP and MINRESPW rules */
	if (all_waived == 0) {
		/* set sensitivity to MINRESP_SENSITIVITY when all these conditions are met */
		if (SRule->MinResp > 0 && /* MinResp is active */
			NonAnonymousPositiveNumberEntries < SRule->MinResp && /* less respondant than the minimum required */
			AnonymousValue == 0.0 && /* there is no anonymous with Value > 0 */
			MINRESP_SENSITIVITY > MaxSensitivity) /* MINRESP_SENSITIVITY is greater than calculated sensitivity */
		{
			MaxSensitivity = MINRESP_SENSITIVITY;
		}

		if ((SRule->Parms->MinRespW) != MINRESPW_UNSPECD_VALUE    /*if - MinRespW is specified by user */
			&& Cell->WeightedNbResp < SRule->Parms->MinRespW      /* &&- less respondant than the minimum required */
			&& MINRESPW_SENSITIVITY > MaxSensitivity              /* &&- MINRESPW_SENSITIVITY is greater than calculated sensitivity */
			) {
			MaxSensitivity = MINRESPW_SENSITIVITY;
		}
	}

	//sensitivity should never be greater than cell value
	//sensitivity should be equal to cell value when it is larger than cell value
	if (MaxSensitivity > TotalValue) {
		//zero cells are not counted
		if (TotalValue != 0.0)
			SRule->NumberSensitivityChanged++;
		MaxSensitivity = TotalValue;
	}

	//set sensitivity to 0.0, if below tolerance
	if (-SRule->Tolerance <= MaxSensitivity && MaxSensitivity < SRule->Tolerance) MaxSensitivity = 0.0;


	*Sensitivity = MaxSensitivity;

	goto normal_cleanup;
error_cleanup:

normal_cleanup:

	return ReturnCode;
}

/*------------------------------------------------------------------------------
Calculate sensitivity using the PTN method, ignoring waivers
------------------------------------------------------------------------------*/
static EIT_RETURNCODE PtnSensitivity (
	STCT_CELL      * Cell,
	STCT_SRULE     * SRule,
	int              ProxySpecified,
	char             ptn_type1,
	char             ptn_type2,
	double         * MaxSensitivityPtr,
	int              UseWaivers,
	double           Cell__TotalFX,
	int              Cell__LargestFXNumberEntries,
	STCT_DATAITEM ** Cell__LargestFX,
	double           Cell__TotalSecondryFX,
	int              Cell__Largst2FXNumberEntries,
	STCT_DATAITEM ** Cell__Largst2FX)
{
	/*-add the following macro definition to "STC_Cell.h":                                                                               */
	/*     #define SELMEM(structptr, selector, memberiftrue, memberiffalse) \                                                            */
	/*     ((selector)?((structptr)->memberiftrue)\                                                                                      */
	/*                :((structptr)->memberiffalse)\                                                                                     */
	/*     )                                                                                                                             */
	/*-add the following parameters to the end of the paramter list of this function:                                                    */
	/*     int             UseWaivers,                                                                                                   */
	/*     double          Cell__TotalFX,                                                                                                */
	/*     int             Cell__LargestFXNumberEntries,                                                                                 */
	/*     STCT_DATAITEM * Cell__LargestFX,                                                                                              */
	/*     double          Cell__TotalSecondryFX,                                                                                        */
	/*     int             Cell__Largst2FXNumberEntries,                                                                                 */
	/*     STCT_DATAITEM * Cell__Largst2FX)                                                                                              */
	/* and then add the appropriate values to the end of the argument list of the 2 calls to this function:                              */
	/*     ReturnCode = PtnSensitivity(Cell, SRule, ProxySpecified, ptn_type1, ptn_type2, &MaxSensitivity);                              */
	/* becomes:                                                                                                                          */
	/*     ReturnCode = PtnSensitivity(Cell, SRule, ProxySpecified, ptn_type1, ptn_type2, &MaxSensitivity, 0,                            */
	/*                                                                                                     Cell->TotalFT,                */
	/*                                                                                                     Cell->LargestFTNumberEntries, */
	/*                                                                                                     Cell->LargestFT,              */
	/*                                                                                                     Cell->TotalSecondryFT,        */
	/*                                                                                                     Cell->Largst2FTNumberEntries, */
	/*                                                                                                     Cell->Largst2FT               */
	/*                                );                                                                                                 */
	/* and:                                                                                                                              */
	/*     ReturnCode = PtnSensitivity(Cell, SRule, ProxySpecified, '1', '1', &MaxSensitivity);                                          */
	/* becomes:                                                                                                                          */
	/*     ReturnCode = PtnSensitivity(Cell, SRule, ProxySpecified, '1', '1', &MaxSensitivity, 1,                                        */
	/*                                                                                         Cell->TotalFW,                            */
	/*                                                                                         Cell->LargestFWNumberEntries,             */
	/*                                                                                         Cell->LargestFW,                          */
	/*                                                                                         Cell->TotalSecondryFW,                    */
	/*                                                                                         Cell->Largst2FWNumberEntries,             */
	/*                                                                                         Cell->Largst2FW                           */
	/*                                );                                                                                                 */
	/*-finally, make the following substitutions in the body of this function:                                                           */
	/*     Cell->TotalFT                --> Cell__TotalFX                                                                                */
	/*     Cell->LargestFTNumberEntries --> Cell__LargestFXNumberEntries                                                                 */
	/*     Cell->LargestFT              --> Cell__LargestFX                                                                              */
	/*     Cell->TotalSecondryFT        --> Cell__TotalSecondryFX                                                                        */
	/*     Cell->Largst2FTNumberEntries --> Cell__Largst2FXNumberEntries                                                                 */
	/*     Cell->Largst2FT              --> Cell__Largst2FX                                                                              */
	/* by doing:                                                                                                                         */
	/*     :'a,'bs/\C\(^\|[^A-Za-z0-9_]\)Cell->\(Total\|Largest\|TotalSecondry\|Largst2\)FT\([^A-Za-z0-9_]\|$\)/\1Cell__\2FX\3/gc        */
	/*     :'a,'bs/\C\(^\|[^A-Za-z0-9_]\)Cell->\(Largest\|Largst2\)FTNumberEntries\([^A-Za-z0-9_]\|$\)/\1Cell__\2FXNumberEntries\3/gc    */
	/* then:                                                                                                                             */
	/*     Cell__LargestFX\[\(.\)\]->FT         --> SELMEM(Cell__LargestFX[\1], UseWaivers, FW, FT)                                      */
	/*     Cell__Largst2FX\[\(.\)\]->FT         --> SELMEM(Cell__Largst2FX[\1], UseWaivers, FW, FT)                                      */
	/*     Cell__LargestFX\[\(.\)\]->SecondryFT --> SELMEM(Cell__LargestFX[\1], UseWaivers, SecondryFW, SecondryFT)                      */
	/*     Cell__Largst2FX\[\(.\)\]->SecondryFT --> SELMEM(Cell__Largst2FX[\1], UseWaivers, SecondryFW, SecondryFT)                      */
	/* by doing:                                                                                                                         */
	/*     :'a,'bs/\C\(^\|[^A-Za-z0-9_]\)Cell__\(Largest\|Largst2\)FX\[\(.\)\]->FT\([^A-Za-z0-9_]\|$\)/\1SELMEM(Cell__\2FX[\3], UseWaivers, FW, FT)\4/gc                         */
	/*     :'a,'bs/\C\(^\|[^A-Za-z0-9_]\)Cell__\(Largest\|Largst2\)FX\[\(.\)\]->SecondryFT\([^A-Za-z0-9_]\|$\)/\1SELMEM(Cell__\2FX[\3], UseWaivers, SecondryFW, SecondryFT)\4/gc */
	int i;
	int number_of_respondents;
	double TotalProxyN;
	double TotalValueN;
	double MaxSensitivity;
	EIT_RETURNCODE ReturnCode = EIE_SUCCEED;
	double MaxSensitivity2;
	
	/*-calculate "TotalProxyN" and "TotalValueN": */
	if (ProxySpecified) {
		/* if (UseAdditiveNoise) {                                       */
		/*     TotalProxyN = Cell->TotalProxyN;                          */
		/* }                                                             */
		/* else {                                                        */
		/*     TotalProxyN = 0.0;                                        */
		/*     for (i = 0; i < Cell->LargestNumberEntries; i++) {        */
		/*         TotalProxyN = TotalProxyN + Cell->Largest[i]->ProxyN; */
		/*     }                                                         */
		/* }                                                             */
		TotalProxyN = 0.0;
		if (Cell->AnonymousDataItem.NumberObservations > 0) {
			TotalProxyN = TotalProxyN + Cell->AnonymousDataItem.ProxyN;
		}
		for (i = 0; i < Cell->Data.NumberEntries; i++) {
			TotalProxyN = TotalProxyN + Cell->Data.Item[i]->ProxyN;
		}
	}
	else {
		/* if (UseAdditiveNoise) {                                       */
		/*     TotalValueN = Cell->TotalValueN;                          */
		/* }                                                             */
		/* else {                                                        */
		/*     TotalValueN = 0.0;                                        */
		/*     for (i = 0; i < Cell->LargestNumberEntries; i++) {        */
		/*         TotalValueN = TotalValueN + Cell->Largest[i]->ValueN; */
		/*     }                                                         */
		/* }                                                             */
		TotalValueN = 0.0;
		if (Cell->AnonymousDataItem.NumberObservations > 0) {
			TotalValueN = TotalValueN + Cell->AnonymousDataItem.ValueN;
		}
		for (i = 0; i < Cell->Data.NumberEntries; i++) {
			TotalValueN = TotalValueN + Cell->Data.Item[i]->ValueN;
		}
	}
	/*-calculate cell sensitivity using PTN method: */
	if      (ptn_type1 == 'n') {
		/*-using PTN_N0 sensitivity rule: */
		/* if (ProxySpecified) {                             */
		/*     MaxSensitivity = Cell__TotalFX - TotalProxyN; */
		/* }                                                 */
		/* else {                                            */
		/*     MaxSensitivity = Cell__TotalFX - TotalValueN; */
		/* }                                                 */
		MaxSensitivity = 0.0;
		for (i = 0; i < SRule->NumberEntries[0]; i = i + 1) {
		    if (i < Cell__LargestFXNumberEntries) {
		        MaxSensitivity = MaxSensitivity + SELMEM(Cell__LargestFX[i], UseWaivers, FW, FT);
		    }
		}
		if (ProxySpecified) {
		    MaxSensitivity = MaxSensitivity - TotalProxyN;
		}
		else {
		    MaxSensitivity = MaxSensitivity - TotalValueN;
		}
	}
	else if (ptn_type1 == '1') {
		/*-using PTN_11 sensitivity rule: */
		number_of_respondents = Cell->Data.NumberEntries + (Cell->AnonymousDataItem.NumberObservations > 0);
		if      (number_of_respondents == 0) {
			IO_PRINT_LINE(M30203);
			ReturnCode = EIE_FAIL;
			goto error_cleanup;
		}
		if (ProxySpecified) {
			if      (number_of_respondents == 1) {
				MaxSensitivity = SELMEM(Cell__LargestFX[0], UseWaivers, FW, FT) - TotalProxyN;
			}
			else if (0 != strcmp(Cell__LargestFX[0]->Key, Cell->LargestFS[0]->Key)) {
				MaxSensitivity = SELMEM(Cell__LargestFX[0], UseWaivers, FW, FT) + Cell->LargestFS[0]->FS - TotalProxyN;
			}
			else { /* (number_of_respondents > 1) && (0 == strcmp(Cell__LargestFX[0]->Key, Cell->LargestFS[0]->Key))) */
				MaxSensitivity = ((SELMEM(Cell__LargestFX[0], UseWaivers, FW, FT) + Cell->LargestFS[1]->FS - TotalProxyN)>
				                  (SELMEM(Cell__LargestFX[1], UseWaivers, FW, FT) + Cell->LargestFS[0]->FS - TotalProxyN))
				                 ?(SELMEM(Cell__LargestFX[0], UseWaivers, FW, FT) + Cell->LargestFS[1]->FS - TotalProxyN)
				                 :(SELMEM(Cell__LargestFX[1], UseWaivers, FW, FT) + Cell->LargestFS[0]->FS - TotalProxyN);
			}
		}
		else { /* (!ProxySpecified) */
			if      (number_of_respondents == 1) {
				MaxSensitivity = SELMEM(Cell__LargestFX[0], UseWaivers, FW, FT) - TotalValueN;
			}
			else if (0 != strcmp(Cell__LargestFX[0]->Key, Cell->LargestFS[0]->Key)) {
				MaxSensitivity = SELMEM(Cell__LargestFX[0], UseWaivers, FW, FT) + Cell->LargestFS[0]->FS - TotalValueN;
			}
			else { /* (number_of_respondents > 1) && (0 == strcmp(Cell__LargestFX[0]->Key, Cell->LargestFS[0]->Key))) */
				MaxSensitivity = ((SELMEM(Cell__LargestFX[0], UseWaivers, FW, FT) + Cell->LargestFS[1]->FS - TotalValueN)>
				                  (SELMEM(Cell__LargestFX[1], UseWaivers, FW, FT) + Cell->LargestFS[0]->FS - TotalValueN))
				                 ?(SELMEM(Cell__LargestFX[0], UseWaivers, FW, FT) + Cell->LargestFS[1]->FS - TotalValueN)
				                 :(SELMEM(Cell__LargestFX[1], UseWaivers, FW, FT) + Cell->LargestFS[0]->FS - TotalValueN);
			}
		}
	}
	else { /* (ptn_type1 != 'n' && ptn_type1 != '1') */
		IO_PRINT_LINE(M30204, ptn_type1);
		ReturnCode = EIE_FAIL;
		goto error_cleanup;
	}
	if (SRule->NumberGroups == 2) {
		/*-calculate cell sensitivity from secondary rule using PTN method: */
		if      (ptn_type2 == 'n') {
			/*-using PTN_N0 sensitivity rule: */
			/* if (ProxySpecified) {                                      */
			/*     MaxSensitivity2 = Cell__TotalSecondryFX - TotalProxyN; */
			/* }                                                          */
			/* else {                                                     */
			/*     MaxSensitivity2 = Cell__TotalSecondryFX - TotalValueN; */
			/* }                                                          */
			MaxSensitivity2 = 0.0;
			for (i = 0; i < SRule->NumberEntries[1]; i = i + 1) {
			    if (i < Cell__Largst2FXNumberEntries) {
			        MaxSensitivity2 = MaxSensitivity2 + SELMEM(Cell__Largst2FX[i], UseWaivers, SecondryFW, SecondryFT);
			    }
			}
			if (ProxySpecified) {
			    MaxSensitivity2 = MaxSensitivity2 - TotalProxyN;
			}
			else {
			    MaxSensitivity2 = MaxSensitivity2 - TotalValueN;
			}
		}
		else if (ptn_type2 == '1') {
			/*-using PTN_11 sensitivity rule: */
			number_of_respondents = Cell->Data.NumberEntries + (Cell->AnonymousDataItem.NumberObservations > 0);
			if      (number_of_respondents == 0) {
				IO_PRINT_LINE(M30203);
				ReturnCode = EIE_FAIL;
				goto error_cleanup;
			}
			if (ProxySpecified) {
				if      (number_of_respondents == 1) {
					MaxSensitivity2 = SELMEM(Cell__Largst2FX[0], UseWaivers, SecondryFW, SecondryFT) - TotalProxyN;
				}
				else if (0 != strcmp(Cell__Largst2FX[0]->Key, Cell->Largst2FS[0]->Key)) {
					MaxSensitivity2 = SELMEM(Cell__Largst2FX[0], UseWaivers, SecondryFW, SecondryFT) + Cell->Largst2FS[0]->SecondryFS - TotalProxyN;
				}
				else { /* ((number_of_respondents > 1) && (0 == strcmp(Cell__Largst2FX[0]->Key, Cell->Largst2FS[0]->Key))) */
					MaxSensitivity2 = ((SELMEM(Cell__Largst2FX[0], UseWaivers, SecondryFW, SecondryFT) + Cell->Largst2FS[1]->SecondryFS - TotalProxyN)>
					                   (SELMEM(Cell__Largst2FX[1], UseWaivers, SecondryFW, SecondryFT) + Cell->Largst2FS[0]->SecondryFS - TotalProxyN))
					                  ?(SELMEM(Cell__Largst2FX[0], UseWaivers, SecondryFW, SecondryFT) + Cell->Largst2FS[1]->SecondryFS - TotalProxyN)
					                  :(SELMEM(Cell__Largst2FX[1], UseWaivers, SecondryFW, SecondryFT) + Cell->Largst2FS[0]->SecondryFS - TotalProxyN);
				}
			}
			else { /* (!ProxySpecified) */
				if      (number_of_respondents == 1) {
					MaxSensitivity2 = SELMEM(Cell__Largst2FX[0], UseWaivers, SecondryFW, SecondryFT) - TotalValueN;
				}
				else if (0 != strcmp(Cell__Largst2FX[0]->Key, Cell->Largst2FS[0]->Key)) {
					MaxSensitivity2 = SELMEM(Cell__Largst2FX[0], UseWaivers, SecondryFW, SecondryFT) + Cell->Largst2FS[0]->SecondryFS - TotalValueN;
				}
				else { /* ((number_of_respondents > 1) && (0 == strcmp(Cell__Largst2FX[0]->Key, Cell->Largst2FS[0]->Key))) */
					MaxSensitivity2 = ((SELMEM(Cell__Largst2FX[0], UseWaivers, SecondryFW, SecondryFT) + Cell->Largst2FS[1]->SecondryFS - TotalValueN)>
					                   (SELMEM(Cell__Largst2FX[1], UseWaivers, SecondryFW, SecondryFT) + Cell->Largst2FS[0]->SecondryFS - TotalValueN))
					                  ?(SELMEM(Cell__Largst2FX[0], UseWaivers, SecondryFW, SecondryFT) + Cell->Largst2FS[1]->SecondryFS - TotalValueN)
					                  :(SELMEM(Cell__Largst2FX[1], UseWaivers, SecondryFW, SecondryFT) + Cell->Largst2FS[0]->SecondryFS - TotalValueN);
				}
			}
		}
		else { /* (ptn_type2 != 'n' && ptn_type2 != '1') */
			IO_PRINT_LINE(M30204, ptn_type2);
			ReturnCode = EIE_FAIL;
			goto error_cleanup;
		}
		MaxSensitivity = (MaxSensitivity>MaxSensitivity2)?MaxSensitivity:MaxSensitivity2;
	}
	*MaxSensitivityPtr = MaxSensitivity;

	goto normal_cleanup;
error_cleanup:

normal_cleanup:

	return ReturnCode;
}

/*------------------------------------------------------------------------------
Initialize the internal data of STCT_SRULE structure
------------------------------------------------------------------------------*/
static void InitAlpha (
	double Alpha[STCM_MAXRULES][STCM_MAXALPHA])
{
	int i, j;

	for (i = 0; i < STCM_MAXRULES; i++)
		for (j = 0; j < STCM_MAXALPHA; j++)
			Alpha[i][j] = 0.0;
}
/*------------------------------------------------------------------------------
Return EIE_TRUE if C2 rule is supported by the implementation
------------------------------------------------------------------------------*/
static EIT_BOOLEAN IsC2Supported (void)
{
#ifdef STC_C2_IS_SUPPORTED
	return EIE_TRUE;
#else
	return EIE_FALSE;
#endif
}
/*------------------------------------------------------------------------------
Return EIE_TRUE if Duffett rule is supported by the implementation
------------------------------------------------------------------------------*/
static EIT_BOOLEAN IsDuffettSupported (void)
{
#ifdef STC_DUFFETT_IS_SUPPORTED
	return EIE_TRUE;
#else
	return EIE_FALSE;
#endif
}
/*------------------------------------------------------------------------------
Allocate the STCT_SRULE structure
------------------------------------------------------------------------------*/
static STCT_SRULE * SRuleAllocate (
	STCT_SRULE_TYPE Type,
	int NumberGroups,
	int NumberEntries[])
{
	int i;
	STCT_SRULE * SRule;
	SRule = STC_AllocateMemory (sizeof *SRule);
	if (SRule == NULL) return NULL;
	SRule->Type = Type;
	SRule->NumberGroups = NumberGroups;
	for (i = 0; i < NumberGroups; i++) {
		SRule->NumberEntries[i] = NumberEntries[i];
	}
	InitAlpha (SRule->Alpha);
	
	/* STCT_SRULE_TYPE Type;                       */
	/* int NumberGroups;                           */
	/* double Alpha[STCM_MAXRULES][STCM_MAXALPHA]; */
	/* int NumberEntries[STCM_MAXRULES];           */
	/* SRule->Tolerance                    =  0.0; */
	/* SRule->MinResp                      =    0; */
	/* SRule->NumberSensitivityChanged     =    0; */
	/* SRule->waiver_flags_present         =    0; */
	/* SRule->Parms->Shadow                =    0; */
	/* SRule->Parms->Weight                =    0; */
	/* SRule->Parms->Proxy                 =    0; */
	/* SRule->Parms->ProxyRatioSpecified   =    0; */
	/* SRule->Parms->ProxyRatio            =  0.0; */
	/* SRule->Parms->ProxyPercentile       =  .10; */
	/* SRule->Parms->ProxyDiagnostics      =    0; */
	/* SRule->Parms->WeightProtectionLevel =  'M'; */ /* ('L'(LOW) = "LOW", 'M'(MEDIUM) = "LINEAR", 'H'(HIGH) = "STEP", 'E'(EXACT) = "EXACT") */
	/* SRule->Parms->MinRespW              =    0; */
	/* SRule->Parms->AdditiveNoise         =    1; */
	/* SRule->Parms->WeightDiagnostics     =    0; */
	/* SRule->Parms->AcceptNegativeValues  =    0; */
	SRule->Parms = NULL;

	return SRule;
}
/*------------------------------------------------------------------------------
Validate the STCT_SRULE structure
------------------------------------------------------------------------------*/
static EIT_RETURNCODE SRuleValidate (
	STCT_SRULE * SRule)
{
	int i, j;

	for (i = 0; i < SRule->NumberGroups; i++) {
		for (j = 0; j < SRule->NumberEntries[i]; j++) {
			if (SRule->Alpha[i][j] < -1.0) {
				EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30144,
					i, j, SRule->Alpha[i][j]);
				return EIE_FAIL;
			}
		}
	}
	for (i = 0; i < SRule->NumberGroups; i++) {
		for (j = 0; j < SRule->NumberEntries[i]-1; j++) {
			if (SRule->Alpha[i][j] < SRule->Alpha[i][j+1]) {
				EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30145,
					i, j, SRule->Alpha[i][j], i, j+1, SRule->Alpha[i][j+1]);
				return EIE_FAIL;
			}
		}
	}
	/*  */
	if (SRule->NumberGroups > 1) {
		for (i = 0; i < SRule->NumberGroups-1; i++) {
			for (j = i+1; j < SRule->NumberGroups; j++) {
				if (SRule->NumberEntries[i] == SRule->NumberEntries[j]) {
					EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30146, i, j);
					return EIE_FAIL;
				}
			}
		}
		for (i = 0; i < SRule->NumberGroups-1; i++) {
			for (j = i+1; j < SRule->NumberGroups; j++) {
				if (SRule->Alpha[i][0] == SRule->Alpha[j][0]) {
					EI_AddMessage (M00044, EIE_MESSAGESEVERITY_WARNING, M20037, i, j);
				}
			}
		}
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Validate NK parameters
------------------------------------------------------------------------------*/
static EIT_RETURNCODE ValidateNkParameters (
	tSList * List,
	double Alpha[STCM_MAXRULES][STCM_MAXALPHA],
	int NumberEntries[STCM_MAXRULES],
	int n)// 1, 2 or 3 referencing
	      //for n = 1 list item 0 and 1
	      //for n = 2 list item 2 and 3
	      //for n = 3 list item 4 and 5
{
	double doubleN;
	char * dString;
	int i;
	double k;
	int N;

	doubleN = strtod (SList_Entry (List, n*2-2), &dString);
	N = (int)doubleN;
	if ((double)N != doubleN) {
		EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30141, doubleN, 1, STCM_MAXALPHA);
		SList_Free (List);
		return EIE_FAIL;
	}
	if (N < 1 || N > STCM_MAXALPHA) {
		EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30141, doubleN, 1, STCM_MAXALPHA);
		SList_Free (List);
		return EIE_FAIL;
	}
	k = strtod (SList_Entry (List, n*2-1), &dString);
	if (k <= 0.0 || k >= 100.0) {
		EI_AddMessage (M00044, EIE_MESSAGESEVERITY_ERROR, M30142, k);
		SList_Free (List);
		return EIE_FAIL;
	}
#define NK_K_WARNING_LIMIT 50.0
	if (k < NK_K_WARNING_LIMIT) {
		EI_AddMessage (M00044, EIE_MESSAGESEVERITY_WARNING, M20032, k, NK_K_WARNING_LIMIT);
	}
	EI_AddMessage (M00044, EIE_MESSAGESEVERITY_INFORMATION, "N = %d", N);
	EI_AddMessage (M00044, EIE_MESSAGESEVERITY_INFORMATION, "K = %g", k);
	NumberEntries[n-1] = N;
	for (i = 0; i < N; i++) {
		Alpha[n-1][i] = 100.0 / k;
	}
	return EIE_SUCCEED;
}
