#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "env.h"

/*Lab4: Your implementation of lab4*/

E_enventry E_VarEntry(Ty_ty ty)
{
	E_enventry e = checked_malloc(sizeof(*e));
	e->kind = E_varEntry;
	e->u.var.ty = ty;
	return e;
}

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result)
{
	E_enventry e = checked_malloc(sizeof(*e));
	e->kind = E_funEntry;
	e->u.fun.formals = formals;
	e->u.fun.result = result;
	return e;
}

S_table E_base_tenv(void)
{
	S_table tenv = S_empty();
	S_enter(tenv,S_Symbol("string"),Ty_String());
	S_enter(tenv,S_Symbol("int"),Ty_Int());
	return tenv;
}

S_table E_base_venv(void)
{
	return S_empty();
}
