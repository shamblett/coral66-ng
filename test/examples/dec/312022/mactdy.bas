10 PRINT " "
20 PRINT "**********"
30 PRINT "***     This program reads a Macro Source file and reformats"
40 PRINT "*** it as  requested and copies it to the output file given."
50 PRINT "***     If  it is for insertion in a Macro Library then rem-"
60 PRINT "*** arks are removed and the file is concatenated as far as"
70 PRINT "*** possible."
80 PRINT "**********"
90 PRINT " "
100 PRINT "Is the output file for library insertion (Y/N) ";
110 LINPUT R$
120 IF R$="Y" THEN 160
130 IF R$="N" THEN 160
140 GO TO 100
150 REM *** Get the names of the input and output files
160 PRINT " "
170 PRINT "What is the input file (Return only to exit)   ";
180 LINPUT A$
190 IF LEN(A$)=0 THEN STOP
200 PRINT " "
210 PRINT "What is the output file                        ";
220 LINPUT B$
230 OPEN A$ FOR INPUT AS FILE #1
240 OPEN B$ FOR OUTPUT AS FILE #2
250 IF END #1 THEN 950
260 LINPUT #1:C$
270 IF LEN(C$)=0 THEN  GO TO 250
280 A=POS(C$,";",1)
290 IF R$="Y" THEN 330
300 IF A=1 THEN 920
310 IF A=0 THEN 330
320 Q$="					"+SEG$(C$,A,132)
330 B=POS(C$,">",1)
340 C$=TRM$(C$)
350 IF A=0 THEN 480
360 C$=SEG$(C$,1,A-1)
370 C$=TRM$(C$)
380 C=LEN(C$)
390 IF C=0 THEN 250
400 D$=SEG$(C$,C,C)
410 IF D$<>" " THEN 440
420 C$=SEG$(C$,1,C-1)
430 GO TO 380
440 IF D$<>"	" THEN 470
450 C$=SEG$(C$,1,C-1)
460 GO TO 380
470 IF B>A THEN C$=C$+">"
480 D$=SEG$(C$,1,1)
490 IF D$=" " THEN 520
500 IF D$="	" THEN 520
510 GO TO 550
520 C$=SEG$(C$,2,132)
530 IF LEN(C$)=0 THEN 250
540 GO TO 480
550 A=POS(C$,".PAGE",1)
560 IF A<>0 THEN 250
570 A=POS(C$,CHR$(12),1)
580 IF A<>0 THEN 250
590 A=POS(C$,".",1)
600 IF A<>1 THEN 780
610 A=POS(C$,".MACRO",1)
620 IF A=0 THEN 660
630 PRINT ";"
640 PRINT #2:";"
650 GO TO 860
660 A=POS(C$,".ENDM",1)
670 IF A<>0 THEN 860
680 A=POS(C$,"..LB..",1)
690 IF A<>0 THEN 860
700 A=POS(C$,".IIF",1)
710 IF A=0 THEN 820
720 N=POS(C$,".NLIST",1)
730 L=POS(C$,".LIST",1)
740 IF L=0 THEN L=N
750 IF L=0 THEN C$="			"+C$
760 IF L<>0 THEN C$="						"+C$
770 GO TO 860
780 A=POS(C$,"IIII",1)
790 IF A=1 THEN 820
800 A=POS(C$,"$",1)
810 IF A<>1 THEN 840
820 C$="	"+C$
830 GO TO 860
840 A=POS(C$,":",1)
850 IF A=0 THEN C$="		"+C$
860 IF LEN(C$)=0 THEN 250
870 IF R$="Y" THEN 920
880 IF LEN(Q$)=0 THEN 920
890 PRINT Q$
900 PRINT #2:Q$
910 Q$=""
920 PRINT C$
930 PRINT #2:C$
940 GO TO 250
950 CLOSE #1
960 CLOSE #2
970 GO TO 160
