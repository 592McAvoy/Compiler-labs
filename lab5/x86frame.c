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

#define WORDSIZE 4

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
F_access Inframe(int offset){
	F_access ac = checked_malloc(sizeof(*ac));

	ac->kind = inFrame;
	ac->u.offset = offset;
	return ac;
}   

F_access InReg(Temp_temp reg){
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
F_accessList makeAccList(U_boolList formals, int* cntp){
	bool esc = formals->head;
	int cnt = *cntp;
	*cntp = cnt+1;

	F_access ac;
	if(esc){
		ac = InFrame(cnt * WORDSIZE);
	}
	else{
		ac = InReg(Temp_newtemp());
	}
	
	if(formals->tail){
		return F_AccessList(ac, makeAccList(formals->tail, cntp));
	}
	else{
		return F_AccessList(ac, NULL);
	}
}

F_frame F_newFrame(Temp_label name, U_boolList formals){
	F_frame f = checked_malloc(sizeof(*f));
	int *argsize = checked_malloc(sizeof(int));
	*argsize = 0;
	
	f->name = name;
	f->formals = makeAccList(formals, argsize);
	f->locals = NULL;
	f->argSize = *argsize;
	f->length = 0;

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
		ac = InFrame(-length * WORDSIZE);
	}
	else{
		ac = InReg(Temp_newtemp());
	}

	f->length = length+1;
	f->locals = F_AccessList(ac,locals);

	return ac;
}

Temp_label F_name(F_frame f){
	return f->name;
}
F_accessList F_formals(F_frame f){
	return f->formals;
}


F_frag F_StringFrag(Temp_label label, string str) {   
	    return NULL;                                      
}                                                     
                                                      
F_frag F_ProcFrag(T_stm body, F_frame frame) {        
	    return NULL;                                      
}                                                     
                                                      
F_fragList F_FragList(F_frag head, F_fragList tail) { 
	    return NULL;                                      
}                                                     

