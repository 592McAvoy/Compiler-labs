#ifndef LIVENESS_H
#define LIVENESS_H

typedef struct Live_moveList_ *Live_moveList;
struct Live_moveList_ {
	G_node src, dst;
	Live_moveList tail;
};

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail);

struct Live_graph {
	G_graph graph;
	Live_moveList moves;
};
Temp_temp Live_gtemp(G_node n);

enum State{
	PRECOLORED=0,SIMPLIFY,SPILL,FREEZE,STACK
	} ;
typedef struct nodeInfo_ *nodeInfo;
struct nodeInfo_{
	Temp_temp reg;
	int degree;
	enum State stat;
	G_node alias;
};

struct Live_graph Live_liveness(G_graph flow);
void Live_showInfo(void *p);
void Live_prMovs(Live_moveList ml);

#endif
