%{
#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"

int yylex(void); /* function prototype */

A_exp absyn_root;

void yyerror(char *s)
{
 EM_error(EM_tokPos, "%s", s);
}
%}


%union {
	int pos;
	int ival;
	string sval;
	A_exp exp;
	A_expList explist;
	A_var var;
	A_decList declist;
	A_dec  dec;
	A_efieldList efieldlist;
	A_efield  efield;
	A_namety namety;
	A_nametyList nametylist;
	A_fieldList fieldlist;
	A_field field;
	A_fundecList fundeclist;
	A_fundec fundec;
	A_ty ty;
	}

%token <sval> ID STRING
%token <ival> INT

%token 
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK 
  LBRACE RBRACE DOT 
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE 
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF 
  BREAK NIL
  FUNCTION VAR TYPE 

%type <exp> exp expseq
%type <explist> actuals  nonemptyactuals sequencing  sequencing_exps
%type <var>  lvalue one oneormore
%type <declist> decs decs_nonempty
%type <dec>  decs_nonempty_s vardec dec
%type <efieldlist> rec rec_nonempty 
%type <efield> rec_one
%type <nametylist> tydec
%type <namety>  tydec_one
%type <fieldlist> tyfields tyfields_nonempty
%type <field> tyfield
%type <ty> ty
%type <fundeclist> fundec
%type <fundec> fundec_one

%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE 
%left PLUS MINUS 
%left  TIMES DIVIDE
%nonassoc UMINUS

%start program

%%

program:	exp {absyn_root = $1;};

lvalue:		ID	{$$ = A_SimpleVar(EM_tokPos, S_Symbol($1));}
			|lvalue DOT ID	{$$ = A_FieldVar(EM_tokPos, $1, S_Symbol($3));}
			|lvalue LBRACK exp RBRACK {$$ = A_SubscriptVar(EM_tokPos, $1, $3);}
			|ID LBRACK exp RBRACK {$$ = A_SubscriptVar(EM_tokPos, A_SimpleVar(EM_tokPos,S_Symbol($1)), $3);}
			;

exp:	lvalue	{$$ = A_VarExp(EM_tokPos, $1);}
		|LPAREN sequencing RPAREN	{$$ = A_SeqExp(EM_tokPos, $2);}
		|NIL	{$$ = A_NilExp(EM_tokPos);}
		|INT	{$$ = A_IntExp(EM_tokPos, $1);}
		|STRING	{$$ = A_StringExp(EM_tokPos, $1);}
		|MINUS exp %prec UMINUS	{$$ = A_OpExp(EM_tokPos, A_minusOp, A_IntExp(EM_tokPos, 0), $2);}
		|ID LPAREN actuals RPAREN	{$$ = A_CallExp(EM_tokPos, S_Symbol($1), $3);}
		|exp PLUS exp	{$$ = A_OpExp(EM_tokPos, A_plusOp, $1, $3);}	   
		|exp MINUS exp	{$$ = A_OpExp(EM_tokPos, A_minusOp, $1, $3);}
		|exp TIMES exp	{$$ = A_OpExp(EM_tokPos, A_timesOp, $1, $3);}
		|exp DIVIDE exp	{$$ = A_OpExp(EM_tokPos, A_divideOp, $1, $3);}
		|exp EQ exp	{$$ = A_OpExp(EM_tokPos, A_eqOp, $1, $3);}	   
		|exp NEQ exp	{$$ = A_OpExp(EM_tokPos, A_neqOp, $1, $3);}
		|exp LT exp	{$$ = A_OpExp(EM_tokPos, A_ltOp, $1, $3);}
		|exp LE exp	{$$ = A_OpExp(EM_tokPos, A_leOp, $1, $3);}
		|exp GT exp	{$$ = A_OpExp(EM_tokPos, A_gtOp, $1, $3);}	   
		|exp GE exp	{$$ = A_OpExp(EM_tokPos, A_geOp, $1, $3);}
		|exp AND exp	{$$ = A_IfExp(EM_tokPos, $1, $3, A_IntExp(EM_tokPos, 0));}
		|exp OR exp		{$$ = A_IfExp(EM_tokPos, $1, A_IntExp(EM_tokPos, 1), $3);}
		|ID LBRACE rec RBRACE	{$$ =  A_RecordExp(EM_tokPos, S_Symbol($1), $3);}	
		|ID LBRACK exp RBRACK OF exp	{$$ = A_ArrayExp(EM_tokPos, S_Symbol($1), $3, $6);} 
		|lvalue ASSIGN exp	{$$ = A_AssignExp(EM_tokPos, $1, $3);}
		|IF exp THEN exp ELSE exp	{$$ = A_IfExp(EM_tokPos, $2, $4, $6);}
		|IF exp THEN exp	{$$ = A_IfExp(EM_tokPos, $2, $4, A_NilExp(EM_tokPos));}
		|WHILE exp DO exp	{$$ = A_WhileExp(EM_tokPos, $2, $4);}
		|FOR ID ASSIGN exp TO exp DO exp	{$$ = A_ForExp(EM_tokPos, S_Symbol($2), $4, $6, $8);}
		|BREAK	{$$ = A_BreakExp(EM_tokPos);}
		|LET decs IN expseq END	{$$ = A_LetExp(EM_tokPos, $2, $4);}
		;

expseq:	sequencing_exps	{$$= A_SeqExp(EM_tokPos, $1);}
		;

sequencing:	exp SEMICOLON sequencing	{$$ = A_ExpList($1, $3);}
			|exp 	{$$ = A_ExpList($1, NULL);}
			|/* empty */	{$$ = NULL;}
			;

sequencing_exps: exp SEMICOLON sequencing_exps	{$$ = A_ExpList($1, $3);}
				|exp	{$$ = A_ExpList($1, NULL);}
				|/* empty */	{$$ = NULL;}	
				;

actuals:	exp COMMA actuals	{$$ = A_ExpList($1, $3);}
			|exp	{$$ = A_ExpList($1, NULL);}
			|/* empty */	{$$ = NULL;}	
			;

vardec:   		VAR ID ASSIGN exp  {$$ = A_VarDec(EM_tokPos,S_Symbol($2),NULL,$4);}
        		|VAR ID COLON ID ASSIGN exp  {$$ = A_VarDec(EM_tokPos,S_Symbol($2),S_Symbol($4),$6);}
              	; 

ty:	ID	{$$ = A_NameTy(EM_tokPos, S_Symbol($1));}
	|LBRACE tyfields RBRACE	{$$ = A_RecordTy(EM_tokPos, $2);}
	|ARRAY OF ID	{$$ = A_ArrayTy(EM_tokPos, S_Symbol($3));}
	;

tyfield:	ID COLON ID	{$$ = A_Field(EM_tokPos, S_Symbol($1), S_Symbol($3));} 
			;

tyfields:	tyfield COMMA tyfields	{$$ = A_FieldList($1, $3);}
			|tyfield	{$$ = A_FieldList($1, NULL);}
			|/* empty */	{$$ = NULL;}
			;

tydec_one:	TYPE ID EQ ty	{$$ = A_Namety(S_Symbol($2), $4);}
			;

tydec:	tydec_one{$$ = A_NametyList($1, NULL);}
		|tydec_one tydec	{$$ = A_NametyList($1, $2);}
		;

fundec_one:	FUNCTION ID LPAREN tyfields RPAREN EQ exp	{$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, NULL, $7);}
			|FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp	{$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, S_Symbol($7), $9);}
			;

fundec: fundec_one	{$$ = A_FundecList($1, NULL);}
		|fundec_one fundec	{$$ = A_FundecList($1, $2);}
		;

dec:	tydec	{$$ = A_TypeDec(EM_tokPos, $1);}
		|vardec	{$$ = $1;}
		|fundec	{$$ = A_FunctionDec(EM_tokPos, $1);}
		;

decs:	dec	{$$ = A_DecList($1, NULL);}
		|dec decs {$$ = A_DecList($1, $2);}
		|/* empty */	{$$ = NULL;}
		;

rec_one:	ID EQ exp	{$$ = A_Efield(S_Symbol($1), $3);}
			;

rec:	rec_one COMMA rec	{$$ = A_EfieldList($1, $3);}
		|rec_one	{$$ = A_EfieldList($1, NULL);}
		|/* empty */	{$$ = NULL;}
		;
