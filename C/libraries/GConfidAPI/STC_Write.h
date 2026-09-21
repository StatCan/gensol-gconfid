#ifndef _STC_WRITE_H
#define _STC_WRITE_H

#include "STC_Cell.h"

typedef EIT_RETURNCODE (*STCT_WRITECELLOBSCALLBACK) (
    STCT_CELL * Cell,
    int NumberDimensions);
typedef EIT_RETURNCODE (*STCT_WRITECONSTRAINTOBSCALLBACK) (
	int ConstraintId,
	int CellId,
	int Coefficient);
typedef EIT_RETURNCODE (*STCT_WRITELARGESTOBSCALLBACK) (
	int CellId,
	char * Key,
	int NumberObservations,
	double Value,
	double TotalValue,
	double Shadow,
	double TotalShadow,
	int WaiverFlag);
typedef EIT_RETURNCODE (*STCT_WRITETARGETSOBSCALLBACK) (
	int CellId,
	char * Id,
	char * PtnVariable,
	double Value);
typedef EIT_RETURNCODE (*STCT_WRITEPAIRSOBSCALLBACK) (
	int CellId,
	char * TargetId,
	double TargetPt,
	char * AttackerId,
	double AttackerSn,
	int RemainderCount,
	double RemainderN);

extern EIT_RETURNCODE STC_WriteCell (STCT_CELL * Cell, int NumberDimensions);
extern void STC_WriteCellObsSetCB (STCT_WRITECELLOBSCALLBACK);
extern EIT_RETURNCODE STC_WriteConstraint (STCT_CELLSET * CellSet, STCT_CELL * RHSCell);
extern void STC_WriteConstraintObsSetCB (STCT_WRITECONSTRAINTOBSCALLBACK);
extern EIT_RETURNCODE STC_WriteLargest (STCT_CELL * Cell,
	int waiver_flags_present,
	PROGRAM_PARM * Parms);
extern void STC_WriteLargestObsSetCB (STCT_WRITELARGESTOBSCALLBACK);
extern EIT_RETURNCODE STC_WriteTargets (
	STCT_CELL * Cell,
	int             Lrgst12FTNumberEntries,
	STCT_DATAITEM** Lrgst12FT,
	int             Lrgst12FSNumberEntries,
	STCT_DATAITEM** Lrgst12FS,
	int             SRuleNumberEntries01,
	int             UsePrimary,
	int             SRule_waiver_flags_present);
extern void STC_WriteTargetsObsSetCB (STCT_WRITETARGETSOBSCALLBACK);
extern EIT_RETURNCODE STC_WritePairs (
	STCT_CELL * Cell,
	int             Lrgst12FTNumberEntries,
	STCT_DATAITEM** Lrgst12FT,
	int             Lrgst12FSNumberEntries,
	STCT_DATAITEM** Lrgst12FS,
	int             UsePrimary,
	int             SRule_waiver_flags_present);
extern void STC_WritePairsObsSetCB (STCT_WRITEPAIRSOBSCALLBACK);

#endif
