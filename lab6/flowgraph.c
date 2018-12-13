#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "errormsg.h"
#include "table.h"

//helper 
typedef struct labelNode_ *labelNode;
struct labelNode_{
	Temp_label label;
	AS_targets targets;
	G_node node;
};
labelNode LabelNode( G_node n, Temp_label lab, AS_targets ts){
	labelNode i = checked_malloc(sizeof(*i));
	i->label = lab;
	i->node = n;
	i->targets = ts;
	return i;
}

typedef struct labelNodeList_ *labelNodeList;
struct labelNodeList_{
	labelNode head;
	labelNodeList tail;
};
labelNodeList LabelNodeList(labelNode h, labelNodeList t){
	labelNodeList l = checked_malloc(sizeof(*l));
	l->head = h;
	l->tail = t;
	return l;
}


Temp_tempList FG_def(G_node n) {
	//your code here.
	AS_instr ins = G_nodeInfo(n);
	switch(ins->kind){
		case I_OPER:return ins->u.OPER.dst;
		case I_LABEL:assert(0);return NULL;
		case I_MOVE:return ins->u.MOVE.dst;
	}
}

Temp_tempList FG_use(G_node n) {
	//your code here
	AS_instr ins = G_nodeInfo(n);
	switch(ins->kind){
		case I_OPER:return ins->u.OPER.src;
		case I_LABEL:assert(0);return NULL;
		case I_MOVE:return ins->u.MOVE.src;
	}
}

bool FG_isMove(G_node n) {
	//your code here.
	AS_instr ins = G_nodeInfo(n);
	return ins->kind == I_MOVE && strstr(ins->u.MOVE.assem,"movq `s0, `d0");
}

static labelNodeList labels;
static labelNodeList waits;

static G_node findLabel(Temp_label label){
	for(labelNodeList p=labels; p; p=p->tail){
		Temp_label lab = p->head->label;
		if(lab == label)
			return p->head->node;
	}
	return NULL;
}
static void fillJmp(labelNode ln){
	Temp_labelList p;
	for(p = ln->targets->labels; p; p=p->tail){
		Temp_label lab = p->head;
		G_node succ = findLabel(lab);
		if(!succ){
			EM_impossible("vanishing jump label %s", Temp_labelstring(lab));
			assert(0);
		}
		G_addEdge(ln->node, succ);
	}
}

void FG_showInfo(void *p){
	AS_instr ins = p;
	AS_print(stdout, ins, Temp_layerMap(F_tempMap, Temp_name()));
}

/* GlowGraph -- [node -> As_inst]*/
G_graph FG_AssemFlowGraph(AS_instrList il) {
	//your code here.
	G_graph g = G_Graph();
	labels = NULL; 
	waits =NULL;

	bool prevLab = FALSE;
	Temp_labelList lab = NULL;
	G_node prevNode = NULL;
	//generate flow graph
	for(AS_instrList ls=il; ls; ls=ls->tail){
		AS_instr ins = ls->head;
		switch(ins->kind){
			case I_LABEL:{
				prevLab = TRUE;
				lab = Temp_LabelList(ins->u.LABEL.label, lab); 
				break;
			}
			case I_MOVE:{
				Temp_tempList dst = ins->u.MOVE.dst;
				Temp_tempList src = ins->u.MOVE.src;
				string ass = ins->u.MOVE.assem;				
				if(strstr(ass,"movq `s0, `d0")){
					if(Temp_int(dst->head) == Temp_int(src->head))
						break;
				} 
			}
			case I_OPER:{
				G_node node = G_Node(g, ins);
				if(prevNode){
					G_addEdge(prevNode, node);
				}
				if(prevLab){
					for(;lab;lab=lab->tail)
						labels = LabelNodeList(LabelNode(node, lab->head, NULL),labels);
					prevLab = FALSE;
				}

				prevNode = node;
				if(ins->u.OPER.jumps){
					if(strstr(ins->u.OPER.assem, "jmp"))
						prevNode = NULL;
					waits = LabelNodeList(LabelNode(node, NULL, ins->u.OPER.jumps), waits);
				}
				break;
			}
			default:assert(0);
		}
	}
	//fill jump control
	for(labelNodeList p=waits;p;p=p->tail){
		fillJmp(p->head);
	}
	return g;
}
