#ifndef _STC_HIERARCHY_H_
#define _STC_HIERARCHY_H_

#include "ilist.h"

#ifndef STCM_CODE_MAX_LENGTH
#define STCM_CODE_MAX_LENGTH 100
#endif

enum STCT_HASDATA_TYPE {
	STCE_HASDATA_TYPE_NO,
	STCE_HASDATA_TYPE_YES
};
typedef enum STCT_HASDATA_TYPE STCT_HASDATA_TYPE;

struct STCT_DECOMPOSITION {
	struct STCT_HTREE ** Branch; //an array of pointer to STCT_HTREE
	int NumberEntries;
	int NumberAllocated;
};
typedef struct STCT_DECOMPOSITION STCT_DECOMPOSITION;

struct STCT_HTREE {
	char * Code;
	int Tag;
	int * SmallestTag;
	int * LargestTag;
	//an array of pointer to STCT_DECOMPOSITION
	struct STCT_DECOMPOSITION ** Decomposition;
	//don't need NumberEntries and NumberAllocated
	//because it grows 1 dimension at a time
	int NumberDecompositions;

	STCT_HASDATA_TYPE HasData;
};
typedef struct STCT_HTREE STCT_HTREE;

struct STCT_HTREEROOT {
	STCT_HTREE ** Dimension; //an array of pointer to STCT_HTREE
	STCT_HTREE *** TagIndex; //an array of array of pointer to STCT_HTREE
	int * LargestTag;
	char ** Name;
	EIT_BOOLEAN HasMultipleDecompositions;
	int NumberEntries;
	int NumberAllocated;
};
typedef struct STCT_HTREEROOT STCT_HTREEROOT;

extern void STC_HTreeRootFree (STCT_HTREEROOT * HTreeRoot);
extern void STC_HTreeRootInitHasData (STCT_HTREEROOT * HTreeRoot);
extern void STC_HTreeRootPrint (STCT_HTREEROOT * HTreeRoot);
extern void STC_HTreeRootPrintDebug (STCT_HTREEROOT * HTreeRoot);
extern void STC_HTreeRootSetHasData (STCT_HTREEROOT * HTreeRoot);
extern EIT_RETURNCODE STC_HTreeRootSetName (STCT_HTREEROOT * HTreeRoot, char * Name, int Index);

extern EIT_RETURNCODE STC_HTreeGetNonInternalTags (STCT_HTREE * HTree, tIList * NonInternalTags);
extern EIT_RETURNCODE STC_HTreeGetNonInternalTagsWithData (STCT_HTREE * HTree, tIList * NonInternalTags);
extern EIT_RETURNCODE STC_HTreeGetTags (STCT_HTREE * HTree, tIList * Tags);
extern EIT_RETURNCODE STC_HTreeGetTagsWithData (STCT_HTREE * HTree, tIList * Tags);
extern EIT_RETURNCODE STC_HTreeGetInternalTags (STCT_HTREE * HTree,
	int Decomposition, tIList * InternalTags);
extern EIT_RETURNCODE STC_HTreeParse (char * p, STCT_HTREEROOT * HTreeRoot[],
	char * CodeFirstCharacterCharacterSet, char * CodeCharacterSet);
extern STCT_HTREE * STC_HTreeSearchCode (STCT_HTREE * HTree, char * Code);

#endif
