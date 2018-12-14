/* escape.c */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "escape.h"



static void 
traverseExp(S_table table, int depth, A_exp a){
	switch(a->kind){
		case A_varExp: {
			A_var var = a->u.var;
			return traverseVar(table, depth, var);
		}
		case A_nilExp: 
		case A_intExp: 
		case A_stringExp: 
			return;
		case A_callExp:{
			A_expList args = get_callexp_args(a);

			for(A_expList exps=args;exps;exps=exps->tail){
					A_exp param = exps->head;
					traverseExp(table, depth, param);
			}
			return;			
		}
	    case A_opExp:{ 
			A_exp left = get_opexp_left(a); 
			A_exp right = get_opexp_right(a);
			traverseExp(table, depth, left);
			traverseExp(table, depth, right);
			return;
		}
		case A_recordExp: {
			S_symbol typ = get_recordexp_typ(a); 
			A_efieldList fields = get_recordexp_fields(a);
			for(A_efieldList fs=fields; fs; fs=fs->tail){
					A_efield f = fs->head;
					S_symbol fname = f->name; A_exp fexp = f->exp;
					traverseExp(table, depth, fexp);
				}
			return;
		}
		case A_seqExp:{
			for(A_expList seq=get_seqexp_seq(a); seq; seq=seq->tail){
				A_exp ex = seq->head;
				traverseExp(table, depth, ex);
			}
			return;
		}
		case A_assignExp:{
			A_var var = get_assexp_var(a); 
			A_exp ex = get_assexp_exp(a);
			traverseVar(table, depth, var);
			traverseExp(table, depth, ex);
			return ;
		} 
		case A_ifExp:{
			A_exp test = get_ifexp_test(a); 
			A_exp then = get_ifexp_then(a);
			A_exp elsee = get_ifexp_else(a);

			traverseExp(table, depth, test);
			traverseExp(table, depth, then);
			traverseExp(table, depth, elsee);

			return;
		}
	    case A_whileExp:{
			A_exp test = get_whileexp_test(a);
			A_exp body = get_whileexp_body(a);

			traverseExp(table, depth, test);
			traverseExp(table, depth, body);
			return;

		}
		case A_forExp:{
			S_symbol var = get_forexp_var(a); 
			A_exp lo = get_forexp_lo(a);
			A_exp hi = get_forexp_hi(a);
			A_exp body = get_forexp_body(a);

			traverseExp(table, depth, lo);
			traverseExp(table, depth, hi);

			S_beginScope(table);
			a->u.forr.escape = 0;
			S_enter(table, var, EscapeEntry(depth,&(a->u.forr.escape)));
			traverseExp(table, depth, body);
			S_endScope(table);

			return;
			
		}
		case A_breakExp: {
			return;
		}
		case A_letExp:{
			A_exp body = get_letexp_body(a);

			S_beginScope(table); 
			for(A_decList decs=get_letexp_decs(a); decs; decs=decs->tail){
				A_dec dec = decs->head;
				traverseDec(table, depth, dec);
				
			}
			traverseExp(table, depth, body);
			S_endScope(table);

			return;
		}
		case A_arrayExp:{
			A_exp size = get_arrayexp_size(a);
			A_exp init = get_arrayexp_init(a);
			
			traverseExp(table, depth, size);
			traverseExp(table, depth, init);

			return;
		}
		default:{
			EM_error(a->pos, "strange exp type %d", a->kind);
			return;
		}
	}
}

static void 
traverseDec(S_table table, int depth, A_dec d){
	switch(d->kind){
		case A_typeDec:
			return;
		case A_functionDec:{		
			for(A_fundecList funcs=get_funcdec_list(d); funcs; funcs=funcs->tail){
				A_fundec func = funcs->head;				
				
				S_beginScope(table);
				A_fieldList params = func->params; 
		 		for(A_fieldList ls=params;ls;ls=ls->tail){
					A_field param = ls->head;
					S_enter(table, param->name, EscapeEntry(depth+1, &param->escape));
					param->escape = FALSE;
				 }
				A_exp body = func->body;
				traverseExp(table, depth+1, body);
				S_endScope(table);
			}
			return;
		}		
		case A_varDec:{
			S_symbol var = get_vardec_var(d);
			A_exp init = get_vardec_init(d);
			//printf("vardec %s\tdepth:%d\n",S_name(var),depth);
			d->u.var.escape = FALSE;
			S_enter(table, var, EscapeEntry(depth, &d->u.var.escape));			
			traverseExp(table, depth, init);
			return;
		}		
		default:{
			EM_error(d->pos, "strange Dec type %d", d->kind);
			return;
		}
	}
}

static void 
traverseVar(S_table table, int depth, A_var v){
	switch(v->kind){
		case A_simpleVar:{
			S_symbol simple = get_simplevar_sym(v);
			escapeEntry esc = S_look(table, simple);
			//printf("var %s\tdepth %d\n",S_name(simple),depth);
			if(esc && depth > esc->depth){
				//printf("escape var %s\n",S_name(simple));
				*esc->escape = TRUE;
			}
			return;
		}
		case A_fieldVar:{
			A_var var = get_fieldvar_var(v); 			
			return traverseVar(table, depth, var);
		}
		case A_subscriptVar:{
			A_var var = get_subvar_var(v);
			A_exp ex = get_subvar_exp(v);
			traverseVar(table, depth, var);
			traverseExp(table, depth, ex);
			return;
		}
		default:{
			EM_error(v->pos, "strange variable type %d", v->kind);
			return;
		}
	}
}

escapeEntry EscapeEntry(int d, bool* e){
	escapeEntry entry = checked_malloc(sizeof(*entry));
	entry->depth = d;
	entry->escape = e;
	return entry;
}

void Esc_findEscape(A_exp exp){
	S_table table = S_empty();
	traverseExp(table, 0, exp);
}
