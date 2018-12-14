#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "semant.h"
#include "helper.h"
#include "translate.h"
#include "escape.h"
#include "printtree.h"

/*Lab5: Your implementation of lab5.*/

struct expty 
{
	Tr_exp exp; 
	Ty_ty ty;
};

//In Lab4, the first argument exp should always be **NULL**.
struct expty expTy(Tr_exp exp, Ty_ty ty)
{
	struct expty e;

	e.exp = exp;
	e.ty = ty;

	return e;
}


/*environment*/
S_table venv;
S_table tenv;
int recursive;

/*helper functions in lab4*/
Ty_tyList makeFormals(S_table tenv, A_fieldList list){
	//empty params
	if(!list){
		return NULL;
	}
	A_field param = list->head;
	S_symbol name = param->name; 
	S_symbol typ = param->typ;
	if(!list->tail){
		return Ty_TyList(Ty_Name(name, S_look(tenv,typ)),NULL);
	}
	else{
		return Ty_TyList(Ty_Name(name, S_look(tenv,typ)),makeFormals(tenv,list->tail));
	}
}

U_boolList makeBoolList(A_fieldList list){
	//empty params
	if(!list){
		return NULL;
	}
	A_field param = list->head;
	if(!list->tail){
		return U_BoolList(param->escape, NULL);
	}
	else{
		return U_BoolList(param->escape, makeBoolList(list->tail));
	}
}

void enterParams(S_table venv, S_table tenv, A_fieldList list, Tr_accessList accs){
	//empty params
	if(!list){
		return;
	}
	A_fieldList params;
	Tr_accessList acc;
	for(params=list, acc=accs->tail; params; params=params->tail, acc=acc->tail){
		A_field param = params->head;
		S_symbol name = param->name; 
		S_symbol typ = param->typ;
		S_enter(venv, name, E_VarEntry(acc->head,S_look(tenv, typ)));
	}
	return;
}

Ty_ty actualTy(Ty_ty ty){
	if(!ty){
		return NULL;
	}

	if(ty->kind == Ty_name){
		return actualTy(ty->u.name.ty);
	}
	else {
		return ty;
	}
}

Ty_fieldList makeFields(S_table tenv, A_fieldList record){
	A_field f = record->head;
	S_symbol name = f->name;
	S_symbol typ = f->typ;

	Ty_ty ty = S_look(tenv, typ);
	Ty_field field;
	if(ty && (ty->kind == Ty_string || ty->kind == Ty_int)){
		field = Ty_Field(name, ty);			
	}
	else if(recursive == 1){
		field = Ty_Field(name, NULL);
	}
	else if(ty && ty->kind == Ty_name){
		field = Ty_Field(name, ty);
	}
	else{
		EM_error(f->pos, "undefined type %s", S_name(f->typ));
		return NULL;
	}

	if(record->tail){	
		return Ty_FieldList(field, makeFields(tenv, record->tail));
	}
	else{
		return Ty_FieldList(field, NULL);
	}				
}

int typeEqual(Ty_ty A, Ty_ty B){
	if(actualTy(A)->kind != actualTy(B)->kind){
		//printf("1");
		return 0;
	}
	if(A->kind!=Ty_name || B->kind!=Ty_name){
		//printf("2");
		return 1;
	}

	Ty_ty tmp = A;	
	while(tmp->kind==Ty_name){
		if(tmp->u.name.sym == B->u.name.sym){
			//printf("3");
			return 1;
		}
		tmp = tmp->u.name.ty;
	}
	//printf("4");

	tmp = B;	
	while(tmp->kind==Ty_name){
		if(tmp->u.name.sym == A->u.name.sym){
			//printf("5");
			return 1;
		}
		tmp = tmp->u.name.ty;
	}
	//printf("6");
	return 0;
	
}

int checkTypeRepeat(A_nametyList list, S_symbol name){
	A_nametyList types;
	int cnt = 0;
	for(types= list;types;types=types->tail){
		A_namety type = types->head;
		S_symbol nn = type->name;
		if(S_name(name) == S_name(nn)){
			cnt += 1;
		}
	}
	return cnt;
}

int checkFuncRepeat(A_fundecList list, S_symbol name){
	A_fundecList funcs;
	int cnt = 0;
	for(funcs=list; funcs; funcs=funcs->tail){
		A_fundec func = funcs->head;
		S_symbol nn = func->name;
		if(S_name(name) == S_name(nn)){
			cnt += 1;
		}		
	}
	return cnt;
}


/* core functions */
F_fragList SEM_transProg(A_exp e){
	//TODO LAB5: do not forget to add the main frame
	venv = E_base_venv();
	tenv = E_base_tenv();
	recursive = 0;

	Temp_label prog = Temp_newlabel();
	Tr_level l = Tr_outermost();

	struct expty body = transExp(venv, tenv, e, l, prog);
	
	//Tr_print(body.exp);

	Tr_procEntryExit(l, body.exp, NULL);
	
	return Tr_getResult(); 
}


Ty_ty		 transTy (              S_table tenv, A_ty a){
	switch(a->kind){
		case A_nameTy:{
			S_symbol name = get_ty_name(a);

			Ty_ty ty = S_look(tenv, name);
			if(ty && (ty->kind == Ty_string || ty->kind == Ty_int)){
				return ty;
			}
			if(recursive == 1){
				return NULL;
			}
			if(ty && ty->kind == Ty_name){
				return ty;
			}
			EM_error(a->pos, "undefined type %s", S_name(name));
			return Ty_Int();
		}
		case A_recordTy:{
			A_fieldList record = get_ty_record(a);
			Ty_fieldList fields = makeFields(tenv, record);
			if(fields){
				return Ty_Record(fields);
			}
			else{
				return Ty_Int();
			}
		}
		case A_arrayTy:{
			//printf("array ty\n");
			S_symbol array = get_ty_array(a);
			Ty_ty ty = S_look(tenv, array);
			//Ty_print(ty);
			if(ty &&(ty->kind == Ty_string || ty->kind == Ty_int )){
				return Ty_Array(ty);
			}
			if(recursive == 1){
				return NULL;
			}
			if(ty && ty->kind == Ty_name){
				return Ty_Array(ty);
			}
			else{
				EM_error(a->pos, "undefined type %s", S_name(array));
				return Ty_Int();
			}
		}
		default:{
			EM_error(a->pos, "strange Type type %d", a->kind);
			return Ty_Int();
		}
	}
}

Tr_exp transDec(S_table venv, S_table tenv, A_dec d, Tr_level l, Temp_label label){
	switch(d->kind){
		case A_typeDec:{
			A_nametyList types;

			recursive = 1;
			for(types= get_typedec_list(d);types;types=types->tail){
				A_namety type = types->head;
				S_symbol name = type->name; A_ty ty = type->ty;

				if(checkTypeRepeat(types,name) != 1){
					EM_error(d->pos, "two types have the same name");
					return Tr_typeDec(); 
				}
				S_enter(tenv,name,Ty_Name(name, transTy(tenv,ty)));
			}

			recursive = 0;
			for(types= get_typedec_list(d);types;types=types->tail){
				A_namety type = types->head;
				S_symbol name = type->name; A_ty ty = type->ty;
				//printf("ty: %s\n",S_name(name));
				S_enter(tenv,name,Ty_Name(name, transTy(tenv,ty)));
			}

			for(types= get_typedec_list(d);types;types=types->tail){
				A_namety type = types->head;
				S_symbol name = type->name; 
				if(!actualTy(S_look(tenv, name))){
					EM_error(d->pos, "illegal type cycle");
					return Tr_typeDec(); 
				}
			}
			return Tr_typeDec(); 
		}
		case A_varDec:{
			S_symbol var = get_vardec_var(d);
			S_symbol typ = get_vardec_typ(d);
			A_exp init = get_vardec_init(d);
			bool esc = get_vardec_esc(d);
			//printf("\nvar: %s\n",S_name(var));
			struct expty initty = transExp(venv, tenv, init, l, label);
		
			if(typ){
				Ty_ty ty = S_look(tenv, typ);
				if(typeEqual(ty, initty.ty) == 1 || initty.ty->kind == Ty_nil){
					Tr_access acc = Tr_allocLocal(l, esc);
					S_enter(venv, var, E_VarEntry(acc, ty));
					return Tr_varDec(acc, initty.exp);
				}
				else{
					EM_error(d->pos, "type mismatch");
					return Tr_typeDec(); 
				}
			}
			else{
				if(actualTy(initty.ty)->kind == Ty_nil){
					EM_error(d->pos, "init should not be nil without type specified");
					return Tr_typeDec(); 
				}
				Tr_access acc = Tr_allocLocal(l, esc);
				S_enter(venv, var, E_VarEntry(acc, initty.ty));
				return Tr_varDec(acc, initty.exp);
			}	
			
		}		
		case A_functionDec:{
			A_fundecList funcs;

			recursive = 1;
			for(funcs=get_funcdec_list(d); funcs; funcs=funcs->tail){
				A_fundec func = funcs->head;

				S_symbol name = func->name;
				A_fieldList params = func->params; 
		 		S_symbol result = func->result; 
				A_exp body = func->body;
				
				if(checkFuncRepeat(funcs, name) != 1){
					EM_error(d->pos, "two functions have the same name");
					return Tr_typeDec(); 
				}

				//Temp_label fname = Temp_newlabel();
				Temp_label fname = name;
				Tr_level newl = Tr_newLevel(l, fname, makeBoolList(params));
				Ty_tyList formals = makeFormals(tenv, params);
				
				if(result){
					S_enter(venv, name, E_FunEntry(
						newl,
						fname,
						formals, S_look(tenv, result)));
				}
				else{
					S_enter(venv, name, E_FunEntry(
						newl,
						fname,
						formals, Ty_Void()));
				}
			}

			recursive = 0;
			for(funcs=get_funcdec_list(d); funcs; funcs=funcs->tail){
				A_fundec func = funcs->head;

				S_symbol name = func->name;
				A_fieldList params = func->params; 
		 		S_symbol result = func->result; 
				A_exp body = func->body;

				//printf("func %s\n",S_name(name));	
				E_enventry f = S_look(venv, name);		
				Temp_label fname = f->u.fun.label;
				Tr_level newl = f->u.fun.level;				
				Ty_tyList formals = makeFormals(tenv, params);
				if(result){
					S_enter(venv, name, E_FunEntry(
						newl,
						fname,
						formals, S_look(tenv, result)));
				}
				else{
					S_enter(venv, name, E_FunEntry(
						newl,
						fname,
						formals, Ty_Void()));
				}
				
				S_beginScope(venv);
				enterParams(venv, tenv, params, Tr_formals(newl));
			
				struct expty resultty = transExp(venv, tenv, body, newl, fname);
				
				S_endScope(venv);

				if(result){
					Ty_ty rty = S_look(tenv, result);
					if(actualTy(rty)->kind == actualTy(resultty.ty)->kind){
						//Tr_exp proc = Tr_functionDec(fname, newl, resultty.exp);
						Tr_procEntryExit(newl, resultty.exp, Tr_formals(newl));
						continue;
					}
					else{
						EM_error(d->pos, "false return type");
						return Tr_typeDec(); 
					}
				}
				else{
					if(actualTy(resultty.ty)->kind == Ty_void){
						//Tr_exp proc = Tr_functionDec(fname, newl, resultty.exp);
						Tr_procEntryExit(newl, resultty.exp, Tr_formals(newl));
						continue;
					}
					else{
						EM_error(d->pos, "procedure returns value");
						return Tr_typeDec(); 
					}
				}

			}			
			return Tr_typeDec();
		}
		default:{
			EM_error(d->pos, "strange Dec type %d", d->kind);
			return Tr_typeDec();
		}
	}
}

struct expty transVar(S_table venv, S_table tenv, A_var v, Tr_level l, Temp_label label){
	switch(v->kind){
		case A_simpleVar:{
			S_symbol simple = get_simplevar_sym(v);

			E_enventry x = S_look(venv, simple) ;
			if  ( x && x->kind == E_varEntry ) 
				return expTy(Tr_simpleVar(get_varentry_access(x),l), get_varentry_type(x));
			else  {
				EM_error(v->pos, "undefined variable %s", S_name(simple));
				return expTy(Tr_err(), Ty_Int());
			}
		}
		case A_fieldVar:{
			A_var var = get_fieldvar_var(v); 
			S_symbol sym = get_fieldvar_sym(v);

			struct expty base = transVar(venv,tenv,var,l,label);
			Ty_ty ty = base.ty;
			if (ty){
				if(actualTy(ty)->kind == Ty_record){
					ty = actualTy(ty);
					int cnt = 0;
					for(Ty_fieldList record=ty->u.record;record;record=record->tail){
						Ty_field field = record->head;
						if(S_name(field->name) == S_name(sym)){
							return expTy(Tr_fieldVar(base.exp, cnt), field->ty);
						}
						cnt += 1;
					}
					EM_error(v->pos, "field %s doesn\'t exists", S_name(sym));
					return expTy(Tr_err(), Ty_Int());
				}
				else{
					EM_error(var->pos, "not a record type", S_name(get_simplevar_sym(var)));
					return expTy(Tr_err(), Ty_Int());
				}
			}
			else  {
				EM_error(var->pos, "undefined variable %s", S_name(get_simplevar_sym(var)));
				return expTy(Tr_err(), Ty_Int());
			}

		}
		case A_subscriptVar:{
			A_var var = get_subvar_var(v);
			A_exp ex = get_subvar_exp(v);

			struct expty base = transVar(venv, tenv, var, l, label);
			struct expty sub = transExp(venv, tenv, ex, l, label);
			Ty_ty ty = base.ty;
			if(ty){
				if(actualTy(ty)->kind == Ty_array){
					if(actualTy(sub.ty)->kind != Ty_int){
						EM_error(var->pos, "array subscirpt not int");
						return expTy(Tr_err(), Ty_Int());
					}
					ty = actualTy(ty);
					return expTy(Tr_subscriptVar(base.exp,sub.exp), ty->u.array);
				}
				else{
					EM_error(var->pos, "array type required");
					return expTy(Tr_err(), Ty_Int());
				}
			}
			else  {
				EM_error(var->pos, "undefined variable %s", S_name(get_simplevar_sym(var)));
				return expTy(Tr_err(), Ty_Int());
			}

		}
		default:{
			EM_error(v->pos, "strange variable type %d", v->kind);
			return expTy(Tr_err(), Ty_Int());
		}
	}
}

struct expty transExp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label){
	switch(a->kind){
		case A_varExp: {
			A_var var = a->u.var;
			return transVar(venv, tenv, var, l, label);
		}
		case A_nilExp: {
			return expTy(Tr_nilExp(), Ty_Nil());
		}
		case A_intExp: {
			int i = a->u.intt;
			return expTy(Tr_intExp(i), Ty_Int());
		}
		case A_stringExp: {
			string str = a->u.stringg;
			return expTy(Tr_stringExp(str), Ty_String());
		}
		case A_callExp:{
			S_symbol func = get_callexp_func(a); 
			A_expList args = get_callexp_args(a);
			
			E_enventry x = S_look(venv, func);
			if(x && x->kind == E_funEntry){
				Ty_tyList formals = get_func_tylist(x); 
				Ty_ty result = get_func_res(x);
				A_expList exps;
				Ty_tyList tys;
				Tr_expList ls = NULL;//parameter tr_explist
				for(exps=args,tys=formals;exps&&tys;exps=exps->tail,tys=tys->tail){
					A_exp param = exps->head;
					Ty_ty ty = tys->head;
					
					struct expty pp = transExp(venv, tenv, param, l, label);
					
					if(actualTy(pp.ty)->kind != actualTy(ty)->kind){
						EM_error(param->pos, "para type mismatch");
						return expTy(Tr_err(), Ty_Int());
					}
					ls = Tr_ExpList(pp.exp, ls);
				}
				if(exps != NULL){
					EM_error(a->pos, "too many params in function %s", S_name(func));
					return expTy(Tr_err(), Ty_Int());
				}
				if(tys != NULL){
					EM_error(a->pos, "too less params in function %s", S_name(func));
					return expTy(Tr_err(), Ty_Int());
				}
				Temp_label fname =  get_func_label(x);
				Tr_level flev =	get_func_level(x);
				return expTy(Tr_callExp(fname, ls, flev, l, S_name(func)), result);
			}
			else{
				EM_error(a->pos, "undefined function %s", S_name(func));
				return expTy(Tr_err(), Ty_Int());
			}
		}
	    case A_opExp:{
			A_oper oper = get_opexp_oper(a); 
			A_exp left = get_opexp_left(a); 
			A_exp right = get_opexp_right(a);

			struct expty le = transExp(venv, tenv, left, l, label);
			struct expty r = transExp(venv, tenv, right, l, label);

			switch(oper){
				//arithmetic calculation
				case A_plusOp:case A_minusOp:case A_timesOp:case A_divideOp:
				{
					if(actualTy(le.ty)->kind != Ty_int){
						EM_error(get_opexp_leftpos(a), "integer required");
						return expTy(Tr_err(), Ty_Int());
					}
					if(actualTy(r.ty)->kind != Ty_int){
						EM_error(get_opexp_rightpos(a), "integer required");
						return expTy(Tr_err(), Ty_Int());
					}					
					return expTy(Tr_arithExp(oper,le.exp,r.exp), Ty_Int());
				}
				//compare
				default:
				{
					if(actualTy(le.ty)->kind == actualTy(r.ty)->kind || le.ty->kind == Ty_nil || r.ty->kind == Ty_nil){
						if(actualTy(le.ty)->kind == Ty_record || actualTy(le.ty)->kind == Ty_array){
							return expTy(Tr_ptrCompExp(oper,le.exp,r.exp), Ty_Int());
						}
						if(actualTy(le.ty)->kind == Ty_string){
							return expTy(Tr_strCompExp(oper,le.exp,r.exp), Ty_Int());
						}
						if(actualTy(le.ty)->kind == Ty_int){
							return expTy(Tr_intCompExp(oper,le.exp,r.exp), Ty_Int());
						}
					} 					
					else{
						EM_error(get_opexp_leftpos(a), "same type required");
						return expTy(Tr_err(), Ty_Int());
					}

				}
			}		
		}
		case A_recordExp: {
			S_symbol typ = get_recordexp_typ(a); 
			A_efieldList fields = get_recordexp_fields(a);

			Ty_ty type = S_look(tenv, typ);
			Ty_ty typp = actualTy(type);
			
			if(typp && typp->kind == Ty_record){
				Ty_fieldList record;
				A_efieldList fs;
				Tr_expList el = NULL;
				int cnt = 0;
				for(record=typp->u.record, fs=fields; record&&fs; record=record->tail,fs=fs->tail){
					Ty_field rec = record->head;
					S_symbol rname = rec->name; Ty_ty rty = rec->ty;
					
					A_efield f = fs->head;
					S_symbol fname = f->name; A_exp fexp = f->exp;
					
					struct expty ety = transExp(venv, tenv, fexp, l, label);

					//if(!(rname == fname && actualTy(rty)->kind == actualTy(ety.ty)->kind)){
					if(!(rname == fname && typeEqual(rty, ety.ty)) && ety.ty->kind != Ty_nil){
						EM_error(fexp->pos, "unmatch type of field %s", S_name(rname));
						return expTy(Tr_err(), Ty_Int());
					}

					cnt += 1;
					el = Tr_ExpList(ety.exp, el);
				}
				if(record != NULL || fs != NULL){
					EM_error(a->pos, "unmatch amount of fields");
					return expTy(Tr_err(), Ty_Int());
				}
				return expTy(Tr_recordExp(el, cnt), type);
			}
			else{
				EM_error(a->pos, "undefined type %s", S_name(typ));
				return expTy(Tr_err(), Ty_Int());
			}
		}
		case A_seqExp:{
			A_expList seq;
			struct expty ety;
			if(!get_seqexp_seq(a)){
				//empty exp:()
				ety.exp = Tr_nilExp();
				ety.ty = Ty_Void();
				return ety;
			}

			Tr_expList ll = NULL;
			for(seq=get_seqexp_seq(a); seq; seq=seq->tail){
				A_exp ex = seq->head;
				ety = transExp(venv, tenv, ex, l, label);
				ll = Tr_ExpList(ety.exp, ll);
			}
			return expTy(Tr_SeqExp(ll), ety.ty);
		}
		case A_assignExp:{
			
			A_var var = get_assexp_var(a); 
			A_exp ex = get_assexp_exp(a);

			if(var->kind == A_simpleVar){
				E_enventry x = S_look(venv, get_simplevar_sym(var));
				if(x->readonly == 1){
					EM_error(a->pos, "loop variable modified");
					return expTy(Tr_err(), Ty_Int());
				}
			}
			
			struct expty vty = transVar(venv, tenv, var, l, label);
			struct expty ety = transExp(venv, tenv, ex, l, label);

			if(typeEqual(vty.ty, ety.ty) == 1 || ety.ty->kind == Ty_nil){
				return expTy(Tr_assignExp(vty.exp, ety.exp), Ty_Void());
			}
			else{
				EM_error(a->pos, "unmatched assign exp");
				return expTy(Tr_err(), Ty_Int());
			}
		} 
		case A_ifExp:{
			//pr_exp(stderr, a, 0);
			A_exp test = get_ifexp_test(a); 
			A_exp then = get_ifexp_then(a);
			A_exp elsee = get_ifexp_else(a);

			struct expty testty = transExp(venv, tenv, test, l, label);
			struct expty thenty = transExp(venv, tenv, then, l, label);
			struct expty elsety = transExp(venv, tenv, elsee, l, label);

			if(actualTy(elsety.ty)->kind == Ty_nil){
				if(actualTy(thenty.ty)->kind == Ty_void){
					return expTy(Tr_ifExp(testty.exp,thenty.exp,elsety.exp, thenty.ty), thenty.ty);
				}
				else{
					//EM_error(a->pos, "if-then exp's body must produce no value");
					return expTy(Tr_ifExp(testty.exp,thenty.exp,elsety.exp, thenty.ty), thenty.ty);
				}
			}
			else{
				if(actualTy(thenty.ty)->kind == actualTy(elsety.ty)->kind){
					return expTy(Tr_ifExp(testty.exp,thenty.exp,elsety.exp, elsety.ty), elsety.ty);
				}
				else{
					EM_error(a->pos, "then exp and else exp type mismatch");
					return expTy(Tr_err(), Ty_Int());
				}
			}
		}
	    case A_whileExp:{
			A_exp test = get_whileexp_test(a);
			A_exp body = get_whileexp_body(a);

			Temp_label done = Temp_newlabel();
			struct expty testty = transExp(venv, tenv, test, l, label);
			//printf("while test\n");
			struct expty bodyty = transExp(venv, tenv, body, l, done);
			//printf("while body\n");

			//Ty_print(bodyty.ty);

			if(bodyty.ty->kind == Ty_void){
				return expTy(Tr_whileExp(testty.exp, bodyty.exp, done), bodyty.ty);
			}
			else{
				EM_error(body->pos, "while body must produce no value");
				return expTy(Tr_err(), Ty_Int());
			}

		}
		case A_forExp:{
			S_symbol var = get_forexp_var(a); 
			A_exp lo = get_forexp_lo(a);
			A_exp hi = get_forexp_hi(a);
			A_exp body = get_forexp_body(a);

			Temp_label done = Temp_newlabel();
			struct expty loty = transExp(venv, tenv, lo, l, label);
			struct expty hity = transExp(venv, tenv, hi, l, label);

			if(loty.ty->kind == Ty_int && loty.ty->kind == hity.ty->kind){
				Tr_access vac = Tr_allocLocal(l, get_forexp_esc(a));
				S_beginScope(venv);
				S_enter(venv, var, E_ROVarEntry(
						vac,
						loty.ty));
				struct expty bodyty = transExp(venv, tenv, body, l, done);
				S_endScope(venv);
				//Tr_print(bodyty.exp);
				if(bodyty.ty->kind == Ty_void){
					Tr_exp forv = Tr_simpleVar(vac, l);
					return expTy(Tr_forExp(forv, loty.exp, hity.exp, bodyty.exp, done), bodyty.ty);
				}
				else{
					EM_error(body->pos, "forbody must produce no value");
					return expTy(Tr_err(), Ty_Int());
				}
			}
			else{
				EM_error(a->pos, "for exp's range type is not integer\nloop variable can't be assigned");
				return expTy(Tr_err(), Ty_Int());
			}
			
		}
		case A_breakExp: {
			return expTy(Tr_breakExp(label), Ty_Void());
		}
		case A_letExp:{
			A_decList decs; 
			A_exp body = get_letexp_body(a);

			Tr_expList li = NULL;
			S_beginScope(tenv); S_beginScope(venv);
			for(decs=get_letexp_decs(a); decs; decs=decs->tail){
				A_dec dec = decs->head;
				Tr_exp ee = transDec(venv, tenv, dec, l, label);
				li = Tr_ExpList(ee, li);
			}
			//printf("dec over\n");
			struct expty bodyty = transExp(venv, tenv, body, l, label);
			S_endScope(venv); S_endScope(tenv);
			//li = Tr_ExpList(bodyty.exp, li);
			//return bodyty;
			return expTy(Tr_letExp(li, bodyty.exp),bodyty.ty);

		}
		case A_arrayExp:{
			S_symbol typ = get_arrayexp_typ(a); 
			A_exp size = get_arrayexp_size(a);
			A_exp init = get_arrayexp_init(a);

			Ty_ty type = S_look(tenv, typ);
			Ty_ty typp = actualTy(type);
			if(typp && typp->kind == Ty_array){
				struct expty sizety = transExp(venv, tenv, size, l, label);
				struct expty initty = transExp(venv, tenv, init, l, label);
				if(sizety.ty->kind != Ty_int){
					EM_error(a->pos, "array size type not integer");
					return expTy(Tr_err(), Ty_Int());
				}
				//if(initty.ty->kind != get_array_kind(typp)){
				if(typeEqual(initty.ty, typp->u.array)==0){
					EM_error(a->pos, "type mismatch");
					return expTy(Tr_err(), Ty_Int());
				}
				//int len = size->u.intt;
				return expTy(Tr_arrayExp(sizety.exp, initty.exp), type);
			}
			else{
				EM_error(a->pos, "undefined type %s", S_name(typ));
				return expTy(Tr_err(), Ty_Int());
			}
		}
		default:{
			EM_error(a->pos, "strange exp type %d", a->kind);
			return expTy(Tr_err(), Ty_Int());
		}
	}
}