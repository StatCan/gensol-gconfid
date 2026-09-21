#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EI_Message.h"
#include "STC_Coordinate.h"
#include "STC_Memory.h"
#include "util.h"

#define TAG_NOT_SET (-1)

/*------------------------------------------------------------------------------
Allocate the STCT_COORDINATE structure
------------------------------------------------------------------------------*/
STCT_COORDINATE * STC_CoordinateAllocate (
	int NumberEntries)
{
	STCT_COORDINATE * Coordinate;
	int i;
	Coordinate = STC_AllocateMemory (NumberEntries * sizeof *Coordinate);
	if (Coordinate == NULL) return NULL;
	Coordinate->NumberEntries = NumberEntries;
	Coordinate->Tag = STC_AllocateMemory (
		Coordinate->NumberEntries * sizeof *Coordinate->Tag);
	if (Coordinate->Tag == NULL) return NULL;
	for (i = 0; i < Coordinate->NumberEntries; i++)
		Coordinate->Tag[i] = TAG_NOT_SET;
	return Coordinate;
}
/*------------------------------------------------------------------------------
Copy the STCT_COORDINATE structure
------------------------------------------------------------------------------*/
void STC_CoordinateCopy (
	STCT_COORDINATE * d,
	STCT_COORDINATE * s)
{
	int i;
	for (i = 0; i < s->NumberEntries; i++) {
		d->Tag[i] = s->Tag[i];
	}
}
/*------------------------------------------------------------------------------
Allocate and copy the STCT_COORDINATE structure
------------------------------------------------------------------------------*/
STCT_COORDINATE * STC_CoordinateDuplicate (
	STCT_COORDINATE * Coordinate)
{
	STCT_COORDINATE * lCoordinate;
	lCoordinate = STC_CoordinateAllocate (Coordinate->NumberEntries);
	if (lCoordinate != NULL)
		STC_CoordinateCopy (lCoordinate, Coordinate);
	return lCoordinate;
}
/*------------------------------------------------------------------------------
Compare 2 STCT_COORDINATE structure for equality
------------------------------------------------------------------------------*/
EIT_BOOLEAN STC_CoordinateEqual (
	STCT_COORDINATE * c1,
	STCT_COORDINATE * c2)
{
	int i;
	if (c1->NumberEntries != c2->NumberEntries) return EIE_FALSE;
	for (i = 0; i < c1->NumberEntries; i++)
		if (c1->Tag[i] != c2->Tag[i])
			return EIE_FALSE;
	return EIE_TRUE;
}
/*------------------------------------------------------------------------------
Free the STCT_COORDINATE structure
------------------------------------------------------------------------------*/
void STC_CoordinateFree (
	STCT_COORDINATE * Coordinate)
{
	if (Coordinate != NULL)
		STC_FreeMemory (Coordinate->Tag);
	STC_FreeMemory (Coordinate);
}
/*------------------------------------------------------------------------------
Print the STCT_COORDINATE structure
------------------------------------------------------------------------------*/
void STC_CoordinatePrint (
	STCT_COORDINATE * Coordinate,
	STCT_HTREEROOT * HTreeRoot)
{
	static char CoordinateStr[3001];//rene: dangereux!
	static char Str[1001];//rene: dangereux!
	int i;
	
	if (Coordinate == NULL)  return;

	//Version 1 -- print tags
	//for (i = 0; i < Coordinate->NumberEntries; i++) {
	//	if (i > 0) EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, ",");
	//	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, "%*d", 3, Coordinate->Tag[i]);
	//}
	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, ".");

	//Version 2 -- print tags on 1 line
	//CoordinateStr[0] = '\0';
	//for (i = 0; i < Coordinate->NumberEntries; i++) {
	//	if (i > 0) strcat (CoordinateStr, ",");
	//	sprintf (Str, "%*d", 3, Coordinate->Tag[i]);
	//	strcat (CoordinateStr, Str);
	//}
	//strcat (CoordinateStr, ".");
	//EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, CoordinateStr);

	//Version 3 -- print codes on 1 line
	CoordinateStr[0] = '\0';
	for (i = 0; i < Coordinate->NumberEntries; i++) {
		if (i > 0) strcat (CoordinateStr, ",");
		sprintf (Str, "%*s", 3, HTreeRoot->TagIndex[i][Coordinate->Tag[i]]->Code);
		strcat (CoordinateStr, Str);
	}
	strcat (CoordinateStr, ".");
	EI_AddMessage ("", EIE_MESSAGESEVERITY_EMPTY, CoordinateStr);
}

