#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"
#include "frame.h"
#include "translate.h"

//LAB5: you can modify anything you want.

struct Tr_access_ {
	Tr_level level;
	F_access access;
};


struct Tr_accessList_ {
	Tr_access head;
	Tr_accessList tail;	
};

struct Tr_level_ {
	F_frame frame;
	Tr_level parent;
};

/* patchlist */
typedef struct patchList_ *patchList;
struct patchList_ 
{
	Temp_label *head; 
	patchList tail;
};

static patchList PatchList(Temp_label *head, patchList tail)
{
	patchList list;

	list = (patchList)checked_malloc(sizeof(struct patchList_));
	list->head = head;
	list->tail = tail;
	return list;
}

void doPatch(patchList tList, Temp_label label)
{
	for(; tList; tList = tList->tail)
		*(tList->head) = label;
}

patchList joinPatch(patchList first, patchList second)
{
	if(!first) return second;
	for(; first->tail; first = first->tail);
	first->tail = second;
	return first;
}

/* Tr_exp */
struct Cx 
{
	patchList trues; 
	patchList falses; 
	T_stm stm;
};

struct Tr_exp_ {
	enum {Tr_ex, Tr_nx, Tr_cx} kind;
	union {
		T_exp ex; //EX
		T_stm nx; //NX
		struct Cx cx; //CX
	} u;
};

static Tr_exp Tr_Ex(T_exp ex){
	Tr_exp e = checked_malloc(sizeof(*e));

	e->kind = Tr_ex;
	e->u.ex = ex;
	return e;
}

static Tr_exp Tr_Nx(T_stm nx){
	Tr_exp e = checked_malloc(sizeof(*e));

	e->kind = Tr_nx;
	e->u.nx = nx;
	return e;
}

static Tr_exp Tr_Cx(patchList trues,patchList falses,T_stm stm){
	Tr_exp e = checked_malloc(sizeof(*e));

	e->kind = Tr_cx;
	e->u.cx.trues = trues;
	e->u.cx.falses = falses;
	e->u.cx.stm = stm;

	return e;
}

static T_exp unEx(Tr_exp e){
	switch (e->kind) {
		case Tr_ex: 
			return e->u.ex ;
	    case Tr_cx: {
			Temp_temp r = Temp_newtemp();
			Temp_label t = Temp_newlabel(), f = Temp_newlabel() ;
			doPatch(e->u.cx.trues, t) ;  doPatch(e->u.cx.falses, f) ;
			return T_Eseq(T_Move(T_Temp(r), T_Const(1)),
					T_Eseq(e->u.cx.stm,
						T_Eseq(T_Label(f),
							T_Eseq(T_Move(T_Temp(r), T_Const(0)),
								T_Eseq(T_Label(t), T_Temp(r))))));
		}
	    case Tr_nx:
			return T_Eseq(e->u.nx, T_Const(0));
    }
}
static T_stm unNx(Tr_exp e){
	switch(e->kind){
		case Tr_ex:
			return T_Exp(e->u.ex);
		case Tr_nx:
			return e->u.nx;
		case Tr_cx:{
			return T_Exp(unEx(e));
		}
	}
}
static struct Cx unCx(Tr_exp e){
	struct Cx cx;
	switch(e->kind){
		case Tr_ex:{
			T_exp ex = e->u.ex;
			T_stm s1 = T_Cjump(T_ne, ex, T_Const(0), NULL, NULL);
			cx.trues = PatchList(&(s1->u.CJUMP.true), NULL);
			cx.falses = PatchList(&(s1->u.CJUMP.false), NULL);
			cx.stm = s1;
			/*if(ex->kind = T_CONST){
				if(ex->u.CONST == 0){
					Temp_label f = *(cx.falses->head);
					cx.stm = T_Jump(T_Name(f),Temp_LabelList(f,NULL));
					return cx;
				}
				else{
					Temp_label t = *(cx.trues->head);
					cx.stm = T_Jump(T_Name(t),Temp_LabelList(t,NULL));
					return cx;
				}
			}*/
			return cx;
		}
		case Tr_nx:
			/*error*/ return cx;
		case Tr_cx:
			return e->u.cx;
	}
}




/* part I */
Tr_level Tr_outermost(void){
	Tr_level l = checked_malloc(sizeof(*l));

	Temp_label lab = Temp_newlabel();
	l->frame = F_newFrame(lab, NULL);
	l->parent = NULL;
	return l;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals){
	Tr_level l = checked_malloc(sizeof(*l));

	U_boolList newl = U_BoolList(1, formals);//add static link
	l->frame = F_newFrame(name, newl);
	l->parent = parent;
	return l;
}

Tr_access Tr_allocLocal(Tr_level level, bool escape){
	Tr_access ac = checked_malloc(sizeof(*ac));

	ac->access = F_allocLocal(level->frame, escape);
	ac->level = level;
}

/* part II */
Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail){
	Tr_accessList list = checked_malloc(sizeof(*list));

	list->head = head;
	list->tail = tail;
	return list;
}

Tr_accessList makeFormals(F_accessList fl, Tr_level level){
	Tr_access ac = checked_malloc(sizeof(*ac));

	ac->level = level;
	ac->access = fl->head;

	if(fl->tail){
		return Tr_AccessList(ac, makeFormals(fl->tail, level));
	}
	else{
		return Tr_AccessList(ac, NULL);
	}
}

Tr_accessList Tr_formals(Tr_level level){
	F_frame f = level->frame;
	F_accessList fl = F_formals(f);
	return makeFormals(fl, level);
}

//transVar
Tr_exp Tr_simpleVar(Tr_access acc, Tr_level l){
	Tr_level vl = acc->level;
	F_access vacc = acc->access;
	T_exp fp = T_Temp(F_FP());//addr of current fp

	//calculate SL
	int SLoff = F_wordsize;//SL is 1 wordsize off FP
	while(l!=vl){
		fp = T_Mem(T_Binop(T_plus, T_Const(SLoff), fp));
		l = l->parent;
	}
	return Tr_Ex(F_exp(vacc, fp));
}

Tr_exp Tr_fieldVar(Tr_access acc, Tr_level l, int cnt){
	Tr_exp e = Tr_simpleVar(acc, l);
	T_exp base = e->u.ex;
	T_exp field = T_Mem(T_Binop(T_plus, 
							T_Binop(T_mul,T_Const(F_wordsize),T_Const(cnt)),base));
	return Tr_Ex(field);
}

Tr_exp Tr_subscriptVar(Tr_access acc, Tr_level l, int off){
	return Tr_fieldVar(acc,l,off);
}