
%{
%}

%%

all		: t
		| all WS t
		;

t		: IDENT
		| NUMBER
		;

NUMBER		: DIGIT					{ $$=$1; }
		| NUMBER DIGIT				{ $$=$1*10+$2; }
		;

DIGIT		: '0' { $$=0; }
		| '1' { $$=1; }
		| '2' { $$=2; }
		| '3' { $$=3; }
		| '4' { $$=4; }
		| '5' { $$=5; }
		| '6' { $$=6; }
		| '7' { $$=7; }
		| '8' { $$=8; }
		| '9' { $$=9; }
		;

IDENT		: CHAR
		| IDENT MIXED_CHAR
		;

MIXED_CHAR	: CHAR
		| DIGIT
		;

CHAR		: 'A' | 'B' | 'C' | 'D' | 'E' | 'F' | 'G' | 'H' | 'I' | 'J' | 'K' | 'L' | 'M'
		| 'N' | 'O' | 'P' | 'Q' | 'R' | 'S' | 'T' | 'U' | 'V' | 'W' | 'X' | 'Y' | 'Z'
		| 'a' | 'b' | 'c' | 'd' | 'e' | 'f' | 'g' | 'h' | 'i' | 'j' | 'k' | 'l' | 'm'
		| 'n' | 'o' | 'p' | 'q' | 'r' | 's' | 't' | 'u' | 'v' | 'w' | 'x' | 'y' | 'z'
		;

WS : '\n' | '\r' | ' ' | 't' ;

%%
