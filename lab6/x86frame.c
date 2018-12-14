#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"

/*Lab5: Your implementation here.*/

const int F_wordsize = 8;

//F_frame_ in PPT
struct F_frame_ {
	Temp_label name;
	
	F_accessList formals;
	F_accessList locals;

	//the number of arguments
	int argSize;
	
	//the number of local variables
	int length;

	//register lists for the frame
	F_accessList calleesaves;
	F_accessList callersaves;
};

//varibales
struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset; //inFrame
		Temp_temp reg; //inReg
	} u;
};

/* functions */
static F_access InFrame(int offset){
	F_access ac = checked_malloc(sizeof(*ac));

	ac->kind = inFrame;
	ac->u.offset = offset;
	return ac;
}   
int F_getFrameOff(F_access acc){
	return acc->u.offset;
}

static F_access InReg(Temp_temp reg){
	F_access ac = checked_malloc(sizeof(*ac));

	ac->kind = inReg;
	ac->u.reg = reg;
	return ac;
}

F_accessList F_AccessList(F_access head, F_accessList tail){
	F_accessList l = checked_malloc(sizeof(*l));

	l->head = head;
	l->tail = tail;
	return l;
}

// param position wait to be reset
F_accessList makeFormalsF(F_frame f, U_boolList formals, int* cntp){
	if(!formals){
		return NULL;
	}
	bool esc = formals->head;
	int cnt = *cntp;
	*cntp = cnt+1;

	F_access ac = F_allocLocal(f, esc);//args;
	
	if(formals->tail){
		return F_AccessList(ac, makeFormalsF(f, formals->tail, cntp));
	}
	else{
		return F_AccessList(ac, NULL);
	}
}

F_accessList F_callerPos(F_frame f){
	Temp_tempList regs =  F_callerSave();
	F_accessList l = NULL;
	F_accessList last = NULL;
	for(;regs;regs=regs->tail){
		F_access acc = F_allocLocal(f,FALSE);
		if(!last){
			last = F_AccessList(acc, NULL);
			l = last;
		}
		else{
			last->tail = F_AccessList(acc, NULL);
			last = last->tail;
		}
	}
	return l;
}

F_accessList F_calleePos(F_frame f){
	Temp_tempList regs = F_calleeSave();
	F_accessList l = NULL;
	F_accessList last = NULL;
	for(;regs;regs=regs->tail){
		F_access acc = F_allocLocal(f,FALSE);
		if(!last){
			last = F_AccessList(acc, NULL);
			l = last;
		}
		else{
			last->tail = F_AccessList(acc, NULL);
			last = last->tail;
		}
	}
	return l;
}

F_frame F_newFrame(Temp_label name, U_boolList formals){
	F_frame f = checked_malloc(sizeof(*f));
	f->length = 0;
	int *argsize = checked_malloc(sizeof(int));
	*argsize = 0;
	
	f->name = name;
	f->formals = makeFormalsF(f, formals, argsize);
	f->locals = NULL;
	f->argSize = *argsize;
	

	f->calleesaves = F_calleePos(f);
	f->callersaves = F_callerPos(f);

	return f;
}

//locals position wait to be reset
F_access F_allocLocal(F_frame f, bool escape){
	int length = f->length;
	F_accessList locals = f->locals;

	F_access ac;
	if(escape){
		ac = InFrame(-(length+1) * F_wordsize);
		f->length = length+1;
	}
	else{
		ac = InReg(Temp_newtemp());
	}

	
	f->locals = F_AccessList(ac, locals);

	return ac;
}

Temp_label F_name(F_frame f){
	return f->name;
}
F_accessList F_formals(F_frame f){
	return f->formals;
}
int F_len(F_frame f){
	return f->length;
}
/* IR translation */
Temp_temp F_FP(void){
	static Temp_temp fp  = NULL;
	if(!fp){
		fp = Temp_newtemp();
		Temp_enter(F_tempMap, fp, "fp");
	}
	return fp;
}
Temp_temp F_SP(void){
	static Temp_temp sp  = NULL;
	if(!sp){
		 sp = Temp_newtemp();
		Temp_enter(F_tempMap, sp, "%rsp");
	}
	return sp;
}
Temp_temp F_RV(void){
	static Temp_temp rv = NULL;
	if(!rv){
		rv = Temp_newtemp();
		Temp_enter(F_tempMap, rv, "%rax");
	}
	return rv;
}
Temp_temp F_ARG(int idx){
	static Temp_temp r0 = NULL;
	static Temp_temp r1 = NULL;
	static Temp_temp r2 = NULL;
	static Temp_temp r3 = NULL;
	static Temp_temp r4 = NULL;
	static Temp_temp r5 = NULL;
	if(!r0){
		r0 = Temp_newtemp();
		Temp_enter(F_tempMap, r0, "%rdi");
	}
	if(!r1){
		r1 = Temp_newtemp();
		Temp_enter(F_tempMap, r1, "%rsi");
	}
	if(!r2){
		r2 = Temp_newtemp();
		Temp_enter(F_tempMap, r2, "%rdx");
	}
	if(!r3){
		r3 = Temp_newtemp();
		Temp_enter(F_tempMap, r3, "%rcx");
	}
	if(!r4){
		r4 = Temp_newtemp();
		Temp_enter(F_tempMap, r4, "%r8");
	}
	if(!r5){
		r5 = Temp_newtemp();
		Temp_enter(F_tempMap, r5, "%r9");
	}
	switch(idx){
		case 0:return r0;
		case 1:return r1;
		case 2:return r2;
		case 3:return r3;
		case 4:return r4;
		case 5:return r5;
		default:assert(0);
	}
}
Temp_tempList F_Args(){
	static Temp_tempList args = NULL;
	if(!args){
		args = Temp_TempList(F_ARG(0),
					Temp_TempList(F_ARG(1),
					Temp_TempList(F_ARG(2),
					Temp_TempList(F_ARG(3),
					Temp_TempList(F_ARG(4),
					Temp_TempList(F_ARG(5),NULL))))));
	}
	return args;
}
Temp_tempList F_callerSave(){
	static Temp_tempList callerSave = NULL;
	static Temp_temp r10 = NULL;
	static Temp_temp r11 = NULL;
	if(!callerSave){
		r10 = Temp_newtemp();
		Temp_enter(F_tempMap, r10, "%r10");
		r11 = Temp_newtemp();
		Temp_enter(F_tempMap, r11, "%r11");
		callerSave = Temp_catList(F_Args(), Temp_TempList(r10, Temp_TempList(r11, NULL)));
	}
	return callerSave;
}
Temp_tempList F_calleeSave(){
	static Temp_tempList calleeSave = NULL;
	static Temp_temp r12 = NULL;
	static Temp_temp r13 = NULL;
	static Temp_temp r14 = NULL;
	static Temp_temp r15 = NULL;
	static Temp_temp rbx = NULL;
	static Temp_temp rbp = NULL;
	
	if(!calleeSave){
		r12 = Temp_newtemp();
		Temp_enter(F_tempMap, r12, "%r12");
		r13 = Temp_newtemp();
		Temp_enter(F_tempMap, r13, "%r13");
		r14 = Temp_newtemp();
		Temp_enter(F_tempMap, r14, "%r14");
		r15 = Temp_newtemp();
		Temp_enter(F_tempMap, r15, "%r15");
		rbx = Temp_newtemp();
		Temp_enter(F_tempMap, rbx, "%rbx");
		rbp = Temp_newtemp();
		Temp_enter(F_tempMap, rbp, "%rbp");
		calleeSave = Temp_TempList(r12, 
						Temp_TempList(r13,
							Temp_TempList(r14, 
								Temp_TempList(r15, 
									Temp_TempList(rbx,
										Temp_TempList(rbp, NULL))))));
	}
	return calleeSave;
}
Temp_tempList F_register(){
	static Temp_tempList regs = NULL;
	if(!regs){
		regs = Temp_TempList(F_SP(),
					Temp_TempList(F_RV(),
						F_calleeSave()));
		regs = Temp_catList(regs, F_callerSave());
	}
	return regs;											
}

T_exp F_exp(F_access acc, T_exp framePtr){
	if(acc->kind == inFrame){
		int off = acc->u.offset;
		return T_Mem(T_Binop(T_plus, T_Const(off), framePtr));
	}
	else{
		return T_Temp(acc->u.reg);
	}
}

T_exp F_externalCall(string s, T_expList args){
	return T_Call(T_Name(Temp_namedlabel(s)), args);
}


/* fragment */
F_frag F_StringFrag(Temp_label label, string str) { 
	F_frag f = checked_malloc(sizeof(*f));
	f->kind = F_stringFrag;
	f->u.stringg.label = label;
	f->u.stringg.str = str;

	return f;                                      
}                                                     
                                                      
F_frag F_ProcFrag(T_stm body, F_frame frame) {        
	F_frag f = checked_malloc(sizeof(*f));
	f->kind = F_procFrag;
	f->u.proc.body = body;
	f->u.proc.frame = frame;

	return f;                                     
}                                                     
                                                      
F_fragList F_FragList(F_frag head, F_fragList tail) { 
	F_fragList l = checked_malloc(sizeof(*l));
	l->head = head;
	l->tail = tail;
	return l;                                      
}                                                     

T_exp F_procChange(F_frame f, T_exp call){
	T_exp fp = T_Temp(F_FP());
	//caller save
	T_stm save = NULL;
	F_accessList al = f->callersaves;
	Temp_tempList tl = F_callerSave();
	for(;tl;tl=tl->tail, al=al->tail){
		T_exp pos = F_exp(al->head, fp);
		if(save){
			save = T_Seq(T_Move(pos, T_Temp(tl->head)), save);
		}
		else{
			save = T_Move(pos, T_Temp(tl->head));
		}
	}

	//caller restore
	T_stm restore = NULL;
	al = f->callersaves;
	tl = F_callerSave();
	for(;tl;tl=tl->tail, al=al->tail){
		T_exp pos = F_exp(al->head, fp);
		if(restore){
			restore = T_Seq(T_Move(T_Temp(tl->head),pos), restore);
		}
		else{
			restore = T_Move(T_Temp(tl->head), pos);
		}
	}

	Temp_temp t = Temp_newtemp();
	T_exp e = T_Eseq(save,
				T_Eseq(T_Move(T_Temp(t), call),
					T_Eseq(restore, T_Temp(t))));
	return e;

}

T_stm F_procEntryExit1(F_frame f, T_stm stm){
	//view change
	T_stm view = NULL;
	int cnt = 0;
	T_exp fp = T_Temp(F_FP());
	for(F_accessList l=f->formals;l;l=l->tail){
		F_access arg = l->head;
		T_exp argpos = F_exp(arg,fp);
		switch(cnt){
			case 0:view=T_Move(argpos,T_Temp(F_ARG(cnt)));break;//rdi SL
			case 1://rsi
			case 2://rdx
			case 3://rcx
			case 4://r8
			case 5:view = T_Seq(T_Move(argpos,T_Temp(F_ARG(cnt))),
								view);break;//r9
			default:{
				int off = (cnt-6+1)*F_wordsize ;
				view = T_Seq(
						T_Move(argpos,T_Mem(T_Binop(T_plus, T_Const(off), fp))),view);
			}
		}
		cnt += 1;
	}

	//callee save
	T_stm save = NULL;
	F_accessList al = f->calleesaves;
	Temp_tempList tl = F_calleeSave();
	for(;tl;tl=tl->tail, al=al->tail){
		T_exp pos = F_exp(al->head, fp);
		if(save){
			save = T_Seq(T_Move(pos, T_Temp(tl->head)), save);
		}
		else{
			save = T_Move(pos, T_Temp(tl->head));
		}
	}

	//callee restore
	T_stm restore = NULL;
	al = f->calleesaves;
	tl = F_calleeSave();
	for(;tl;tl=tl->tail, al=al->tail){
		T_exp pos = F_exp(al->head, fp);
		if(restore){
			restore = T_Seq(T_Move(T_Temp(tl->head),pos), restore);
		}
		else{
			restore = T_Move(T_Temp(tl->head), pos);
		}
	}

	if(!view){
		return T_Seq(save,T_Seq(stm, restore));
	}
	return T_Seq(save,T_Seq(view, T_Seq(stm, restore)));
}
AS_instrList F_procEntryExit2(AS_instrList body){
	static Temp_tempList returnSink = NULL ;
	if (!returnSink)  
		returnSink = Temp_TempList(F_SP(), F_calleeSave());
    return AS_splice(body, 
				AS_InstrList(AS_Oper("ret", NULL, returnSink, NULL), NULL));

}
AS_proc F_procEntryExit3(F_frame frame, AS_instrList body){
	int len = frame->length;
	string fn =  S_name(F_name(frame));
	char target[100];
	char length[20];
	sprintf(target, "%s_framesize", fn);
	sprintf(length, "%d", F_wordsize*len);
	AS_rewriteFrameSize(body, target, length);

	return AS_Proc("", body, "");
}

