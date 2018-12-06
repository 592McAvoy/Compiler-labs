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
	Temp_tempList calleesaves;
	Temp_tempList callersaves;
	Temp_tempList specialregs;
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

	F_access ac;
	/*if(esc){
		ac = InFrame(cnt * F_wordsize);
	}
	else{
		ac = InReg(Temp_newtemp());
	}*/
	if(cnt == 0){
		ac = InFrame(F_wordsize);//SL
	}
	else{
		ac = F_allocLocal(f, esc);//args
	}
	
	if(formals->tail){
		return F_AccessList(ac, makeFormalsF(f, formals->tail, cntp));
	}
	else{
		return F_AccessList(ac, NULL);
	}
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
	

	f->calleesaves = NULL;
	f->callersaves = NULL;
	f->specialregs = NULL;

	return f;
}

//locals position wait to be reset
F_access F_allocLocal(F_frame f, bool escape){
	int length = f->length;
	F_accessList locals = f->locals;

	F_access ac;
	if(escape){
		ac = InFrame(-(length+1) * F_wordsize);
	}
	else{
		ac = InReg(Temp_newtemp());
	}

	f->length = length+1;
	f->locals = F_AccessList(ac, locals);

	return ac;
}

Temp_label F_name(F_frame f){
	return f->name;
}
F_accessList F_formals(F_frame f){
	return f->formals;
}

/* IR translation */
Temp_temp F_FP(void){
	Temp_temp fp = Temp_newtemp();
	Temp_enter(F_tempMap, fp, "fp");
	return fp;
}
Temp_temp F_SP(void){
	Temp_temp sp = Temp_newtemp();
	Temp_enter(F_tempMap, sp, "sp");
	return sp;
}
Temp_temp F_RV(void){
	Temp_temp rv = Temp_newtemp();
	Temp_enter(F_tempMap, rv, "rv");
	return rv;
}
Temp_temp F_PC(void){
	Temp_temp pc = Temp_newtemp();
	Temp_enter(F_tempMap, pc, "pc");
	return pc;
}
Temp_temp F_ARG(int idx){
	Temp_temp arg = Temp_newtemp();
	char r[100];
	sprintf(r, "arg%d", idx);
	Temp_enter(F_tempMap, arg, r);
	return arg;
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


T_stm F_procEntryExit1(F_frame f, T_stm stm){
	//view change
	T_stm view = NULL;
	int cnt = 0;
	T_exp fp = T_Temp(F_FP());
	for(F_accessList l=f->formals;l;l=l->tail){
		F_access arg = l->head;
		T_exp argpos = F_exp(arg,fp);
		T_stm tmp = T_Move(argpos,T_Temp(Temp_newtemp()));
		switch(cnt){
			case 0:break;//SL
			case 1:view=tmp;break;//rdi
			case 2://rsi
			case 3://rdx
			case 4://rcx
			case 5://r8
			case 6:view = T_Seq(tmp,view);break;//r9
			default:{
				int off = (cnt-7+1)*F_wordsize ;
				view = T_Seq(
						T_Move(argpos,T_Mem(T_Binop(T_plus, T_Const(off), fp))),view);
			}
		}
		cnt += 1;
	}
	//callee save

	
	if(!view){
		return stm;
	}
	return T_Seq(view,stm);
}
AS_instrList F_procEntryExit2(AS_instrList body);
AS_proc F_procEntryExit3(F_frame frame, AS_instrList body);

