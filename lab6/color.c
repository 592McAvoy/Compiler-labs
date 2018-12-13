#include <stdio.h>
#include <string.h>

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
#include "table.h"

static Temp_map coloring = NULL;
Temp_map COL_map(){	
	if(!coloring){
		coloring = Temp_empty();
	}
	return coloring;
}
void COL_clearMap(){
	coloring = NULL;
}

Temp_tempList COL_allColor(){
	Temp_tempList l = NULL;
	for(Temp_tempList p =F_register();p;p=p->tail){
		l = Temp_TempList(p->head,l);
	}
	return l;
}
void prTmps(Temp_tempList l){
	Temp_map map = Temp_layerMap(coloring, F_tempMap);
	printf("\t\tcolors:");
	for(;l;l=l->tail){
		string color = Temp_look(map, l->head);
		printf(" %s ",color);
	}
	printf("\n");
}
Temp_tempList COL_rmColor(G_node t, Temp_tempList l){
	Temp_temp c = Live_gtemp(t);
	Temp_map map = Temp_layerMap(coloring, F_tempMap);
	string color = Temp_look(map, c);
	//printf("\t\t\trm color %s\n",color);
	Temp_tempList last = NULL;
	for(Temp_tempList p=l;p;p=p->tail){
		string i = Temp_look(map, p->head);
		assert(i);
		if(!strcmp(i, color)){
			if(last){
				last->tail = p->tail;
			}
			else{
				l = l->tail;
			}
			break;
		}
		last = p;
	}
	//prTmps(l);
	return l;
}
void COL_assignColor(G_node t, Temp_tempList colors){
	string color = Temp_look(F_tempMap, colors->head);
	assert(color);
	Temp_temp rr = Live_gtemp(t);
	Temp_enter(coloring, rr, color);
}
void COL_sameColor(G_node from, G_node to){
	Temp_map map = Temp_layerMap(coloring, F_tempMap);
	string color = Temp_look(map, Live_gtemp(from));
	assert(color);
	Temp_enter(coloring, Live_gtemp(to), color);
}

struct COL_result COL_color(G_graph ig, Temp_map initial, Temp_tempList regs, Live_moveList moves)
{
	//your code here.
	struct COL_result ret;
	return ret;
}
