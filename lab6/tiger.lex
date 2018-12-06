%{
/* Lab2 Attention: You are only allowed to add code in this file and start at Line 26.*/
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "y.tab.h"
#include "errormsg.h"

int charPos=1;

int yywrap(void)
{
 charPos=1;
 return 1;
}

void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}

/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/

int comm_cnt = 0;

int isNum(char ch){
  if (ch >= '0' && ch <='9'){
    return 1;
  }
  return 0;
}

char* eatFormat(char* ptr){
  //printf("into eatFormat:%s\n",ptr);
  char* p = ptr+1;
  char ch = *p;
  //printf("char %s\n",p);
  while(ch != '\\'){
    //printf("char %s\n",p);
    p++;
    ch = *p;
  }
  return ++p;
}

/* @function: getstr
 * @input: a string literal
 * @output: the string value for the input which has all the escape sequences 
 * translated into their meaning.
 */
char *getstr(const char *str)
{
	//optional: implement this function if you need it

  char* final = (char*)checked_malloc(yyleng);
  memset(final,0,yyleng);
  char* ptr = (char *)str;
  //printf("init string:%s\n",ptr);
  ptr++;
  char ch = *ptr;
  int i=0;
  while(ch != '\"'){
    //printf("char: %c\n",ch); 

    /* escape sequences */
    if(ch == '\\'){
      if(*(ptr+1) == 'n'){            // \n
        ptr += 2;
        final[i] = '\n';
      }
      else if(*(ptr+1) == 't'){            // \t
        ptr += 2;
        final[i] = '\t';
      }
      else if(isNum(*(ptr+1)) && isNum(*(ptr+2)) && isNum(*(ptr+3))){        // \ddd
        char num[4];
        num[0] = *(ptr+1);
        num[1] = *(ptr+2);
        num[2] = *(ptr+3);
        num[3] = '\0';
        final[i] = atoi(num);
        ptr += 4;
      }
      else if(*(ptr+1) == '\"'){           // \"
        ptr += 2;
        final[i] = '\"';
        
      }
      else if(*(ptr+1) == '^'){ // \^c
        ch = *(ptr+2);
        ptr += 3;
        final[i] = ch - 64;
      } 
      else if(*(ptr+1) == '\\'){           // \\
        //printf("ptr:%s \n",ptr);
        ptr+=2;
        //final[i] = '\\';
        final[i] = ch;
        //ptr++;        
      }
      else {                            // \f------f\
        //ptr += 1;
        ptr = eatFormat(ptr);
        i--;
      }
      /*else{
        final[i] = ch;
        ptr++;
      }*/
    }
    else{
      final[i] = ch;
      ptr++;
    }
    
    //printf("i: %d final:%s\n",i,final);
    i++;
    ch = *ptr;
    if(i == yyleng){
      break;
    }
  }

  final[i] = '\0';

  //if(i == 0){
  //  return "(null)";
  //}
  return final;
}

%}
  /* You can add lex definitions here. */


blank	("\t"|" "|"\r")
letter	[A-Za-z]
digit	[0-9]
EOF  《EOF》

%s COMMENT
%s STR


%%
  /* 
  * Below is an example, which you can wipe out
  * and write reguler expressions and actions of your own.
  */ 

<INITIAL>"\n" {adjust(); EM_newline(); continue;}

 /* Reserve words */
<INITIAL>"array" {adjust(); return ARRAY;}
<INITIAL>"if" {adjust(); return IF;}
<INITIAL>"then" {adjust(); return THEN; }
<INITIAL>"else" {adjust(); return ELSE;}
<INITIAL>"while" {adjust(); return WHILE;}
<INITIAL>"for" {adjust(); return FOR; }
<INITIAL>"to" {adjust(); return TO; }
<INITIAL>"do" {adjust(); return DO;}
<INITIAL>"let" {adjust(); return LET;}
<INITIAL>"in" {adjust(); return IN;}
<INITIAL>"end" {adjust(); return END;}
<INITIAL>"of" {adjust(); return OF;}
<INITIAL>"break" {adjust(); return BREAK;}
<INITIAL>"nil" {adjust(); return NIL;}
<INITIAL>"function" {adjust(); return FUNCTION;}
<INITIAL>"var" {adjust(); return VAR; }
<INITIAL>"type" {adjust(); return TYPE; }

 /* Punctuation symbols */
<INITIAL>"," {adjust(); return COMMA ;}
<INITIAL>":" {adjust(); return COLON ;}
<INITIAL>";" {adjust(); return SEMICOLON ;}
<INITIAL>\( {adjust(); return LPAREN ;}
<INITIAL>\) {adjust(); return RPAREN ;}
<INITIAL>"[" {adjust(); return LBRACK ;}
<INITIAL>"]" {adjust(); return RBRACK ;}
<INITIAL>"{" {adjust(); return LBRACE ;}
<INITIAL>"}" {adjust(); return RBRACE ;}
<INITIAL>"." {adjust(); return DOT ;}
<INITIAL>"+" {adjust(); return PLUS ;}
<INITIAL>"-" {adjust(); return MINUS ;}
<INITIAL>"*" {adjust(); return TIMES ;}
<INITIAL>"/" {adjust(); return DIVIDE ;}
<INITIAL>"=" {adjust(); return EQ ;}
<INITIAL>"<>" {adjust(); return NEQ ;}
<INITIAL>"<" {adjust(); return LT ;}
<INITIAL>"<=" {adjust(); return LE ;}
<INITIAL>">" {adjust(); return GT ;}
<INITIAL>">=" {adjust(); return GE ;}
<INITIAL>"&" {adjust(); return AND ;}
<INITIAL>"|" {adjust(); return OR ;}
<INITIAL>":=" {adjust(); return ASSIGN ;}

 /* Identifier */
<INITIAL>{letter}({letter}|{digit}|_)* {adjust(); yylval.sval=String(yytext); return ID;}

 /* Inter literal */
<INITIAL>{digit}+ {adjust(); yylval.ival=atoi(yytext); return INT;}

 /* Blank */
<INITIAL>{blank}+ {adjust();continue;}

 /* String literal */
<INITIAL>\"(\\\"|[^\"])*\" {adjust();yylval.sval=String(getstr(yytext));return STRING;}

 /* Comment literal */
<INITIAL>"/*" {adjust(); comm_cnt++; BEGIN COMMENT;}
<COMMENT>"/*" {adjust(); comm_cnt++;}
<COMMENT>"*/" {adjust(); comm_cnt--; if(comm_cnt==0){BEGIN INITIAL;} else{continue;}}
<COMMENT>{blank}+ {adjust();continue;}
<COMMENT>"\n" {adjust();continue;}
<COMMENT>. {adjust();continue;}


 /* Error handle */
<INITIAL>. {EM_error(charPos, yytext); adjust();continue;}



