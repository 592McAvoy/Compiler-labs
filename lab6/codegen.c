#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "codegen.h"
#include "table.h"

//Lab 6: put your code here
static void emit(AS_instr inst);
static void munchStm(T_stm stm);
static Temp_temp munchExp(T_exp exp);
static Temp_tempList munchArgs(int cnt, T_expList args);

static string fsStr = NULL;

//helper func
static bool isPlainExp(T_exp e){
    return e->kind == T_CONST || e->kind == T_TEMP ;
}

static Temp_temp RMfp(){
    Temp_temp n = Temp_newtemp();
    char s[100];
    sprintf(s, "leaq %s(`s0), `d0", fsStr);
    emit(AS_Move(String(s), Temp_TempList(n,NULL), Temp_TempList(F_SP(),NULL)));
    return n;
}

static void MemOp(Temp_temp src, Temp_temp dst, T_exp e){
    if(src == F_FP()) src = RMfp();
    if(dst == F_FP()) dst = RMfp();
    switch(e->kind){
        case T_CONST:{
            char mstr[100];
            sprintf(mstr,"movq %d(`s0), `d0", e->u.CONST);
            emit(AS_Move(String(mstr), Temp_TempList(dst,NULL), Temp_TempList(src,NULL)));
            return;
        }
        case T_TEMP:{
            Temp_temp t = e->u.TEMP;
            if(t == F_FP()){
                t = RMfp();
            }
            string mstr = "movq (`s0,`s1), `d0";
            emit(AS_Move(mstr, Temp_TempList(dst,NULL), Temp_TempList(src,Temp_TempList(t, NULL))));
            return;
        }
    }
}
static void MovOp(Temp_temp src, Temp_temp dst, T_exp e){
    if(src == F_FP()) src = RMfp();
    if(dst == F_FP()) dst = RMfp();
    switch(e->kind){
        case T_CONST:{
            char mstr[100];
            sprintf(mstr,"movq `s0, %d(`s1)", e->u.CONST);
            //printf("%s\n",mstr);
            emit(AS_Move(String(mstr), NULL, Temp_TempList(src,Temp_TempList(dst,NULL))));
            return;
        }
        case T_TEMP:{
            Temp_temp t = e->u.TEMP;
            if(t == F_FP()){
                t = RMfp();
            }
            string mstr = "movq `s0, (`s1,`s2)";
            emit(AS_Move(mstr, NULL, Temp_TempList(t,Temp_TempList(dst,Temp_TempList(e->u.TEMP, NULL)))));
            return;
        }
    }
}

static void munchStm(T_stm stm){
    switch(stm->kind){
        case T_SEQ:{
            EM_impossible("T_SEQ stm should not exist");
            return;            
        }
        case T_LABEL:{
            Temp_label LABEL = stm->u.LABEL;
            emit(AS_Label(Temp_labelstring(LABEL), LABEL));
            return;
        }
        case T_JUMP:{
            T_exp expp = stm->u.JUMP.exp; 
            Temp_labelList jumps = stm->u.JUMP.jumps;
            if(expp->kind != T_NAME){
                EM_impossible("T_JUMP should have a T_NAME exp");
                return;
            }
            emit(AS_Oper("jmp `j0", NULL, NULL, AS_Targets(jumps)));
            return;
        }
        case T_CJUMP:{
            T_relOp op = stm->u.CJUMP.op;            
			Temp_label t = stm->u.CJUMP.true;
            Temp_temp left = munchExp(stm->u.CJUMP.left);
            Temp_temp right = munchExp(stm->u.CJUMP.right);
            emit(AS_Oper("cmpq `s0, `s1", NULL, 
                    Temp_TempList(right, Temp_TempList(left, NULL)),NULL));
            string jstr;
            switch(op){
                case T_eq:jstr="je `j0";break;
                case T_ne:jstr="jne `j0";break;
                case T_lt:jstr="jl `j0";break;
                case T_gt:jstr="jg `j0";break;
                case T_le:jstr="jle `j0";break;
                case T_ge:jstr="jge `j0";break;
		        case T_ult:jstr="jb `j0";break;
                case T_ule:jstr="jbe `j0";break;
                case T_ugt:jstr="ja `j0";break;
                case T_uge:jstr="jae `j0";break;
            }
            emit(AS_Oper(jstr, NULL, NULL, AS_Targets(Temp_LabelList(t, NULL))));
            return;
        }
        case T_EXP:{
            Temp_temp s = munchExp(stm->u.EXP);
            Temp_temp d = Temp_newtemp();
            emit(AS_Move("movq `s0, `d0", Temp_TempList(d,NULL), Temp_TempList(s,NULL)));
            return;
        }
        case T_MOVE:{
            Temp_temp s = munchExp(stm->u.MOVE.src);
            T_exp dst = stm->u.MOVE.dst;
            if(dst->kind == T_TEMP){
                emit(AS_Move("movq `s0, `d0", Temp_TempList(dst->u.TEMP,NULL), Temp_TempList(s,NULL)));
                return;
            }
            if(dst->kind == T_MEM){
                T_exp MEM = dst->u.MEM;
                if(MEM->kind == T_TEMP){
                    Temp_temp TEMP = MEM->u.TEMP;
                    emit(AS_Move("movq `s0, (`s1)", NULL,Temp_TempList(s,Temp_TempList(TEMP,NULL))));
                    return;
                }     
                if(MEM->kind == T_CONST){
                    Temp_temp d = munchExp(MEM);
                    emit(AS_Move("movq `s0, (`s1)", NULL,Temp_TempList(s,Temp_TempList(d,NULL))));
                    return;
                }       
                if(MEM->kind == T_BINOP){
                    T_binOp op = MEM->u.BINOP.op;
                    T_exp left = MEM->u.BINOP.left;
                    T_exp right = MEM->u.BINOP.right;
                    if(!isPlainExp(left) && !isPlainExp(right)){
                        Temp_temp d = munchExp(MEM);
                        emit(AS_Move("movq `s0, (`s1)", NULL ,Temp_TempList(s,Temp_TempList(d,NULL))));
                        return;
                    }
                    if(!isPlainExp(left)){
                        Temp_temp d = munchExp(left);
                        MovOp(s,d,right);
                        return;
                    }
                    if(!isPlainExp(right)){
                        Temp_temp d = munchExp(right);
                        MovOp(s, d, left);
                        return;
                    }
                    else{
                        if(right->kind == T_TEMP)
                            MovOp(s, right->u.TEMP, left);
                        else if(left->kind == T_TEMP)
                            MovOp(s, left->u.TEMP, right);
                        else{
                            EM_impossible("unexpected MEM(CONST op CONST)");assert(0);
                        }
                        return;
                    }
                }
            }
            EM_impossible("T_MOVE dst must be T_MEM or T_TEMP");assert(0);
            return;
        }
	    
    }
}

static Temp_temp munchExp(T_exp e){
    switch(e->kind){        
        case T_BINOP:{  /*op S, D  :  D = D op S */
            T_binOp op = e->u.BINOP.op;
            Temp_temp left = munchExp(e->u.BINOP.left);
            Temp_temp right = munchExp(e->u.BINOP.right);
            string opstr;
            switch(op){
               case T_plus:opstr = "addq `s1, `d0";break;
               case T_minus:opstr = "subq `s1, `d0";break;
               case T_mul:opstr = "imulq `s1, `d0";break;
               case T_div:{
                   emit(AS_Move("movq `s0, `d0",Temp_TempList(F_RV(),NULL),Temp_TempList(left,NULL)));
                   emit(AS_Oper("cqto",Temp_TempList(F_RV(),Temp_TempList(F_ARG(2), NULL)),NULL,NULL));
                   emit(AS_Oper("idivq `s0", Temp_TempList(F_RV(),Temp_TempList(F_ARG(2), NULL)), Temp_TempList(right,NULL), NULL));
                   return F_RV();
               } 
               case T_and: case T_or:
               case T_lshift: case T_rshift: case T_arshift:
               case T_xor:EM_impossible("unsupported T_Binop operation");assert(0);
            }

            if(e->u.BINOP.left->kind == T_TEMP){
                Temp_temp n = Temp_newtemp();
                emit(AS_Move("movq `s0, `d0", Temp_TempList(n,NULL), Temp_TempList(left,NULL)));
            
                emit(AS_Oper(opstr, Temp_TempList(n, NULL),
                        Temp_TempList(n, Temp_TempList(right,NULL)), NULL));
                return n;
            }
            
            emit(AS_Oper(opstr, Temp_TempList(left, NULL),
                        Temp_TempList(left, Temp_TempList(right,NULL)), NULL));
            return left;
        }
        case T_MEM:{
            T_exp MEM = e->u.MEM;
            Temp_temp dst = Temp_newtemp();
            if(MEM->kind == T_TEMP){
                Temp_temp TEMP = MEM->u.TEMP;
                if(TEMP == F_FP()){
                    TEMP = RMfp();
                }
                emit(AS_Move("movq (`s0), `d0", Temp_TempList(dst,NULL),Temp_TempList(TEMP,NULL)));
                return dst;
            }     
            if(MEM->kind == T_CONST){
                Temp_temp src = munchExp(MEM);
                emit(AS_Move("movq (`s0), `d0", Temp_TempList(dst,NULL),Temp_TempList(src,NULL)));
                return dst;
            }       
            if(MEM->kind == T_BINOP){
                T_binOp op = MEM->u.BINOP.op;
                T_exp left = MEM->u.BINOP.left;
                T_exp right = MEM->u.BINOP.right;
                if(!isPlainExp(left) && !isPlainExp(right)){
                    Temp_temp src = munchExp(MEM);
                    emit(AS_Move("movq (`s0), `d0", Temp_TempList(dst,NULL),Temp_TempList(src,NULL)));
                    return dst;
                }
                if(!isPlainExp(left)){
                    Temp_temp src = munchExp(left);
                    MemOp(src, dst, right);
                    return dst;
                }
                if(!isPlainExp(right)){
                    Temp_temp src = munchExp(right);
                    MemOp(src, dst, left);
                    return dst;
                }
                else{
                    if(right->kind == T_TEMP)
                        MemOp(right->u.TEMP, dst, left);
                    else if(left->kind == T_TEMP)
                        MemOp(left->u.TEMP, dst, right);
                    else{
                        EM_impossible("unexpected MEM(CONST op CONST)");assert(0);
                    }
                    return dst;
                }
            }
            else{
                EM_impossible("unexpected MEM( exp )");
                assert(0);
            }
        }
        case T_TEMP:{
            Temp_temp t = e->u.TEMP;
            if (t == F_FP()){
                t=RMfp();
            }
           return t;
        }
        case T_ESEQ:{
            EM_impossible("T_ESEQ exp should not exist");
            assert(0);
        }
        case T_NAME:{
            //EM_impossible("T_NAME exp should exist in JUMP/SJUMP/CALL, or string representation wait to be complement");
            //assert(0);
            Temp_label NAME = e->u.NAME;
            Temp_temp dst = Temp_newtemp();
            char str[100];
            sprintf(str,"leaq %s, `d0", Temp_labelstring(NAME));
            emit(AS_Oper(String(str), Temp_TempList(dst, NULL), NULL, NULL));
            return dst;
        }
		case T_CONST:{
            Temp_temp d = Temp_newtemp();
            char cstr[100];
            sprintf(cstr, "movq $%d, `d0", e->u.CONST);
            emit(AS_Move(String(cstr), Temp_TempList(d,NULL), NULL));
            return d;
        }
        case T_CALL:{
            T_exp fun = e->u.CALL.fun; 
            T_expList args = e->u.CALL.args;
            Temp_tempList regs = munchArgs(0, args);
            Temp_tempList calldefs = Temp_TempList(F_RV(), F_callerSave());
            if(fun->kind != T_NAME){
                EM_impossible("T_CALL func name must be a label");assert(0);
            }
            char call[100];
            sprintf(call, "call %s", Temp_labelstring(fun->u.NAME));
            emit(AS_Oper(String(call), calldefs, regs, NULL));
            return F_RV();
        }
    }
}

static Temp_tempList munchArgs(int cnt, T_expList args){
    if(!args)
        return NULL;
    
    T_exp arg = args->head;
    //printf("cnt %d\n",cnt);
    Temp_temp src = munchExp(arg);
    if(cnt < 6){
        Temp_temp dst = F_ARG(cnt);
        Temp_tempList tl = Temp_TempList(dst, munchArgs(cnt+1, args->tail));
        //emit(AS_Oper("pushq `s0", NULL, Temp_TempList(dst, NULL), NULL));
        emit(AS_Move("movq `s0, `d0", Temp_TempList(dst,NULL),Temp_TempList(src,NULL)));
        return tl;
    }
    else{
        munchArgs(cnt+1, args->tail);
        char str[100];
        sprintf(str, "movq `s0, %d(`s1)", (cnt-6)*F_wordsize);
        emit(AS_Move(String(str), NULL, Temp_TempList(src,Temp_TempList(F_SP(),NULL))));
        return NULL;
    }

}




static AS_instrList asList, Last;
static void emit(AS_instr inst) {
    if(!Last){
        Last = AS_InstrList(inst, NULL);
        asList = Last;
    }
    else{
        Last->tail = AS_InstrList(inst, NULL);
        Last = Last->tail;
    }
}


AS_instrList F_codegen(F_frame f, T_stmList stmList) {
    asList = NULL;
    Last = NULL;
    AS_instrList list; 

    char tmp[100];
    sprintf(tmp, "%s_framesize", Temp_labelstring(F_name(f)));
    fsStr = String(tmp);

    /*miscellaneous initializations as necessary */
    char init[100];
    sprintf(init, "subq $%s, `d0", fsStr);
    emit(AS_Oper(String(init), Temp_TempList(F_SP(),NULL), Temp_TempList(F_SP(),NULL), NULL));

    for (T_stmList sl=stmList; sl; sl = sl->tail){
        T_stm stm = sl->head;
        munchStm(stm);
    }  
    char fini[100];
    sprintf(fini, "addq $%s, `d0", fsStr);
    emit(AS_Oper(String(fini), Temp_TempList(F_SP(),NULL), Temp_TempList(F_SP(),NULL), NULL));

    list = F_procEntryExit2(asList);
    return list ;
}

