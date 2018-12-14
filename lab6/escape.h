/* escape.h */

#ifndef __ESCAPE_H_
#define __ESCAPE_H_

#include "absyn.h"
#include "symbol.h"
#include "helper.h"

typedef struct escapeEntry_ *escapeEntry;
struct escapeEntry_ {
	int depth;
    bool* escape;
};

escapeEntry EscapeEntry(int d, bool *e);

void Esc_findEscape(A_exp exp);

static void traverseExp(S_table, int depth, A_exp);
static void traverseDec(S_table, int depth, A_dec);
static void traverseVar(S_table, int depth, A_var);


#endif