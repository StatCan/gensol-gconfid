/*------------------------------------------------------------------------------
group parser
This is the productions list for the parser:

groups      -> group
			|  group groups
group       -> coordinate ';'
			|  coordinate ':' group
coordinate  -> code
            |  code coordinate
code        -> TOKEN_CODE

TOKEN_CODE  {list of characters}
------------------------------------------------------------------------------*/
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "EI_Message.h"
#include "MessageGConfidAPI.h"
#include "STC_Build.h"
#include "STC_Coordinate.h"
#include "STC_Hierarchy.h"
#include "STC_Memory.h"
#include "STC_Group.h"
#include "util.h"

#define COORDINATE_ALLOCATION_SIZE    10
#define MESSAGE_ALLOCATION_SIZE    10000
#define GROUP_ALLOCATION_SIZE         10
#define PARSER_MAX_ERROR_COUNT         3

/*
set DEBUG to 1 to activate the debugging print statements.
set DEBUG to 0 to deactivate the debugging print statements.
If DEBUG is zero, most compilers will not generate any code for the debugging
statements.
*/
enum {DEBUG = 0};

enum tTokenId {
	eTokenIdCode,
	eTokenIdColon,
	eTokenIdDone,
	eTokenIdError,
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
	int TagIndex;
	char Code[STCM_CODE_MAX_LENGTH+1];
	/* points to the STCT_GROUPROOT receive by STC_GroupParse() */
	STCT_GROUPROOT * GroupRoot;
	/* points to the hierarchy strings. */
	char * GroupString;
	/* points somewhere in GroupString. the scanner moves it as it scans the string. */
	char * GroupStringPtr;
	/* space for error message */
	char Message[MESSAGE_ALLOCATION_SIZE+1];
	/* number of errors found */
	int ErrorCount;
	STCT_HTREEROOT * HTreeRoot;
};
typedef struct tContext tContext;


static EIT_BOOLEAN CodesAreRelated (STCT_HTREE * HTree, char * Code1, char * Code2);
static void ContextPrint (tContext * Context);
static EIT_BOOLEAN CoordinatesAreRelated (STCT_HTREEROOT * HTreeRoot,
	STCT_COORDINATE * Coordinate1, STCT_COORDINATE * Coordinate2);
static EIT_RETURNCODE GroupAddCoordinate (STCT_GROUP * Group);
static void GroupFree (STCT_GROUP * Group);
static void GroupPrint (STCT_GROUP * Group, STCT_HTREEROOT * HTreeRoot);
static EIT_RETURNCODE GroupRootAddGroup (STCT_GROUPROOT * GroupRoot);
static STCT_GROUPROOT * GroupRootAllocate (void);
static EIT_RETURNCODE GroupRootValidate (STCT_GROUPROOT * GroupRoot, STCT_HTREEROOT * HTreeRoot, STCT_RANGEROOT * RangeRoot);
static EIT_RETURNCODE InstallCode (tToken * Token);
static EIT_RETURNCODE InstallCoordinate (tToken * Token);
static EIT_RETURNCODE InstallGroup (tToken * Token);
static EIT_RETURNCODE InstallGroups (tToken * Token);
static EIT_RETURNCODE Match (tTokenId Id, tToken * Token);
static EIT_RETURNCODE MatchCode (tToken * Token);
static EIT_RETURNCODE MatchCoordinate (tToken * Token);
static EIT_RETURNCODE MatchGroup (tToken * Token);
static EIT_RETURNCODE MatchGroups (tToken * Token);
static void ReportError (char * Message);
static EIT_RETURNCODE Scanner (tToken * Token);
static char * TokenIdTranslator (tTokenId TokenId);
static void TokenPrint (tToken * Token);


static char * mCodeFirstCharacterCharacterSet;
static char * mCodeCharacterSet;
static tContext mContext;


/*------------------------------------------------------------------------------
Parse a group specification. Puts the result in the STCT_GROUPROOT structure.
It is the programmer's responsability to free that structure.
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_GroupParse (
	char * GroupString,
	STCT_GROUPROOT ** GroupRoot,
	STCT_HTREEROOT * HTreeRoot,
	STCT_RANGEROOT * RangeRoot,
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
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Group=%s\n", GroupString);
	}
#endif

	mContext.GroupString = STC_StrDup (GroupString);
	if (mContext.GroupString == NULL) return EIE_FAIL;
	UTIL_DropBlanksSafeWithAccentuatedCharacters (mContext.GroupString);
	mContext.GroupStringPtr = mContext.GroupString;
	mContext.TagIndex = 0;
	mContext.Code[0] = '\0';
	mContext.Message[0] = '\0';
	mContext.ErrorCount = 0;
	mContext.GroupRoot = GroupRootAllocate ();
	if (mContext.GroupRoot == NULL) return EIE_FAIL;
	mContext.HTreeRoot = HTreeRoot;

	crc = EIE_SUCCEED;
	Done = EIE_FALSE;
	while (!Done) {
		rc = Scanner (&Token);
		if (rc == EIE_SUCCEED) {
			rc = MatchGroups (&Token);
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
				EI_AddMessage (M00047, EIE_MESSAGESEVERITY_ERROR,
					M30003); /* Too many errors. Stop processing. */
			}
			else {
				/* advance to the next edits */
				mContext.GroupStringPtr = strchr (mContext.GroupStringPtr, ';');
				if (mContext.GroupStringPtr == NULL)
					Done = EIE_TRUE;
				else {
					mContext.GroupStringPtr++;
					if (*mContext.GroupStringPtr == '\0') {
						Done = EIE_TRUE;
					}
					else {
						//reset Context information and start over.
						//sacrifice space already allocated in mContext.GroupRoot.
						//in some cases the state of the structure is such that I can't call
						//STC_GroupRootFree() without crashing the program!
						mContext.GroupRoot = GroupRootAllocate ();
						if (mContext.GroupRoot == NULL) return EIE_FAIL;
					}
				}
			}
		}
	}

	if (crc == EIE_SUCCEED) {
		rc = GroupRootValidate (mContext.GroupRoot, HTreeRoot, RangeRoot);
		if (rc != EIE_SUCCEED) {
			STC_GroupRootFree (mContext.GroupRoot);
			STC_FreeMemory (mContext.GroupString);
			return EIE_FAIL;
		}
	}

	STC_FreeMemory (mContext.GroupString);

	*GroupRoot = mContext.GroupRoot;

	return crc;
}
/*------------------------------------------------------------------------------
Print a STCT_GROUP structure
------------------------------------------------------------------------------*/
void STC_GroupPrint (
	STCT_GROUP * Group,
	STCT_HTREEROOT * HTreeRoot)
{
	GroupPrint (Group, HTreeRoot);
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
}
/*------------------------------------------------------------------------------
Free the STCT_GROUPROOT structure
------------------------------------------------------------------------------*/
void STC_GroupRootFree (
	STCT_GROUPROOT * GroupRoot)
{
	int i;

	if (GroupRoot == NULL) return;

	for (i = 0; i < GroupRoot->NumberEntries; i++) {
		GroupFree (GroupRoot->Group[i]);
	}
	STC_FreeMemory (GroupRoot->Group);
	STC_FreeMemory (GroupRoot);
}
/*------------------------------------------------------------------------------
Print the STCT_GROUPROOT structure
------------------------------------------------------------------------------*/
void STC_GroupRootPrint (
	STCT_GROUPROOT * GroupRoot,
	STCT_HTREEROOT * HTreeRoot)
{
	int i;

	if (GroupRoot == NULL) return;

	for (i = 0; i < GroupRoot->NumberEntries; i++) {
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, M10040 "\n", i+1);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, M10040 "\n", i+1);
		GroupPrint (GroupRoot->Group[i], HTreeRoot);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "");
}


/*------------------------------------------------------------------------------
check if codes are related
2 codes are related if one is a descendant of the other.
------------------------------------------------------------------------------*/
static EIT_BOOLEAN CodesAreRelated (
	STCT_HTREE * HTree,
	char * Code1,//precondition: Code1 exist in HTree
	char * Code2)//precondition: Code2 exist in HTree
{
	STCT_HTREE * HTree1;
	STCT_HTREE * HTree2;

	/* find Code1 in hierarchy */
	HTree1 = STC_HTreeSearchCode (HTree, Code1);
	HTree2 = STC_HTreeSearchCode (HTree1, Code2); /* check if Code2 is a descendant of Code1 */
	if (HTree2 == NULL) {
		/* find Code2 in hierarchy */
		HTree1 = STC_HTreeSearchCode (HTree, Code2);
		HTree2 = STC_HTreeSearchCode (HTree1, Code1); /* check if Code1 is a descendant of Code2 */
		if (HTree2 == NULL)
			return EIE_FALSE;
	}

	return EIE_TRUE;
}
/*------------------------------------------------------------------------------
print context structure (debug function).
------------------------------------------------------------------------------*/
static void ContextPrint (
	tContext * Context)
{
	int GroupIndex = mContext.GroupRoot->NumberEntries-1;
	int CoordinateIndex = mContext.GroupRoot->Group[mContext.GroupRoot->NumberEntries-1]->NumberEntries-1;

	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY,
		"group=%d "
		"coordinate=%d "
		"code=%d "
		"tag=%d\n",
		GroupIndex,
		CoordinateIndex,
		mContext.TagIndex,
		mContext.GroupRoot->Group[GroupIndex]->Coordinate[CoordinateIndex]->Tag[mContext.TagIndex]);
	EI_PrintMessages ();
}
/*------------------------------------------------------------------------------
check if coordinates are related.
2 coordinates are related if all their codes are related.
------------------------------------------------------------------------------*/
static EIT_BOOLEAN CoordinatesAreRelated (
	STCT_HTREEROOT * HTreeRoot,
	STCT_COORDINATE * Coordinate1,
	STCT_COORDINATE * Coordinate2)
{
	char * Code1;
	char * Code2;
	int i, number;

	number = 0;
	for (i = 0; i < Coordinate1->NumberEntries; i++) {
		if (Coordinate1->Tag[i] == Coordinate2->Tag[i])
			number++;
		else {
			Code1 = HTreeRoot->TagIndex[i][Coordinate1->Tag[i]]->Code;
			Code2 = HTreeRoot->TagIndex[i][Coordinate2->Tag[i]]->Code;
			if (CodesAreRelated (HTreeRoot->Dimension[i], Code1, Code2))
				number++;
			else
				return EIE_FALSE;
		}
	}
	if (number != HTreeRoot->NumberEntries)
		return EIE_FALSE;

	return EIE_TRUE;
}
/*------------------------------------------------------------------------------
Add a group in the group root
------------------------------------------------------------------------------*/
static EIT_RETURNCODE GroupAddCoordinate (
	STCT_GROUP * Group)
{
	void * Ptr;
	STCT_COORDINATE * Coordinate;
	Coordinate = STC_CoordinateAllocate (mContext.HTreeRoot->NumberEntries);
	if (Group->NumberAllocated == Group->NumberEntries) {
		Group->NumberAllocated += COORDINATE_ALLOCATION_SIZE;
		Ptr = STC_ReallocateMemory (
			Group->NumberEntries * sizeof *Group->Coordinate,
			Group->NumberAllocated * sizeof *Group->Coordinate,
			Group->Coordinate);
		if (Ptr == NULL) return EIE_FAIL;
		Group->Coordinate = Ptr;
	}
	Group->Coordinate[Group->NumberEntries++] = Coordinate;
	mContext.TagIndex = 0;
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Allocate the STCT_GROUP structure
------------------------------------------------------------------------------*/
static STCT_GROUP * GroupAllocate (
	void)
{
	STCT_GROUP * Group;

	Group = STC_AllocateMemory (sizeof *Group);
	if (Group == NULL) return NULL;
	Group->NumberAllocated = 0;
	Group->NumberEntries = 0;
	Group->Coordinate = NULL;
	return Group;
}
/*------------------------------------------------------------------------------
Free the STCT_GROUP structure
------------------------------------------------------------------------------*/
static void GroupFree (
	STCT_GROUP * Group)
{
	int i;

	for (i = 0; i < Group->NumberEntries; i++) {
		STC_CoordinateFree (Group->Coordinate[i]);
	}
	STC_FreeMemory (Group->Coordinate);
	STC_FreeMemory (Group);
}
/*------------------------------------------------------------------------------
Print the STCT_GROUP structure
------------------------------------------------------------------------------*/
static void GroupPrint (
	STCT_GROUP * Group,
	STCT_HTREEROOT * HTreeRoot)
{
	static char GroupStr[4001];//rene: dangereux!
	static char Str[1001];//rene: dangereux!
	int i, j;

	GroupStr[0] = '\0';
	strcat (GroupStr, "    ");
	for (i = 0; i < Group->NumberEntries; i++) {
		if (i > 0) strcat (GroupStr, " : ");
		for (j = 0; j < Group->Coordinate[i]->NumberEntries; j++) {
			if (j > 0) strcat (GroupStr, ",");
			sprintf (Str, "%s", HTreeRoot->TagIndex[j][Group->Coordinate[i]->Tag[j]]->Code);
			strcat (GroupStr, Str);
		}
		strcat (GroupStr, ".");
	}
	strcat (GroupStr, " ; ");

	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, GroupStr);
	EI_PrintMessages ();
}
/*------------------------------------------------------------------------------
Add a group in the group root
------------------------------------------------------------------------------*/
static EIT_RETURNCODE GroupRootAddGroup (
	STCT_GROUPROOT * GroupRoot)
{
	void * Ptr;
	STCT_GROUP * Group;
	Group = GroupAllocate ();
	if (GroupRoot->NumberAllocated == GroupRoot->NumberEntries) {
		GroupRoot->NumberAllocated += GROUP_ALLOCATION_SIZE;
		Ptr = STC_ReallocateMemory (
			GroupRoot->NumberEntries * sizeof *GroupRoot->Group,
			GroupRoot->NumberAllocated * sizeof *GroupRoot->Group,
			GroupRoot->Group);
		if (Ptr == NULL) return EIE_FAIL;
		GroupRoot->Group = Ptr;
	}
	GroupRoot->Group[GroupRoot->NumberEntries++] = Group;
	mContext.TagIndex = 0;
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Allocate the STCT_GROUPROOT structure
------------------------------------------------------------------------------*/
STCT_GROUPROOT * GroupRootAllocate (void)
{
	STCT_GROUPROOT * GroupRoot;

	GroupRoot = STC_AllocateMemory (sizeof *GroupRoot);
	if (GroupRoot == NULL) return NULL;
	GroupRoot->NumberAllocated = 0;
	GroupRoot->NumberEntries = 0;
	GroupRoot->Group = NULL;
	return GroupRoot;
}
/*------------------------------------------------------------------------------
Validate the STCT_GROUPROOT structure
------------------------------------------------------------------------------*/
static EIT_RETURNCODE GroupRootValidate (
	STCT_GROUPROOT * GroupRoot,
	STCT_HTREEROOT * HTreeRoot,
	STCT_RANGEROOT * RangeRoot)
{
	char * CodeJ;
	char * CodeK;
	STCT_COORDINATE * CoordinateJ;
	STCT_COORDINATE * CoordinateK;
	EIT_RETURNCODE crc; /* cumulative return code */
	int i, j, k, l;
	STCT_GROUP * GroupI;
	STCT_HTREE * HTreeJ;
	STCT_HTREE * HTreeK;
	int number;
	STCT_RANGE * RangeJ;
	STCT_RANGE * RangeK;
	EIT_RETURNCODE rc;

	if (GroupRoot == NULL) return EIE_SUCCEED;

	crc = EIE_SUCCEED;
	for (i = 0; i < GroupRoot->NumberEntries; i++) {
		GroupI = GroupRoot->Group[i];

		//EI_AddMessage ("", 4, "Group %d\n", i+1);
		//GroupPrint (GroupI, HTreeRoot);

		for (j = 0; j < GroupI->NumberEntries-1; j++) {
			CoordinateJ = GroupI->Coordinate[j];

			//checks if GroupI->Coordinate[j] and Group->Coordinate[k] are related in the hierarchy
			//EI_AddMessage ("", 4, "Coordinate %d\n", j+1);
			//for (k = j+1; k < Group->NumberEntries; k++) {
			//	CoordinateK = GroupI->Coordinate[k];
			//	//EI_AddMessage ("", 4, "checking group %d coordinate index %d and coordinate index %d\n", i+1, j+1, k+1);
			//	if (CoordinatesAreRelated (mContext.HTreeRoot, CoordinateJ, CoordinateK)) {
			//		EI_AddMessage ("", 4, "Two coordinates of group %d are related (coordinates %d and %d are related)." "\n", i+1, j+1, k+1);
			//		STC_CoordinatePrint (CoordinateJ, HTreeRoot);
			//		EI_AddMessage ("", 4, " and \n");
			//		STC_CoordinatePrint (CoordinateK, HTreeRoot);
			//		EI_AddMessage ("", 4, "\n\n");
			//		return EIE_FAIL;
			//	}
			//}

			//checks if GroupI->Coordinate[j] and Group->Coordinate[k] are related in the range meaning,
			//do they share the same data

			for (k = j+1; k < GroupI->NumberEntries; k++){
				CoordinateK = GroupI->Coordinate[k];

				number = 0;
				for (l = 0; l < CoordinateJ->NumberEntries; l++) {
					CodeJ = HTreeRoot->TagIndex[l][CoordinateJ->Tag[l]]->Code;
					HTreeJ = STC_HTreeSearchCode (HTreeRoot->Dimension[l], CodeJ);
					RangeJ = STC_RangeAllocate ();
					if (RangeJ == NULL) return EIE_FAIL;
					rc = STC_GetEndCodes (HTreeJ, RangeRoot->Dimension[l], 0, RangeJ);
					if (rc != EIE_SUCCEED) return EIE_FAIL;

					CodeK = HTreeRoot->TagIndex[l][CoordinateK->Tag[l]]->Code;
					HTreeK = STC_HTreeSearchCode (HTreeRoot->Dimension[l], CodeK);
					RangeK = STC_RangeAllocate ();
					if (RangeK == NULL) return EIE_FAIL;
					rc = STC_GetEndCodes (HTreeK, RangeRoot->Dimension[l], 0, RangeK);
					if (rc != EIE_SUCCEED) return EIE_FAIL;

					if (STC_RangeIntersect (RangeJ, RangeK)) {
						number++;
					}
					STC_RangeFree (RangeJ);
					STC_RangeFree (RangeK);
				}
				if (number == RangeRoot->NumberEntries) {
					EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, M10040 "\n", i+1);
					GroupPrint (GroupI, HTreeRoot);
					EI_AddMessage (M00052, EIE_MESSAGESEVERITY_ERROR, M30165 "\n", i+1, j+1, k+1);
					crc = EIE_FAIL;
				}
			}
		}
	}
	return crc;
}
/*------------------------------------------------------------------------------
validate the group-hierarchy-range combinaison
------------------------------------------------------------------------------*/
//EIT_RETURNCODE STC_ValidateGroupCodes (
//	STCT_GROUPROOT * GroupRoot,
//	STCT_HTREEROOT * HTreeRoot,
//	STCT_RANGEROOT * RangeRoot)
//{
//	char * CodeJ;
//	char * CodeK;
//	STCT_COORDINATE * CoordinateJ;
//	STCT_COORDINATE * CoordinateK;
//	int i, j, k;
//	STCT_GROUP * Group;
//	STCT_HTREE * HTreeJ;
//	STCT_HTREE * HTreeK;
//	STCT_RANGE * RangeJ;
//	STCT_RANGE * RangeK;
//	EIT_RETURNCODE rc;
//
//	EI_AddMessage ("", 4, "STC_ValidateGroupCodes() to be finish!\n");
//
//	return EIE_SUCCEED;
//
//	if (GroupRoot == NULL) return EIE_SUCCEED;
//
//	for (i = 0; i < GroupRoot->NumberEntries; i++){
//		Group = GroupRoot->Group[i];
//		for (j = 0; j < Group->NumberEntries-1; j++){
//			CoordinateJ = Group->Coordinate[j];
//			CodeJ = HTreeRoot->TagIndex[i][CoordinateJ->Tag[i]]->Code;
//			HTreeJ = STC_HTreeSearchCode (HTreeRoot->Dimension[i], CodeJ);
//			RangeJ = STC_RangeAllocate ();
//			rc = STC_GetEndCodes (HTreeJ, RangeRoot->Dimension[j], 0, RangeJ);
//			if (rc != EIE_SUCCEED) EIE_FAIL;
//			for (k = j+1; k < Group->NumberEntries; k++){
//				CoordinateK = Group->Coordinate[k];
//				CodeK = HTreeRoot->TagIndex[i][CoordinateK->Tag[i]]->Code;
//				HTreeK = STC_HTreeSearchCode (HTreeRoot->Dimension[i], CodeK);
//				RangeK = STC_RangeAllocate ();
//				rc = STC_GetEndCodes (HTreeK, RangeRoot->Dimension[i], 0, RangeK);
//				if (rc != EIE_SUCCEED) EIE_FAIL;
//				EI_AddMessage ("", 4, "to finish!\n");
//				//if (RangeIntersect (RangeJ, RangeK)) {
//				//	EI_AddMessage ("Range validation", EIE_MESSAGESEVERITY_ERROR, "Group have cells with repeating codes or range.");
//				//	//EI_AddMessage ("Range validation", EIE_MESSAGESEVERITY_ERROR,
//				//	//	M30111,//The decomposition has 2 overlapping codes or ranges
//				//	//	i+1, HTree->Code,
//				//	//	HTree->Decomposition[i]->Branch[j]->Code,
//				//	//	HTree->Decomposition[i]->Branch[k]->Code);
//				//	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Range %d" "\n", j+1);
//				//	STC_RangePrint (RangeJ);
//				//	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Range %d\n", k+1);
//				//	STC_RangePrint (RangeK);
//				//	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
//				//	STC_RangeFree (RangeJ);
//				//	STC_RangeFree (RangeK);
//				//	return EIE_FAIL;
//				//}
//				STC_RangeFree (RangeK);
//			}
//			STC_RangeFree (RangeJ);
//		}
//	}
//	return EIE_SUCCEED;
//}
/*------------------------------------------------------------------------------
A code was found
------------------------------------------------------------------------------*/
static EIT_RETURNCODE InstallCode (
	tToken * Token)
{
	STCT_COORDINATE * Coordinate;
	STCT_GROUP * Group;
	STCT_HTREE * HTree;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "InstallCode\n");
#endif

	//ContextPrint (&mContext);

	if (mContext.TagIndex >= mContext.HTreeRoot->NumberEntries) {
		sprintf (mContext.Message, M30160 "\n",
			mContext.HTreeRoot->NumberEntries);
		ReportError (mContext.Message);
		return EIE_FAIL;
	}

	Group = mContext.GroupRoot->Group[mContext.GroupRoot->NumberEntries-1];
	Coordinate = Group->Coordinate[Group->NumberEntries-1];

	HTree = STC_HTreeSearchCode (mContext.HTreeRoot->Dimension[mContext.TagIndex], Token->Code);

	if (HTree == NULL) {
		sprintf (mContext.Message, M30161 "\n",
			Token->Code, mContext.HTreeRoot->Name[mContext.TagIndex]);
		ReportError (mContext.Message);
		return EIE_FAIL;
	}

	Coordinate->Tag[mContext.TagIndex] = HTree->Tag;
	mContext.TagIndex++;

	EI_PrintMessages ();
	fflush (NULL);
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
A coordinate was found
------------------------------------------------------------------------------*/
static EIT_RETURNCODE InstallCoordinate (
	tToken * Token)
{
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "InstallCoordinate\n");
#endif
	EI_PrintMessages ();
	fflush (NULL);

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
A group was found
------------------------------------------------------------------------------*/
static EIT_RETURNCODE InstallGroup (
	tToken * Token)
{
	EIT_RETURNCODE rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "InstallGroup\n");
#endif
	rc = GroupAddCoordinate (mContext.GroupRoot->Group[mContext.GroupRoot->NumberEntries-1]);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
A groups was found
------------------------------------------------------------------------------*/
static EIT_RETURNCODE InstallGroups (
	tToken * Token)
{
	EIT_RETURNCODE rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "InstallGroups\n");
#endif
	rc = GroupRootAddGroup (mContext.GroupRoot);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

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
static EIT_RETURNCODE MatchCode (
	tToken * Token)
{
	int rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MatchCode\n");
#endif

	if (Token->Id == eTokenIdCode) {
		rc = InstallCode (Token);
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
static EIT_RETURNCODE MatchCoordinate (
	tToken * Token)
{
	int rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MatchCoordinate\n");
#endif
	rc = InstallCoordinate (Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	rc = MatchCode (Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	if (Token->Id != eTokenIdColon && Token->Id != eTokenIdSemiColon) {
		rc = MatchCoordinate (Token);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE MatchGroup (
	tToken * Token)
{
	int rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MatchGroup\n");
#endif

	rc = InstallGroup (Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	rc = MatchCoordinate (Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	if (Token->Id == eTokenIdColon) {
		if (mContext.TagIndex < mContext.HTreeRoot->NumberEntries) {
			sprintf (mContext.Message, M30162 "\n",
				mContext.HTreeRoot->NumberEntries);
			ReportError (mContext.Message);
			return EIE_FAIL;
		}
		rc = Match (eTokenIdColon, Token);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
		rc = MatchGroup (Token);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}
	else if (Token->Id == eTokenIdSemiColon) {
		if (mContext.TagIndex < mContext.HTreeRoot->NumberEntries) {
			sprintf (mContext.Message, M30162 "\n",
				mContext.HTreeRoot->NumberEntries);
			ReportError (mContext.Message);
			return EIE_FAIL;
		}
		rc = Match (eTokenIdSemiColon, Token);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE MatchGroups (
	tToken * Token)
{
	int rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MatchGroups\n");
#endif

	rc = InstallGroups (Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	rc = MatchGroup (Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	if (mContext.GroupRoot->Group[mContext.GroupRoot->NumberEntries-1]->NumberEntries == 1) {
		ReportError (M30163);
		return EIE_FAIL;
	}

	if (Token->Id != eTokenIdDone) {
		rc = MatchGroups (Token);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Adds a message to the message list.
------------------------------------------------------------------------------*/
static void ReportError (
	char * Message)
{
	if (strlen (mContext.GroupString) > (MESSAGE_ALLOCATION_SIZE/2)) {
		//set ErrorCount to MAX because we are modifying the string
		mContext.ErrorCount = PARSER_MAX_ERROR_COUNT;
		mContext.GroupString[(MESSAGE_ALLOCATION_SIZE/2)-5] = '\0';
		strcat (mContext.GroupString, "...");
	}
	UTIL_ReportError (M00047, M00002, Message, mContext.GroupString, mContext.GroupStringPtr);
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

	rc = UTIL_ConsumeComments (mContext.GroupStringPtr, &n, &Message);
	if (rc != EIE_SUCCEED) {
		ReportError (Message);
		return EIE_FAIL;
	}
	p = mContext.GroupStringPtr+n;

	n = strspn (p, mCodeFirstCharacterCharacterSet);
	if (n > 0) {
		/* found code */
		n = strspn (p, mCodeCharacterSet);
		if (n > STCM_CODE_MAX_LENGTH) {
			Token->Id = eTokenIdError;
			ReportError (M30151);
			p += n;
			mContext.GroupStringPtr = p;
			return EIE_FAIL;
		}
		strncpy (Temp, p, n);
		Temp[n] = '\0';
		p += strlen (Temp);
		Token->Id = eTokenIdCode;
		strcpy (Token->Code, Temp);
	}
	else if (*p == ':') {
		/* found Colon */
		p++;
		Token->Id = eTokenIdColon;
		strcpy (Token->Code, "");
	}
	else if (*p == ';') {
		/* found SemiColon */
		p++;
		Token->Id = eTokenIdSemiColon;
		strcpy (Token->Code, "");
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
		ReportError (M30164);
		return EIE_FAIL;
	}
#ifdef _DEBUG
	if (DEBUG) {
		TokenPrint (Token);
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "p=<%s>\n", p);
	}
#endif
	mContext.GroupStringPtr = p;

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
	case eTokenIdCode:
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%s %s\n", TokenIdTranslator (Token->Id),
			Token->Code);
		break;
	}
}
