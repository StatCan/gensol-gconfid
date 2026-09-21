/*------------------------------------------------------------------------------
hierarchy parser
This is the productions list for the parser:

hierarchies -> hierarchy
			|  hierarchy hierarchies
hierarchy   -> levels ';'
			|  ';'
levels      -> level
			|  level ':' levels
level       -> parent enfants
parent      -> code
enfants     -> enfant
			|  enfant enfants
			|  enfant increment
increment   -> '-'number enfants
enfant      -> code
code        -> TOKEN_CODE
number      -> TOKEN_NUMBER

TOKEN_NUMBER   {list of digits}
TOKEN_CODE     {list of digits}
------------------------------------------------------------------------------*/
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "EI_Message.h"
#include "MessageGConfidAPI.h"
#include "slist.h"
#include "STC_Hierarchy.h"
#include "STC_Memory.h"
#include "util.h"


#define CHILD_ALLOCATION_SIZE        30
#define DIMENSION_ALLOCATION_SIZE     5
#define MESSAGE_ALLOCATION_SIZE   10000
#define PARSER_MAX_ERROR_COUNT       10
#define TAG_NOT_SET                (-1)


/*
set DEBUG to 1 to activate the debugging print statements.
set DEBUG to 0 to deactivate the debugging print statements.
If DEBUG is zero, most compilers won't generate any code for the debugging
statements.
*/
enum {DEBUG = 0};

enum tStatus {
	eNewDimension,
	eNewDecomposition
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
	int DimensionIndex;
	STCT_HTREE * CurrentBranch;
	char First[STCM_CODE_MAX_LENGTH+1];
	char Increment[STCM_CODE_MAX_LENGTH+1];
	/* points to the STCT_HIERARCHY receive by EI_HierarchyParse() */
	STCT_HTREEROOT * HTreeRoot;
	/* points to the hierarchy strings. */
	char * HierarchyString;
	/* points somewhere in HierarchyString. the scanner moves it as it scans the string. */
	char * HierarchyStringPtr;
	/* space for error message */
	char Message[MESSAGE_ALLOCATION_SIZE+1];
	/* number of errors found */
	int ErrorCount;

	/* when parsing, hold semi formed hierarchies */
	STCT_HTREE ** HTreeList;
	int HTreeListNumberEntries;
	int HTreeListNumberAllocated;
};
typedef struct tContext tContext;

static EIT_RETURNCODE CheckIdenticalDecompositions (STCT_HTREE * HTree);
static EIT_RETURNCODE CheckRepeatedCodes (STCT_HTREE * HTree);
static EIT_RETURNCODE HTreeAddBranch (STCT_HTREE * HTree, STCT_HTREE * Branch);
static EIT_RETURNCODE HTreeAddCode (STCT_HTREE * HTree, char * Code);
static STCT_HTREE * HTreeAllocate (char * Code);
static int HTreeFindLargestTag (STCT_HTREE * HTree, int Decomposition);
static void HTreeFindLargestTag1 (STCT_HTREE * HTree, int * LargestTag);
static int HTreeFindSmallestTag (STCT_HTREE * HTree, int Decomposition);
static void HTreeFindSmallestTag1 (STCT_HTREE * HTree, int * SmallestTag);
static int HTreeFindLargestTagAllDecompositions (STCT_HTREE * HTree, int * LargestTag);
static EIT_RETURNCODE HTreeGetCodes (STCT_HTREE * HTree, tSList * Codes);
static EIT_BOOLEAN HTreeHasMultipleDecompositions (STCT_HTREE * HTree);
static EIT_RETURNCODE HTreeIndexCreate (STCT_HTREE * HTree, int LargestTag, STCT_HTREE *** HTreeIndex);
static void HTreeListPrint (void);
static EIT_RETURNCODE HTreeListReallocate (void);
static void HTreePrint (STCT_HTREE * HTree, int Level);
static EIT_RETURNCODE HTreeRootAddDimension (STCT_HTREEROOT * HTreeRoot);
static STCT_HTREEROOT * HTreeRootAllocate (void);
static EIT_BOOLEAN HTreeRootHasMultipleDecompositions (STCT_HTREEROOT * HTreeRoot);
static EIT_RETURNCODE HTreeRootIndexCreate (STCT_HTREEROOT * HTreeRoot);
static void HTreeRootIndexPrint (STCT_HTREEROOT * HTreeRoot);
static void HTreeRootTagNodes (STCT_HTREEROOT * HTreeRoot);
static EIT_RETURNCODE HTreeRootValidate (STCT_HTREEROOT * HTreeRoot);
static STCT_HTREE * HTreeSearchTag (STCT_HTREE * HTree, int Tag);
static void HTreeTagNodes (STCT_HTREE * HTree, int * Tag);
static EIT_RETURNCODE HTreeValidate (STCT_HTREE * HTree);
static EIT_RETURNCODE InstallChild (tToken * Token);
static EIT_RETURNCODE InstallParent (tToken * Token);
static EIT_RETURNCODE Match (tTokenId Id, tToken * Token);
static EIT_RETURNCODE MatchChild (tToken * Token);
static EIT_RETURNCODE MatchChildren (tToken * Token);
static EIT_RETURNCODE MatchHierarchies (tToken * Token);
static EIT_RETURNCODE MatchHierarchy (tToken * Token);
static EIT_RETURNCODE MatchIncrement (tToken * Token);
static EIT_RETURNCODE MatchLevel (tToken * Token);
static EIT_RETURNCODE MatchLevels (tToken * Token);
static EIT_RETURNCODE MatchParent (tToken * Token);
static void ReportError (char * Message);
static EIT_RETURNCODE Scanner (tToken * Token);
static EIT_RETURNCODE TerminateHierarchy (void);
static char * TokenIdTranslator (tTokenId TokenId);
static void TokenPrint (tToken * Token);


static char * mCodeFirstCharacterCharacterSet;
static char * mCodeCharacterSet;
static tContext mContext;


/*------------------------------------------------------------------------------
Parse string to create a tree then validate and create tag indexes for the tree
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_HTreeParse (
	char * HierarchyString,
	STCT_HTREEROOT ** HTreeRoot,
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
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Hierarchy=%s\n", HierarchyString);
	}
#endif

	mContext.HierarchyString = STC_StrDup (HierarchyString);
	if (mContext.HierarchyString == NULL) return EIE_FAIL;
	UTIL_DropBlanksSafeWithAccentuatedCharacters (mContext.HierarchyString);
	mContext.HierarchyStringPtr = mContext.HierarchyString;
	mContext.Status = eNewDimension;
	mContext.DimensionIndex = 0;
	mContext.First[0] = '\0';
	mContext.Increment[0] = '\0';
	mContext.Message[0] = '\0';
	mContext.ErrorCount = 0;
	mContext.HTreeRoot = HTreeRootAllocate ();
	if (mContext.HTreeRoot == NULL) return EIE_FAIL;

	mContext.HTreeList = NULL;
	mContext.HTreeListNumberEntries = 0;
	mContext.HTreeListNumberAllocated = 0;

	crc = EIE_SUCCEED;
	Done = EIE_FALSE;
	while (!Done) {
		rc = Scanner (&Token);
		if (rc == EIE_SUCCEED) {
			rc = MatchHierarchies (&Token);
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
				EI_AddMessage (M00045, EIE_MESSAGESEVERITY_ERROR,
					M30003); /* Too many errors. Stop processing. */
			}
			else {
				/* advance to the next edits */
				mContext.HierarchyStringPtr = strchr (mContext.HierarchyStringPtr, ';');
				if (mContext.HierarchyStringPtr == NULL)
					Done = EIE_TRUE;
				else {
					mContext.HierarchyStringPtr++;
					if (*mContext.HierarchyStringPtr == '\0') {
						Done = EIE_TRUE;
					}
					else {
						//reset Context information
						mContext.Status = eNewDimension;
						mContext.DimensionIndex = 0;
						mContext.First[0] = '\0';
						mContext.Increment[0] = '\0';
						//waste the space already allocated.
						//in some cases the state of the structure is such that I can't call
						//STC_HTreeRootFree() without crashing the program!
						mContext.HTreeRoot = HTreeRootAllocate ();
						if (mContext.HTreeRoot == NULL) return EIE_FAIL;
					}
				}
			}
		}
	}

	STC_FreeMemory (mContext.HierarchyString);
	STC_FreeMemory (mContext.HTreeList);

	if (crc == EIE_SUCCEED) {
		int i, j, k;

		rc = HTreeRootValidate (mContext.HTreeRoot);
		if (rc != EIE_SUCCEED) return EIE_FAIL;

		HTreeRootTagNodes (mContext.HTreeRoot);

		mContext.HTreeRoot->LargestTag = STC_AllocateMemory (
			mContext.HTreeRoot->NumberEntries * sizeof *mContext.HTreeRoot->LargestTag);
		if (mContext.HTreeRoot->LargestTag == NULL) return EIE_FAIL;
		for (i = 0; i < mContext.HTreeRoot->NumberEntries; i++) {
			int LargestTag = -INT_MAX;
			mContext.HTreeRoot->LargestTag[i] =
				HTreeFindLargestTagAllDecompositions (
					mContext.HTreeRoot->Dimension[i], &LargestTag);
		}

		rc = HTreeRootIndexCreate (mContext.HTreeRoot);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
#ifdef _DEBUG
		if (DEBUG) HTreeRootIndexPrint (mContext.HTreeRoot);
#endif
		mContext.HTreeRoot->HasMultipleDecompositions =
			HTreeRootHasMultipleDecompositions (mContext.HTreeRoot);

		for (i = 0; i < mContext.HTreeRoot->NumberEntries; i++) {
			for (j = 0; j <= mContext.HTreeRoot->LargestTag[i]; j++) {
				if (mContext.HTreeRoot->TagIndex[i][j] != NULL) {
					for (k = 0; k < mContext.HTreeRoot->TagIndex[i][j]->NumberDecompositions; k++) {
						//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "i=%d j=%d k=%d\n", i, j, k);
						//EI_PrintMessages ();
						mContext.HTreeRoot->TagIndex[i][j]->SmallestTag[k] =
							HTreeFindSmallestTag (mContext.HTreeRoot->TagIndex[i][j], k);
						mContext.HTreeRoot->TagIndex[i][j]->LargestTag[k] =
							HTreeFindLargestTag (mContext.HTreeRoot->TagIndex[i][j], k);
					}
				}
			}
		}
	}

	*HTreeRoot = mContext.HTreeRoot;

	return crc;
}


/*------------------------------------------------------------------------------
Check identical decompositions like "0 1 2: 0 1 2;"
------------------------------------------------------------------------------*/
static EIT_RETURNCODE CheckIdenticalDecompositions (
	STCT_HTREE * HTree)
{
	tSList ** Codes;
	EIT_BOOLEAN Equal;
	int i, j;

	if (HTree->NumberDecompositions < 2) return EIE_SUCCEED;

	Codes = STC_AllocateMemory (HTree->NumberDecompositions * sizeof *Codes);
	if (Codes == NULL) return EIE_FAIL;
	for (i = 0; i < HTree->NumberDecompositions; i++) {
		SList_New (&Codes[i]);
		if (Codes[i] == NULL) return EIE_FAIL;
		for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
			if (SList_Add (HTree->Decomposition[i]->Branch[j]->Code, Codes[i]) == eSListFail)
				return EIE_FAIL;
		}
		SList_Sort (Codes[i], eSListSortAscending);
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Codes of decomp %d\n", i);
		//SList_Print (Codes[i]);
	}

	for (i = 0; i < HTree->NumberDecompositions-1; i++) {
		for (j = i+1; j < HTree->NumberDecompositions; j++) {
			Equal = SList_Equal (Codes[i], Codes[j]);
			if (Equal) {
				EI_AddMessage ("", EIE_MESSAGESEVERITY_ERROR, M30115 "\n", HTree->Code);//2 identical decompositions
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, M30112 "\n");
				SList_Print (Codes[i]);
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
				for (i = 0; i < HTree->NumberDecompositions; i++) {
					SList_Free (Codes[i]);
				}
				STC_FreeMemory (Codes);
				return EIE_FAIL;
			}
		}
	}

	for (i = 0; i < HTree->NumberDecompositions; i++) {
		SList_Free (Codes[i]);
	}
	STC_FreeMemory (Codes);

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
check if codes are in two branches of a tree
------------------------------------------------------------------------------*/
static EIT_RETURNCODE CheckRepeatedCodes (
	STCT_HTREE * HTree)
{
	STCT_HTREE * Branch;
	tSList ** Codes;
	int i, j;
	EIT_RETURNCODE rc;
	int IntersectionIndex;

	if (HTree->NumberDecompositions == 0) return EIE_SUCCEED;
	Codes = STC_AllocateMemory (HTree->Decomposition[0]->NumberEntries * sizeof *Codes);
	if (Codes == NULL) return EIE_FAIL;
	for (i = 0; i < HTree->Decomposition[0]->NumberEntries; i++) {
		Branch = HTree->Decomposition[0]->Branch[i];
		SList_New (&Codes[i]);
		if (Codes[i] == NULL) return EIE_FAIL;
		rc = HTreeGetCodes (Branch, Codes[i]);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}

	for (i = 0; i < HTree->Decomposition[0]->NumberEntries-1; i++) {
		for (j = i+1; j < HTree->Decomposition[0]->NumberEntries; j++) {
			IntersectionIndex = SList_Intersect (Codes[i], Codes[j]);
			if (IntersectionIndex != -1) {
				EI_AddMessage ("", EIE_MESSAGESEVERITY_ERROR, M30116 "\n", SList_Entry (Codes[i], IntersectionIndex));
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n" M30117 "\n");//First branch
				SList_Print (Codes[i]);
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n" M30118 "\n");//Second branch
				SList_Print (Codes[j]);
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
				for (i = 0; i < HTree->Decomposition[0]->NumberEntries; i++) {
					SList_Free (Codes[i]);
				}
				STC_FreeMemory (Codes);
				return EIE_FAIL;
			}
		}
	}

	for (i = 0; i < HTree->Decomposition[0]->NumberEntries; i++) {
		SList_Free (Codes[i]);
	}
	STC_FreeMemory (Codes);

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Add a branch to a tree. Called when it's no the first time a code in found in the hierarchy.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE HTreeAddBranch (
	STCT_HTREE * HTree,
	STCT_HTREE * Branch)
{
	int DecompositionIndex;
	void * Ptr;

	DecompositionIndex = HTree->NumberDecompositions-1;

	if (HTree->Decomposition[DecompositionIndex]->NumberAllocated == HTree->Decomposition[DecompositionIndex]->NumberEntries) {
		HTree->Decomposition[DecompositionIndex]->NumberAllocated += CHILD_ALLOCATION_SIZE;
		Ptr = STC_ReallocateMemory (
			HTree->Decomposition[DecompositionIndex]->NumberEntries * sizeof *HTree->Decomposition[DecompositionIndex]->Branch,
			HTree->Decomposition[DecompositionIndex]->NumberAllocated * sizeof *HTree->Decomposition[DecompositionIndex]->Branch,
			HTree->Decomposition[DecompositionIndex]->Branch);
		if (Ptr == NULL) return EIE_FAIL;
		HTree->Decomposition[DecompositionIndex]->Branch = Ptr;
	}
	HTree->Decomposition[DecompositionIndex]->Branch[HTree->Decomposition[DecompositionIndex]->NumberEntries++] = Branch;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Add a code to a tree. Called the first time a code in found in the hierarchy.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE HTreeAddCode (
	STCT_HTREE * HTree,
	char * Code)
{
	STCT_HTREE * Branch;

	Branch = HTreeAllocate (Code);
	if (Branch == NULL) return EIE_FAIL;

	return HTreeAddBranch (HTree, Branch);
}
/*------------------------------------------------------------------------------
Allocate the tree structure
------------------------------------------------------------------------------*/
static STCT_HTREE * HTreeAllocate (
	char * Code)
{
	STCT_HTREE * HTree;

	HTree = STC_AllocateMemory (sizeof *HTree);
	if (HTree == NULL) return NULL;
	HTree->Code = STC_StrDup (Code);
	if (HTree->Code == NULL) return NULL;
	HTree->NumberDecompositions = 0;
	HTree->SmallestTag = NULL;
	HTree->LargestTag = NULL;
	HTree->Decomposition = NULL;
	HTree->Tag = TAG_NOT_SET;
	HTree->HasData = STCE_HASDATA_TYPE_NO;
	return HTree;
}
/*------------------------------------------------------------------------------
Find the largest tag of the tree
------------------------------------------------------------------------------*/
static int HTreeFindLargestTag (
	STCT_HTREE * HTree,
	int Decomposition)
{
	int i;
	int LargestTag;

	if (HTree->NumberDecompositions == 0) return HTree->Tag;

	LargestTag = -INT_MAX;
	for (i = 0; i < HTree->Decomposition[Decomposition]->NumberEntries; i++) {
		HTreeFindLargestTag1 (HTree->Decomposition[Decomposition]->Branch[i], &LargestTag);
	}

	return LargestTag;
}
/*------------------------------------------------------------------------------
Find the largest tag of the tree
------------------------------------------------------------------------------*/
static void HTreeFindLargestTag1 (
	STCT_HTREE * HTree,
	int * LargestTag)
{
	int i;

	if (HTree->NumberDecompositions == 0) {
		if (HTree->Tag > *LargestTag)
			*LargestTag = HTree->Tag;
	}
	else {
		for (i = 0; i < HTree->Decomposition[0]->NumberEntries; i++) {
			HTreeFindLargestTag1 (HTree->Decomposition[0]->Branch[i], LargestTag);
		}
	}
}
/*------------------------------------------------------------------------------
Find the smallest tag of the tree
------------------------------------------------------------------------------*/
static int HTreeFindSmallestTag (
	STCT_HTREE * HTree,
	int Decomposition)
{
	int i;
	int SmallestTag;

	if (HTree->NumberDecompositions == 0) return HTree->Tag;

	SmallestTag = INT_MAX;
	for (i = 0; i < HTree->Decomposition[Decomposition]->NumberEntries; i++) {
		HTreeFindSmallestTag1 (HTree->Decomposition[Decomposition]->Branch[i], &SmallestTag);
	}

	return SmallestTag;
}
/*------------------------------------------------------------------------------
Find the smallest tag of the tree
------------------------------------------------------------------------------*/
static void HTreeFindSmallestTag1 (
	STCT_HTREE * HTree,
	int * SmallestTag)
{
	int i;
	//{
	//static c = 0;
	//EI_AddMessage ("", 4, "HTreeFindSmallestTag appele %d fois\n", ++c);
	//}
	if (HTree->NumberDecompositions == 0) {
		if (HTree->Tag < *SmallestTag)
			*SmallestTag = HTree->Tag;
	}
	else {
		for (i = 0; i < HTree->Decomposition[0]->NumberEntries; i++) {//c'est vraiment 0 que je veux ici
			HTreeFindSmallestTag1 (HTree->Decomposition[0]->Branch[i], SmallestTag);
		}
	}
}
/*------------------------------------------------------------------------------
Find the largest tag of the tree
----------------------------------------------------------------------------*/
static int HTreeFindLargestTagAllDecompositions (
	STCT_HTREE * HTree,
	int * LargestTag)
{
	int i, j;

	if (HTree->NumberDecompositions == 0)
		if (HTree->Tag > *LargestTag)
			*LargestTag = HTree->Tag;

	for (i = 0; i < HTree->NumberDecompositions; i++) {
		for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
			HTreeFindLargestTagAllDecompositions (
				HTree->Decomposition[i]->Branch[j], LargestTag);
		}
	}

	return *LargestTag;
}
/*------------------------------------------------------------------------------
Get all the codes of the tree
------------------------------------------------------------------------------*/
static EIT_RETURNCODE HTreeGetCodes (
	STCT_HTREE * HTree,
	tSList * Codes)
{
	int i, j;
	EIT_RETURNCODE rc;

	if (SList_AddNoDup (HTree->Code, Codes) == eSListFail)
		return EIE_FAIL;

	for (i = 0; i < HTree->NumberDecompositions; i++) {
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "decomposition %d de %d\n", i, HTree->Code);
		for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
			rc = HTreeGetCodes (HTree->Decomposition[i]->Branch[j], Codes);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Check if the hierarchy has multiple decompositions
------------------------------------------------------------------------------*/
static EIT_BOOLEAN HTreeHasMultipleDecompositions (
	STCT_HTREE * HTree)
{
	int i;

	if (HTree->NumberDecompositions > 1) return EIE_TRUE;

	if (HTree->NumberDecompositions == 1) {
		for (i = 0; i < HTree->Decomposition[0]->NumberEntries; i++) {
			if (HTreeHasMultipleDecompositions (HTree->Decomposition[0]->Branch[i]))
				return EIE_TRUE;
		}
	}
	return EIE_FALSE;
}
/*------------------------------------------------------------------------------
Create an index for the tree.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE HTreeIndexCreate (
	STCT_HTREE * HTree,
	int LargestTag,
	STCT_HTREE *** HTreeIndex)
{
	int i;
	STCT_HTREE ** lHTreeIndex;
	int n;

	n = LargestTag + 1;//add 1 because it starts at zero
	lHTreeIndex = STC_AllocateMemory (n * sizeof *lHTreeIndex);
	if (lHTreeIndex == NULL) return EIE_FAIL;
	for (i = 0; i < n; i++) {
		lHTreeIndex[i] = HTreeSearchTag (HTree, i);//even if NULL (NULL is not found)
	}

	*HTreeIndex = lHTreeIndex;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
print all HTree in HTreeList. debug function.
------------------------------------------------------------------------------*/
static void HTreeListPrint (void)
{
	int i;
	EI_AddMessage ("", 4, "\nprinting HTreeList\n");
	for (i = 0; i < mContext.HTreeListNumberEntries; i++) {
		EI_AddMessage ("", 4, "\nprinting HTreeList i=%d\n", i);
		HTreePrint (mContext.HTreeList[i], 0);
	}
	EI_PrintMessages ();
}
/*------------------------------------------------------------------------------
Add a dimension to the STCT_HTREEROOT structure
------------------------------------------------------------------------------*/
static EIT_RETURNCODE HTreeListReallocate (void)
{
#define HTREELIST_ALLOCATION_SIZE 20
	void * Ptr;
	if (mContext.HTreeListNumberAllocated == mContext.HTreeListNumberEntries) {
		mContext.HTreeListNumberAllocated += HTREELIST_ALLOCATION_SIZE;
		Ptr = STC_ReallocateMemory (
			mContext.HTreeListNumberEntries * sizeof *mContext.HTreeList,
			mContext.HTreeListNumberAllocated * sizeof *mContext.HTreeList,
			mContext.HTreeList);
		if (Ptr == NULL) return EIE_FAIL;
		mContext.HTreeList = Ptr;
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
print htree. copy of function in STC_Hierarchy.c
------------------------------------------------------------------------------*/
static void HTreePrint (
	STCT_HTREE * HTree,
	int Level)
{
	int i, j;

	if (HTree == NULL)
		return;

	if (HTree->NumberDecompositions > 0)
		for (i = 0; i < HTree->NumberDecompositions; i++) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%*sCode %s - Decomposition [%d/%d]\n",
				Level*4, " ", HTree->Code, i+1, HTree->NumberDecompositions);
			for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
				HTreePrint (HTree->Decomposition[i]->Branch[j], Level+1);
			}
		}
	else {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%*sCode %s - Decomposition [%d/%d]\n",
			Level*4, " ", HTree->Code, 0, HTree->NumberDecompositions);
	}
}
/*------------------------------------------------------------------------------
Add a dimension to the STCT_HTREEROOT structure
------------------------------------------------------------------------------*/
static EIT_RETURNCODE HTreeRootAddDimension (
	STCT_HTREEROOT * HTreeRoot)
{
	void * Ptr;
	if (HTreeRoot->NumberAllocated == HTreeRoot->NumberEntries) {
		HTreeRoot->NumberAllocated += DIMENSION_ALLOCATION_SIZE;
		Ptr = STC_ReallocateMemory (
			HTreeRoot->NumberEntries * sizeof *HTreeRoot->Dimension,
			HTreeRoot->NumberAllocated * sizeof *HTreeRoot->Dimension,
			HTreeRoot->Dimension);
		if (Ptr == NULL) return EIE_FAIL;
		HTreeRoot->Dimension = Ptr;
		Ptr = STC_ReallocateMemory (
			HTreeRoot->NumberEntries * sizeof *HTreeRoot->Name,
			HTreeRoot->NumberAllocated * sizeof *HTreeRoot->Name,
			HTreeRoot->Name);
		if (Ptr == NULL) return EIE_FAIL;
		HTreeRoot->Name = Ptr;
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Allocate the STCT_HTREEROOT structure
------------------------------------------------------------------------------*/
static STCT_HTREEROOT * HTreeRootAllocate (void)
{
	STCT_HTREEROOT * HTreeRoot;

	HTreeRoot = STC_AllocateMemory (sizeof *HTreeRoot);
	if (HTreeRoot == NULL) return NULL;
	HTreeRoot->NumberAllocated = 0;
	HTreeRoot->NumberEntries = 0;
	HTreeRoot->HasMultipleDecompositions = EIE_FALSE;
	HTreeRoot->Dimension = NULL;
	HTreeRoot->TagIndex = NULL;
	HTreeRoot->LargestTag = NULL;
	HTreeRoot->Name = NULL;
	return HTreeRoot;
}
/*------------------------------------------------------------------------------
Check if the hierarchy has multiple decompositions
------------------------------------------------------------------------------*/
static EIT_BOOLEAN HTreeRootHasMultipleDecompositions (
	STCT_HTREEROOT * HTreeRoot)
{
	int i;
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		if (HTreeHasMultipleDecompositions (HTreeRoot->Dimension[i]))
			return EIE_TRUE;
	}
	return EIE_FALSE;
}
/*------------------------------------------------------------------------------
Create an index for the STCT_HTREEROOT structure
------------------------------------------------------------------------------*/
static EIT_RETURNCODE HTreeRootIndexCreate (
	STCT_HTREEROOT * HTreeRoot)
{
	int i;
	EIT_RETURNCODE rc;
	HTreeRoot->TagIndex = STC_AllocateMemory (HTreeRoot->NumberEntries * sizeof *HTreeRoot->TagIndex);
	if (HTreeRoot->TagIndex == NULL) return EIE_FAIL;
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		rc = HTreeIndexCreate (HTreeRoot->Dimension[i], HTreeRoot->LargestTag[i], &HTreeRoot->TagIndex[i]);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Print the STCT_HTREEROOT structure
debug function
------------------------------------------------------------------------------*/
static void HTreeRootIndexPrint (
	STCT_HTREEROOT * HTreeRoot)
{
	int i, j;
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Hierarchies TagIndex.\n");
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Hierarchies %d.\n", i);
		for (j = 0; j <= HTreeRoot->LargestTag[i]; j++) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%d %d %s\n",
				i, j, HTreeRoot->TagIndex[i][j] ? HTreeRoot->TagIndex[i][j]->Code : "NULL");
		}
	}
	EI_PrintMessages ();
}
/*------------------------------------------------------------------------------
Set the tags for the STCT_HTREEROOT structure
------------------------------------------------------------------------------*/
static void HTreeRootTagNodes (
	STCT_HTREEROOT * HTreeRoot)
{
	int i;
	int Tag;
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		Tag = 0;
		HTreeTagNodes (HTreeRoot->Dimension[i], &Tag);
	}
}
/*------------------------------------------------------------------------------
Validate the STCT_HTREEROOT structure
------------------------------------------------------------------------------*/
static EIT_RETURNCODE HTreeRootValidate (
	STCT_HTREEROOT * HTreeRoot)
{
	int i;
	EIT_RETURNCODE rc;
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		rc = HTreeValidate (HTreeRoot->Dimension[i]);
		if (rc != EIE_SUCCEED) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, M30119 "\n", i+1);//Dimension i is not valid
			return EIE_FAIL;
		}
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Search for a Tag in a HTree.
do not use this function. use the TagIndex in HTreeRoot instead.
this function exist to build the index.
------------------------------------------------------------------------------*/
static STCT_HTREE * HTreeSearchTag (
	STCT_HTREE * HTree,
	int Tag)
{
	int i, j;
	STCT_HTREE * t;

	if (HTree->Tag == Tag) return HTree;
	for (i = 0; i < HTree->NumberDecompositions; i++) {
		for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
			t = HTreeSearchTag (HTree->Decomposition[i]->Branch[j], Tag);
			if (t != NULL) return t;
		}
	}
	return NULL;
}
/*------------------------------------------------------------------------------
Give a tag to every node of the tree
------------------------------------------------------------------------------*/
static void HTreeTagNodes (
	STCT_HTREE * HTree,
	int * Tag)
{
	int i, j;

	HTree->Tag = (*Tag)++;
	for (i = 0; i < HTree->NumberDecompositions; i++) {
		for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
			HTreeTagNodes (HTree->Decomposition[i]->Branch[j], Tag);
		}
	}
}
/*------------------------------------------------------------------------------
Validate a tree
------------------------------------------------------------------------------*/
static EIT_RETURNCODE HTreeValidate (
	STCT_HTREE * HTree)
{
	int i, j;
	int rc;

	//Check identical decompositions like "0 1 2: 0 1 2;"
	rc = CheckIdenticalDecompositions (HTree);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	for (i = 0; i < HTree->NumberDecompositions; i++) {//rene: en faire une fonction
		//check que enfant pas repete
		for (j = 0; j < HTree->Decomposition[i]->NumberEntries-1; j++) {
			if (HTree->Decomposition[i]->Branch[j]->Code == HTree->Decomposition[i]->Branch[j+1]->Code) {
				EI_AddMessage ("", EIE_MESSAGESEVERITY_ERROR, M30120 "\n",//Code has two children codes with the same value
					HTree->Code, HTree->Decomposition[i]->Branch[j]->Code);
				return EIE_FAIL;
			}
		}
		//validate hierarchy at a lower level
		for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
			rc = HTreeValidate (HTree->Decomposition[i]->Branch[j]);//rene: pourquoi ici et pas en bas?
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
	}

	rc = CheckRepeatedCodes (HTree);
	if (rc != EIE_SUCCEED) return EIE_FAIL;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE InstallChild (
	tToken * Token)
{
	char * aCode;
	int aCodePositionInHTreeList;
	tSList * CodeList;
	int First;
	EIT_BOOLEAN Found;
	STCT_HTREE * HTree;
	STCT_HTREE * HTree1;
	int i, j;
	int Increment;
	int Last;
	EIT_RETURNCODE rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "InstallChild\n");
#endif
	SList_New (&CodeList);
	if (CodeList == NULL) return EIE_FAIL;
	if (mContext.Increment[0] == '\0') {
		if (SList_Add (Token->Code, CodeList) == eSListFail)
			return EIE_FAIL;
		Increment = -1;
	}
	else {
		if (UTIL_IsNumeric (Token->Code, &Last) == EIE_FALSE) {
			sprintf (mContext.Message, M30153 "\n", Token->Code);
			ReportError (mContext.Message);
			return EIE_FAIL;
		}
		if (UTIL_IsNumeric (mContext.First, &First) == EIE_FALSE) {
			sprintf (mContext.Message, M30154 "\n", mContext.First);
			ReportError (mContext.Message);
			return EIE_FAIL;
		}
		if (UTIL_IsNumeric (mContext.Increment, &Increment) == EIE_FALSE) {
			sprintf (mContext.Message, M30155 "\n", mContext.Increment);
			ReportError (mContext.Message);
			return EIE_FAIL;
		}
		if (Increment == 0) {
			ReportError (M30156);
			return EIE_FAIL;
		}
		if (First > Last) {
			sprintf (mContext.Message, M30157 "\n", Last, First);
			ReportError (mContext.Message);
			return EIE_FAIL;
		}
		for (i = First+Increment; i <= Last; i += Increment) {
			char Code[STCM_CODE_MAX_LENGTH+1];
			sprintf (Code, "%d", i);
			if (SList_Add (Code, CodeList) == eSListFail)
				return EIE_FAIL;
		}
	}
	for (i = 0; i < SList_NumEntries (CodeList); i++) {
		aCode = SList_Entry (CodeList, i);
#ifdef _DEBUG
		if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "search code '%s'\n", aCode);
#endif
		//find aCode in HTreeList
		HTree = NULL;
		for (j = 0; j < mContext.HTreeListNumberEntries && HTree == NULL; j++) {
			HTree = STC_HTreeSearchCode (mContext.HTreeList[j], aCode);
		}
		aCodePositionInHTreeList = j-1;//KEEP this for later, we might need it

		if (HTree == NULL) {
#ifdef _DEBUG
			if (DEBUG) {
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "not found\n");
				EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " add a  new      child '%s' to current code '%s' at intervalle %d\n", aCode, mContext.CurrentBranch->Code, Increment);
			}
#endif
			rc = HTreeAddCode (mContext.CurrentBranch, aCode);
			if (rc != EIE_SUCCEED) return EIE_FAIL;
		}
		else {
#ifdef _DEBUG
			if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "found\n");
#endif
			//l'enfant est il le parent?
			if (strcmp (aCode, mContext.CurrentBranch->Code) == 0) {
				sprintf (mContext.Message, M30158 "\n",
					aCode, mContext.CurrentBranch->Code);
				ReportError (mContext.Message);
				return EIE_FAIL;
			}
			//l'enfant est il un ancetre du parent?
			HTree1 = STC_HTreeSearchCode (HTree, mContext.CurrentBranch->Code); //look for parent code in child tree
			if (HTree1 != NULL) {
				sprintf (mContext.Message, M30159 "\n",
					aCode, mContext.CurrentBranch->Code);
				ReportError (mContext.Message);
				return EIE_FAIL;
			}
#ifdef _DEBUG
			if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " add an existing child '%s' to current code '%s' at intervalle %d\n", aCode, mContext.CurrentBranch->Code, Increment);
#endif
			rc = HTreeAddBranch (mContext.CurrentBranch, HTree);//add a reference to aCode node in the current branch
			if (rc != EIE_SUCCEED) return EIE_FAIL;

			//we just added a reference to the aCode in a branch
			//if aCode is in the root of one of HTreeList item,
			//then this item is not needed anymore.
			Found = EIE_FALSE;
			for (j = 0; j < mContext.HTreeListNumberEntries && !Found; j++) {
				if (strcmp (aCode, mContext.HTreeList[j]->Code) == 0)
					Found = EIE_TRUE;
			}
			if (Found) {
				//aCode is in the root of one of HTreeList item 
				if (aCodePositionInHTreeList+1 != mContext.HTreeListNumberEntries) {
					//aCode is NOT in the last entry of HTreeList
					//move last to aCode position
					mContext.HTreeList[aCodePositionInHTreeList] = mContext.HTreeList[mContext.HTreeListNumberEntries-1];
				}
				mContext.HTreeListNumberEntries--;
			}
		}
	}
	strcpy (mContext.First, Token->Code);
	mContext.Increment[0] = '\0';
	SList_Free (CodeList);

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE InstallParent (
	tToken * Token)
{
	STCT_HTREE * HTree;
	int i;
	void * Ptr;
	EIT_RETURNCODE rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "InstallParent");
#endif
	if (mContext.Status == eNewDimension) {
#ifdef _DEBUG
		if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "add a new dimension, code '%s'\n", Token->Code);
#endif
		//mContext.CurrentBranch = 
		rc = HTreeRootAddDimension (mContext.HTreeRoot);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
		mContext.HTreeRoot->NumberEntries++;
		mContext.Status = eNewDecomposition;
		
		mContext.HTreeListNumberEntries = 0;
		rc = HTreeListReallocate ();
		if (rc != EIE_SUCCEED) return EIE_FAIL;
		mContext.HTreeList[mContext.HTreeListNumberEntries] = HTreeAllocate (Token->Code);
		mContext.CurrentBranch = mContext.HTreeList[mContext.HTreeListNumberEntries];
		mContext.HTreeListNumberEntries++;
	}
	else if (mContext.Status == eNewDecomposition) {
#ifdef _DEBUG
		if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "add a decomposition for code '%s'\n", Token->Code);
#endif
		//mContext.CurrentBranch = STC_HTreeSearchCode (mContext.HTreeRoot->Dimension[mContext.DimensionIndex], Token->Code);
		mContext.CurrentBranch = NULL;
		for (i = 0; i < mContext.HTreeListNumberEntries && mContext.CurrentBranch == NULL; i++) {
			mContext.CurrentBranch = STC_HTreeSearchCode (mContext.HTreeList[i], Token->Code);
		}
		if (mContext.CurrentBranch == NULL) {
			rc = HTreeListReallocate ();
			if (rc != EIE_SUCCEED) return EIE_FAIL;
			mContext.HTreeList[mContext.HTreeListNumberEntries] = HTreeAllocate (Token->Code);
			mContext.CurrentBranch = mContext.HTreeList[mContext.HTreeListNumberEntries];
			mContext.HTreeListNumberEntries++;
		}
	}
	HTree = mContext.CurrentBranch;

	Ptr = STC_ReallocateMemory (
		HTree->NumberDecompositions * sizeof *HTree->SmallestTag,
		(HTree->NumberDecompositions+1) * sizeof *HTree->SmallestTag,
		HTree->SmallestTag);
	if (Ptr == NULL) return EIE_FAIL;
	HTree->SmallestTag = Ptr;
	HTree->SmallestTag[HTree->NumberDecompositions] = -1;
	Ptr = STC_ReallocateMemory (
		HTree->NumberDecompositions * sizeof *HTree->LargestTag,
		(HTree->NumberDecompositions+1) * sizeof *HTree->LargestTag,
		HTree->LargestTag);
	if (Ptr == NULL) return EIE_FAIL;
	HTree->LargestTag = Ptr;
	HTree->LargestTag[HTree->NumberDecompositions] = -1;
	Ptr = STC_ReallocateMemory (
		HTree->NumberDecompositions * sizeof *HTree->Decomposition,
		(HTree->NumberDecompositions+1) * sizeof *HTree->Decomposition,
		HTree->Decomposition);
	if (Ptr == NULL) return EIE_FAIL;
	HTree->Decomposition = Ptr;
	HTree->Decomposition[HTree->NumberDecompositions] = STC_AllocateMemory (
		sizeof *HTree->Decomposition[HTree->NumberDecompositions]);
	if (HTree->Decomposition[HTree->NumberDecompositions] == NULL) return EIE_FAIL;
	HTree->Decomposition[HTree->NumberDecompositions]->Branch = NULL;
	HTree->Decomposition[HTree->NumberDecompositions]->NumberAllocated = 0;
	HTree->Decomposition[HTree->NumberDecompositions]->NumberEntries = 0;

	HTree->NumberDecompositions++;

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
static EIT_RETURNCODE MatchHierarchies (
	tToken * Token)
{
	int rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MatchHierarchies\n");
#endif
	rc = MatchHierarchy (Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	while (Token->Id != eTokenIdDone) {
		mContext.DimensionIndex++;
		rc = MatchHierarchy (Token);
		if (rc != EIE_SUCCEED) return EIE_FAIL;
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static EIT_RETURNCODE MatchHierarchy (
	tToken * Token)
{
	int rc;
#ifdef _DEBUG
	if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "MatchHierarchy\n");
#endif
	rc = MatchLevels (Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	rc = Match (eTokenIdSemiColon, Token);
	if (rc != EIE_SUCCEED) return EIE_FAIL;
	rc = TerminateHierarchy ();
	if (rc != EIE_SUCCEED) return EIE_FAIL;

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
	strcpy (mContext.Increment, Token->Code);//InstallIncrement

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
An error was found in the object to parse. Report error to the user.
------------------------------------------------------------------------------*/
static void ReportError (
	char * Message)
{
	if (strlen (mContext.HierarchyString) > (MESSAGE_ALLOCATION_SIZE/2)) {
		//set ErrorCount to MAX because we are modifying the string
		mContext.ErrorCount = PARSER_MAX_ERROR_COUNT;
		mContext.HierarchyString[(MESSAGE_ALLOCATION_SIZE/2)-5] = '\0';
		strcat (mContext.HierarchyString, "..."); 
	}
	UTIL_ReportError (M00045, M00002, Message, mContext.HierarchyString, mContext.HierarchyStringPtr);
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

	rc = UTIL_ConsumeComments (mContext.HierarchyStringPtr, &n, &Message);
	if (rc != EIE_SUCCEED) {
		ReportError (Message);
		return EIE_FAIL;
	}
	p = mContext.HierarchyStringPtr + n;

	n = strspn (p, mCodeFirstCharacterCharacterSet);
	if (n > 0) {
		/* found code */
		n = strspn (p, mCodeCharacterSet);
		if (n > STCM_CODE_MAX_LENGTH) {
			Token->Id = eTokenIdError;
			ReportError (M30151);
			p += n;
			mContext.HierarchyStringPtr = p;
			return EIE_FAIL;
		}

		strncpy (Temp, p, n);
		Temp[n] = '\0';
#ifdef _DEBUG
		if (DEBUG) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Code=%s\n", Temp);
		}
#endif
		p += strlen (Temp);
		Token->Id = eTokenIdCode;
		strcpy (Token->Code, Temp);
	}
	else if (*p == '-' && (n = strspn (p+1, "0123456789")) > 0) {//isdigit (*(p+1))) {
		/* found increment */

		p++;//skip the '-'
		if (n > STCM_CODE_MAX_LENGTH) {
			Token->Id = eTokenIdError;
			ReportError (M30152);
			p += n;
			mContext.HierarchyStringPtr = p;
			return EIE_FAIL;
		}

		strncpy (Temp, p, n);
		Temp[n] = '\0';
#ifdef _DEBUG
		if (DEBUG) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Code=%s\n", Temp);
#endif
		p += strlen (Temp);
		Token->Id = eTokenIdIncrement;
		strcpy (Token->Code, Temp);
	}
	else if (*p == ':') {
		/* found Colon */
		p++;
		Token->Id = eTokenIdColon;
		strcpy (Token->Code, "");
		mContext.Status = eNewDecomposition;
	}
	else if (*p == ';') {
		/* found SemiColon */
		p++;
		Token->Id = eTokenIdSemiColon;
		strcpy (Token->Code, "");
		mContext.Status = eNewDimension;
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
	mContext.HierarchyStringPtr = p;

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
make sure there is only one root to the hierarchy.
------------------------------------------------------------------------------*/
static EIT_RETURNCODE TerminateHierarchy (void)
{
	int i;
	static char RootheadnameStr[1001];

	if (mContext.HTreeListNumberEntries > 1 ) {
		//ReportError (M30166);
		EI_AddMessage (M00045, EIE_MESSAGESEVERITY_ERROR, M30166);

		// Print out multiple root nodes if any
		strcpy(RootheadnameStr, "");
	    for (i = 0; i < mContext.HTreeListNumberEntries; i++) {	
			 if (i > 0) strcat(RootheadnameStr,", ");
			 strcat (RootheadnameStr, mContext.HTreeList[i]->Code);		      		     
	     }
		 EI_AddMessage (M00045, EIE_MESSAGESEVERITY_ERROR, M30167, RootheadnameStr);
		 EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " ");
		 EI_AddMessage (M00045, EIE_MESSAGESEVERITY_ERROR, M30003); /* Too many errors. Stop processing. */
		 EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " ");
	     EI_PrintMessages ();

		return EIE_FAIL;
	}

	mContext.HTreeRoot->Dimension[mContext.DimensionIndex] = mContext.HTreeList[0];

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
