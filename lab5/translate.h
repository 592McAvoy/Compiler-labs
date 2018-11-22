#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "util.h"
#include "absyn.h"
#include "temp.h"
#include "frame.h"

/* Lab5: your code below */

typedef struct Tr_exp_ *Tr_exp;

typedef struct Tr_access_ *Tr_access;

typedef struct Tr_accessList_ *Tr_accessList;

typedef struct Tr_level_ *Tr_level;

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

Tr_level Tr_outermost(void);

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);

Tr_accessList Tr_formals(Tr_level level);

Tr_access Tr_allocLocal(Tr_level level, bool escape);

/* IR translation 
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
};*/

//transVar
Tr_exp Tr_simpleVar(Tr_access acc, Tr_level l);
Tr_exp Tr_fieldVar(Tr_access acc, Tr_level l, int cnt);
Tr_exp Tr_subscriptVar(Tr_access acc, Tr_level l, int off);

//transExp
Tr_exp Tr_nilExp(){
    return Tr_Ex(T_Const(0));
}
Tr_exp Tr_intExp(int i){
    return Tr_Ex(T_Const(i));
}
Tr_exp Tr_stringExp;
Tr_exp Tr_callExp;
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
    patchList trues = PatchList(&get_cjump_true(s),NULL);
    patchList falses = PatchList(&get_cjump_false(s),NULL);
    return Tr_Cx(trues,falses,s);
}
Tr_exp Tr_strCompExp(A_oper op, Tr_exp left, Tr_exp right){
    T_relOp rop;
    switch(op){
        case A_eqOp:rop=T_eq;break;
        case A_neqOp:rop=T_ne;break;
        case A_ltOp:rop=T_lt;break;
        case A_leOp:rop=T_le;break;
        case A_gtOp:rop=T_gt;break;
        case A_geOp:rop=T_ge;break;
    }
    //to be realize;
    return NULL;
}
Tr_exp Tr_ptrCompExp(A_oper op, Tr_exp left, Tr_exp right){
    T_relOp rop;
    switch(op){
        case A_eqOp:rop=T_eq;break;
        case A_neqOp:rop=T_ne;break;
        case A_ltOp:rop=T_lt;break;
        case A_leOp:rop=T_le;break;
        case A_gtOp:rop=T_gt;break;
        case A_geOp:rop=T_ge;break;
    }
    //to be realize;
    return NULL;
}
Tr_exp Tr_recordExp;
Tr_exp Tr_assignExp;
Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee){
    Temp_label t = Temp_newlabel();
    Temp_label f = Temp_newlabel();
    struct Cx testCx = unCx(test);
    doPatch(testCx.u.trues, t);
    doPatch(testCx.u.falses,f);

    T_stm testStm = testCx.stm;//Cx(t,f)
    T_stm thenStm = unNx(then);//t
    T_stm elseStm = unNx(elsee);//f

    T_stm s = T_Seq(T_Seq(testStm,
                        T_Seq(T_Label(t),
                            T_Seq(thenStm,
                                T_Seq(T_label(f), elseStm)))));
}
#endif
