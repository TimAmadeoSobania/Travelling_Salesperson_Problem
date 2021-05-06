#ifndef _COORDS_H_
#define _COORDS_H_

#include "tsp.h"
#include "route_struct.h"

void populate_distance_matrix(double **distance_p, int *num_cities, char city_names[MAX_CITIES][MAX_CITY_NAME_LENGTH]);

#endif // ifndef _COORDS_H_