#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EI_Message.h"
#include "ilist.h"
#include "slist.h"
#include "STC_Hierarchy.h"
#include "STC_Memory.h"

static void HTreeFree (STCT_HTREE * HTree, STCT_HTREE * Root);
static void HTreeInitHasData (STCT_HTREE * HTree);
static void HTreePrint (STCT_HTREE * HTree, int Level);
static void HTreePrintDebug (STCT_HTREE * HTree, int Level);
static STCT_HASDATA_TYPE HTreeSetHasData (STCT_HTREE * HTree);
//static void HTreeSort (STCT_HTREE * HTree);
static void RemoveReferences (STCT_HTREE * HTree, int Tag);
//static void SortChildren (STCT_HTREE ** HTree, int n);

/*------------------------------------------------------------------------------
Free the STCT_HTREEROOT structure
------------------------------------------------------------------------------*/
void STC_HTreeRootFree (
	STCT_HTREEROOT * HTreeRoot)
{
	int i;

	if (HTreeRoot == NULL) return;

	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		HTreeFree (HTreeRoot->Dimension[i], HTreeRoot->Dimension[i]);
		STC_FreeMemory (HTreeRoot->TagIndex[i]);
		STC_FreeMemory (HTreeRoot->Name[i]);
	}
	STC_FreeMemory (HTreeRoot->Dimension);
	STC_FreeMemory (HTreeRoot->TagIndex);
	STC_FreeMemory (HTreeRoot->LargestTag);
	STC_FreeMemory (HTreeRoot->Name);
	HTreeRoot->NumberAllocated = 0;
	HTreeRoot->NumberEntries = 0;
	HTreeRoot->HasMultipleDecompositions = EIE_FALSE;
	HTreeRoot->Dimension = NULL;
	HTreeRoot->TagIndex = NULL;
	HTreeRoot->LargestTag = NULL;
	HTreeRoot->Name = NULL;
	STC_FreeMemory (HTreeRoot);
}
/*------------------------------------------------------------------------------
Set the HasData member of the hierarchy.
------------------------------------------------------------------------------*/
void STC_HTreeRootInitHasData (
	STCT_HTREEROOT * HTreeRoot)
{
	int i;

	if (HTreeRoot == NULL) return;

	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		HTreeInitHasData (HTreeRoot->Dimension[i]);
	}
}
/*------------------------------------------------------------------------------
Set the HasData member of the hierarchy.
------------------------------------------------------------------------------*/
void STC_HTreeRootSetHasData (
	STCT_HTREEROOT * HTreeRoot)
{
	int i;

	if (HTreeRoot == NULL) return;

	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		HTreeSetHasData (HTreeRoot->Dimension[i]);
	}
}
/*------------------------------------------------------------------------------
Set the Name member of the hierarchy.
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_HTreeRootSetName (
	STCT_HTREEROOT * HTreeRoot,
	char * Name,
	int Index)
{
#define NAME_LENGTH 100
	if (HTreeRoot == NULL) return EIE_SUCCEED;
	if (Name == NULL) return EIE_SUCCEED;
	if (Index < 0 || Index >= HTreeRoot->NumberEntries) return EIE_FAIL;

	if (strlen (Name) <= NAME_LENGTH) {
		HTreeRoot->Name[Index] = STC_StrDup (Name);
		if (HTreeRoot->Name[Index] == NULL) return EIE_FAIL;
	}
	else {
		HTreeRoot->Name[Index] = STC_AllocateMemory (NAME_LENGTH + 1);
		if (HTreeRoot->Name[Index] == NULL) return EIE_FAIL;
		strncpy (HTreeRoot->Name[Index], Name, NAME_LENGTH);
		HTreeRoot->Name[Index][NAME_LENGTH] = '\0';
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Print the STCT_HTREEROOT structure
------------------------------------------------------------------------------*/
void STC_HTreeRootPrint (
	STCT_HTREEROOT * HTreeRoot)
{
	int i;
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Dimension %s\n", HTreeRoot->Name[i]);
		HTreePrint (HTreeRoot->Dimension[i], 1);
	}
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
}
/*------------------------------------------------------------------------------
Print the STCT_HTREEROOT structure for debug purposes
------------------------------------------------------------------------------*/
void STC_HTreeRootPrintDebug (
	STCT_HTREEROOT * HTreeRoot)
{
	int i;
	for (i = 0; i < HTreeRoot->NumberEntries; i++) {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Dimension %s LargestTag %d\n",
			HTreeRoot->Name[i], HTreeRoot->LargestTag[i]);
		HTreePrintDebug (HTreeRoot->Dimension[i], 1);
	}
}


/*------------------------------------------------------------------------------
STC_HTreeGetInternalCodes()
------------------------------------------------------------------------------*/
//EIT_RETURNCODE STC_HTreeGetInternalCodes (//rene: pourquoi pas toutes les decompositions?
//	STCT_HTREE * HTree,
//	int Decomposition,
//	tSList * InternalCodes)
//{
//	int i;
//	tSListReturn SLrc;
//	EIT_RETURNCODE rc;
//
//	if (HTree->NumberDecompositions == 0) {
//		//Internal code
//		if (SList_AddNoDup (HTree->Code, InternalCodes) == eSListFail)
//			return EIE_FAIL;
//	}
//	else {
//		//Not an internal code
//		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "decomposition #%d du code %d\n", Decomposition, HTree->Code);
//		for (i = 0; i < HTree->Decomposition[Decomposition]->NumberEntries; i++) {
//			rc = STC_HTreeGetInternalCodes (HTree->Decomposition[Decomposition]->Branch[i],
//				0, // it's really 0 I want here, and not Decomposition.
//				InternalCodes);
//			if (rc != EIE_SUCCEED)
//				return EIE_FAIL;
//		}
//	}
//
//	return EIE_SUCCEED;
//}
/*------------------------------------------------------------------------------
Get all the internal tags of the tree
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_HTreeGetInternalTags (//rene: pourquoi pas toutes les decompositions?
	STCT_HTREE * HTree,
	int Decomposition,
	tIList * InternalTags)
{
	int i;
	EIT_RETURNCODE rc;

	if (HTree->NumberDecompositions == 0) {
		//Internal tag
		if (IList_AddNoDup (HTree->Tag, InternalTags) == eIListFail)
			return EIE_FAIL;
	}
	else {
		//Not an internal tag
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "decomposition #%d du code %d\n", Decomposition, HTree->Tag);
		for (i = 0; i < HTree->Decomposition[Decomposition]->NumberEntries; i++) {
			rc = STC_HTreeGetInternalTags (HTree->Decomposition[Decomposition]->Branch[i],
				0, // it's really 0 I want here, and not Decomposition.
				InternalTags);
			if (rc != EIE_SUCCEED)
				return EIE_FAIL;
		}
	}

	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
STC_HTreeGetNonInternalCodes()
------------------------------------------------------------------------------*/
//EIT_RETURNCODE STC_HTreeGetNonInternalCodes (//rene: pourquoi toutes les decompositions?
//	STCT_HTREE * HTree,
//	tSList * NonInternalCodes)
//{
//	int i, j;
//	EIT_RETURNCODE rc;
//
//	for (i = 0; i < HTree->NumberDecompositions; i++) {
//		if (HTree->Decomposition[i]->NumberEntries != 0) {
//			//Not an internal code
//			if (SList_AddNoDup (HTree->Code, NonInternalCodes) == eSListFail)
//				return EIE_FAIL;
//
//			for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
//				rc = STC_HTreeGetNonInternalCodes (HTree->Decomposition[i]->Branch[j],
//					NonInternalCodes);
//				if (rc != EIE_SUCCEED)
//					return EIE_FAIL;
//			}
//		}
//	}
//	return EIE_SUCCEED;
//}
/*------------------------------------------------------------------------------
Get all the not internal tags of the tree
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_HTreeGetNonInternalTags (//rene: pourquoi toutes les decompositions?
	STCT_HTREE * HTree,
	tIList * NonInternalTags)
{
	int i, j;
	EIT_RETURNCODE rc;

	for (i = 0; i < HTree->NumberDecompositions; i++) {
		if (HTree->Decomposition[i]->NumberEntries != 0) {
			//Not an internal tag
			if (IList_AddNoDup (HTree->Tag, NonInternalTags) == eIListFail)
				return EIE_FAIL;

			for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
				rc = STC_HTreeGetNonInternalTags (HTree->Decomposition[i]->Branch[j],
					NonInternalTags);
				if (rc != EIE_SUCCEED)
					return EIE_FAIL;
			}
		}
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Get all the not internal tags of the tree with data
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_HTreeGetNonInternalTagsWithData (//rene: pourquoi toutes les decompositions?
	STCT_HTREE * HTree,
	tIList * NonInternalTags)
{
	int i, j;
	EIT_RETURNCODE rc;

	if (HTree->HasData == STCE_HASDATA_TYPE_YES) {
		for (i = 0; i < HTree->NumberDecompositions; i++) {
			if (HTree->Decomposition[i]->NumberEntries != 0) {
				//Not an internal tag
				if (IList_AddNoDup (HTree->Tag, NonInternalTags) == eIListFail)
					return EIE_FAIL;

				for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
					rc = STC_HTreeGetNonInternalTagsWithData (HTree->Decomposition[i]->Branch[j],
						NonInternalTags);
					if (rc != EIE_SUCCEED)
						return EIE_FAIL;
				}
			}
		}
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Get all the tags of the tree
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_HTreeGetTags (//rene: pourquoi toutes les decompositions?
	STCT_HTREE * HTree,
	tIList * Tags)
{
	int i, j;
	EIT_RETURNCODE rc;

	if (IList_AddNoDup (HTree->Tag, Tags) == eIListFail)
		return EIE_FAIL;

	for (i = 0; i < HTree->NumberDecompositions; i++) {
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "decomposition %d de %d\n", i, HTree->Tag);
		for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
			rc = STC_HTreeGetTags (HTree->Decomposition[i]->Branch[j], Tags);
			if (rc != EIE_SUCCEED)
				return EIE_FAIL;
		}
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Get all the tags of the tree with data
It's important to visit every decompositions
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_HTreeGetTagsWithData (
	STCT_HTREE * HTree,
	tIList * Tags)
{
	int i, j;
	EIT_RETURNCODE rc;

	if (HTree->HasData == STCE_HASDATA_TYPE_YES) {
		if (IList_AddNoDup (HTree->Tag, Tags) == eIListFail)
			return EIE_FAIL;

		for (i = 0; i < HTree->NumberDecompositions; i++) {
			//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "decomposition %d de %d\n", i, HTree->Tag);
			for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
				rc = STC_HTreeGetTagsWithData (HTree->Decomposition[i]->Branch[j], Tags);
				if (rc != EIE_SUCCEED)
					return EIE_FAIL;
			}
		}
	}
	return EIE_SUCCEED;
}
/*------------------------------------------------------------------------------
Search for a code in the tree. returns NULL if not found.
------------------------------------------------------------------------------*/
STCT_HTREE * STC_HTreeSearchCode (
	STCT_HTREE * HTree,
	char * Code)
{
	int i, j;
	STCT_HTREE * t;

	if (strcmp (HTree->Code, Code) == 0)
		return HTree;
	for (i = 0; i < HTree->NumberDecompositions; i++) {
		for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
			t = STC_HTreeSearchCode (HTree->Decomposition[i]->Branch[j], Code);
			if (t != NULL)
				return t;
		}
	}
	return NULL;
}


/*------------------------------------------------------------------------------
Free the tree.
Be careful, a branch can be multiple times in the hierarchy.
------------------------------------------------------------------------------*/
static void HTreeFree (
	STCT_HTREE * HTree,
	STCT_HTREE * Root)
{
	int i, j;

	if (HTree->Decomposition != NULL) {
		for (i = 0; i < HTree->NumberDecompositions; i++) {
			if (HTree->Decomposition[i] != NULL) {
				for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
					if (HTree->Decomposition[i]->Branch[j] != NULL) { // we must check for NULL, it might have been freed earlier
						HTreeFree (HTree->Decomposition[i]->Branch[j], Root); //free the Branch
						HTree->Decomposition[i]->Branch[j] = NULL;						
					}
				}
				STC_FreeMemory (HTree->Decomposition[i]->Branch);
				HTree->Decomposition[i]->Branch = NULL;
				HTree->Decomposition[i]->NumberAllocated = 0;
				HTree->Decomposition[i]->NumberEntries = 0;
				STC_FreeMemory (HTree->Decomposition[i]);
				HTree->Decomposition[i] = NULL;
			}
		}
		HTree->NumberDecompositions = 0;
		STC_FreeMemory (HTree->SmallestTag);
		STC_FreeMemory (HTree->LargestTag);
		STC_FreeMemory (HTree->Decomposition);
		HTree->SmallestTag = NULL;
		HTree->LargestTag = NULL;
		HTree->Decomposition = NULL;
	}

	RemoveReferences (Root, HTree->Tag); //remove all references to this branch
	STC_FreeMemory (HTree->Code);
	HTree->Code = NULL;
	STC_FreeMemory (HTree);
}
/*------------------------------------------------------------------------------
Set the HasData member of the hierarchy.
------------------------------------------------------------------------------*/
static void HTreeInitHasData (
	STCT_HTREE * HTree)
{
	int i, j;

	if (HTree->NumberDecompositions > 0) {
		for (i = 0; i < HTree->NumberDecompositions; i++) {
			for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
				HTreeInitHasData (HTree->Decomposition[i]->Branch[j]);
			}
		}
	}
	HTree->HasData = STCE_HASDATA_TYPE_NO;
}
/*------------------------------------------------------------------------------
Set the HasData member of the hierarchy.
------------------------------------------------------------------------------*/
static STCT_HASDATA_TYPE HTreeSetHasData (
	STCT_HTREE * HTree)
{
	STCT_HASDATA_TYPE CumulativeHasData;
	STCT_HASDATA_TYPE HasData;
	int i, j;

	if (HTree->NumberDecompositions > 0) {
		CumulativeHasData = STCE_HASDATA_TYPE_NO;
		for (i = 0; i < HTree->NumberDecompositions; i++) {
			for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
				HasData = HTreeSetHasData (HTree->Decomposition[i]->Branch[j]);
				if (HasData == STCE_HASDATA_TYPE_YES)
					CumulativeHasData = STCE_HASDATA_TYPE_YES;
			}
		}
		HTree->HasData = CumulativeHasData;
	}
	else
		CumulativeHasData = HTree->HasData;
	return CumulativeHasData;
}
/*------------------------------------------------------------------------------
Print the tree
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
Print the tree
------------------------------------------------------------------------------*/
static void HTreePrintDebug (
	STCT_HTREE * HTree,
	int Level)
{
	int i, j;

	if (HTree == NULL)
		return;

	if (HTree->NumberDecompositions > 0)
		for (i = 0; i < HTree->NumberDecompositions; i++) {
			EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%*sCode %s (%d) - Decomposition [%d/%d] - Tag [%d/%d] HasData (%d)\n",
				Level*4, " ", HTree->Code, HTree->Tag, i+1, HTree->NumberDecompositions,
				HTree->SmallestTag[i], HTree->LargestTag[i], HTree->HasData);
			for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
				HTreePrintDebug (HTree->Decomposition[i]->Branch[j], Level+1);
			}
		}
	else {
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%*sCode %s (%d) - Decomposition [%d/%d] HasData (%d)\n",
			Level*4, " ", HTree->Code, HTree->Tag, 0, HTree->NumberDecompositions, HTree->HasData);
	}
}
/*------------------------------------------------------------------------------
Sort a tree
------------------------------------------------------------------------------*/
//static void HTreeSort (
//	STCT_HTREE * HTree)
//{
//	int i, j;
//	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Sort: Tag %d\n", HTree->Tag);
//	for (i = 0; i < HTree->NumberDecompositions; i++) {
//		SortChildren (HTree->Decomposition[i]->Branch, HTree->Decomposition[i]->NumberEntries);
//		for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
//			HTreeSort (HTree->Decomposition[i]->Branch[j]);
//		}
//	}
//}
/*------------------------------------------------------------------------------
Sets to NULL any reference to nodes identified with Tag.
This function has to be defencive because it is used by the HTreeFree() function
and any part of the tree can already be freed.
------------------------------------------------------------------------------*/
static void RemoveReferences (
	STCT_HTREE * HTree,
	int Tag)
{
	int i, j;

	if (HTree->Decomposition != NULL) {
		for (i = 0; i < HTree->NumberDecompositions; i++) {
			if (HTree->Decomposition[i] != NULL) {
				for (j = 0; j < HTree->Decomposition[i]->NumberEntries; j++) {
					if (HTree->Decomposition[i]->Branch[j] != NULL) {
						if (HTree->Decomposition[i]->Branch[j]->Tag == Tag) {
							//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "removing a reference to Tag %d\n", Tag);
							HTree->Decomposition[i]->Branch[j] = NULL;
						}
						else {
							RemoveReferences (HTree->Decomposition[i]->Branch[j], Tag);
						}
					}
				}
			}
		}
	}
}
/*------------------------------------------------------------------------------
Sort the children of the tree in ascending tag order.
------------------------------------------------------------------------------*/
//static void SortChildren (
//	STCT_HTREE ** HTree,
//	int n)
//{
//	int i, j;
//	STCT_HTREE * TempHTree;
//	for (i = 0; i < n-1; i++) {
//		for (j = i+1; j < n; j++) {
//			if (HTree[i]->Tag > HTree[j]->Tag) {
//				TempHTree = HTree[i];
//				HTree[i] = HTree[j];
//				HTree[j] = TempHTree;
//			}
//		}
//	}
//}
