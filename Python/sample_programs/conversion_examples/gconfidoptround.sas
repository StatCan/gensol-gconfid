data Ex_data;
   ID='01'; Industry="1"; Region="A"; Freq=35; output;
   ID='02'; Industry="1"; Region="B"; Freq=3; output;
   ID='03'; Industry="1"; Region="C"; Freq=5; output;
   ID='04'; Industry="2"; Region="A"; Freq=6; output;
   ID='05'; Industry="2"; Region="B"; Freq=3; output;
   ID='06'; Industry="2"; Region="C"; Freq=9; output;
run;

proc sensitivity
 data=Ex_data
 outcell=outcell
 outconstraint=outconstraint
 srule="arb -1"
 hierarchy="TOT_INDUSTRY 1 2;
 TOT_REGION A B C;"
 ;
 id ID;
 var Freq;
 dimension Industry Region;
run;

data outconstraint2;
   set outconstraint;
   where (coefficient eq -1);
   keep constraintid;
run;

data outcell2;
   set outcell;
   weight=1;
   if (cellid eq 1) then CellLB=40;
   rename TotalVar=Total;
   keep cellId TotalVar weight cellLB;
run

%GConfidOptRound(
    InCells=outcell2,
    InConstraints=outconstraint2,
    InCellConstraints=outconstraint,
    OutCells=outround,
    base=5
    );
