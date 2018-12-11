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

Temp_map COL_map(){
	static Temp_map coloring = NULL;
	if(!coloring){
		coloring = Temp_empty();
	}
	return coloring;
}

Temp_tempList COL_allColor(){
	return F_register();
}
Temp_tempList COL_rmColor(G_node t, Temp_tempList l){
	Temp_temp c = Live_gtemp(t);
	Temp_map map = Temp_layerMap(COL_map(), F_tempMap);
	string color = Temp_look(map, c);
	Temp_tempList last = NULL;
	for(Temp_tempList p=l;p;p=p->tail){
		string i = Temp_look(map, p->head);
		assert(i);
		if(i == color){
			if(last){
				last->tail = p->tail;
			}
			else{
				l = l->tail;
			}
			break;
		}
	}
	return l;
}
void COL_assignColor(G_node t, Temp_tempList colors){
	string color = Temp_look(F_tempMap, colors->head);
	assert(color);
	Temp_temp rr = Live_gtemp(t);
	Temp_enter(COL_map(), rr, color);
}
void COL_sameColor(G_node from, G_node to){
	Temp_map map = Temp_layerMap(COL_map(), F_tempMap);
	string color = Temp_look(map, Live_gtemp(from));
	assert(color);
	Temp_enter(COL_map(), Live_gtemp(to), color);
}

struct COL_result COL_color(G_graph ig, Temp_map initial, Temp_tempList regs, Live_moveList moves)
{
	//your code here.
	struct COL_result ret;
	return ret;
}
