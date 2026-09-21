%let num=4;
data test&num;
INPUT ident$ reg$ cla$ WaiverFlag val proxyvar; 
CARDS;
SA064 R1 C1 0 90 0.5
SA021 R1 C1 1 160 0.5
SA021 R1 C2 1 210 0.5
SA059 R1 C2 0 20 0.5
SA033 R1 C2 1 120 0.5
SA033 R1 C3 0 110 0.5
SA076 R1 C3 0 30 0.5
SA082 R1 C3 1 200 0.5
SA114 R2 C1 1 120 0.5
SA128 R2 C2 0 90 0.5
SA177 R2 C2 0 50 0.5
SA105 R2 C2 0 80 0.4
SA105 R2 C3 0 40 0.4
SA161 R2 C3 0 30 0.4
SA143 R2 C3 0 10 0.4
RUN;
PROC SENSITIVITY DATA=test&num 
OUTCELL=outcell&num 
OUTCONSTRAINT=outconstraint&num 
OUTLARGEST=outlargest&num
HIERARCHY="Tot_Reg R1 R2; Tot_Cla C1 C2 C3;" 
SRULE="pq 0.2"
proxyratio=0.2
proxydiag
AcceptNegative
;
ID ident; 
VAR val;
DIMENSION reg cla; 
PROXYSIZE proxyvar; 
PWAIVER WaiverFlag;
RUN;