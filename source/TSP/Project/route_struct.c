#include "route_struct.h"

void init_route(struct route *route) {
	int i;

	for (i = 0; i < MAX_CITIES; i++) {
		route->route[i] = -1;
		route->cityUsed[i] = 0;
	}
	route->depth = 0;
	route->length = 0;
	route->full_eval_1st_cities = 0;
	route->unused_full_eval_1st_cities = 0;
	route->remaining_depth = 0;
}
