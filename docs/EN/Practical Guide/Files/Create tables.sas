data Data_sup;
id=0;
do i=1 to 10;
do j=1 to 10;
 do k=1 to 5;
 id=id+1;
 Entid=left(put(id,3.));
 if i=1 then Province="10";
 else if i=2 then Province="11";
 else if i=3 then Province="12";
 else if i=4 then Province="13";
 else if i=5 then Province="24";
 else if i=6 then Province="35";
 else if i=7 then Province="46";
 else if i=8 then Province="47";
 else if i=9 then Province="48";
 else Province="59";
 if j=1 then Industry="A";
 else if j=2 then Industry="B";
 else if j=3 then Industry="C";
 else if j=4 then Industry="D";
 else if j=5 then Industry="E";
 else if j=6 then Industry="F";
 else if j=7 then Industry="G";
 else if j=8 then Industry="H";
 else if j=9 then Industry="I";
 else Industry="J";
 Value=round(((ranuni(128009)+0.25)**3.5)*1000)+1;
 if Industry="C" and Province="59" then Value=Value+200;
 if Industry="H" and Province="24" then Value=Value+100;
 output;
 end;
 end;
end;
drop i j k id;
run;

proc sensitivity
 data=Data_sup
 outcell=outcell
 outconstraint=outconstraint
 srule="pq 0.15"
 hierarchy="TOT_PROVINCE 10 11 12 13
 24 35 46 47 48 59;
 TOT_INDUSTRY A B C D E F
 G H I J;"
 ;
 id Entid;
 var Value;
 dimension Province Industry;
run;


data outcell;
set outcell;
if Industry="J" and
 Province not in ("TOT_PROVINCE","12")
 then cvar_J12=1;
else if Province="12" then cvar_J12=22000;
else cvar_J12=Totalnoise; /*Replace Total with 
TotalNoise as of version 1.07.002.*/
run;

%suppress(
 InCell = outcell,
 Constraint = outconstraint,
 OutCell = outpattern_2A,
 cvar1 = cvar_J12
);

proc import file="\\fld6filer\meth\SDC-CSD\Research\Python Suppression\Gconfid Pratical guide\Files\output.csv"
    out=work.outpython
    dbms=csv;
	informat Province $15. Industry $15. ;
	format Province $15. Industry $15.;
run;



%reportCells(
colname=Industry,
rowname=Province,
incell=mydata
);

proc import datafile="\\fld6filer\meth\SDC-CSD\Research\Python Suppression\Gconfid Pratical guide\Files\outputsizephase2.xlsx"
dbms=xlsx
out=work.mydata
replace;
getnames=yes;
run;


%reportCells(
colname=Industry,
rowname=Province,
incell=mydata
);
