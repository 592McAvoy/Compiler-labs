#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "liveness.h"
#include "table.h"

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail) {
	Live_moveList lm = (Live_moveList) checked_malloc(sizeof(*lm));
	lm->src = src;
	lm->dst = dst;
	lm->tail = tail;
	return lm;
}


Temp_temp Live_gtemp(G_node n) {
	//your code here.
	return (Temp_temp)G_nodeInfo(n);
}


static G_graph cfGraph; //cfnode -> temp
static G_nodeList revFlow; 
static TAB_table liveTab; // flownode -> in/out
static TAB_table TNtab; //temp -> cfnode
static Live_moveList movs;

//data structure
typedef struct liveInfo_ *liveInfo;
struct liveInfo_{
	Temp_tempList in;
	Temp_tempList out;
};
liveInfo LiveInfo(Temp_tempList in, Temp_tempList out){
	liveInfo i = checked_malloc(sizeof(*i));
	i->in = in;
	i->out = out;
	return i;
}



//helper
static int pool[100];
static int cnt;
static bool inPool(Temp_temp t){	
	int mark = Temp_int(t);
	for(int i=0;i<cnt;i++){
		if(pool[i] == mark){
			return TRUE;
		}
	}
	pool[cnt] = mark;
	cnt += 1;
	return FALSE;
}
bool inList(Temp_tempList list, Temp_temp t){
	for(;list;list=list->tail){
		Temp_temp tt = list->head;
		if(Temp_int(tt) == Temp_int(t)){
			return TRUE;
		}
	}
	return FALSE;
}
bool Equal(Temp_tempList A, Temp_tempList B){
	Temp_tempList list = A;
	for(; A&&B; A=A->tail,B=B->tail){
		if(Temp_int(A->head) != Temp_int(B->head)){
			break;
		}
	}
	if(A != NULL || B != NULL)
		return FALSE;
	return TRUE;
}
Temp_tempList Union(Temp_tempList A, Temp_tempList B){
	Temp_tempList list = A;
	for(;B;B=B->tail){
		Temp_temp tt = B->head;
		if(!inList(A, tt)){
			list = Temp_TempList(tt, list);
		}
	}
	return list;
}
Temp_tempList Minus(Temp_tempList A, Temp_tempList B){
	Temp_tempList list = NULL;
	for(;A;A=A->tail){
		Temp_temp tt = A->head;
		if(!inList(B, tt)){
			list = Temp_TempList(tt, list);
		}
	}
	return list;
}

//procedure
void Live_showInfo(void *p){
	Temp_temp t = p;
	Temp_map map = Temp_layerMap(F_tempMap, Temp_name());
	printf("%s\t",Temp_look(map, t));
}
static void genGraph(G_graph flow){
	cnt = 0;
	for(G_nodeList nodes=G_nodes(flow);nodes;nodes=nodes->tail){
		G_node fnode = nodes->head;
		for(Temp_tempList tp = FG_def(fnode);tp;tp=tp->tail){
			Temp_temp t = tp->head;
			if(!inPool(t)){
				G_node cfnode = G_Node(cfGraph,t);
				TAB_enter(TNtab, t, cfnode);
			}
		}
		for(Temp_tempList tp = FG_use(fnode);tp;tp=tp->tail){
			Temp_temp t = tp->head;
			if(!inPool(t)){
				G_node cfnode = G_Node(cfGraph,t);
				TAB_enter(TNtab, t, cfnode);
			}
		}
		TAB_enter(liveTab, fnode, LiveInfo(NULL, NULL));
		revFlow = G_NodeList(fnode, revFlow);
	}
}
static void loopAnalyse(){
	bool stable = FALSE;
	while(!stable){
		stable = TRUE;
		for(G_nodeList np=revFlow;np;np=np->tail){
			G_node fnode = np->head;

			liveInfo old = TAB_look(liveTab, fnode);
			assert(old);

			Temp_tempList out = old->out;
			for(G_nodeList sp=G_succ(fnode);sp;sp=sp->tail){
				G_node succ = sp->head;
				liveInfo tmp = TAB_look(liveTab, succ);
				assert(tmp);
				out = Union(out, tmp->in);
			}

			Temp_tempList in = Union(FG_use(fnode), Minus(out, FG_def(fnode)));
			//Temp_tempList in = Minus(Union(out, FG_use(fnode)), FG_def(fnode));

			if(!Equal(in, old->in) || !Equal(out, old->out)){
				stable = FALSE;
			}

			TAB_enter(liveTab, fnode, LiveInfo(in, out));
		}
	}
}
static void addConf(){
	for(G_nodeList np=revFlow;np;np=np->tail){
		G_node fnode = np->head;

		liveInfo info = TAB_look(liveTab, fnode);
		Temp_tempList in = info->in;

		//add conflicts among [in]
		for(Temp_tempList p1=in;p1;p1=p1->tail){
			G_node cf1 = TAB_look(TNtab, p1->head);
			for(Temp_tempList p2=p1->tail;p2;p2=p2->tail){
				G_node cf2 = TAB_look(TNtab, p2->head);
				G_addEdge(cf1, cf2);
			}
		}

		//move
		if(FG_isMove(fnode)){
			Temp_temp dst = FG_def(fnode)->head;
			G_node d = TAB_look(TNtab, dst);
			Temp_temp src = FG_use(fnode)->head;
			G_node s = TAB_look(TNtab, src);
			//G_rmEdge(s, d);
			movs = Live_MoveList(s, d, movs);
		}
	}
}

/* Conflict graph -- [node -> temp]*/
struct Live_graph Live_liveness(G_graph flow) {
	//your code here.
	struct Live_graph lg;

	cfGraph = G_Graph();
	revFlow = NULL;
	liveTab = TAB_empty();
	TNtab = TAB_empty();
	movs = NULL;

	//generate a empty conflict graph with no edge in it 
	genGraph(flow);
	
	//loop find in/out
	loopAnalyse();

	//add conflict edges according to [in]
	addConf();
	
	lg.graph = cfGraph;
	lg.moves = movs;

	return lg;
}


