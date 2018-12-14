
/*Lab5: This header file is not complete. Please finish it with more definition.*/

#ifndef FRAME_H
#define FRAME_H

#include "tree.h"
#include "assem.h"

/* declaration for frames */
typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access;

typedef struct F_accessList_ *F_accessList;
struct F_accessList_ {F_access head; F_accessList tail;};
F_accessList F_AccessList(F_access head, F_accessList tail);

F_frame F_newFrame(Temp_label name, U_boolList formals);

Temp_label F_name(F_frame f);
F_accessList F_formals(F_frame f);
F_access F_allocLocal(F_frame f, bool escape);
int F_len(F_frame f);
int F_getFrameOff(F_access acc);

/* temp map*/
Temp_map F_tempMap;

/* declaration for IR translation */
Temp_temp F_FP(void);//frame ptr
Temp_temp F_SP(void);//stack ptr
Temp_temp F_RV(void);//return value
Temp_temp F_ARG(int idx);//func arg_i

Temp_tempList F_Args();//func regs
Temp_tempList F_callerSave();//callersave regs
Temp_tempList F_calleeSave();//calleesave regs
Temp_tempList F_register();//all regs

extern const int F_wordsize;
T_exp F_exp(F_access acc, T_exp framePtr);
T_exp F_externalCall(string s, T_expList args);

/* procedure complement */
T_exp F_procChange(F_frame f, T_exp call);
T_stm F_procEntryExit1(F_frame f, T_stm stm);
AS_instrList F_procEntryExit2(AS_instrList body);
AS_proc F_procEntryExit3(F_frame frame, AS_instrList body);

/* declaration for fragments */
typedef struct F_frag_ *F_frag;
struct F_frag_ {enum {F_stringFrag, F_procFrag} kind;
			union {
				struct {Temp_label label; string str;} stringg;
				struct {T_stm body; F_frame frame;} proc;
			} u;
};

F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);

typedef struct F_fragList_ *F_fragList;
struct F_fragList_ 
{
	F_frag head; 
	F_fragList tail;
};

F_fragList F_FragList(F_frag head, F_fragList tail);

#endif
