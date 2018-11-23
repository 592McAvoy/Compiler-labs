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

/* Tr_expList*/
struct Tr_expList_{
	Tr_exp head;
	Tr_expList tail;
};
Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail){
	Tr_expList l = checked_malloc(sizeof(*l));

	l->head = head;
	l->tail = tail;
	return l;
}

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
	while(l != vl){
		fp = T_Mem(T_Binop(T_plus, T_Const(SLoff), fp));
		l = l->parent;
	}
	return Tr_Ex(F_exp(vacc, fp));
}

Tr_exp Tr_fieldVar(Tr_access acc, Tr_level l, int cnt){
	Tr_exp e = Tr_simpleVar(acc, l);
	T_exp base = unEx(e);
	int offset = F_wordsize * cnt;
	T_exp field = T_Mem(T_Binop(T_plus, T_Const(offset),base));
	return Tr_Ex(field);
}

Tr_exp Tr_subscriptVar(Tr_access acc, Tr_level l, int off){
	return Tr_fieldVar(acc,l,off);
}

//transExp
Tr_exp Tr_nilExp(){
    return Tr_Ex(T_Const(0));
}
Tr_exp Tr_intExp(int i){
    return Tr_Ex(T_Const(i));
}
Tr_exp Tr_stringExp(string str){
    Temp_label lab = Temp_newlabel();
    F_frag strf = F_StringFrag(lab, str);
    //add strf to global frag table
    return Tr_Ex(T_Name(lab));
}
Tr_exp Tr_callExp(Temp_label fname, Tr_expList params, Tr_level fl, Tr_level envl){
    T_expList args = NULL;
    for(Tr_expList l=params; l; l=l->tail){
        Tr_exp param = l->head;
        args = T_ExpList(param, args);
    }

    //calculate SL
    Tr_level target = fl->parent;
    T_exp fp = T_Temp(F_FP());//addr of current fp	
	int SLoff = F_wordsize;//SL is 1 wordsize off FP
	while(envl != target){
		fp = T_Mem(T_Binop(T_plus, T_Const(SLoff), fp));
		envl = envl->parent;
	}
    args = T_ExpList(fp, args);

    return Tr_Ex(T_Call(T_NAME(fname), args));
}
Tr_exp Tr_arithExp(A_oper op, Tr_exp left, Tr_exp right){
    T_binOp bop;
    switch(op){
        case A_plusOp:bop=T_plus;break;
        case A_minusOp:bop=T_minus;break;
        case A_timesOp:bop=T_mul;break;
        case A_divideOp:bop=T_div;break;
    }
    T_exp e = T_Binop(bop,unEx(left),unEx(right));
    return Tr_Ex(e);
}
Tr_exp Tr_intCompExp(A_oper op, Tr_exp left, Tr_exp right){
    T_relOp rop;
    switch(op){
        case A_eqOp:rop=T_eq;break;
        case A_neqOp:rop=T_ne;break;
        case A_ltOp:rop=T_lt;break;
        case A_leOp:rop=T_le;break;
        case A_gtOp:rop=T_gt;break;
        case A_geOp:rop=T_ge;break;
    }
    T_stm s = T_Cjump(rop, unEx(left), unEx(right),NULL, NULL);
    patchList trues = PatchList(&(s->u.CJUMP.true),NULL);
    patchList falses = PatchList(&(s->u.CJUMP.false),NULL);
    
    return Tr_Cx(trues,falses,s);
}
Tr_exp Tr_strCompExp(A_oper op, Tr_exp left, Tr_exp right){
    T_relOp rop;
    switch(op){
        case A_ltOp:
        case A_leOp:
        case A_gtOp:
        case A_geOp:return Tr_intCompExp(op, left, right)
        case A_eqOp:rop=T_eq;break;
        case A_neqOp:rop=T_ne;break;
    }
    T_exp func = F_externalCall("stringEqual", T_ExpList(unEx(left),T_ExpList(unEx(right))));
    T_stm s T_Cjump(rop, func, T_Const(1), NULL, NULL);
    patchList trues = PatchList(&(s->u.CJUMP.true),NULL);
    patchList falses = PatchList(&(s->u.CJUMP.false),NULL);
    return Tr_Cx(trues,falses,s);  
}
Tr_exp Tr_ptrCompExp(A_oper op, Tr_exp left, Tr_exp right){
    return Tr_intCompExp(op,left,right);
}
Tr_exp Tr_recordExp(Tr_expList list, int cnt){
    Temp_temp r = Temp_newtemp();
    T_exp base = T_Temp(r);

    T_stm fill;
    for(int i=1,Tr_expList lp=list; i<=cnt; i++, lp=lp->tail){
        Tr_exp e = lp->head;
        int off = (cnt-i) * F_wordsize;
        
        T_stm move = T_Move(T_Mem(T_Binop(T_plus, T_Const(off), base)),unEx(e));

        if(i == 1)
            fill = move;
        else
            fill = T_Seq(move, s);
    }

    int total = cnt * F_wordsize;
    T_stm init = T_Move(base, F_externalCall("malloc", T_ExpList(T_Const(total),NULL)));

    T_exp finall = T_Eseq(T_Seq(init,fill), base); 
    return Tr_Ex(finall);    
}
Tr_exp Tr_assignExp(Tr_exp pos, Tr_exp val){
    return Tr_Nx(T_Move(unEx(pos), unEx(val)));
}
Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee){
    Temp_label t = Temp_newlabel();
    Temp_label f = Temp_newlabel();
    struct Cx testCx = unCx(test);
    doPatch(testCx.trues, t);
    doPatch(testCx.falses,f);

    T_stm testStm = testCx.stm;//Cx(t,f)

    T_stm thenStm;//t
    if(then->kind == Tr_cx){
        thenStm = then->u.cx.stm;
    }
    else{
        thenStm = unNx(then);
    }

    T_stm elseStm;//f
    if(elsee->kind == Tr_cx){
        elseStm = elsee->u.cx.stm;
    }
    else{
        elseStm = unNx(elsee);
    }

    T_stm s = T_Seq(testStm,
                    T_Seq(T_Label(t),
                        T_Seq(thenStm,
                            T_Seq(T_label(f), elseStm))));
    return Tr_Nx(s);
}
Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body, Temp_label done){
    Temp_label start = Temp_newlabel();
    
    struct Cx testCx = unCx(test);
    doPatch(testCx.trues,start);
    doPatch(testCx.falses,done);
    T_stm testStm = testCx.stm;

    T_stm s = T_Seq(T_Label(start),
                T_Seq(testStm,
                    T_Seq(unNx(body,
                        T_Seq(T_Jump(T_Name(start),Temp_LabelList(start,NULL)),
                            T_Label(done))))));
    return Tr_Nx(s);
}
Tr_exp Tr_forExp(Tr_exp lo, Tr_exp hi, Tr_exp body, Temp_label done){
    T_exp low = unEx(lo);
    T_exp high = unEx(hi);    
    Temp_temp r = Temp_newtemp();
    T_exp i = T_Temp(r);
    Temp_label loop = Temp_newlabel();
    Temp_label pass = Temp_newlabel();

    T_stm init = T_Move(i,low);
    T_stm test = T_Cjump(T_le, i, high, T_Name(loop), T_Name(done));
    T_stm update = T_Binop(T_plus, i, T_Const(1));
    T_stm bodyy = unNx(body);

    T_stm s = T_Seq(init,
                T_Seq(T_Cjump(T_le, i, high, T_Name(pass),T_Name(done)),
                    T_Seq(T_Label(pass),
                        T_Seq(boddy,
                            T_Seq(T_Cjump(T_ne, i, high, T_Name(loop), T_Name(done)),
                                T_Seq(T_Label(loop),
                                    T_Seq(update,
                                        T_Seq(boddy, 
                                            T_Seq(test, T_Label(done))))))))));
    return Tr_Nx(s);

}
Tr_exp Tr_breakExp(Temp_label done){
    return Tr_nx(T_Jump(T_Name(done),Temp_LabelList(done,NULL)));
}
Tr_exp Tr_arrayExp(int size, Tr_exp initvar){
    Temp_temp r = Temp_newtemp();
    T_exp base = T_Temp(r);

    T_stm init = T_Move(base, F_externalCall("initArray", T_ExpList(T_Const(size),
                                                               T_ExpList(unEx(initvar),NULL))));

    T_exp finall = T_Eseq(init, base); 
    return Tr_Ex(finall);   
} 