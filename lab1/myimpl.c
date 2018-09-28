#include "prog1.h"
#include <stdio.h>
#include <string.h>

int max(int a, int b);
int argsExp(A_exp exp);
int argsInExps(A_expList exps);
int maxargs(A_stm stm);

typedef struct node_* node;
struct node_{
	string id;
	int value;
	node next;
} ;
int getValue(string id);
void saveValue(string id, int value);
void interp(A_stm stm);
int interpExp(A_exp exp);
void interpStm(A_stm stm);
void printExps(A_expList exps);

node head = NULL;

/* --------- uilt functions for interp ------------*/
int getValue(string id){
	if(head == NULL){
		printf("head is empty\n");
		return  -1;
	}

	node ptr = head;
	do{
		if(!strcmp(id, ptr->id)){
			return ptr->value;
		}
		ptr = ptr->next;
	}
	while(ptr != NULL);

	printf("did not find value of %s\n",id);
	return  -1;
}

void saveValue(string id, int value){
	node new = checked_malloc(sizeof *new);
	new->id = id; 
	new->value = value;
	new->next = head;

	head = new;
}

void printExps(A_expList exps){
	switch(exps->kind){
		case 0://PairExplist
		{
			A_exp exp = exps->u.pair.head;
			A_expList explist = exps->u.pair.tail;
			printf("%d ",interpExp(exp));
			printExps(explist);
			return;			
		}
		case 1://LastExp
		{
			printf("%d\n",interpExp(exps->u.last));
			return;
		}
	}
}

void interpStm(A_stm stm){
	switch(stm->kind){
		case 0:	//A_CompoundStm:
		{
			A_stm stm1 = stm->u.compound.stm1;
			A_stm stm2 = stm->u.compound.stm2;
			interp(stm1);
			interp(stm2);
			return;
		}
		case 1:	//A_assignStm:
		{
			string id = stm->u.assign.id;
			A_exp exp = stm->u.assign.exp;
			saveValue(id, interpExp(exp));
			return;
		}
		case 2:	//A_printStm :
		{
			A_expList exps = stm->u.print.exps;
			printExps(exps);
			return;
		}
	}
}

int interpExp(A_exp exp){
	switch(exp->kind){
		case 0:	 //IdExp
		{
			string id = exp->u.id;
			return getValue(id);
		}
		case 1: //NumExp
		{
			int num = exp->u.num;
			return num;
		}			
		case 2: //OpExp
		{
			A_exp left = exp->u.op.left;
			A_exp right = exp->u.op.right;
			int num1 = interpExp(left);
			int num2 = interpExp(right);
			switch(exp->u.op.oper){
				case A_plus: return num1+num2;
				case A_minus: return num1-num2;
				case A_times: return num1*num2;
				case A_div: return num1/num2;
			}
		}
		case 3: //EseqExp
		{
			A_stm stm = exp->u.eseq.stm;
			A_exp exp_ = exp->u.eseq.exp;
			interpStm(stm);
			return interpExp(exp_);
		}
	}
}

/* ------------------------- END -------------------------*/

/* --------- util functions for maxargs -----------*/
int max(int a, int b){
	return a>b ? a:b;
}
int argsExp(A_exp exp){
	switch(exp->kind){
		case 0:	//IdExp
		case 1: //NumExp
			return 0;
		case 2: //OpExp
		{
			A_exp left = exp->u.op.left;
			A_exp right = exp->u.op.right;
			int num1 = argsExp(left);
			int num2 = argsExp(right);
			return max(num1,num2);
		}
		case 3: //EseqExp
		{
			A_stm stm = exp->u.eseq.stm;
			A_exp exp_ = exp->u.eseq.exp;
			int num1 = maxargs(stm);
			int num2 = argsExp(exp_);
			return max(num1,num2);
		}
	}
	return 0;
}
int argsInExps(A_expList exps){
	switch(exps->kind){
		case 0://PairExplist
		{
			A_exp exp = exps->u.pair.head;
			A_expList explist = exps->u.pair.tail;
			return 1 + argsInExps(explist);
		}
		case 1://LastExp
		{
			return 1;
		}
	}

}
/* -------------- END ----------------- */

int maxargs(A_stm stm)
{
	switch(stm->kind){
		case 0:	//A_CompoundStm:
		{
			A_stm stm1 = stm->u.compound.stm1;
			A_stm stm2 = stm->u.compound.stm2;
			int num1 = maxargs(stm1);
			int num2 = maxargs(stm2);
			return max(num1, num2);
		}
		case 1:	//A_assignStm:
		{
			A_exp exp = stm->u.assign.exp;
			return argsExp(exp);
		}
		case 2:	//A_printStm :
		{
			A_expList exps = stm->u.print.exps;
			return argsInExps(exps);
		}
	}
	return 0;
}

void interp(A_stm stm)
{
	interpStm(stm);
}
