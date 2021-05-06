#ifndef _ROUTE_STRUCT_H_
#define _ROUTE_STRUCT_H_

#include <stddef.h>

#define MAX_CITIES 24

struct route {
	int route[MAX_CITIES];
	// route[0] = 1st city, route[1] = 2nd city, etc.
	// -1 for "no city"

	int cityUsed[MAX_CITIES];
	// cityUsed[0] = 1 => city 0 is used in this route

	int depth;
	// Number of cities in this route

	double length;
	// Cost of this route

	int full_eval_1st_cities;
	// Number of first place cities that are fully evaluated

	int unused_full_eval_1st_cities;
	// Number of first place cities that are fully evaluated and unsed
	// in this route

	int remaining_depth;
	// Number of cities that are missing in this route
};


void init_route(struct route *route);

#endif // ifndef _ROUTE_STRUCT_H_
