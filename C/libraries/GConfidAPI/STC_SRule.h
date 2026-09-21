#ifndef _STC_SENSITIVITYRULE_H_
#define _STC_SENSITIVITYRULE_H_

#define STCM_MAXRULES 3
#define STCM_MAXALPHA 5
#define MINRESPW_UNSPECD_VALUE              (-3.4028e+38)

#include "ilist.h"
#include "slist.h"

#include "internal_rules.h"

/*-the declaration of "PROGRAM_PARM" has been moved here from "sensitiv.c"  */
/* so that a pointer to the instance "Parms" if this struct can replace in  */
/* the struct "STCT_SRULE" defined below the individual members of          */
/* "PROGRAM_PARM" that were included in it before, so only a single         */
/* pointer has to be assigned to provide access to the contents of "Parms"  */
/* from the instance "SRule" of "STCT_SRULE" (instead of having to assign   */
/* each of the individual members of "Parms" to the corresponding member of */
/* "SRule"), and so that the definition of "STCT_SRULE" won't have to be    */
/* changed if new members are added to "PROGRAM_PARM":                      */
struct PROGRAM_PARM {
	char * SRuleString;
	char * HierarchyString;
	char * RangeString;
	char * GroupString;
	int M;
	double X;
	double Y;
	double Z;
	double Tolerance;
	int MinResp;
	int Verbose;
	int Shadow;
	int Weight;        /*implements a bool: did user specify option WEIGHT_GRM_NB?  0- not specified, 1- specified*/
	int Proxy;
	int ProxyRatioSpecified;
	double ProxyRatio;
	double ProxyPercentile;
	int ProxyDiagnostics;
	char WeightProtectionLevel; /* ('L'(LOW) = "LOW", 'M'(MEDIUM) = "LINEAR", 'H'(HIGH) = "STEP", 'E'(EXACT) = "EXACT") */
	double MinRespW;
	int AdditiveNoise;
	int WeightDiagnostics;
	int AcceptNegativeValues;
	EIT_BOOLEAN PrintCodes;
	int MessageQuota;
};
typedef struct PROGRAM_PARM PROGRAM_PARM;

enum STCT_SRULE_TYPE {
	STCE_SRULE_TYPE_ARB,
	STCE_SRULE_TYPE_C2,
	STCE_SRULE_TYPE_DUFFETT,
	STCE_SRULE_TYPE_NK,
	STCE_SRULE_TYPE_PQ
};
typedef enum STCT_SRULE_TYPE STCT_SRULE_TYPE;

#define STCM_SRULE_TYPE_ARB_STRING		"ARB"
#define STCM_SRULE_TYPE_C2_STRING		"C2"
#define STCM_SRULE_TYPE_DUFFETT_STRING	"DUFFETT"
#define STCM_SRULE_TYPE_NK_STRING		"NK"
#define STCM_SRULE_TYPE_PQ_STRING		"PQ"


struct STCT_SRULE {
	STCT_SRULE_TYPE Type;
	int NumberGroups;
    double Alpha[STCM_MAXRULES][STCM_MAXALPHA];
    int NumberEntries[STCM_MAXRULES];
	double Tolerance;
	int MinResp;
	int NumberSensitivityChanged;
	int waiver_flags_present;
	PROGRAM_PARM * Parms;
};
typedef struct STCT_SRULE STCT_SRULE;

#ifdef STC_C2_IS_SUPPORTED
#define STC_C2_OR_DUFFETT_IS_SUPPORTED
#endif
#ifdef STC_DUFFETT_IS_SUPPORTED
#define STC_C2_OR_DUFFETT_IS_SUPPORTED
#endif
#ifdef STC_C2_IS_SUPPORTED
#ifdef STC_DUFFETT_IS_SUPPORTED
#define STC_C2_AND_DUFFETT_IS_SUPPORTED
#endif
#endif

#include "STC_Cell.h"

extern void STC_SRuleFree (STCT_SRULE *);
extern EIT_RETURNCODE STC_SRuleParse (char * s, STCT_SRULE ** SRule);
extern void STC_SRulePrint (STCT_SRULE *);
extern EIT_RETURNCODE STC_SRuleSensitivity (STCT_SRULE * SRule,
	double entval[], int n, double TotalValue, EIT_BOOLEAN HasAnonymous,
	double AnonymousValue, int NonAnonymousPositiveNumberEntries,
	int entwaivers[], STCT_CELL * Cell,
	int waiver_flags_present,
	int WeightSpecified, int ProxySpecified,
	int UsePtnSensitivity, int UseAdditiveNoise, int NonPtnSensitivityVariable,
	char ptn_type1,
	char ptn_type2,
	double * Sensitivity,
	double * RawSensitivity
);

#endif
