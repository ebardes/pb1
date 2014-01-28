%{
%}

%%
stmt	: stmt single WS
	|
	;

single	: NUMBER '=' NUMBER			{ setOutput($1, $3); }
	| 'm' 'd' NUMBER			{ mouseDown($3); }
	| 'm' 'u' NUMBER			{ mouseUp($3); }
	| 'm' 'x' SIGNED_NUMBER			{ mouseX($3); }
	| 'm' 'y' SIGNED_NUMBER			{ mouseY($3); }
	;

SIGNED_NUMBER
	: '-' NUMBER				{ $$ = -$2; }
	| '+' NUMBER				{ $$ = $2; }
	| NUMBER				{ $$ = $1; }
	;

NUMBER	: DIGIT					{ $$=$1; }
	| NUMBER DIGIT				{ $$=$1*10+$2; }
	;

DIGIT	: '0' { $$=0; }
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

WS	: WS WHITE_SPACE_CHAR
	| WHITE_SPACE_CHAR
	;

WHITE_SPACE_CHAR : '\n' | '\r' | ' ' | 't' ;

%%
