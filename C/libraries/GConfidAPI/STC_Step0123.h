#ifndef _STC_STEP0123_H_
#define _STC_STEP0123_H_

extern EIT_RETURNCODE STC_Step0123 (STCT_CELLSET * Segment, STCT_SRULE * SRule,
	STCT_HTREEROOT * HTreeRoot, int M, double X, double Y, double Z, int N,
	int NN, int Verbose, int NumberDimensions,
	int * NumberCombinaisonsCalculated, int * NumberCombinaisonsSensitive);

#endif
