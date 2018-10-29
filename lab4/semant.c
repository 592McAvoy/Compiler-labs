#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "helper.h"
#include "env.h"
#include "semant.h"

/*Lab4: Your implementation of lab4*/


typedef void* Tr_exp;
struct expty 
{
	Tr_exp exp; 
	Ty_ty ty;
};

/*environment*/
S_table venv;
S_table tenv;

/*helper functions*/
Ty_tyList makeFormals(S_table tenv, A_fieldList list){
	A_fieldList params;
	Ty_tyList formals = NULL;
	for(params=list; params; params=params->tail){
		A_field param = params->head;
		S_symbol name = param->name; 
		S_symbol typ = param->typ;
		formals = Ty_TyList(Ty_Name(name, S_look(tenv,typ)),formals);
	}
	return formals;
}

void enterParams(S_table venv, S_table tenv, A_fieldList list){
	A_fieldList params;
	for(params=list; params; params=params->tail){
		A_field param = params->head;
		S_symbol name = param->name; 
		S_symbol typ = param->typ;
		S_enter(venv, name, E_VarEntry(S_look(tenv, typ)));
	}
	return;
}


//In Lab4, the first argument exp should always be **NULL**.
struct expty expTy(Tr_exp exp, Ty_ty ty)
{
	struct expty e;

	e.exp = exp;
	e.ty = ty;

	return e;
}



void SEM_transProg(A_exp exp){
	venv = E_base_venv();
	tenv = E_base_tenv();
	S_enter(tenv,S_Symbol("string"),Ty_String());
	S_enter(tenv,S_Symbol("int"),Ty_Int());

	transExp(venv, tenv, exp);
}

Ty_ty		 transTy (              S_table tenv, A_ty a){
	switch(a->kind){
		case A_nameTy:{
			S_symbol name = get_ty_name(a);
			Ty_ty ty = S_look(tenv, name);
			if(ty && (ty->kind == Ty_string || ty->kind == Ty_int || ty->kind == Ty_name)){
				return ty;		
			}
			else{
				EM_error(a->pos, “undefined typename %s”, S_name(name));
				return Ty_Int();
			}
		}
		case A_recordTy:{
			A_fieldList record;
			Ty_fieldList fields = NULL;
			for(record=get_ty_record(a);record;record=record->tail){
				A_field f = record->head;
				Ty_ty ty = S_look(tenv, f->typ);
				if(!(ty && (ty->kind == Ty_string || ty->kind == Ty_int || ty->kind == Ty_name))){					
					EM_error(a->pos, “undefined typename %s”, S_name(f->typ));
					return Ty_Int();
				}
				Ty_field field = Ty_Field(f->name, ty);
				fileds = Ty_FieldList(filed, fields);
			}
			return Ty_Record(fields);
		}
		case A_arrayTy:{
			S_symbol array = get_ty_array(a);
			Ty_ty ty = S_look(tenv, array);
			if(ty &&(ty->kind == Ty_string || ty->kind == Ty_int || ty->kind == Ty_name)){
				return Ty_Array(ty);
			}
			else{
				EM_error(a->pos, “undefined typename %s”, S_name(array));
				return Ty_Int();
			}
		}
		default:{
			EM_error(a->pos, “strange Type type %d”, a->kind);
			return Ty_Int();
		}
	}
}

void		 transDec(S_table venv, S_table tenv, A_dec d){
	switch(d->kind){
		case A_typeDec:{
			A_nametyList types;
			for(types= get_typedec_list(d);types;types=types->tail){
				A_namety type = types->head;
				S_symbol name = type->name; A_ty ty = type->ty;
				S_enter(tenv,name,Ty_Name(name, transTy(tenv,ty)));
			}
			return;
		}
		case A_varDec:{
			S_symbol var = get_vardec_var(d);
			S_symbol typ = get_vardec_typ(d);
			A_exp init = get_vardec_init(d);
			if(typ){
				Ty_ty ty = S_look(tenv, typ);
				if(ty){
					S_enter(venv, var, E_VarEntry(ty));
				}
				else{
					EM_error(d->pos, “undefined typename %s”, S_name(typ));
				}
			}
			else{
				S_enter(venv, var, E_VarEntry(Ty_Nil()));
			}
			return;
		}		
		case A_functionDec{
			A_fundecList funcs;
			for(funcs=get_funcdec_list(d); funcs; funcs=funcs->tail){
				A_fundec func = funcs->head;

				S_symbol name = func->name;
				A_fieldList params = func->params; 
		 		S_symbol result = func->result; 
				A_exp body = func->body;

				Ty_tyList formals = makeFormals(tenv, params);
				if(result){
					S_enter(venv, name, E_FunEntry(formals, S_look(tenv, result)));
				}
				else{
					S_enter(venv, name, E_FunEntry(formals, Ty_Void()));
				}
				
				S_beginScope(venv);
				enterParams(venc, tenv, params);
				transExp(venv, tenv, body);
				S_endScope(venv);
			}
			return;
		}
		default:{
			EM_error(d->pos, “strange Dec type %d”, d->kind);
			return;
		}
	}
}

struct expty transVar(S_table venv, S_table tenv, A_var v){
	switch(v->kind){
		case A_simpleVar:{
			S_symbol simple = get_simplevar_sym(v);

			E_enventry x = S_look(venv, simple) ;
			if  ( x && x->kind == E_varEntry ) 
				return expTy(NULL, get_varentry_type(x));
			else  {
				EM_error(v->pos, “undefined variable %s”, S_name(simple));
				return expTy(NULL, Ty_Int());
			}
		}
		case A_fieldVar:{
			A_var var = get_fieldvar_var(v); 
			S_symbol sym = get_fieldvar_sym(v);

			E_enventry x = S_look(venv, get_simplevar_sym(var));
			if  ( x && x->kind == E_varEntry ) {
				Ty_ty ty = get_varentry_type(x);
				if(ty->kind == Ty_record){
					for(Ty_fieldList record=ty->u.record;record;record=record->tail){
						Ty_field field = record->head;
						if(field->name == sym){
							return expTy(NULL, field->ty);
						}
					}
					EM_error(v->pos, “variable %s does not have field %s”, S_name(get_simplevar_sym(var)), S_name(sym));
					return expTy(NULL, Ty_Int());
				}
				else{
					EM_error(var->pos, “type of variable %s is not record”, S_name(get_simplevar_sym(var)));
					return expTy(NULL, Ty_Int());
				}
			}
			else  {
				EM_error(var->pos, “undefined variable %s”, S_name(get_simplevar_sym(var)));
				return expTy(NULL, Ty_Int());
			}

		}
		case A_subscriptVar:{
			A_var var = get_subvar_var(v);
			A_exp ex = get_subvar_exp(v);

			E_enventry x = S_look(venv, get_simplevar_sym(var));
			if  ( x && x->kind == E_varEntry ) {
				Ty_ty ty = get_varentry_type(x);
				if(ty->kind == Ty_array){
					return expTy(NULL, get_array_kind(ty));
				}
				else{
					EM_error(var->pos, “type of variable %s is not array”, S_name(get_simplevar_sym(var)));
					return expTy(NULL, Ty_Int());
				}
			}
			else  {
				EM_error(var->pos, “undefined variable %s”, S_name(get_simplevar_sym(var)));
				return expTy(NULL, Ty_Int());
			}

		}
		default:{
			EM_error(v->pos, “strange variable type %d”, v->kind);
			return expTy(NULL, Ty_Int());
		}
	}
}

struct expty transExp(S_table venv, S_table tenv, A_exp a){
	switch(a->type){
		case A_varExp: {
			A_var var = a->u.var;
			return transVar(venv, tenv, var);
		}
		case A_nilExp: {
			return expTy(NULL, Ty_Nil());
		}
		case A_intExp: {
			return expTy(NULL, Ty_Int());
		}
		case A_stringExp: {
			return expTy(NULL, Ty_String());
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
				for(exps=args,tys=formals;exps&&tys;exps=exps->tail,tys=tys->tail){
					A_exp param = exps->head;
					Ty_ty ty = tys->head;
					struct expty pp = transExp(venv, tenv, param);
					if(pp.ty->kind != ty->kind){
						EM_error(param->pos, “unmatch type of param”);
						return expTy(NULL, Ty_Int());
					}
				}
				if(exps != NULL || tys != NULL){
					EM_error(a->pos, “unmatch amount of params”);
					return expTy(NULL, Ty_Int());
				}
				return expTy(NULL, result);
			}
			else{
				EM_error(a->pos, “undefined function %s”, S_name(func));
				return expTy(NULL, Ty_Int());
			}
		}
	    case A_opExp:{
			A_oper oper = get_opexp_oper(a); 
			A_exp left = get_opexp_left(a); 
			A_exp right = get_opexp_right(a);

			struct expty l = transExp(venv, tenv, left);
			struct expty r = transExp(venv, tenv, right);

			if(oper == A_eqOp || oper == A_neqOp){
				if(l.ty->kind == r.ty->kind && (l.ty->kind == Ty_record || l.ty->kind == Ty_array)){
					return expTy(NULL, Ty_Int);
				}
			}
			if(l.ty->kind != Ty_int){
				EM_error(get_opexp_leftpos(a), “integer required”);
			}
			if(r.ty->kind != Ty_int){
				EM_error(get_opexp_rightpos(a), “integer required”);
			}
			return expTy(NULL, Ty_Int());
		}
		case A_recordExp: {
			S_symbol typ = get_recordexp_typ(a); 
			A_efieldList fields = get_recordexp_fields(a);

			Ty_ty type = S_look(tenv, typ);
			if(type && type->kind == Ty_record){
				Ty_fieldList record;
				A_efieldList fs;
				for(record=type->u.record, fs=fields; record&&fs; record=record->tail,fs=fs->tail){
					Ty_field rec = record->head;
					S_symbol rname = rec->name; Ty_ty rty = rec->ty;

					A_efield f = fs->head;
					S_symbol fname = f->name; A_exp fexp = f->exp;
					struct ety = transExp(venv, tenv, fexp);

					if(!(rname == fname && rty->kind == ety.ty->kind)){
						EM_error(fexp->pos, “unmatch type of field %s”, S_name(rname));
						return expTy(NULL, Ty_Int());
					}
				}
				if(record != NULL || fs != NULL){
					EM_error(a->pos, “unmatch amount of fields”);
					return expTy(NULL, Ty_Int());
				}
				return expTy(NULL, type);
			}
			else{
				EM_error(a->pos, “undefined record type %s”, S_name(typ));
				return expTy(NULL, Ty_Int());
			}
		}
		case A_seqExp:{
			A_expList seq;
			struct expty ety;
			for(seq=get_seqexp_seq(a); seq; seq=seq->tail){
				A_exp ex = seq->head;
				ety = transExp(venv, tenv, ex);
			}
			return ety;
		}
		case A_assignExp:{
			A_var var = get_assexp_var(a); 
			A_exp ex = get_assexp_exp(a);

			struct expty vty = transVar(venv, tenv, var);
			struct expty ety = transExp(venv, tenv, ex);

			if(vty.ty->kind == ety.ty->kind){
				return expTy(NULL, Ty_Void());
			}
			else{
				EM_error(a->pos, “type does not match”);
				return expTy(NULL, Ty_Int());
			}
		} 
		case A_ifExp:{
			A_exp test = get_ifexp_test(a); 
			A_exp then = get_ifexp_then(a);
			A_exp elsee = get_ifexp_else(a);

			struct expty testty = transExp(venv, tenv, test);
			struct expty thenty = transExp(venv, tenv, then);
			struct expty elsety = transExp(venv, tenv, elsee);

			if(elsety.ty->kind == Ty_nil || elsety.ty->kind == thenty.ty->kind){
				return expTy(NULL, elsety.ty);
			}
			else{
				EM_error(a->pos, “types of then_exp and else_exp dont match ”);
				return expTy(NULL, Ty_Int());
			}
		}
	    case A_whileExp:{
			A_exp test = get_whileexp_test(a);
			A_exp body = get_whileexp_body(a);

			struct expty testty = transExp(venv, tenv, test);
			struct expty bodyty = transExp(venv, tenv, body);

			if(bodyty.ty->kind == Ty_void){
				return expTy(NULL, bodyty.ty);
			}
			else{
				EM_error(body->pos, “type of body_exp is not _void”);
				return expTy(NULL, Ty_Int());
			}

		}
		case A_forExp{
			S_symbol var = get_forexp_var(a); 
			A_exp lo = get_forexp_lo(a);
			A_exp hi = get_forexp_hi(a);
			A_exp body = get_forexp_body(a);

			struct expty loty = transExp(venv, tenv, lo);
			struct expty hity = transExp(venv, tenv, hi);

			if(loty.ty->kind == Ty_int && loty.ty->kind == hity.ty->kind){
				S_beginScope(venv);
				S_enter(venv, var, E_VarEntry(loty.ty));
				struct expty bodyty = transExp(venv, tenv, body);
				S_endScope(venv);
				if(bodyty.ty->kind == Ty_void){
					return expTy(NULL, bodyty.ty);
				}
				else{
					EM_error(body->pos, “type of body_exp is not _void”);
					return expTy(NULL, Ty_Int());
				}
			}
			else{
				EM_error(a->pos, “types of lo_exp and hi_exp dont match ”);
				return expTy(NULL, Ty_Int());
			}
			
		}
		case A_breakExp: {
			return expTy(NULL, Ty_Void());
		}
		case A_letExp:{
			A_decList decs; 
			A_exp body = get_letexp_body(a);

			S_beginScope(tenv); S_beginScope(venv);
			for(decs=get_letexp_decs(a); decs; decs=decs->tail){
				A_dec dec = decs->head;
				transDec(venv, tenv, dec);
			}
			struct expty bodyty = transExp(venv, tenv, body);
			S_endScope(venv); S_endScope(venv);

			return bodyty;

		}
		case A_arrayExp:{
			S_symbol typ = get_arrayexp_typ(a); 
			A_exp size = get_arrayexp_size(a);
			A_exp init = get_arrayexp_init(a);

			Ty_ty type = S_look(tenv, typ);
			if(type && type->kind == Ty_array){
				struct expty sizety = transExp(venv, tenv, size);
				struct expty initty = transExp(venv, tenv, init);
				if(sizety.ty->kind != Ty_int){
					EM_error(a->pos, “array size type not integer”);
					return expTy(NULL, Ty_Int());
				}
				if(initty.ty->kind != get_array_kind(type)){
					EM_error(a->pos, “init type does not match”);
					return expTy(NULL, Ty_Int());
				}
				return expty(NULL, Ty_Array());
			}
			else{
				EM_error(a->pos, “undefined array type %s”, S_name(typ));
				return expTy(NULL, Ty_Int());
			}
		}
		default:{
			EM_error(a->pos, “strange exp type %d”, a->kind);
			return expTy(NULL, Ty_Int());
		}
	}
}






