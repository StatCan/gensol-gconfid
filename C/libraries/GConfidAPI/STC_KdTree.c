#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EI_Message.h"
#include "STC_Cell.h"
#include "STC_Coordinate.h"
#include "STC_KdTree.h"
#include "STC_Memory.h"

static STCT_KDTREE * KdTreeAllocate (STCT_CELL * Cell);
static STCT_KDTREE * KdTreeInsert (STCT_KDTREE * KdTree, STCT_CELL * Cell, int Level);
static void KdTreePrint (STCT_KDTREE * KdTree, STCT_HTREEROOT * HTreeRoot, int Depth);
static EIT_RETURNCODE KdTreeRangeSearch (STCT_KDTREE * KdTree, STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate, int Level, STCT_CELLSET * CellSet);
#ifdef NEVERTOBEDEFINED
static void KdTreeRangeSearchOriginal (STCT_KDTREE * KdTree, STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate, int Level, STCT_CELLSET * CellSet);
#endif

/*------------------------------------------------------------------------------
Free the KdTree
------------------------------------------------------------------------------*/
void STC_KdTreeFree (
	STCT_KDTREE * KdTree)
{
	if (!KdTree) return;
	if (KdTree->Left) STC_KdTreeFree (KdTree->Left);
	if (KdTree->Right) STC_KdTreeFree (KdTree->Right);
	STC_FreeMemory (KdTree);
}
/*------------------------------------------------------------------------------
Insert a cell in the KdTree
------------------------------------------------------------------------------*/
STCT_KDTREE * STC_KdTreeInsert (
	STCT_KDTREE * KdTree,
	STCT_CELL * Cell)
{
	return KdTreeInsert (KdTree, Cell, 0);
}
/*------------------------------------------------------------------------------
Print the KdTree (high level.)
Use KdTreePrint() to do the actual work
------------------------------------------------------------------------------*/
void STC_KdTreePrint (
	STCT_KDTREE * KdTree,
	STCT_HTREEROOT * HTreeRoot)
{
	KdTreePrint (KdTree, HTreeRoot, 0);
}
/*------------------------------------------------------------------------------
Print the KdTree without indentation
------------------------------------------------------------------------------*/
void STC_KdTreePrintFlat (
	STCT_KDTREE * KdTree,
	STCT_HTREEROOT * HTreeRoot)
{
	if (!KdTree) return;
	STC_CoordinatePrint (KdTree->Cell->Coordinate, HTreeRoot);
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	if (KdTree->Left) STC_KdTreePrintFlat (KdTree->Left, HTreeRoot);
	if (KdTree->Right) STC_KdTreePrintFlat (KdTree->Right, HTreeRoot);
}


/*------------------------------------------------------------------------------
Get all the cells that are between 2 dimensions (high level.)
Use KdTreeRangeSearch() to do the actual work
------------------------------------------------------------------------------*/
EIT_RETURNCODE STC_KdTreeRangeSearch (
	 STCT_KDTREE * KdTree,
	 STCT_COORDINATE * SmallestCoordinate,
	 STCT_COORDINATE * LargestCoordinate,
	 STCT_CELLSET * CellSet)
{
	return KdTreeRangeSearch (KdTree, SmallestCoordinate, LargestCoordinate, 0, CellSet);
}
/*------------------------------------------------------------------------------
Get the cell for a coordinate
------------------------------------------------------------------------------*/
STCT_CELL * STC_KdTreeSearch (
	STCT_KDTREE * KdTree,
	STCT_COORDINATE * Coordinate)
{
	int Level;

	for (Level = 0; KdTree != NULL; Level = (Level+1)%Coordinate->NumberEntries) {
		if (STC_CoordinateEqual (KdTree->Cell->Coordinate, Coordinate) != EIE_FALSE) {
			return KdTree->Cell;
		}
		if (Coordinate->Tag[Level] > KdTree->Cell->Coordinate->Tag[Level])
			KdTree = KdTree->Right;
		else
			KdTree = KdTree->Left;
	}
	return NULL;
}


/*------------------------------------------------------------------------------
Allocate the KdTree
------------------------------------------------------------------------------*/
STCT_KDTREE * KdTreeAllocate (
	STCT_CELL * Cell)
{
	STCT_KDTREE * KdTree;

	KdTree = STC_AllocateMemory (sizeof *KdTree);
	if (KdTree == NULL) {
		return NULL;
	}
	KdTree->Cell = Cell;
	KdTree->Left = NULL;
	KdTree->Right = NULL;
	return KdTree;
}
/*------------------------------------------------------------------------------
Insert a cell in the KdTree
------------------------------------------------------------------------------*/
static STCT_KDTREE * KdTreeInsert (
	STCT_KDTREE * KdTree,
	STCT_CELL * Cell,
	int Level)
{
	if (KdTree == NULL) {
		KdTree = KdTreeAllocate (Cell);
	}
	else {
		if (STC_CoordinateEqual (Cell->Coordinate, KdTree->Cell->Coordinate) == EIE_FALSE) {
			if (Cell->Coordinate->Tag[Level] > KdTree->Cell->Coordinate->Tag[Level]) {
				KdTree->Right = KdTreeInsert (KdTree->Right, Cell, (Level+1)%KdTree->Cell->Coordinate->NumberEntries);
				if (KdTree->Right == NULL)
					return NULL;
			}
			else {
				KdTree->Left = KdTreeInsert (KdTree->Left, Cell, (Level+1)%KdTree->Cell->Coordinate->NumberEntries);
				if (KdTree->Left == NULL)
					return NULL;
			}
		}
	}
	return KdTree;
}
/*------------------------------------------------------------------------------
Print the KdTree
------------------------------------------------------------------------------*/
static void KdTreePrint (
	STCT_KDTREE * KdTree,
	STCT_HTREEROOT * HTreeRoot,
	int Depth)
{
	int i;

	if (!KdTree) return;

	if (Depth == 0)
		EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Root:");
	else {
		for (i = 0; i < Depth; i++)
		   EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, " ");
		if (KdTree->Left == NULL && KdTree->Right == NULL)
		   EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Term:");
		else
		   EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "Node:");
	}
	STC_CoordinatePrint (KdTree->Cell->Coordinate, HTreeRoot);
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
	KdTreePrint (KdTree->Left, HTreeRoot, Depth+1);
	KdTreePrint (KdTree->Right, HTreeRoot, Depth+1);
}
/*------------------------------------------------------------------------------
KdTreeRangeSearchOriginal()
------------------------------------------------------------------------------*/
//static void KdTreeRangeSearchOriginal (
//    STCT_KDTREE * KdTree,
//    STCT_COORDINATE * SmallestCoordinate,
//    STCT_COORDINATE * LargestCoordinate,
//    int Level,
//    STCT_CELLSET * CellSet)
//{
//    int j;
//    if (KdTree == NULL)
//		return;
//    if (SmallestCoordinate->Code[Level] <= KdTree->Cell->Coordinate->Code[Level])
//		KdTreeRangeSearchOriginal (KdTree->Left, SmallestCoordinate, LargestCoordinate, (Level+1)%KdTree->Cell->Coordinate->NumberEntries,
//			CellSet);
//    for (j = 0; j < KdTree->Cell->Coordinate->NumberEntries &&
//			SmallestCoordinate->Code[j] <= KdTree->Cell->Coordinate->Code[j] &&
//			LargestCoordinate->Code[j] >= KdTree->Cell->Coordinate->Code[j]; j++)
//		;
//    if (j == KdTree->Cell->Coordinate->NumberEntries) {
//		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "found (Coordinate) ");
//		//STC_CoordinatePrint (KdTree->Cell->Coordinate, 2);
//		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
//		if (KdTree->Cell->IsInternal)
//			STC_CellSetAddLast (CellSet, KdTree->Cell);
//	}
//    if (LargestCoordinate->Code[Level] > KdTree->Cell->Coordinate->Code[Level])
//		KdTreeRangeSearchOriginal (KdTree->Right, SmallestCoordinate, LargestCoordinate, (Level+1)%KdTree->Cell->Coordinate->NumberEntries,
//			CellSet);
//}
/*------------------------------------------------------------------------------
Get all the cells that are between 2 dimensions
------------------------------------------------------------------------------*/
static EIT_RETURNCODE KdTreeRangeSearch (
	STCT_KDTREE * KdTree,
	STCT_COORDINATE * SmallestCoordinate,
	STCT_COORDINATE * LargestCoordinate,
	int Level,
	STCT_CELLSET * CellSet)
{
	int j;
	EIT_RETURNCODE rc;

	if (!KdTree)
		return EIE_SUCCEED;
	if (SmallestCoordinate->Tag[Level] <= KdTree->Cell->Coordinate->Tag[Level]) {
		rc = KdTreeRangeSearch (KdTree->Left, SmallestCoordinate, LargestCoordinate,
			(Level+1)%KdTree->Cell->Coordinate->NumberEntries, CellSet);
		if (rc != EIE_SUCCEED)
			return EIE_FAIL;
	}
	for (j = 0; j < KdTree->Cell->Coordinate->NumberEntries &&
			SmallestCoordinate->Tag[j] <= KdTree->Cell->Coordinate->Tag[j] &&
			LargestCoordinate->Tag[j] >= KdTree->Cell->Coordinate->Tag[j]; j++)
		;
	if (j == KdTree->Cell->Coordinate->NumberEntries) {
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "found (Coordinate) ");
		//STC_CoordinatePrint (KdTree->Cell->Coordinate, 2);
		//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "\n");
		if (KdTree->Cell->IsInternal) {
			rc = STC_CellSetAddLast (CellSet, KdTree->Cell);
			if (rc != EIE_SUCCEED)
				return EIE_FAIL;
		}
	}
	if (LargestCoordinate->Tag[Level] > KdTree->Cell->Coordinate->Tag[Level]) {
		rc = KdTreeRangeSearch (KdTree->Right, SmallestCoordinate, LargestCoordinate,
			(Level+1)%KdTree->Cell->Coordinate->NumberEntries, CellSet);
		if (rc != EIE_SUCCEED)
			return EIE_FAIL;
	}
	return EIE_SUCCEED;
}
