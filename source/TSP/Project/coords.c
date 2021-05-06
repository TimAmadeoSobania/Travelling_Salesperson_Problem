#define _USE_MATH_DEFINES

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "tsp.h"
#include "route_struct.h"

double gps2distance(double latitude1, double longitude1, double latitude2, double longitude2, double radius) {
	/*
	a = sin^2(dLat/2) + cos lat1 * cos lat2 * sin^2(dLong/2)
	c = 2 * atan2( sqrt a, sqrt (1−a) )
	d = R * c
	*/

	double dLat;
	double dLong;
	double lat1;
	double lat2;
	double a;
	double c;
	double d;

	dLat = latitude2 - latitude1;
	dLat *= (M_PI / 180);
	lat1 = latitude1 * M_PI / 180;
	lat2 = latitude2 * M_PI / 180;
	dLong = longitude2 - longitude1;
	dLong *= (M_PI / 180);

	a = pow(sin(dLat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(dLong / 2), 2);
	c = 2 * atan2(sqrt(a), sqrt(1 - a));
	d = radius * c;

	return d;
}

void read_coordinates_file(char *filename, double coord[MAX_CITIES][2], int *num_cities, char city_names[MAX_CITIES][MAX_CITY_NAME_LENGTH]) {
	FILE *file;
	int city;
	char latitude[20];
	char longitude[20];
	int rc;

	file = fopen(filename, "r");
	if (*num_cities == 0) {
		rc = fscanf(file, "%d", num_cities);
	}
	else {
		fscanf(file, "%*d");
	}
	//printf("# cities: %d\n", *num_cities);
	for (city = 0; city < *num_cities; city++) {
		rc = fscanf(file, "%s%*d%s%s", city_names[city], latitude, longitude);
		coord[city][0] = atof(latitude);
		coord[city][1] = atof(longitude);
		//printf("Read city %2d %s\t\tat %6.3f %6.3f\n", city, city_names[city], coord[city][0], coord[city][1]);
	}
	fclose(file);
}

void populate_distance_matrix(double **distance_p, int *num_cities, char city_names[MAX_CITIES][MAX_CITY_NAME_LENGTH]) {
	double coord[MAX_CITIES][2];
	int city1;
	int city2;
	double *distance;

	read_coordinates_file("city_coord.csv", coord, num_cities, city_names);
	*distance_p = (double*)malloc(sizeof(double) * *num_cities * *num_cities);
	distance = *distance_p;
	for (city1 = 0; city1 < *num_cities; city1++) {
		for (city2 = 0; city2 < *num_cities; city2++) {
			*(distance + city1 * *num_cities + city2) = gps2distance(coord[city1][0], coord[city1][1], coord[city2][0],
				coord[city2][1], EARTH_RADIUS);
			//printf("Distance between %d and %d: %f\n", city1, city2, *(distance + city1 * *num_cities + city2));
		}
	}
}
