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

typedef struct Tr_expList_ *Tr_expList;
Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail);
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
Tr_exp Tr_nilExp();
Tr_exp Tr_intExp(int i);
Tr_exp Tr_stringExp(string str);
Tr_exp Tr_callExp(Temp_label fname, Tr_expList params, Tr_level fl, Tr_level envl);
Tr_exp Tr_arithExp(A_oper op, Tr_exp left, Tr_exp right);
Tr_exp Tr_intCompExp(A_oper op, Tr_exp left, Tr_exp right);
Tr_exp Tr_strCompExp(A_oper op, Tr_exp left, Tr_exp right);
Tr_exp Tr_ptrCompExp(A_oper op, Tr_exp left, Tr_exp right);
Tr_exp Tr_recordExp(Tr_expList list, int cnt);
Tr_exp Tr_assignExp(Tr_exp pos, Tr_exp val);
Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee);
Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body, Temp_label done);
Tr_exp Tr_forExp(Tr_exp lo, Tr_exp hi, Tr_exp body, Temp_label done);
Tr_exp Tr_breakExp(Temp_label done);
Tr_exp Tr_arrayExp(int size, Tr_exp initvar);

#endif
