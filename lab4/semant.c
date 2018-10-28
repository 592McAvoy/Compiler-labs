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
				S_enter(venv, name, E_FunEntry(formals, S_look(tenv, result)));

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
struct expty transExp(S_table venv, S_table tenv, A_exp a);






