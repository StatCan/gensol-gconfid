#ifndef _STC_CELL_H_
#define _STC_CELL_H_

/* #include "EI_Message.h" */
#include "STC_Coordinate.h"
#include "STC_Hierarchy.h"

#include "ilist.h"
#include "slist.h"

#define STC_CELLSTATUS_SENSITIVE    'S'
#define STC_CELLSTATUS_NOTSENSITIVE 'V'
#define STC_CELLSTATUS_PUBLISH      'P'
#define STC_CELLSTATUS_DONOTPUBLISH 'X'
#define STC_CELLSTATUS_NOTSET       'N'

#define STC_CELLTYPE_AGGREGATE  'A'
#define STC_CELLTYPE_CELL       'C'
#define STC_CELLTYPE_GROUP      'G'

#define STCM_MAXLARGESTS 5 //MUST be max (STCM_MAXALPHA, N, NN)

#define FAVCOST_NOT_CALCULATED -2.987e-111

struct STCT_DATAITEM {
	char * Key;
	int NumberObservations;
	double Value;
	double Shadow;
	double Proxy;
	double ValuePt;
	double ValuePw;
	double ValueN;
	double ValueSn;
	double ValueX;
	double ProxyPt;
	double ProxyPw;
	double ProxyN; 
	double ProxySn;
	double ProxyX;
	double FT;                     /* Value + Proxy */
	double FW;                     /* Value + Proxy */
	double FS;                     /* Value + Proxy */
	double Weight;
	double WeightedValue;
	double WeightedShadow;
	double WeightedProxy;
	double AbsVar;                 /* [W]AbsVar is used to calculate output*/
	double WAbsVar;                /* variable WeightedNbRespondents */
	double SecondryValuePt;
	double SecondryValuePw;
	double SecondryProxyPt;
	double SecondryProxyPw;
	double SecondryFT;             /* Value + Proxy */
	double SecondryFW;             /* Value + Proxy */
	double SecondryFS;             /* Value + Proxy */
	double WeightedValueX;
	double WeightedProxyX;
	int    MixedSignStatus;    /*-equal to -1 if "Value" <=0.0 in all  contributing records,                     */
	                           /*           0 if "Value" ==0.0 in all  contributing records,                     */
	                           /*           1 if "Value" >=0.0 in all  contributing records, and                 */
	                           /*           2 if "Value" < 0.0 in some contributing records  and > 0.0 in others */
	int waiver_flag;
};
typedef struct STCT_DATAITEM STCT_DATAITEM;

struct STCT_DATA {
	int AllocationIncrement;
	int NumberAllocated;
	int NumberEntries;
	STCT_DATAITEM ** Item;
};
typedef struct STCT_DATA STCT_DATA;

struct STCT_CELL {
	int CellId;
	STCT_COORDINATE * Coordinate;
	STCT_DATA Data;
	EIT_BOOLEAN IsInternal;
	char Type;
	int TotalNumberObservations;
	int NonAnonymousPositiveNumberEntries;

	double TotalValue;
	double TotalShadow;
	double TotalProxy;
	double TotalValuePt;
	double TotalValuePw;
	double TotalValueN;
	double TotalValueSn;
	double TotalValueX;
	double TotalProxyPt;
	double TotalProxyPw;
	double TotalProxyN;
	double TotalProxySn;
	double TotalProxyX;
	double TotalFT;                /* Value + Proxy */
	double TotalFW;                /* Value + Proxy */
	double TotalFS;                /* Value + Proxy */

	double TotalWeight;

	double TotalWeightedValue;
	double TotalWeightedShadow;
	double TotalWeightedProxy;
	double WeightedNbResp;         /* SUM{WResp(i, c)} calculated in STC_SRuleSensitivity using  DataItemPtr->[W]AbsVar*/
	double TotalSecondryValuePt;
	double TotalSecondryValuePw;
	double TotalSecondryProxyPt;
	double TotalSecondryProxyPw;
	double TotalSecondryFT;        /* Value + Proxy */
	double TotalSecondryFW;        /* Value + Proxy */
	double TotalSecondryFS;        /* Value + Proxy */
	double TotalWeightedValueX;
	double TotalWeightedProxyX;
	int    TotalMixedSignStatus; /*-equal to -1 if "Value" <=0.0 in all  contributing records,                     */
	                             /*           0 if "Value" ==0.0 in all  contributing records,                     */
	                             /*           1 if "Value" >=0.0 in all  contributing records, and                 */
	                             /*           2 if "Value" < 0.0 in some contributing records  and > 0.0 in others */

	STCT_DATAITEM AnonymousDataItem;
	char AnonymousKey[1];

	double Sensitivity;
	double Sensitivity_nowaivers;
	double Sensitivity_noproxy;
	double Sensitivity_noweights;
	char   WhichNoise; /*-allowed values: 'v'=ValueX, 'p'=ProxyX, 'V'=WeightedValueX, 'P'=WeightedProxyX, 'n'=ValueN, 'N'=ProxyN */
	double FavCost;
	int LargestNumberEntries;
	STCT_DATAITEM * Largest  [STCM_MAXLARGESTS]; /* <-- pointers to the n dataitems with the largest Value         values */
	int LargestPYNumberEntries;
	STCT_DATAITEM * LargestPY[STCM_MAXLARGESTS]; /* <-- pointers to the n dataitems with the largest Proxy         values */
	int LargestWVNumberEntries;
	STCT_DATAITEM * LargestWV[STCM_MAXLARGESTS]; /* <-- pointers to the n dataitems with the largest WeightedValue values */
	int LargestWPNumberEntries;
	STCT_DATAITEM * LargestWP[STCM_MAXLARGESTS]; /* <-- pointers to the n dataitems with the largest WeightedProxy values */
	int LargestFTNumberEntries;
	STCT_DATAITEM * LargestFT[STCM_MAXLARGESTS]; /* <-- pointers to the n dataitems with the largest FT            values */
	int LargestFWNumberEntries;
	STCT_DATAITEM * LargestFW[STCM_MAXLARGESTS]; /* <-- pointers to the n dataitems with the largest FW            values */
	int LargestFSNumberEntries;
	STCT_DATAITEM * LargestFS[STCM_MAXLARGESTS]; /* <-- pointers to the n dataitems with the largest FS            values */
	int Largst2FTNumberEntries;
	STCT_DATAITEM * Largst2FT[STCM_MAXLARGESTS]; /* <-- pointers to the n dataitems with the largest SecondryFT    values */
	int Largst2FWNumberEntries;
	STCT_DATAITEM * Largst2FW[STCM_MAXLARGESTS]; /* <-- pointers to the n dataitems with the largest SecondryFW    values */
	int Largst2FSNumberEntries;
	STCT_DATAITEM * Largst2FS[STCM_MAXLARGESTS]; /* <-- pointers to the n dataitems with the largest SecondryFS    values */
};
typedef struct STCT_CELL STCT_CELL;

/*-example:                                                                               */
/*     |SELECT_DI_VAL(Cell->Data.Item[3*i+224], WeightSpecified, ProxySpecified)          */
/* expands to:                                                                            */
/*     |((WeightSpecified)?((ProxySpecified)?(Cell->Data.Item[3*i+224])->WeightedProxyX\  */
/*     |                                    :(Cell->Data.Item[3*i+224])->WeightedValueX)\ */
/*     |                  :((ProxySpecified)?(Cell->Data.Item[3*i+224])->ValueX\          */
/*     |                                    :(Cell->Data.Item[3*i+224])->ValueX)\         */
/*     |)                                                                                 */
/* (all on one line--the newline characters at the end of each line are escaped,          */
/*  so they're dropped                                                                    */
/* )                                                                                      */
#define SELECT_DI_VAL(DataItemPtr, WeightSpecified, ProxySpecified) \
((WeightSpecified)?((ProxySpecified)?(DataItemPtr)->WeightedProxyX\
                                    :(DataItemPtr)->WeightedValueX)\
                  :((ProxySpecified)?(DataItemPtr)->ProxyX\
                                    :(DataItemPtr)->ValueX)\
)

/*-example:                                                                                                                    */
/*     |SELECT_DI_VAL2(Cell->Data.Item[3*i+224], 'p')                                                                          */
/* expands to:                                                                                                                 */
/*     |(('p')=='v'?(Cell->Data.Item[3*i+224])->ValueX\                                                                        */
/*     |:('p')=='p'?(Cell->Data.Item[3*i+224])->ProxyX\                                                                        */
/*     |:('p')=='V'?(Cell->Data.Item[3*i+224])->WeightedValueX\                                                                */
/*     |:('p')=='P'?(Cell->Data.Item[3*i+224])->WeightedProxyX\                                                                */
/*     |:('p')=='n'?(Cell->Data.Item[3*i+224])->ValueN\                                                                        */
/*     |:('p')=='N'?(Cell->Data.Item[3*i+224])->ProxyN\                                                                        */
/*     |:report_error_in_conditional_expression_returning_double("Invalid ValueType '%c' in call to SELECT_DI_VAL2()", ('p'))\ */
/*     |)                                                                                                                      */
/* (all on one line--the newline characters at the end of each line are escaped,                                               */
/*  so they're dropped                                                                                                         */
/* )                                                                                                                           */
#define SELECT_DI_VAL2(DataItemPtr, ValueType) \
((ValueType)=='v'?(DataItemPtr)->ValueX\
:(ValueType)=='p'?(DataItemPtr)->ProxyX\
:(ValueType)=='V'?(DataItemPtr)->WeightedValueX\
:(ValueType)=='P'?(DataItemPtr)->WeightedProxyX\
:(ValueType)=='n'?(DataItemPtr)->ValueN\
:(ValueType)=='N'?(DataItemPtr)->ProxyN\
:report_error_in_conditional_expression_returning_double("Invalid ValueType '%c' in call to SELECT_DI_VAL2()", (ValueType))\
)

/*-example:                                                                                                                    */
/*     |SELECT_CE_VAL2(CellSet[3*i+224], 'p')                                                                                  */
/* expands to:                                                                                                                 */
/*     |(('p')=='v'?(CellSet[3*i+224])->TotalValueX\                                                                           */
/*     |:('p')=='p'?(CellSet[3*i+224])->TotalProxyX\                                                                           */
/*     |:('p')=='V'?(CellSet[3*i+224])->TotalWeightedValueX\                                                                   */
/*     |:('p')=='P'?(CellSet[3*i+224])->TotalWeightedProxyX\                                                                   */
/*     |:('p')=='n'?(CellSet[3*i+224])->TotalValueN\                                                                           */
/*     |:('p')=='N'?(CellSet[3*i+224])->TotalProxyN\                                                                           */
/*     |:report_error_in_conditional_expression_returning_double("Invalid ValueType '%c' in call to SELECT_CE_VAL2()", ('p'))\ */
/*     |)                                                                                                                      */
/* (all on one line--the newline characters at the end of each line are escaped,                                               */
/*  so they're dropped                                                                                                         */
/* )                                                                                                                           */
#define SELECT_CE_VAL2(CellPtr, ValueType) \
((ValueType)=='v'?(CellPtr)->TotalValueX\
:(ValueType)=='p'?(CellPtr)->TotalProxyX\
:(ValueType)=='V'?(CellPtr)->TotalWeightedValueX\
:(ValueType)=='P'?(CellPtr)->TotalWeightedProxyX\
:(ValueType)=='n'?(CellPtr)->TotalValueN\
:(ValueType)=='N'?(CellPtr)->TotalProxyN\
:report_error_in_conditional_expression_returning_double("Invalid ValueType '%c' in call to SELECT_CE_VAL2()", (ValueType))\
)

/*-example (used for internal cells ('CI') only):                                                                              */
/*     |SELECT_CI_VAL2(CellSet[3*i+224], 'p')                                                                                  */
/* expands to:                                                                                                                 */
/*     |(('p')=='v'?(CellSet[3*i+224])->TotalValue\                                                                            */
/*     |:('p')=='p'?(CellSet[3*i+224])->TotalProxy\                                                                            */
/*     |:('p')=='V'?(CellSet[3*i+224])->TotalWeightedValue\                                                                    */
/*     |:('p')=='P'?(CellSet[3*i+224])->TotalWeightedProxy\                                                                    */
/*     |:report_error_in_conditional_expression_returning_double("Invalid ValueType '%c' in call to SELECT_CI_VAL2()", ('p'))\ */
/*     |)                                                                                                                      */
/* (all on one line--the newline characters at the end of each line are escaped,                                               */
/*  so they're dropped                                                                                                         */
/* )                                                                                                                           */
#define SELECT_CI_VAL2(CellPtr, ValueType) \
((ValueType)=='v'?(CellPtr)->TotalValue\
:(ValueType)=='p'?(CellPtr)->TotalProxy\
:(ValueType)=='V'?(CellPtr)->TotalWeightedValue\
:(ValueType)=='P'?(CellPtr)->TotalWeightedProxy\
:report_error_in_conditional_expression_returning_double("Invalid ValueType '%c' in call to SELECT_CI_VAL2()", (ValueType))\
)

/*-for example:                                                                                                                */
/*     |COMPAR_LT_FT_FS(Cell->Data.Item[child_index_1], Cell->Data.Item[parent_index], 'T')                                    */
/* expands to:                                                                                                                 */
/*     |(('T')=='T'?(Cell->Data.Item[child_index_1])->FT < (Cell->Data.Item[parent_index])->FT || \                            */
/*     |((Cell->Data.Item[child_index_1])->FT == (Cell->Data.Item[parent_index])->FT && \                                      */
/*     |strcmp((Cell->Data.Item[child_index_1])->Key, (Cell->Data.Item[parent_index])->Key) < 0)\                              */
/*     |:('T')=='S'?(Cell->Data.Item[child_index_1])->FS < (Cell->Data.Item[parent_index])->FS || \                            */
/*     |((Cell->Data.Item[child_index_1])->FS == (Cell->Data.Item[parent_index])->FS && \                                      */
/*     |strcmp((Cell->Data.Item[child_index_1])->Key, (Cell->Data.Item[parent_index])->Key) < 0)\                              */
/*     |:('T')=='W'?(Cell->Data.Item[child_index_1])->FW < (Cell->Data.Item[parent_index])->FW || \                            */
/*     |((Cell->Data.Item[child_index_1])->FW == (Cell->Data.Item[parent_index])->FW && \                                      */
/*     |strcmp((Cell->Data.Item[child_index_1])->Key, (Cell->Data.Item[parent_index])->Key) < 0)\                              */
/*     |:report_error_in_conditional_expression_returning_int("Invalid ValueType '%c' in call to  COMPAR_LT_FT_FS()", ('T'))\  */
/*     |)                                                                                                                      */
/* (all on one line--the newline characters at the end of each line are escaped,                                               */
/*  so they're dropped                                                                                                         */
/* )                                                                                                                           */
/* (the 1st argument should be an expression returning a pointer to an "STCT_DATAITEM" instance                                */
/*  the 2nd argument should be an expression returning a pointer to an "STCT_DATAITEM" instance                                */
/*  the 3rd argument should be an expression returning a character (char) whose value is either 'T', 'S', or 'W'               */
/* )                                                                                                                           */
/* (if the 3rd argument is 'T' then this returns True (1) if and only if:                                                      */
/*      the "FT"  member of the 1st argument is <  the "FT"  member of the 2nd argument or                                     */
/*     (the "FT"  member of the 1st argument is == the "FT"  member of the 2nd argument and                                    */
/*      the "Key" member of the 1st argument is <  the "Key" member of the 2nd argument);                                      */
/*  if the 3rd argument is 'S' then this returns True (1) if and only if:                                                      */
/*      the "FS"  member of the 1st argument is <  the "FS"  member of the 2nd argument or                                     */
/*     (the "FS"  member of the 1st argument is == the "FS"  member of the 2nd argument and                                    */
/*      the "Key" member of the 1st argument is <  the "Key" member of the 2nd argument);                                      */
/*  if the 3rd argument is 'W' then this returns True (1) if and only if:                                                      */
/*      the "FW"  member of the 1st argument is <  the "FW"  member of the 2nd argument or                                     */
/*     (the "FW"  member of the 1st argument is == the "FW"  member of the 2nd argument and                                    */
/*      the "Key" member of the 1st argument is <  the "Key" member of the 2nd argument)                                       */
/* )                                                                                                                           */
#define COMPAR_LT_FT_FS(DataItemPtr1, DataItemPtr2, WhichF) \
((WhichF)=='T'?(DataItemPtr1)->FT < (DataItemPtr2)->FT || ((DataItemPtr1)->FT == (DataItemPtr2)->FT && strcmp((DataItemPtr1)->Key, (DataItemPtr2)->Key) < 0)\
:(WhichF)=='S'?(DataItemPtr1)->FS < (DataItemPtr2)->FS || ((DataItemPtr1)->FS == (DataItemPtr2)->FS && strcmp((DataItemPtr1)->Key, (DataItemPtr2)->Key) < 0)\
:(WhichF)=='W'?(DataItemPtr1)->FW < (DataItemPtr2)->FW || ((DataItemPtr1)->FW == (DataItemPtr2)->FW && strcmp((DataItemPtr1)->Key, (DataItemPtr2)->Key) < 0)\
:report_error_in_conditional_expression_returning_int("Invalid ValueType '%c' in call to COMPAR_LT_FT_FS()", (WhichF))\
)

/*-for example:                                                                                                                */
/*     |COMPAR_GT_FT_FS(Cell->Data.Item[child_index_1], Cell->Data.Item[parent_index], 'T')                                    */
/*-expands to:                                                                                                                 */
/*     |(('T')=='T'?(Cell->Data.Item[child_index_1])->FT > (Cell->Data.Item[parent_index])->FT || \                            */
/*     |((Cell->Data.Item[child_index_1])->FT == (Cell->Data.Item[parent_index])->FT && \                                      */
/*     |strcmp((Cell->Data.Item[child_index_1])->Key, (Cell->Data.Item[parent_index])->Key) > 0)\                              */
/*     |:('T')=='S'?(Cell->Data.Item[child_index_1])->FS > (Cell->Data.Item[parent_index])->FS || \                            */
/*     |((Cell->Data.Item[child_index_1])->FS == (Cell->Data.Item[parent_index])->FS && \                                      */
/*     |strcmp((Cell->Data.Item[child_index_1])->Key, (Cell->Data.Item[parent_index])->Key) > 0)\                              */
/*     |:('T')=='W'?(Cell->Data.Item[child_index_1])->FW > (Cell->Data.Item[parent_index])->FW || \                            */
/*     |((Cell->Data.Item[child_index_1])->FW == (Cell->Data.Item[parent_index])->FW && \                                      */
/*     |strcmp((Cell->Data.Item[child_index_1])->Key, (Cell->Data.Item[parent_index])->Key) > 0)\                              */
/*     |:report_error_in_conditional_expression_returning_int("Invalid ValueType '%c' in call to COMPAR_GT_FT_FS()", ('T'))\   */
/*     |)                                                                                                                      */
/* (all on one line--the newline characters at the end of each line are escaped,                                               */
/*  so they're dropped                                                                                                         */
/* )                                                                                                                           */
/* (the 1st argument should be an expression returning a pointer to an "STCT_DATAITEM" instance                                */
/*  the 2nd argument should be an expression returning a pointer to an "STCT_DATAITEM" instance                                */
/*  the 3rd argument should be an expression returning a character (char) whose value is either 'T', 'S', or 'W'               */
/* )                                                                                                                           */
/* (if the 3rd argument is 'T' then this returns True (1) if and only if:                                                      */
/*      the "FT"  member of the 1st argument is >  the "FT"  member of the 2nd argument or                                     */
/*     (the "FT"  member of the 1st argument is == the "FT"  member of the 2nd argument and                                    */
/*      the "Key" member of the 1st argument is >  the "Key" member of the 2nd argument);                                      */
/*  if the 3rd argument is 'S' then this returns True (1) if and only if:                                                      */
/*      the "FS"  member of the 1st argument is >  the "FS"  member of the 2nd argument or                                     */
/*     (the "FS"  member of the 1st argument is == the "FS"  member of the 2nd argument and                                    */
/*      the "Key" member of the 1st argument is >  the "Key" member of the 2nd argument);                                      */
/*  if the 3rd argument is 'W' then this returns True (1) if and only if:                                                      */
/*      the "FW"  member of the 1st argument is >  the "FW"  member of the 2nd argument or                                     */
/*     (the "FW"  member of the 1st argument is == the "FW"  member of the 2nd argument and                                    */
/*      the "Key" member of the 1st argument is >  the "Key" member of the 2nd argument)                                       */
/* )                                                                                                                           */
#define COMPAR_GT_FT_FS(DataItemPtr1, DataItemPtr2, WhichF) \
((WhichF)=='T'?(DataItemPtr1)->FT > (DataItemPtr2)->FT || ((DataItemPtr1)->FT == (DataItemPtr2)->FT && strcmp((DataItemPtr1)->Key, (DataItemPtr2)->Key) > 0)\
:(WhichF)=='S'?(DataItemPtr1)->FS > (DataItemPtr2)->FS || ((DataItemPtr1)->FS == (DataItemPtr2)->FS && strcmp((DataItemPtr1)->Key, (DataItemPtr2)->Key) > 0)\
:(WhichF)=='W'?(DataItemPtr1)->FW > (DataItemPtr2)->FW || ((DataItemPtr1)->FW == (DataItemPtr2)->FW && strcmp((DataItemPtr1)->Key, (DataItemPtr2)->Key) > 0)\
:report_error_in_conditional_expression_returning_int("Invalid ValueType '%c' in call to COMPAR_GT_FT_FS()", (WhichF))\
)

/*-("SELMEM" is short for "SELECT_MEMBER") example:                              */
/*     |SELMEM(Cell->LargestFX[i], UseWaivers, FW, FT)                           */
/* expands to:                                                                   */
/*     |((UseWaivers)?((Cell->LargestFX[i])->FW)\                                */
/*     |             :((Cell->LargestFX[i])->FT)\                                */
/*     |)                                                                        */
/* (all on one line--the newline characters at the end of each line are escaped, */
/*  so they're dropped                                                           */
/* )                                                                             */
#define SELMEM(structptr, selector, memberiftrue, memberiffalse) \
((selector)?((structptr)->memberiftrue)\
           :((structptr)->memberiffalse)\
)

/*-for example:                                                                                                                */
/*     |SENSVAR((void *)DataItemPtr, 0, Value)                                                                                 */
/* expands to:                                                                                                                 */
/*     |(0)?((STCT_CELL *)((void *)DataItemPtr))->Total ## Value:((STCT_DATAITEM)((void *)DataItemPtr))->Value                 */
/* which simplifies to:                                                                                                        */
/*     |((STCT_DATAITEM)((void *)DataItemPtr))->Value                                                                          */
/* which simplifies further to:                                                                                                */
/*     |DataItemPtr->Value                                                                                                     */
/* and:                                                                                                                        */
/*     |SENSVAR((void *)CellPtr, 1, Value)                                                                                     */
/* expands to:                                                                                                                 */
/*     |(1)?((STCT_CELL *)((void *)CellPtr))->Total ## Value:((STCT_DATAITEM)((void *)CellPtr))->Value                         */
/* which simplifies to:                                                                                                        */
/*     |((STCT_CELL *)((void *)CellPtr))->Total ## Value                                                                       */
/* which simplifies further to:                                                                                                */
/*     |CellPtr->TotalValue                                                                                                    */
#define SENSVAR(structptr, iscell, member) ((iscell)?((STCT_CELL *)(structptr))->Total ## member:((STCT_DATAITEM *)(structptr))->member)

#define DUMP_DOUBLE(oufile, hexdigits, doublebufname, doublenumber) { \
    doublebufname = (doublenumber); \
    fwrite(((hexdigits)+(((unsigned char*)&doublebufname))[7]), 2, 1, (oufile)); \
    fwrite(((hexdigits)+(((unsigned char*)&doublebufname))[6]), 2, 1, (oufile)); \
    fwrite(((hexdigits)+(((unsigned char*)&doublebufname))[5]), 2, 1, (oufile)); \
    fwrite(((hexdigits)+(((unsigned char*)&doublebufname))[4]), 2, 1, (oufile)); \
    fwrite(((hexdigits)+(((unsigned char*)&doublebufname))[3]), 2, 1, (oufile)); \
    fwrite(((hexdigits)+(((unsigned char*)&doublebufname))[2]), 2, 1, (oufile)); \
    fwrite(((hexdigits)+(((unsigned char*)&doublebufname))[1]), 2, 1, (oufile)); \
    fwrite(((hexdigits)+(((unsigned char*)&doublebufname))[0]), 2, 1, (oufile)); \
}

#define DUMP_STRING(oufile, formatstringbuf, fieldlen, stringval) { \
    sprintf((formatstringbuf), "%%-%ds", (fieldlen)); \
    fprintf((oufile), (formatstringbuf), (stringval)); \
}

#define DUMP_CHAR(oufile, charval) { \
    fprintf((oufile), "%c", (charval)); \
}

struct STCT_CELLSET {
	STCT_CELL ** Cell;
	int AllocationIncrement;
	int NumberAllocated;
	int NumberEntries;
};
typedef struct STCT_CELLSET STCT_CELLSET;

#include "STC_SRule.h"

//operations on Cell
extern EIT_RETURNCODE STC_CellAdjustAllocation (STCT_CELL * Cell);
extern EIT_RETURNCODE STC_CellAdd (STCT_CELL * Cell, const char * Key,
	const double Value, const double Shadow, const double Proxy, const double Weight, int waiver_flags_present, int waiver_flag);
extern STCT_CELL * STC_CellAllocate (const int AllocationSize);
extern EIT_RETURNCODE STC_CellConcat (STCT_CELL * Dest, STCT_CELL * Src);
extern STCT_CELL * STC_CellDuplicate (STCT_CELL * Cell);
extern EIT_RETURNCODE STC_CellFill (STCT_CELL * Cell, int n, int KeyLength,
	int MaxKey, int MaxValue, int PourcentVide);
extern void STC_CellFree (STCT_CELL * Cell);
extern void STC_CellFreeItems (STCT_CELL * Cell);
extern EIT_BOOLEAN STC_CellHasAnonymous (STCT_CELL * Cell);
extern EIT_BOOLEAN STC_CellHasAnonymousOnly (STCT_CELL * Cell);
extern void STC_CellInitCellId (void);
extern EIT_BOOLEAN STC_CellIsSensitive (STCT_CELL * Cell);
extern STCT_CELL * STC_CellMerge (STCT_CELL * Src1, STCT_CELL * Src2);
extern void STC_CellNextCellId (STCT_CELL * Cell);
extern void STC_CellPrint (STCT_CELL * Cell);
extern void STC_CellPrintInfo (STCT_CELL * Cell, int PrintHeader);
extern void STC_CellPrintLargest (STCT_CELL * Cell);
extern EIT_RETURNCODE STC_CellSensitivity (STCT_CELL * Cell, STCT_SRULE * SRule, STCT_COORDINATE * Coordinate);
extern EIT_RETURNCODE FindLargestFX (STCT_CELL * Cell, STCT_DATAITEM ** LargestFX, int * LargestFXNumberEntriesPtr, char WhichF);
extern void RemoveDuplicateKeys (STCT_CELL * Cell);
extern void SortKeys (STCT_CELL * Cell);

//operations on CellSet
extern EIT_RETURNCODE STC_CellSetAddAtIndex (STCT_CELLSET * CellSet,
	STCT_CELL * Cell, int Index);
extern EIT_RETURNCODE STC_CellSetAddLast (STCT_CELLSET * CellSet,
	STCT_CELL * Cell);
extern EIT_RETURNCODE STC_CellSetAdjustAllocation (
	STCT_CELLSET * CellSet);
extern EIT_RETURNCODE STC_CellSetAdjustCellAllocation (
	STCT_CELLSET * CellSet);
extern STCT_CELLSET * STC_CellSetAllocate (int AllocationSize);
extern STCT_CELL * STC_CellSetAsCell (STCT_CELLSET * CellSet);
extern void STC_CellSetFree (STCT_CELLSET * CellSet);
extern int STC_CellSetGetConstraintId (void);
extern void STC_CellSetInitConstraintId (void);
extern int STC_CellSetNextConstraintId (void);
extern void STC_CellSetPrint (STCT_CELLSET * CellSet);
extern void STC_CellSetPrintCellInfo (STCT_CELLSET * CellSet);
extern void STC_CellSetPrintCoordinate (STCT_CELLSET * CellSet,
	STCT_HTREEROOT * HTreeRoot);
extern void STC_CellSetPrintInfo (STCT_CELLSET * CellSet);
extern void STC_CellSetPrintLargest (STCT_CELLSET * CellSet);
extern STCT_CELL * STC_CellSetRemoveLast (STCT_CELLSET * CellSet);
extern STCT_CELL * STC_CellSetRemoveAtIndex (STCT_CELLSET * CellSet,
	int Index);
extern STCT_CELL * STC_CellSetSensitivity (STCT_CELLSET * Cell,
	STCT_SRULE * SRule, STCT_COORDINATE * Coordinate);
extern STCT_CELLSET * STC_CellSetShallowDuplicate (STCT_CELLSET * CellSet);
extern void STC_CellSetShallowFree (STCT_CELLSET * CellSet);

#endif
