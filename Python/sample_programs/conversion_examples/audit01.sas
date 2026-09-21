%let QuestionNumber=3; 

data data; QuestionNumber=1;
   do while (QuestionNumber <= &QuestionNumber); 
      Key='111'; Income=101; Prov='1'; Naics='101';output; Key='211'; Income=101; Prov='2'; Naics='101';output; 
      Key='121'; Income=202; Prov='1'; Naics='202';output; Key='131'; Income=103; Prov='1'; Naics='103';output; 
      Key='132'; Income=203; Prov='1'; Naics='203';output; Key='133'; Income=303; Prov='1'; Naics='303';output;
    QuestionNumber = QuestionNumber + 1;
   end;
run;
 
proc sensitivity 
   data=data
   outconstraint=outconstraint
   outcell=outcell
   outlargest=outlargest
   hierarchy="0 1 2; 0 1 2 3;" 
   srule="pq .15"
   range=";1 101 201 301: 2 102 202 302: 3 103 203 303;"
   ;
id Key;
var Income;
dimension Prov Naics;
by QuestionNumber;
run;

%Suppress(InCell=outcell,
   Constraint=outconstraint,
   OutCell=suppresseddata, 
   CFunction1=DIGITS,
   CFunction2=INFORMATION,
   By=QuestionNumber, 
   ScaleCost=none, 
   DebugInfo=no)

%Audit(InCell=suppresseddata,
   Constraint=outconstraint,
   OutCell=auditoutdata,
   LBFactor=0.5,
   UBFactor=1.5,
   By=QuestionNumber,
   DebugInfo=no,
   ReportLevel=2)
