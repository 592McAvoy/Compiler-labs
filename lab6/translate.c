#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "translate.h"

//LAB5: you can modify anything you want.

F_fragList funcfrags = NULL;
F_fragList strfrags = NULL;

struct Tr_access_ {
	Tr_level level;
	F_access access;
};

struct Tr_level_ {
	F_frame frame;
	Tr_level parent;
};

/* Tr_expList*/

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
		case Tr_nx:{
            cx.trues = NULL;
            cx.falses = NULL;
            cx.stm = NULL;
            printf("error UnCx(T_stm)\n");
			/*error*/ return cx;
        }
		case Tr_cx:
			return e->u.cx;
	}
}

void Tr_print(Tr_exp e){
    printStmList(stderr, T_StmList(unNx(e),NULL));
}




/* part I */
Tr_level Tr_outermost(void){
	Tr_level l = checked_malloc(sizeof(*l));

	Temp_label lab = Temp_namedlabel("tigermain");
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

Tr_accessList makeFormalsT(F_accessList fl, Tr_level level){
	Tr_access ac = checked_malloc(sizeof(*ac));

	ac->level = level;
	ac->access = fl->head;

	if(fl->tail){
		return Tr_AccessList(ac, makeFormalsT(fl->tail, level));
	}
	else{
		return Tr_AccessList(ac, NULL);
	}
}

Tr_accessList Tr_formals(Tr_level level){
	F_frame f = level->frame;
	F_accessList fl = F_formals(f);
	return makeFormalsT(fl, level);
}

Tr_exp Tr_err(){
    return Tr_Ex(T_Const(0));
}

//transVar
Tr_exp Tr_simpleVar(Tr_access acc, Tr_level l){
	Tr_level vl = acc->level;
	F_access vacc = acc->access;
	T_exp fp = T_Temp(F_FP());//addr of current fp

	//calculate SL
	int SLoff = -F_wordsize;//SL is 1 wordsize off FP
	while(l != vl){
		fp = T_Mem(T_Binop(T_plus, T_Const(SLoff), fp));
		l = l->parent;
	}
	return Tr_Ex(F_exp(vacc, fp));
}

Tr_exp Tr_fieldVar(Tr_exp base, int cnt){
	T_exp b = unEx(base);
	int offset = F_wordsize * cnt;
	T_exp field = T_Mem(T_Binop(T_plus, T_Const(offset), b));
	return Tr_Ex(field);
}

Tr_exp Tr_subscriptVar(Tr_exp base, Tr_exp off){
	T_exp b = unEx(base);
	T_exp field = T_Mem(T_Binop(T_plus, 
                            T_Binop(T_mul, T_Const(F_wordsize), unEx(off)), b));
	return Tr_Ex(field);
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
    strfrags = F_FragList(strf, strfrags);
    return Tr_Ex(T_Name(lab));
}
Tr_exp Tr_callExp(Temp_label fname, Tr_expList params, Tr_level fl, Tr_level envl, string func){
    T_expList args = NULL;
    F_frame envf = envl->frame;
    int cnt = 0;
    for(Tr_expList l=params; l; l=l->tail){
        Tr_exp param = l->head;
        args = T_ExpList(unEx(param), args);
        cnt+=1;
    }

    //alloc space for spilled args
    while(cnt>5){
        F_allocLocal(envf, TRUE);
        cnt -= 1;
    }
    
    if(!fname){
        return Tr_Ex(F_externalCall(func, args));
    }
    
    //calculate SL
    Tr_level target = fl->parent;
    T_exp fp = T_Temp(F_FP());//addr of current fp	
	int SLoff = -F_wordsize;//SL is 1 wordsize off FP
	while(envl != target){
		fp = T_Mem(T_Binop(T_plus, T_Const(SLoff), fp));
		envl = envl->parent;
	}
    args = T_ExpList(fp, args);

    return Tr_Ex(T_Call(T_Name(fname), args));
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
        case A_geOp:return Tr_intCompExp(op, left, right);
        case A_eqOp:rop=T_eq;break;
        case A_neqOp:rop=T_ne;break;
    }
    T_exp func = F_externalCall("stringEqual", T_ExpList(unEx(left),T_ExpList(unEx(right),NULL)));
    T_stm s = T_Cjump(rop, func, T_Const(1), NULL, NULL);
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
    Tr_expList lp;
    int i;
    for(i=1,lp=list; i<=cnt; i++, lp=lp->tail){
        Tr_exp e = lp->head;
        int off = (cnt-i) * F_wordsize;
        
        T_stm move = T_Move(T_Mem(T_Binop(T_plus, T_Const(off), base)),unEx(e));

        if(i == 1)
            fill = move;
        else
            fill = T_Seq(move, fill);
    }

    int total = cnt * F_wordsize;
    T_stm init = T_Move(base, F_externalCall("malloc", T_ExpList(T_Const(total),NULL)));

    T_exp finall = T_Eseq(T_Seq(init,fill), base); 
    
    return Tr_Ex(finall);    
}
Tr_exp Tr_SeqExp(Tr_expList list){
    T_exp e = unEx(list->head);
    T_exp s = NULL;
    Tr_expList p;
    for(p=list->tail;p;p=p->tail){
        if(s){
            s = T_Eseq(unNx(p->head),s);
        }
        else{
            s = T_Eseq(unNx(p->head),e);
        }
    }
    if(!s){
        return Tr_Ex(e);
    }
    return Tr_Ex(s);
}
Tr_exp Tr_assignExp(Tr_exp pos, Tr_exp val){
    return Tr_Nx(T_Move(unEx(pos), unEx(val)));
}
Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee, Ty_ty type){
    Temp_label t = Temp_newlabel();
    Temp_label f = Temp_newlabel();
    Temp_label next = Temp_newlabel();

    struct Cx testCx = unCx(test);
    doPatch(testCx.trues, t);
    doPatch(testCx.falses,f);

    T_stm testStm = testCx.stm;//Cx(t,f)

    if(type->kind == Ty_void){
        T_stm thenStm = unNx(then);
        T_stm elseStm = unNx(elsee);
        T_exp e = T_Eseq(testStm,
                T_Eseq(T_Label(t),
                    T_Eseq(thenStm, 
                        T_Eseq(T_Jump(T_Name(next), Temp_LabelList(next,NULL)),
                            T_Eseq(T_Label(f),
                                T_Eseq(elseStm,
                                    T_Eseq(T_Jump(T_Name(next), Temp_LabelList(next,NULL)),
                                        T_Eseq(T_Label(next), T_Const(0)))))))));

        return Tr_Nx(T_Exp(e));
    }

    T_exp thenexp = unEx(then);
    T_exp elseexp = unEx(elsee);

    Temp_temp r = Temp_newtemp();
    
    T_exp e = T_Eseq(testStm,
                T_Eseq(T_Label(t),
                    T_Eseq(T_Move(T_Temp(r),thenexp), 
                        T_Eseq(T_Jump(T_Name(next), Temp_LabelList(next,NULL)),
                            T_Eseq(T_Label(f),
                                T_Eseq(T_Move(T_Temp(r),elseexp), 
                                    T_Eseq(T_Jump(T_Name(next), Temp_LabelList(next,NULL)),
                                        T_Eseq(T_Label(next), T_Temp(r)))))))));
    return Tr_Ex(e);
}
Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body, Temp_label done){
    Temp_label check = Temp_newlabel();
    Temp_label run = Temp_newlabel();
    
    struct Cx testCx = unCx(test);
    
    doPatch(testCx.trues,run);
    doPatch(testCx.falses,done);
    
    T_stm testStm = testCx.stm;

    T_exp e = T_Eseq(T_Label(check),
                T_Eseq(testStm,
                    T_Eseq(T_Label(run),
                        T_Eseq(unNx(body),
                            T_Eseq(T_Jump(T_Name(check),Temp_LabelList(check,NULL)),
                                T_Eseq(T_Label(done),T_Const(0)))))));
    
    return Tr_Nx(T_Exp(e));
}
Tr_exp Tr_forExp(Tr_exp forvar, Tr_exp lo, Tr_exp hi, Tr_exp body, Temp_label done){
    T_exp low = unEx(lo);
    T_exp high = unEx(hi);    
    T_exp i = unEx(forvar);

    Temp_label loop = Temp_newlabel();
    Temp_label pass = Temp_newlabel();

    T_stm init = T_Move(i,low);
    T_stm update = T_Move(i,T_Binop(T_plus, i, T_Const(1)));
    T_stm bodyy1 = unNx(body);
    T_stm bodyy2 = unNx(body);

    /*
    T_exp e = T_Eseq(init,
                T_Eseq(T_Cjump(T_gt, i,high, done, pass),
                    T_Eseq(T_Label(pass),
                        T_Eseq(unNx(body),
                            T_Eseq(T_Cjump(T_eq, i, high, done, loop),
                                T_Eseq(T_Label(loop), 
                                    T_Eseq(update,
                                        T_Eseq(bodyy2,
                                            T_Eseq(T_Cjump(T_le, i, high, loop, done),
                                                T_Eseq(T_Label(done), T_Const(0)))))))))));*/
    T_exp e = T_Eseq(init,
                    T_Eseq(T_Cjump(T_gt, i, high, done, loop),
                                T_Eseq(T_Label(loop), 
                                    T_Eseq(bodyy2,
                                        T_Eseq(update,
                                            T_Eseq(T_Cjump(T_le, i, high, loop, done),
                                                T_Eseq(T_Label(done), T_Const(0))))))));
    //return Tr_Nx(fin);
    return Tr_Nx(T_Exp(e));

}
Tr_exp Tr_breakExp(Temp_label done){
    return Tr_Nx(T_Jump(T_Name(done),Temp_LabelList(done,NULL)));
}
Tr_exp Tr_letExp(Tr_expList dec, Tr_exp body){
    T_exp e = unEx(body);
    T_exp s = NULL;
    for(Tr_expList p=dec;p;p=p->tail){
        if(s){
            s = T_Eseq(unNx(p->head), s);
        }
        else{
            s = T_Eseq(unNx(p->head), e);
        }
    }
    if(!s){
        return body;
    }
    return Tr_Ex(s);
}
Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp initvar){
    Temp_temp r = Temp_newtemp();
    T_exp base = T_Temp(r);
    T_stm init = T_Move(base, F_externalCall("initArray", T_ExpList(unEx(size),
                                                               T_ExpList(unEx(initvar),NULL))));
    //T_stm init = T_Move(base, F_externalCall("malloc", T_ExpList(T_Binop(T_mul, T_Const(F_wordsize),unEx(size)),NULL)));
    T_exp finall = T_Eseq(init, base); 
    return Tr_Ex(finall);   
} 

//transDec
Tr_exp Tr_typeDec(){
    return Tr_Ex(T_Const(0));
}
Tr_exp Tr_varDec(Tr_access acc, Tr_exp init){
    T_exp pos = unEx(Tr_simpleVar(acc, acc->level));
    return Tr_Nx(T_Move(pos, unEx(init)));
}
//wait to be re-design
Tr_exp Tr_functionDec(Temp_label fname, Tr_level l, Tr_exp fbody){
    /*T_exp SP = T_Temp(F_SP());
    T_exp FP = T_Temp(F_FP());
    T_exp PC = T_Temp(F_PC());
    T_exp RV = T_Temp(F_RV());
    //Prologue
    //1.Pseudo-instructions to announce the beginning of a function;
    //2.A label definition of the function name
    T_stm s1 = T_Label(fname);
    //3.adjust FP,SP
    T_exp updateSP = T_Binop(T_minus, T_Const(F_wordsize), SP);
    T_stm agjust = T_Seq(T_Exp(updateSP),
                        T_Seq(T_Move(T_Mem(SP),FP),//static link
                            T_Seq(T_Exp(updateSP),
                                T_Seq(T_Move(T_Mem(SP),T_Binop(T_plus, T_Const(1), PC)),//return addr
                                    T_Seq(T_Move(FP, SP),//new frame
                                    T_Move(PC, T_Name(fname)))))));//move PC
    //adjust SP
    //4.enter params    
    //5.Store instructions to save any callee-saved registers
    //6.The function body
    /*printf("function body:\n");
    Tr_print(fbody);
    printf("\n\n");*/

    //Epilogue
    //7.An instruction to move the return value to RA register
    /*T_stm moveRet = T_Move(RV, unEx(fbody));
    //8.Load instructions to restore the callee-save registers
    //9.An instruction to reset the stack pointer (to deallocate the frame)
    //10.A return instruction (Jump to the return address)
    T_stm ret = T_Seq(T_Move(PC, T_Mem(FP)),//restore PC
                    T_Seq(T_Move(SP, T_Mem(T_Binop(T_plus,T_Const(2*F_wordsize),FP))),//restore SP
                    T_Move(FP, T_Mem(T_Binop(T_plus,T_Const(F_wordsize),FP)))));//restore FP
    //11.Pseduo-instructions, as needed, to announce the end of a function
*/
    return Tr_Ex(T_Const(0));
}

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals){
	F_frame f = level->frame;
	//fill holes
	T_stm s = T_Move(T_Temp(F_RV()), unEx(body)); 
    s = F_procEntryExit1(f, s);

	F_frag proc = F_ProcFrag(s, f);

	funcfrags = F_FragList(proc, funcfrags);
}

F_fragList Tr_getResult(void){
    F_fragList p = strfrags;
    if(!p) return funcfrags;
    for(;p->tail;p=p->tail){}
    p->tail = funcfrags;
	return strfrags;
}