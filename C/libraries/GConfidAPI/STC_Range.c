/*------------------------------------------------------------------------------
range parser
This is the productions list for the parser:

ranges      -> range
			|  range ranges
range       -> levels ';'
			|  ';'
levels      -> level
			|  level ':' levels
level       -> parent enfants
parent      -> code
enfants     -> enfant
			|  enfant enfants
			|  enfant increment
increment   -> '-1' enfants
enfant      -> code
code        -> TOKEN_CODE
number      -> TOKEN_NUMBER

TOKEN_NUMBER   {list of digits}
TOKEN_CODE     {list of digits}
------------------------------------------------------------------------------*/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EI_Message.h"
#include "MessageGConfidAPI.h"
#include "STC_Memory.h"
#include "STC_Range.h"
#include "slist.h"
#include "util.h"


#define ITEM_ALLOCATION_SIZE         50
#define MESSAGE_ALLOCATION_SIZE   10000
#define DIMENSION_ALLOCATION_SIZE     5
#define PARSER_MAX_ERROR_COUNT       10


/*
set DEBUG to 1 to activate the debugging print statements.
set DEBUG to 0 to deactivate the debugging print statements.
If DEBUG is zero, most compilers will not generate any code for the debugging
statements.
*/
enum {DEBUG = 0};

enum tStatus {
	eNewRange,
	eNewCode
};
typedef enum tStatus tStatus;

enum tTokenId {
	eTokenIdCode,
	eTokenIdColon,
	eTokenIdDone,
	eTokenIdError,
	eTokenIdIncrement,
	eTokenIdSemiColon,
	eTokenIdUnknownCharacter
};
typedef enum tTokenId tTokenId;

struct tToken {
	tTokenId Id;
	char Code[STCM_CODE_MAX_LENGTH+1];
};
typedef struct tToken tToken;

struct tContext {
	int Status;
	int RangeIndex;
	char Code[STCM_CODE_MAX_LENGTH+1];
	char First[STCM_CODE_MAX_LENGTH+1];
	EIT_BOOLEAN Increment;
	/* points to the STCT_RANGEROOT receive by STC_RangeParse() */
	STCT_RANGEROOT * RangeRoot;
	/* points to the hierarchy strings. */
	char * RangeString;
	/* points somewhere in RangeString. the scanner moves it as it scans the string. */
	char * RangeStringPtr;
	/* space for error message */
	char Message[MESSAGE_ALLOCATION_SIZE+1];
	/* number of errors found */
	int ErrorCount;
};
typedef struct tContext tContext;


static EIT_RETURNCODE CheckIfDisjoint (STCT_RANGE * Range, char * Code,
	char * Data, int Lo, int Hi);
static EIT_RETURNCODE InstallChild (tToken * Token);
static EIT_RETURNCODE InstallParent (tToken * Token);
static EIT_RETURNCODE Match (tTokenId Id, tToken * Token);
static EIT_RETURNCODE MatchChild (tToken * Token);
static EIT_RETURNCODE MatchChildren (tToken * Token);
static EIT_RETURNCODE MatchIncrement (tToken * Token);
static EIT_RETURNCODE MatchLevel (tToken * Token);
static EIT_RETURNCODE MatchLevels (tToken * Token);
static EIT_RETURNCODE MatchParent (tToken * Token);
static EIT_RETURNCODE MatchRange (tToken * Token);
static EIT_RETURNCODE MatchRanges (tToken * Token);
static EIT_RETURNCODE RangeAddItem (STCT_RANGE * Range, char * Code, char * Data1, char * Data2);
static EIT_RETURNCODE RangeAddItem1 (STCT_RANGE * Range, char * Code, char * Data, int Lo, int Hi);
static void RangeItemFree (STCT_RANGEITEM * Item);
static EIT_RETURNCODE RangeRootAddRange (STCT_RANGEROOT * RangeRoot);
static STCT_RANGEROOT * RangeRootAllocate (void);
static void RangeRootSimplify (STCT_RANGEROOT * RangeRoot);
static void RangeRootSort (STCT_RANGEROOT * RangeRoot);
static EIT_RETURNCODE RangeRootValidate (STCT_RANGEROOT * RangeRoot);
static void RangeSimplify (STCT_RANGE * Range);
static void RangeSort (STCT_RANGE * Range);
static EIT_RETURNCODE RangeValidate (STCT_RANGE * Range);
static void ReportError (char * Message);
static EIT_RETURNCODE Scanner (tToken * Token);
static char * TokenIdTranslator (tTokenId TokenId);
static void TokenPrint (tToken * Token);


static char * mCodeFirstCharacterCharacterSet;
static char * mCodeCharacterSet;
static tContext mContext;


/*------------------------------------------------------------------------------
Add a code into the range
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_RangeAddItem (
	STCT_RANGE * Range,
	char * Code)
{
	EIT_RETURNCODE rc;
	rc = RangeAddItem (Range, Code, Code, NULL);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	RangeSort (Range);//not optimal! but does not happen often...

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Allocate the STCT_RANGE structure
------------------------------------------------------------------------------*/
STCT_RANGE * STC_RangeAllocate (void)
{
	STCT_RANGE * Range;

	Range = STC_AllocateMemory (sizeof *Range);
	if (Range == NULL) return NULL;
	Range->NumberAllocated = 0;
	Range->NumberEntries = 0;
	Range->Item = NULL;
	return Range;
}
/*------------------------------------------------------------------------------
Free the STCT_RANGE structure
------------------------------------------------------------------------------*/
void STC_RangeFree (
	STCT_RANGE * Range)
{
	int i;

	for (i = 0; i < Range->NumberEntries; i++) {
		RangeItemFree (Range->Item[i]);
	}
	STC_FreeMemory (Range->Item);
	STC_FreeMemory (Range);
}
/*------------------------------------------------------------------------------
check if 2 ranges intersect.
c est a dire qu'ils inclus chacun au moins un meme code
------------------------------------------------------------------------------*/
EIT_BOOLEAN STC_RangeIntersect (
	STCT_RANGE * R1,
	STCT_RANGE * R2)
{
	int i, j;

	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "r1\n");
	//STC_RangePrint (R1);
	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "r2\n");
	//STC_RangePrint (R2);

	for (i = 0; i < R1->NumberEntries && R1->Item[i]->Lo != STCM_LOHI_NOT_SET; i++) {
		for (j = 0; j < R2->NumberEntries && R2->Item[j]->Lo != STCM_LOHI_NOT_SET; j++) {
			//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "lohi i=%d j=%d\n", i, j);
			if (R1->Item[i]->Lo == R2->Item[j]->Lo ||
				R1->Item[i]->Lo == R2->Item[j]->Hi ||
				R1->Item[i]->Hi == R2->Item[j]->Lo ||
				R1->Item[i]->Hi == R2->Item[j]->Hi)
				return EIE_TRUE;
			if ((R1->Item[i]->Lo < R2->Item[j]->Lo && R2->Item[j]->Lo < R1->Item[i]->Hi) ||
				(R1->Item[i]->Lo < R2->Item[j]->Hi && R2->Item[j]->Hi < R1->Item[i]->Hi))
				return EIE_TRUE;
			if ((R2->Item[j]->Lo < R1->Item[i]->Lo && R1->Item[i]->Lo < R2->Item[j]->Hi) ||
				(R2->Item[j]->Lo < R1->Item[i]->Hi && R1->Item[i]->Hi < R2->Item[j]->Hi))
				return EIE_TRUE;
		}
	}
	for (i = 0; i < R1->NumberEntries; i++) {
		if (R1->Item[i]->Lo != STCM_LOHI_NOT_SET) continue;
		for (j = 0; j < R2->NumberEntries; j++) {
			if (R2->Item[j]->Lo != STCM_LOHI_NOT_SET) continue;
			//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "data i=%d j=%d\n", i, j);
			if (!strcmp (R1->Item[i]->Data, R2->Item[j]->Data))
				return EIE_TRUE;
		}
	}
	return EIE_FALSE;
}
/*------------------------------------------------------------------------------
Parse a range specification. Puts the result in the STCT_RANGEROOT structure.
It is the programmer's responsability to free that structure.
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_RangeParse (
	char * RangeString,
	STCT_RANGEROOT ** RangeRoot,
	char * CodeFirstCharacterCharacterSet,
	char * CodeCharacterSet)
{
	EIT_RETURNCODE crc; /* cumulative return code */
	EIT_BOOLEAN Done;
	EIT_RETURNCODE rc;
	tToken Token;

	mCodeFirstCharacterCharacterSet = CodeFirstCharacterCharacterSet;
	mCodeCharacterSet = CodeCharacterSet;

#ifdef _DEBUG
	if (DEBUG) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "FirstCharacterCharacterSet=%s\n", mCodeFirstCharacterCharacterSet);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "CharacterSet=%s\n", mCodeCharacterSet);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Range=%s\n", RangeString);
	}
#endif

	mContext.RangeString = STC_StrDup (RangeString);
	if (mContext.RangeString == NULL) return EIE_FAIL;
	UTIL_DropBlanksSafeWithAccentuatedCharacters (mContext.RangeString);
	mContext.RangeStringPtr = mContext.RangeString;
	mContext.Status = eNewRange;
	mContext.RangeIndex = 0;
	mContext.Code[0] = '\0';
	mContext.First[0] = '\0';
	mContext.Message[0] = '\0';
	mContext.ErrorCount = 0;
	mContext.Increment = EIE_FALSE;
	mContext.RangeRoot = RangeRootAllocate ();
	if (mContext.RangeRoot == NULL) return EIE_FAIL;

	crc = EIE_SUCCEED;
	Done = EIE_FALSE;
	while (!Done) {
		rc = Scanner (&Token);
		if (rc == EIE_SUCCEED) {
			rc = MatchRanges (&Token);
			if (rc == EIE_SUCCEED) {
				Done = EIE_TRUE;
			}
		}

		/* handle error if any of the previous function call failed */
		if (rc != EIE_SUCCEED) {
			crc = EIE_FAIL;
			mContext.ErrorCount++;
			if (mContext.ErrorCount >= PARSER_MAX_ERROR_COUNT) {
				/* quit when PARSER_MAX_ERROR_COUNT errors are found */
				Done = EIE_TRUE;
				EI_AddMessage (M00046, EIE_MESSAGESEVERITY_ERROR,
					M30003); /* Too many errors. Stop processing. */
			}
			else {
				/* advance to the next edits */
				mContext.RangeStringPtr = strchr (mContext.RangeStringPtr, ';');
				if (mContext.RangeStringPtr == NULL)
					Done = EIE_TRUE;
				else {
					mContext.RangeStringPtr++;
					if (*mContext.RangeStringPtr == '\0') {
						Done = EIE_TRUE;
					}
					else {
						//reset Context information and start over.
						mContext.Status = eNewRange;
						mContext.RangeIndex = 0;
						mContext.First[0] = '\0';
						mContext.Increment = EIE_FALSE;
						//sacrifice space already allocated in mContext.RangeRoot.
						//in some cases the state of the structure is such that I can't call
						//STC_RangeRootFree() without crashing the program!
						mContext.RangeRoot = RangeRootAllocate ();
						if (mContext.RangeRoot == NULL) return EIE_FAIL;
					}
				}
			}
		}
	}
	STC_FreeMemory (mContext.RangeString);

	*RangeRoot = mContext.RangeRoot;
	RangeRootSort (*RangeRoot);
	RangeRootSimplify (*RangeRoot);
#ifdef _DEBUG
	if (DEBUG) STC_RangeRootPrint (mContext.RangeRoot);
#endif
	rc = RangeRootValidate (*RangeRoot);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	/* empty context */
	mContext.Code[0] = '\0';
	mContext.ErrorCount = 0;
	mContext.First[0] = '\0';
	mContext.Increment = EIE_FALSE;
	mContext.Message[0] = '\0';
	mContext.RangeIndex = 0;
	mContext.RangeRoot = NULL;
	mContext.RangeString = NULL;
	mContext.RangeStringPtr = NULL;
	mContext.Status = eNewRange;

	return crc;
}
/*------------------------------------------------------------------------------
Print the STCT_RANGE structure
------------------------------------------------------------------------------*/
void STC_RangePrint (
	STCT_RANGE * Range)
{
	int i;

	if (Range->NumberEntries == 0) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, M10037 "\n");//Range is empty
	}
	for (i = 0; i < Range->NumberEntries; i++) {
		if (Range->Item[i]->Lo != STCM_LOHI_NOT_SET)
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    " M10035 "\n",//Code %s Lo %d Hi %d
				Range->Item[i]->Code, Range->Item[i]->Lo, Range->Item[i]->Hi);
		else
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "    " M10036 "\n",//Code %s Data %s
				Range->Item[i]->Code, Range->Item[i]->Data);
	}
	EI_PrintMessages ();
}
/*------------------------------------------------------------------------------
Allocate the STCT_RANGEROOT structure with NumberAllocated ranges
------------------------------------------------------------------------------*/
STCT_RANGEROOT * STC_RangeRootAllocate (
	int NumberAllocated)
{
	int i;
	STCT_RANGEROOT * RangeRoot;
	EIT_RETURNCODE rc;

	RangeRoot = RangeRootAllocate ();
	if (RangeRoot == NULL) return NULL;
	for (i = 0; i < NumberAllocated; i++) {
		rc = RangeRootAddRange (RangeRoot);
		if (rc == EIE_FAIL) return NULL;
	}
	return RangeRoot;
}
/*------------------------------------------------------------------------------
Free the STCT_RANGEROOT structure
------------------------------------------------------------------------------*/
void STC_RangeRootFree (
	STCT_RANGEROOT * RangeRoot)
{
	int i;

	for (i = 0; i < RangeRoot->NumberEntries; i++) {
		STC_RangeFree (RangeRoot->Dimension[i]);
		STC_FreeMemory (RangeRoot->Name[i]);
	}
	STC_FreeMemory (RangeRoot->Dimension);
	STC_FreeMemory (RangeRoot->Name);
	STC_FreeMemory (RangeRoot);
}
/*------------------------------------------------------------------------------
Print the STCT_RANGEROOT structure
------------------------------------------------------------------------------*/
void STC_RangeRootPrint (
	STCT_RANGEROOT * RangeRoot)
{
	int i;

	for (i = 0; i < RangeRoot->NumberEntries; i++) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, M10034 "\n", RangeRoot->Name[i]);
		STC_RangePrint (RangeRoot->Dimension[i]);
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
}


/*------------------------------------------------------------------------------
Set the Name member of the hierarchy.
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_RangeRootSetName (
	STCT_RANGEROOT * RangeRoot,
	char * Name,
	int Index)
{
#define NAME_LENGTH 100
	if (Index < 0 || Index >= RangeRoot->NumberEntries) return EIE_FAIL;

	if (strlen (Name) <= NAME_LENGTH) {
		RangeRoot->Name[Index] = STC_StrDup (Name);
		if (RangeRoot->Name[Index] == NULL) return EIE_FAIL;
	}
	else {
		RangeRoot->Name[Index] = STC_AllocateMemory (NAME_LENGTH + 1);
		if (RangeRoot->Name[Index] == NULL) return EIE_FAIL;
		strncpy (RangeRoot->Name[Index], Name, NAME_LENGTH);
		RangeRoot->Name[Index][NAME_LENGTH] = '\0';
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Get all range items for a code
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_RangeSearchFromCodesToRanges (
	STCT_RANGE * Range,
	char * Code,
	STCT_RANGE * Range2)
{
	int i;
	EIT_RETURNCODE rc;

	for (i = 0; i < Range->NumberEntries; i++) {
		if (strcmp (Range->Item[i]->Code, Code) == 0) {
			rc = RangeAddItem1 (Range2, Range->Item[i]->Code, Range->Item[i]->Data, Range->Item[i]->Lo, Range->Item[i]->Hi);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
	}

	RangeSort (Range2);
	RangeSimplify (Range2);
#ifdef _DEBUG
	if (DEBUG) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "<code2range>\n");
		STC_RangePrint (Range2);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "</code2range>\n");
	}
#endif
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
simplify the range without taking the code into consideration.
------------------------------------------------------------------------------*/
void STC_RangeSimplifyAggressively (
	STCT_RANGE * Range)
{
	int i, j;
	int RangeNumberEntries;

	for (i = 0; i < Range->NumberEntries-1; i++) {
		if (Range->Item[i] != NULL) {
			if (Range->Item[i]->Data[0] != '\0') continue;
			for (j = i+1; j < Range->NumberEntries; j++) {
				if (Range->Item[j] != NULL) {
					if (Range->Item[i]->Hi == Range->Item[j]->Lo || Range->Item[i]->Hi+1 == Range->Item[j]->Lo) {
						Range->Item[i]->Hi = Range->Item[j]->Hi;
						RangeItemFree (Range->Item[j]);
						Range->Item[j] = NULL;
					}
				}
			}
		}
	}

	for (i = 0; i < Range->NumberEntries-1; i++) {
		for (j = i+1; j < Range->NumberEntries; j++) {
			if (Range->Item[i] == NULL && Range->Item[j] != NULL) {
				Range->Item[i] = Range->Item[j];
				Range->Item[j] = NULL;
				break;//next i
			}
		}
	}

	RangeNumberEntries = 0;
	for (i = 0; i < Range->NumberEntries && Range->Item[i] != NULL; i++) {
		RangeNumberEntries++;
	}
	Range->NumberEntries = RangeNumberEntries;
}


/*------------------------------------------------------------------------------
check if the new range entry is disjoint from the range as a whole
------------------------------------------------------------------------------*/
static EIT_RETURNCODE CheckIfDisjoint (
	STCT_RANGE * Range,
	char * Code,
	char * Data,
	int Lo,
	int Hi)
{
	int i;

	//EI_AddMessage ("", 4, "CheckIfDisjoint (%s <%s> %d %d);\n", Code, Data, Lo, Hi);

	if (Lo != STCM_LOHI_NOT_SET) {
		for (i = 0; i < Range->NumberEntries; i++) {
			if (strcmp (Code, Range->Item[i]->Code) == 0) {
				if ((Lo >= Range->Item[i]->Lo && Lo <= Range->Item[i]->Hi) ||
					(Hi >= Range->Item[i]->Lo && Hi <= Range->Item[i]->Hi)) {
					if (Lo == Hi) {
						if (mContext.RangeRoot == NULL) {
							/*
							Klugde alert!
							This fonction is called (indirectly) from the RangeParser and sometimes
							it is NOT called from the RangeParser. When it is not called by the RangeParser
							the mContext is NOT valid. In those times, ReportError() CANNOT be used.
							*/
							EI_AddMessage ("Validation", EIE_MESSAGESEVERITY_ERROR, M30107 "\n", Lo);
							return EIE_FAIL;
						}
						else {
							sprintf (mContext.Message, M30107 "\n", Lo);
							ReportError (mContext.Message);
							return EIE_FAIL;
						}
					}
				}
			}
		}
	}
	else {
		/* find the index of the first alpha */
		for (i = 0; i < Range->NumberEntries && Range->Item[i]->Lo != STCM_LOHI_NOT_SET; i++)
			;

		for (; i < Range->NumberEntries; i++) {
			if (strcmp (Code, Range->Item[i]->Code) == 0) {
				if (strcmp (Data, Range->Item[i]->Data) == 0) {
					sprintf (mContext.Message, M30113 "\n", Data);
					ReportError (mContext.Message);
					return EIE_FAIL;
				}
			}
		}
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
A child was found
------------------------------------------------------------------------------*/
static EIT_RETURNCODE InstallChild (
	tToken * Token)
{
	EIT_RETURNCODE rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "InstallChild\n");
#endif
	if (mContext.Increment) {
		//increment
		rc = RangeAddItem (mContext.RangeRoot->Dimension[mContext.RangeIndex], mContext.Code, mContext.First, Token->Code);
	}
	else {
		//no increment
		rc = RangeAddItem (mContext.RangeRoot->Dimension[mContext.RangeIndex], mContext.Code, Token->Code, NULL);
	}

	strcpy (mContext.First, Token->Code);
	mContext.Increment = EIE_FALSE;

	return rc;
}
/*------------------------------------------------------------------------------
A parent was found
------------------------------------------------------------------------------*/
static EIT_RETURNCODE InstallParent (
	tToken * Token)
{
	EIT_RETURNCODE rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "InstallParent\n");
#endif
	if (mContext.Status == eNewRange) {
#ifdef _DEBUG
		if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "add a new dimension, code %s\n", Token->Code);
#endif
		rc = RangeRootAddRange (mContext.RangeRoot);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
		mContext.Status = eNewCode;
	}
	strcpy (mContext.Code, Token->Code);
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Make sure the last token read match a specified token id.
If it does, call the scanner to find the next token.
If it does not, returns EIE_FAIL.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE Match (
	tTokenId Id,
	tToken * Token)
{
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Looking for '%s', found '%s'." "\n",
		TokenIdTranslator (Id), TokenIdTranslator (Token->Id));
#endif
	if (Id != Token->Id) {
		sprintf (mContext.Message, M30007 "\n",
			TokenIdTranslator (Id), TokenIdTranslator (Token->Id));
		ReportError (mContext.Message);
		return EIE_FAIL;
	}

	return Scanner (Token);
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE MatchChild (
	tToken * Token)
{
	int rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MatchChild\n");
#endif
	rc = InstallChild (Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	rc = Match (eTokenIdCode, Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE MatchChildren (
	tToken * Token)
{
	int rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MatchChildren\n");
#endif
	if (Token->Id == eTokenIdCode) {
		rc = MatchChild (Token);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
		if (Token->Id == eTokenIdIncrement) {
			rc = MatchIncrement (Token);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
		else if (Token->Id == eTokenIdCode) {
			rc = MatchChildren (Token);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
	}
	else {
		sprintf (mContext.Message, M30007 "\n",
			TokenIdTranslator (eTokenIdCode), TokenIdTranslator (Token->Id));
		ReportError (mContext.Message);
		return EIE_FAIL;
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE MatchIncrement (
	tToken * Token)
{
	int rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MatchIncrement\n");
#endif
	mContext.Increment = EIE_TRUE;

	if (mContext.First[0] == '\0') {
		ReportError (M30148 "\n");
		return EIE_FAIL;
	}
	rc = Match (eTokenIdIncrement, Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	rc = MatchChildren (Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE MatchLevel (
	tToken * Token)
{
	int rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MatchLevel\n");
#endif
	rc = MatchParent (Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	rc = MatchChildren (Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE MatchLevels (
	tToken * Token)
{
	int rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MatchLevels\n");
#endif
	rc = MatchLevel (Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	while (Token->Id != eTokenIdSemiColon) {
		rc = Match (eTokenIdColon, Token);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
		rc = MatchLevel (Token);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE MatchParent (
	tToken * Token)
{
	int rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MatchParent\n");
#endif
	if (Token->Id == eTokenIdCode) {
		rc = InstallParent (Token);
		if (rc != EIE_SUCCEED) return EIE_FAIL;

		rc = Match (eTokenIdCode, Token);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}
	else {
		sprintf (mContext.Message, M30007 "\n",
			TokenIdTranslator (eTokenIdCode), TokenIdTranslator (Token->Id));
		ReportError (mContext.Message);
		return EIE_FAIL;
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE MatchRange (
	tToken * Token)
{
	int rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MatchRange\n");
#endif
	if (Token->Id == eTokenIdSemiColon) {
		rc = RangeRootAddRange (mContext.RangeRoot);//empty range
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}
	else {
		rc = MatchLevels (Token);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}
	rc = Match (eTokenIdSemiColon, Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE MatchRanges (
	tToken * Token)
{
	int rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MatchRanges\n");
#endif
	rc = MatchRange (Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	while (Token->Id != eTokenIdDone) {
		mContext.RangeIndex++;
		rc = MatchRange (Token);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Add a range item to the range
------------------------------------------------------------------------------*/
static EIT_RETURNCODE RangeAddItem (
	STCT_RANGE * Range,
	char * Code,
	char * Data1,
	char * Data2)
{
	char Data[STCM_CODE_MAX_LENGTH+1];
	int Hi;
	int Lo;
	EIT_RETURNCODE rc;

	if (UTIL_IsNumeric (Data1, &Lo) == EIE_TRUE) {
		Hi = Lo;
		if (Data2 != NULL && UTIL_IsNumeric (Data2, &Hi) == EIE_FALSE) {
			ReportError (M30149 "\n");
			return EIE_FAIL;
		}
		strcpy (Data, "");
	}
	else {
		Lo = STCM_LOHI_NOT_SET;
		Hi = STCM_LOHI_NOT_SET;
		if (Data2 != NULL) {
			ReportError (M30150 "\n");
			return EIE_FAIL;
		}
		strcpy (Data, Data1);
	}

	rc = RangeAddItem1 (Range, Code, Data, Lo, Hi);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Add a range item to the range
------------------------------------------------------------------------------*/
static EIT_RETURNCODE RangeAddItem1 (
	STCT_RANGE * Range,
	char * Code,
	char * Data,
	int Lo,
	int Hi)
{
	STCT_RANGEITEM * Item;
	void * Ptr;
	EIT_RETURNCODE rc;

	rc = CheckIfDisjoint (Range, Code, Data, Lo, Hi);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	if (Range->NumberEntries > 0 &&
		Lo != STCM_LOHI_NOT_SET &&
		strcmp (Range->Item[Range->NumberEntries-1]->Code, Code) == 0 &&
		(Lo == Range->Item[Range->NumberEntries-1]->Hi || Lo == Range->Item[Range->NumberEntries-1]->Hi+1)) {
		Range->Item[Range->NumberEntries-1]->Hi = Hi;
	}
	else {
		if (Range->NumberAllocated == Range->NumberEntries) {
			Range->NumberAllocated += ITEM_ALLOCATION_SIZE;
			Ptr = STC_ReallocateMemory (
				Range->NumberEntries * sizeof *Range->Item,
				Range->NumberAllocated * sizeof *Range->Item,
				Range->Item);
			if (Ptr == NULL) return EIE_FAIL;
			Range->Item = Ptr;
		}
		Item = STC_AllocateMemory (sizeof *Item);
		if (Item == NULL) return EIE_FAIL;
		Item->Code = STC_StrDup (Code);
		if (Item->Code == NULL) return EIE_FAIL;
		Item->Data = STC_StrDup (Data);
		if (Item->Data == NULL) return EIE_FAIL;
		Item->Lo = Lo;
		Item->Hi = Hi;

		Range->Item[Range->NumberEntries++] = Item;
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Free the STCT_RANGE structure
------------------------------------------------------------------------------*/
static void RangeItemFree (
	STCT_RANGEITEM * Item)
{
	STC_FreeMemory (Item->Code);
	STC_FreeMemory (Item->Data);
	STC_FreeMemory (Item);
}
/*------------------------------------------------------------------------------
Allocate the STCT_RANGEROOT structure
------------------------------------------------------------------------------*/
STCT_RANGEROOT * RangeRootAllocate (void)
{
	STCT_RANGEROOT * RangeRoot;

	RangeRoot = STC_AllocateMemory (sizeof *RangeRoot);
	if (RangeRoot == NULL) return NULL;
	RangeRoot->NumberAllocated = 0;
	RangeRoot->NumberEntries = 0;
	RangeRoot->Dimension = NULL;
	RangeRoot->Name = NULL;
	return RangeRoot;
}
/*------------------------------------------------------------------------------
Add a range in the range root
------------------------------------------------------------------------------*/
static EIT_RETURNCODE RangeRootAddRange (
	STCT_RANGEROOT * RangeRoot)
{
	void * Ptr;
	STCT_RANGE * Range;
	Range = STC_RangeAllocate ();
	if (RangeRoot->NumberAllocated == RangeRoot->NumberEntries) {
		RangeRoot->NumberAllocated += DIMENSION_ALLOCATION_SIZE;
		Ptr = STC_ReallocateMemory (
			RangeRoot->NumberEntries * sizeof *RangeRoot->Dimension,
			RangeRoot->NumberAllocated * sizeof *RangeRoot->Dimension,
			RangeRoot->Dimension);
		if (Ptr == NULL) return EIE_FAIL;
		RangeRoot->Dimension = Ptr;
		Ptr = STC_ReallocateMemory (
			RangeRoot->NumberEntries * sizeof *RangeRoot->Name,
			RangeRoot->NumberAllocated * sizeof *RangeRoot->Name,
			RangeRoot->Name);
		if (Ptr == NULL) return EIE_FAIL;
		RangeRoot->Name = Ptr;
	}
	RangeRoot->Dimension[RangeRoot->NumberEntries++] = Range;
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static void RangeRootSimplify (
	STCT_RANGEROOT * RangeRoot)
{
	int i;

	for (i = 0; i < RangeRoot->NumberEntries; i++) {
		RangeSimplify (RangeRoot->Dimension[i]);
	}
}
/*------------------------------------------------------------------------------
Sort all ranges in the STCT_RANGEROOT structure
------------------------------------------------------------------------------*/
static void RangeRootSort (
	STCT_RANGEROOT * RangeRoot)
{
	int i;

	for (i = 0; i < RangeRoot->NumberEntries; i++) {
		RangeSort (RangeRoot->Dimension[i]);
	}
}
/*------------------------------------------------------------------------------
Validate the STCT_RANGEROOT structure
------------------------------------------------------------------------------*/
static EIT_RETURNCODE RangeRootValidate (
	STCT_RANGEROOT * RangeRoot)
{
	int i;
	EIT_RETURNCODE rc;

	for (i = 0; i < RangeRoot->NumberEntries; i++) {
		rc = RangeValidate (RangeRoot->Dimension[i]);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
simplify the range taking the code into consideration.
------------------------------------------------------------------------------*/
static void RangeSimplify (
	STCT_RANGE * Range)
{
	int i, j;
	int RangeNumberEntries;

	for (i = 0; i < Range->NumberEntries-1; i++) {
		if (Range->Item[i] != NULL) {
			if (Range->Item[i]->Data[0] != '\0') continue;
			for (j = i+1; j < Range->NumberEntries; j++) {
				if (Range->Item[j] != NULL) {
					if (strcmp (Range->Item[i]->Code, Range->Item[j]->Code) == 0 &&
						(Range->Item[i]->Hi == Range->Item[j]->Lo || Range->Item[i]->Hi+1 == Range->Item[j]->Lo)) {
						Range->Item[i]->Hi = Range->Item[j]->Hi;
						//RangeItemFree (Range->Item[j]);
						Range->Item[j] = NULL;
					}
				}
			}
		}
	}

	for (i = 0; i < Range->NumberEntries-1; i++) {
		for (j = i+1; j < Range->NumberEntries; j++) {
			if (Range->Item[i] == NULL && Range->Item[j] != NULL) {
				Range->Item[i] = Range->Item[j];
				Range->Item[j] = NULL;
				break;//next i
			}
		}
	}

	RangeNumberEntries = 0;
	for (i = 0; i < Range->NumberEntries && Range->Item[i] != NULL; i++) {
		RangeNumberEntries++;
	}
	Range->NumberEntries = RangeNumberEntries;
}
/*------------------------------------------------------------------------------
Sort ranges in the STCT_RANGE structure
------------------------------------------------------------------------------*/
static void RangeSort (
	STCT_RANGE * Range)
{
	int index, gap, i, j;
	STCT_RANGEITEM * TempItem;

	if (Range->NumberEntries == 0) return;

	/* move numeric range to the top and alpha to the bottom. */
	/* the bottom alpha are sorted after this loop. */
	for (gap = Range->NumberEntries/2; gap > 0; gap /= 2) {
		for (i = gap; i < Range->NumberEntries; i++) {
			for (j = i-gap; j >= 0; j -= gap) {
				if (strcmp (Range->Item[j]->Data, Range->Item[j+gap]->Data) <= 0)
					break;
				/* move the items */
				TempItem           = Range->Item[j];
				Range->Item[j]     = Range->Item[j+gap];
				Range->Item[j+gap] = TempItem;
			}
		}
	}

	/* find the index of the first alpha */
	for (index = 0; index < Range->NumberEntries && Range->Item[index]->Lo != STCM_LOHI_NOT_SET; index++)
		;

	/* sort top numeric range */
	for (gap = index/2; gap > 0; gap /= 2) {
		for (i = gap; i < index; i++) {
			for (j = i-gap; j >= 0; j -= gap) {
				if (Range->Item[j]->Lo < Range->Item[j+gap]->Lo ||
					(Range->Item[j]->Lo == Range->Item[j+gap]->Lo && Range->Item[j]->Hi < Range->Item[j+gap]->Hi))
					break;
				/* move the items */
				TempItem           = Range->Item[j];
				Range->Item[j]     = Range->Item[j+gap];
				Range->Item[j+gap] = TempItem;
			}
		}
	}
}
/*------------------------------------------------------------------------------
Validate the STCT_RANGE structure
------------------------------------------------------------------------------*/
static EIT_RETURNCODE RangeValidate (
	STCT_RANGE * Range)
{
	int i;
	int nError = 0;
	int rc = EIE_SUCCEED;

	for (i = 0; i < Range->NumberEntries; i++) {
		if (Range->Item[i]->Lo > Range->Item[i]->Hi) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_ERROR, M30122 "\n",
				Range->Item[i]->Lo, Range->Item[i]->Hi,
				Range->Item[i]->Code, Range->Item[i]->Lo, Range->Item[i]->Hi);
			rc = EIE_FAIL;
			if (nError++ > 10) return EIE_FAIL;
		}
	}
	return rc;
}
/*------------------------------------------------------------------------------
Adds a message to the message list.
------------------------------------------------------------------------------*/
static void ReportError (
	char * Message)
{
	if (strlen (mContext.RangeString) > (MESSAGE_ALLOCATION_SIZE/2)) {
		//set ErrorCount to MAX because we are modifying the string
		mContext.ErrorCount = PARSER_MAX_ERROR_COUNT;
		mContext.RangeString[(MESSAGE_ALLOCATION_SIZE/2)-5] = '\0';
		strcat (mContext.RangeString, "...");
	}
	UTIL_ReportError (M00046, M00002, Message, mContext.RangeString, mContext.RangeStringPtr);
}
/*------------------------------------------------------------------------------
lexical analyser.
reads the input and finds the next token.
set Token->Id to eTokenIdUnknownCharacter if the token is not recognise.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE Scanner (
	tToken * Token)
{
	char * Message;
	size_t n;
	char * p;
	EIT_RETURNCODE rc;
	char Temp[STCM_CODE_MAX_LENGTH+1];

	rc = UTIL_ConsumeComments (mContext.RangeStringPtr, &n, &Message);
	if (rc != EIE_SUCCEED) {
		ReportError (Message);
		return EIE_FAIL;
	}
	p = mContext.RangeStringPtr+n;

	n = strspn (p, mCodeFirstCharacterCharacterSet);
	if (n > 0) {
		/* found code */
		n = strspn (p, mCodeCharacterSet);
		if (n > STCM_CODE_MAX_LENGTH) {
			Token->Id = eTokenIdError;
			ReportError (M30151);
			p += n;
			mContext.RangeStringPtr = p;
			return EIE_FAIL;
		}
		strncpy (Temp, p, n);
		Temp[n] = '\0';
#ifdef _DEBUG
		if (DEBUG) {
			size_t n1;
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Code='%s'\n", Temp);
			n1 = strspn (Temp, "0123456789");
			if (n == n1)
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Code est numeric\n");
			else
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Code est alphanumeric\n");
		}
#endif
		p += strlen (Temp);
		Token->Id = eTokenIdCode;
		strcpy (Token->Code, Temp);
	}
	else if (*p == '-' && *(p+1) == '1' && *(p+2) == ' ') {
		/* found increment */

		p += 3;//skip the '-', 1 and ' '

		Token->Id = eTokenIdIncrement;
		strcpy (Token->Code, "1");
	}
	else if (*p == ':') {
		/* found Colon */
		p++;
		Token->Id = eTokenIdColon;
		strcpy (Token->Code, "");
		mContext.Status = eNewCode;
		mContext.First[0] = '\0';
		mContext.Increment = EIE_FALSE;
	}
	else if (*p == ';') {
		/* found SemiColon */
		p++;
		Token->Id = eTokenIdSemiColon;
		strcpy (Token->Code, "");
		mContext.Status = eNewRange;
		mContext.First[0] = '\0';
		mContext.Increment = EIE_FALSE;
	}
	else if (*p == '\0') {
		/* found the end */
		Token->Id = eTokenIdDone;
		strcpy (Token->Code, "");
	}
	else {
		/* found an unknown character */
		p++;
		Token->Id = eTokenIdUnknownCharacter;
		strcpy (Token->Code, "");
	}
#ifdef _DEBUG
	if (DEBUG) {
		TokenPrint (Token);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "p=<%s>\n", p);
	}
#endif
	mContext.RangeStringPtr = p;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
returns a description for a token.
------------------------------------------------------------------------------*/
static char * TokenIdTranslator (
	tTokenId TokenId)
{
	switch (TokenId) {
	case eTokenIdCode:             return M00048; /* Code */
	case eTokenIdColon:            return M00004; /* Colon */
	case eTokenIdDone:             return M00005; /* Done */
	case eTokenIdError:            return M00006; /* Error */
	case eTokenIdIncrement:        return M00049; /* Increment */
	case eTokenIdSemiColon:        return M00011; /* SemiColon */
	case eTokenIdUnknownCharacter: return M00014; /* Unknown Character */
	/* no default */
	}
	return M00042; /* Unknown token */
}
/*------------------------------------------------------------------------------
print token and value if applicable (debug function).
------------------------------------------------------------------------------*/
static void TokenPrint (
	tToken * Token)
{
	switch (Token->Id) {
	case eTokenIdDone:
	case eTokenIdError:
	case eTokenIdSemiColon:
	case eTokenIdUnknownCharacter:
	default:
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%s\n", TokenIdTranslator (Token->Id));
		break;
	case eTokenIdIncrement:
	case eTokenIdCode:
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%s %s\n", TokenIdTranslator (Token->Id),
			Token->Code);
		break;
	}
}
