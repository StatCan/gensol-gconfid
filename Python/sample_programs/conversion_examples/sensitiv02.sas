data test2;
INPUT ident$ reg$ cla$ WaiverFlag val; 
CARDS;
SA064 R1 C1 0 90
SA021 R1 C1 1 160
SA021 R1 C2 1 210
SA059 R1 C2 0 20
SA033 R1 C2 1 120
SA033 R1 C3 0 110
SA076 R1 C3 0 30
SA082 R1 C3 1 200
SA114 R2 C1 1 120
SA128 R2 C2 0 90
SA177 R2 C2 0 50
SA105 R2 C2 0 80
SA105 R2 C3 0 40
SA161 R2 C3 0 30
SA143 R2 C3 0 10
RUN;

PROC SENSITIVITY
    DATA=test2
    OUTCELL=outcell
    OUTCONSTRAINT=outconstraint
    OUTLARGEST=outlargest
    HIERARCHY="Tot_Reg R1 R2; Tot_Cla C1 C2 C3;" 
    SRULE="pq 0.2"
    ;
    ID ident; 
    VAR val;
    DIMENSION reg cla; 
    PWAIVER WaiverFlag;
RUN;
