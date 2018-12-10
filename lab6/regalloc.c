#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "liveness.h"
#include "color.h"
#include "regalloc.h"
#include "table.h"
#include "flowgraph.h"
#include "listop.h"

const int REG_NUM = 16;

//node set
static G_nodeList precolored = NULL;
static G_nodeList coloredNode = NULL;
static G_nodeList spilledNode = NULL;
static G_nodeList coalescedNode = NULL;

//nodeWorklist
static G_nodeList spillWorkList = NULL;
static G_nodeList freezeWorkList = NULL;
static G_nodeList simplifyWorkList = NULL;

//moveList
static Live_moveList worklistMoves = NULL;
static Live_moveList coalescedMoves = NULL;
static Live_moveList freezedMoves = NULL;
static Live_moveList constrainedMoves = NULL;
static Live_moveList activeMoves = NULL;


//stack
static G_nodeList nodeStack;

//helper
void RA_showInfo(void *p){
	nodeInfo t = p;
	Temp_map map = Temp_layerMap(F_tempMap, Temp_name());
	printf("%s\t stat:%d\tdegree:%d\t",Temp_look(map, t->reg), t->stat, t->degree);
}
static void clear(){
	//node set
	precolored = NULL;
	coloredNode = NULL;
	spilledNode = NULL;
	coalescedNode = NULL;

	//nodeWorklist
	spillWorkList = NULL;
	freezeWorkList = NULL;
	simplifyWorkList = NULL;

	//moveList
	worklistMoves = NULL;
	coalescedMoves = NULL;
	freezedMoves = NULL;
	constrainedMoves = NULL;
	activeMoves = NULL;

	nodeStack = NULL;
}
static void Push(G_node node, enum State st){
	switch(st){
		case PRECOLORED: precolored=G_NodeList(node, precolored);break;
		case SIMPLIFY: simplifyWorkList=G_NodeList(node, simplifyWorkList);break;
		case SPILL: spillWorkList=G_NodeList(node, spillWorkList);break;
		case FREEZE: freezeWorkList=G_NodeList(node, freezeWorkList);break;
		case STACK: nodeStack = G_NodeList(node, nodeStack);break;
		default:assert(0);
	}
	
}
static bool MoveRelated(G_node node){
	return inMoveList(node, worklistMoves) || inMoveList(node, activeMoves);
}

static G_nodeList Adjacent(G_node node){
	G_nodeList adj = G_adj(node);
	G_nodeList locked = NL_Union(nodeStack, coalescedNode);
	return NL_Minus(adj, locked);
}
static void EnableMoves(G_nodeList nodes){
	for(;nodes;nodes=nodes->tail){
		G_node node = nodes->head;
		Live_moveList rel = RMrelatedMovs(node, activeMoves);
		//deal with first-mov-related err
		if(inMoveList(node, activeMoves)){
			activeMoves = activeMoves->tail;
		}
		worklistMoves->tail = rel;
	}	
}
static void DecDegree(G_node node){
	nodeInfo info = G_nodeInfo(node);
	int d = info->degree;
	info->degree = d-1;
	if(d == REG_NUM){
		EnableMoves(G_NodeList(node, Adjacent(node)));
		spillWorkList = NL_rmNode(spillWorkList, node);
		if(MoveRelated(node)){
			info->stat = FREEZE;
			Push(node, FREEZE);
		}
		else{
			info->stat = SIMPLIFY;
			Push(node, SIMPLIFY);
		}
	}
}
static G_node GetAlias(G_node node){
	if(G_inNodeList(node, coalescedNode)){
		nodeInfo info = G_nodeInfo(node);
		assert(info->alias);
		return GetAlias(info->alias);
	}
	else{
		return node;
	}
}
static void AddWorkList(G_node node){
	nodeInfo info = G_nodeInfo(node);
	if(!G_inNodeList(node, precolored)
			&& !MoveRelated(node)
				&& info->degree < REG_NUM){
		freezeWorkList = NL_rmNode(freezeWorkList, node);
		Push(node, SIMPLIFY);
	}
}
static bool OK(G_node t, G_node r){
	nodeInfo tinfo = G_nodeInfo(t);
	return (tinfo->degree<REG_NUM 
		|| G_inNodeList(t,precolored)
		|| G_inNodeList(t, G_adj(r)));
}
static bool Check(G_node u, G_node v){
	if(G_inNodeList(u, precolored)){
		bool pass = TRUE;
		for(G_nodeList nl=Adjacent(v);nl;nl=nl->tail){
			G_node t = nl->head;
			if(!OK(t,u)){
				pass = FALSE;
				break;
			}
		}
		if(pass) return TRUE;
	}
	else{
		G_nodeList nodes = NL_Union(Adjacent(u),Adjacent(v));
		int cnt = 0;
		for(;nodes;nodes=nodes->tail){
			G_node n = nodes->head;
			nodeInfo info = G_nodeInfo(n);
			if(info->degree >= REG_NUM) cnt += 1;
		}
		if(cnt < REG_NUM) return TRUE;
	}
	return FALSE;
}
static void Combine(G_node u, G_node v){
	if(G_inNodeList(v, freezeWorkList))
		freezeWorkList = NL_rmNode(freezeWorkList, v);
	else
		spillWorkList = NL_rmNode(spillWorkList, v);

	coalescedNode = G_NodeList(v, coalescedNode);
	nodeInfo vinfo = G_nodeInfo(v);
	vinfo->alias = u;
	EnableMoves(G_NodeList(v, NULL));
	for(G_nodeList nl=Adjacent(v);nl;nl=nl->tail){
		G_node t = nl->head;
		if(!G_inNodeList(t, G_succ(u))){
			G_addEdge(t, u);
		}
		DecDegree(t);
	}
	nodeInfo uinfo = G_nodeInfo(u);
	if(uinfo->degree>=REG_NUM && G_inNodeList(u, freezeWorkList)){
		freezeWorkList = NL_rmNode(freezeWorkList, u);
		spillWorkList = G_NodeList(u, spillWorkList);
	}
	
}



//core procedure
static void MakeWorkList(G_graph cfgraph){
	G_nodeList nl = G_nodes(cfgraph);
	for(;nl;nl=nl->tail){
		G_node node = nl->head;
		nodeInfo info = G_nodeInfo(node);
		int degree = G_degree(node);
		info->degree = degree;
		if(Temp_look(F_tempMap, info->reg)){
			info->stat = PRECOLORED;
			Push(node, PRECOLORED);
			continue;
		}		
		enum State st;
		if(degree >= REG_NUM){
			st = SPILL;
		}
		else if(MoveRelated(node)){
			st = FREEZE;
		}
		else{
			st = SIMPLIFY;
		}
		info->stat = st;
		Push(node, st);
	}
}
static void Simplify(){
	G_node node = simplifyWorkList->head;
	simplifyWorkList = simplifyWorkList->tail;	
	nodeInfo info = G_nodeInfo(node);
	info->stat = STACK;
	Push(node, STACK);
	for(G_nodeList nl=Adjacent(node);nl;nl=nl->tail){
		DecDegree(nl->head);
	}
}
static void Coalesce(){
	Live_moveList p = worklistMoves;
	worklistMoves = worklistMoves->tail;
	G_node src = GetAlias(p->src);
	G_node dst = GetAlias(p->dst);

	G_node u,v;
	if(G_inNodeList(src, precolored)){
		u = src; v = dst;
	}
	else{
		u = dst; v = src;
	}

	if(u == v){
		coalescedMoves = Live_MoveList(p->src, p->dst, coalescedMoves);
		AddWorkList(u);
	}
	else if(G_inNodeList(v, precolored) || G_inNodeList(u, G_adj(v))){
		constrainedMoves = Live_MoveList(p->src, p->dst, constrainedMoves);
		AddWorkList(u);
		AddWorkList(v);
	}
	else if(Check(u, v)){
		coalescedMoves = Live_MoveList(p->src, p->dst, coalescedMoves);
		Combine(u, v);
		AddWorkList(u);
	}
	else{
		activeMoves = Live_MoveList(p->src, p->dst, activeMoves);
	}
}
/*

procedure Main()
 	LivenessAnalysis()
	Build()
	MakeWorklist()
	repeat
	    if simplifyWorklist != {} then Simplify()
	    else if worklistMoves != {} then Coalesce()
	    else if freezeWorklist != {} then Freeze()
	    else if spillWorklist != {} then SelectSpill()
	until simplifyWorklist = {} && worklistMove={} &&
	         freezeWorklis={} && spillWorklist={}
	AssignColor()
	if spilledNodes != {} then
	    RewriteProgram(spilledNodes)
	    Main() 

*/

struct RA_result RA_regAlloc(F_frame f, AS_instrList il) {
	clear();
	//Flowgraph
	G_graph fg = FG_AssemFlowGraph(il);  /* 10.1 */
		//G_show(stdout, G_nodes(fg), FG_showInfo);
		//printf("\n-------====flow graph=====-----\n");

	//liveness analysis
	struct Live_graph lg = Live_liveness(fg);  /* 10.2 */
		//G_show(stdout, G_nodes(lg.graph), Live_showInfo);
		//printf("\n-------==== CF graph=====-----\n");
		//Live_prMovs(lg.moves);

	worklistMoves = lg.moves;
	MakeWorkList(lg.graph);
		G_show(stdout, simplifyWorkList, RA_showInfo);
		printf("\n-------==== init =====-----\n");

	bool empty = FALSE;
	while(!empty){
		empty = TRUE;
		if(simplifyWorkList){
			empty = FALSE;
			Simplify();
			G_show(stdout, nodeStack, RA_showInfo);
			printf("\n-------==== simplify =====-----\n");
		}
		else if(worklistMoves){
			empty = FALSE;
			Coalesce();
		}
	}
	

	struct RA_result ret;
	return ret;
}
