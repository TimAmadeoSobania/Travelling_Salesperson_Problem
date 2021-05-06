/********************************************************************

Project group: HPC2020_Project2013

Original authors:
(sorted by lastname)
	- Demirci Anil
	- Topp Manuel  

Optimizing authors:
(sorted by lastname)
	- Leserri David
	- Sobania Tim

*********************************************************************/

#define DEBUG_ARRAY_SIZE 10

#define _USE_MATH_DEFINES
#define START_CITY 0 // You shall not modify this value!
#define ENABLE_BRANCH_AND_BOUND // Undefine to disable branch-and-bound.
#define STRUCT_FIELD_COUNT 7
#define USED_CPU "Cluster"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>


#include "tsp.h"
#include "coords.h"
#include "route_struct.h"
#include "local_stack.h"

#include <mpi.h>
#include <stddef.h>

void push_first_work_local(struct local_stack *local_work, int num_cities) {
	struct route root_route;

	init_route(&root_route);

	/*
	Add the first city to the initial route.
	You ask why?
	Any route that does not start with city 0 (or START_CITY) is effectively
	one route that starts with city 0 (or START_CITY) where you choose to begin
	somewhere in the middle.
	*/
	if (START_CITY < num_cities) {
		if (!root_route.cityUsed[START_CITY]) {
			// city is not used in this route, so we will add it now.
			// Append city to the route:
			root_route.route[root_route.depth] = START_CITY;
			// Flag city as used in this route:
			root_route.cityUsed[START_CITY] = 1;
			// Increment the number of cities in this route:
			root_route.depth++;
			root_route.remaining_depth = num_cities - root_route.depth;
			if (root_route.depth == num_cities) {
				printf("Only one city !?\n");
			}
			else {
				// Push to stack:
				push(local_work, &root_route);
				//printf("Depth: %d. \n", root_route.depth);
			}
		}
		else {
			printf("City %d is used in the empty route !?\n", START_CITY);
		}
	}
	else {
		printf("No cities !?\n");
	}
}

void print_route(struct route route, char *pre_string) {
	int city;
	char route_string[MAX_CITIES];

	for (city = 0; city < MAX_CITIES; city++) {
		if (route.route[city] == -1) {
			route_string[city] = 0;
			break;
		}
		else {
			route_string[city] = route.route[city] + 48;
		}
	}
	printf("%sRoute %s\tlength %f\n", pre_string, route_string, route.length);
}


void init_mpi_new_type(MPI_Datatype* newtype) {
	int blocklen[STRUCT_FIELD_COUNT] = { MAX_CITIES, MAX_CITIES, 1, 1, 1, 1, 1 };
	MPI_Datatype type[STRUCT_FIELD_COUNT] = { MPI_INT, MPI_INT, MPI_INT, MPI_DOUBLE, MPI_INT, MPI_INT, MPI_INT };
	MPI_Aint offsets[STRUCT_FIELD_COUNT];

	offsets[0] = offsetof(struct route, route);
	offsets[1] = offsetof(struct route, cityUsed);
	offsets[2] = offsetof(struct route, depth);
	offsets[3] = offsetof(struct route, length);
	offsets[4] = offsetof(struct route, full_eval_1st_cities);
	offsets[5] = offsetof(struct route, unused_full_eval_1st_cities);
	offsets[6] = offsetof(struct route, remaining_depth);

	MPI_Type_create_struct(STRUCT_FIELD_COUNT, blocklen, offsets, type, newtype);
	MPI_Type_commit(newtype);
}

void update_best(struct route* best, struct route new) {
	int i;
	if (new.length < best->length) {
		for (i = 0; i < MAX_CITIES; i++) {
			best->route[i] = new.route[i];
			best->cityUsed[i] = new.cityUsed[i];
		}
		best->depth = new.depth;
		best->length = new.length;
	}
}

void update_best_global(struct route* best, struct route new, int MyRank) {
	int i;

	MPI_Datatype newtype;
	init_mpi_new_type(&newtype);
	MPI_Request req;
	if (new.length < best->length) {
		for (i = 0; i < MAX_CITIES; i++) {
			best->route[i] = new.route[i];
			best->cityUsed[i] = new.cityUsed[i];
		}
		best->depth = new.depth;
		best->length = new.length;
		MPI_Ibcast(&best, 1, newtype, MyRank, MPI_COMM_WORLD, &req);
	}
}


void get_best_route(struct route best, struct route *target) {
	int i;

	for (i = 0; i < MAX_CITIES; i++) {
		target->route[i] = best.route[i];
		target->cityUsed[i] = best.cityUsed[i];
	}
	target->depth = best.depth;
	target->length = best.length;
}





int expand_top_route(struct local_stack *local_work, int *num_cities, struct route *best_route, double *distance, int MyRank) {
	struct route current_route;
	struct route new_route;
	int city;
	double *distance_offset;

	if (pop(local_work, &current_route)) {
		new_route = current_route;
		// Don't worry, this does a deep copy of current_route.route
		// Increment the number of cities in this route:
		new_route.depth++;
		distance_offset = distance + current_route.route[current_route.depth - 1] * *num_cities;
		// Try to append every possible city to this route:
		for (city = 0; city < *num_cities; city++) {
			if (!current_route.cityUsed[city]) {
				// city is not used in this route, so we will add it now.
				// Append city to the route:
				new_route.route[current_route.depth] = city;
				// Flag city as used in this route:
				new_route.cityUsed[city] = 1;
				/*
				Add the distance between the last and the newly appended city
				to the route's total length:
				If we add the first city, there is no distance to add, yet.
				*/
				if (current_route.depth > 0) {
					new_route.length += *(distance_offset + city);
				}
				#ifdef ENABLE_BRANCH_AND_BOUND
				if (new_route.length >= best_route->length) {
					
					//If this route is already longer than the best one, it will
					//not be better once it is finished, so we reject it.
					
					//continue;
					// We do not use continue any more. Therfor we use an else case.
				}
				else {
				#endif // ifdef ENABLE_BRANCH_AND_BOUND
					if (new_route.depth == *num_cities) {
						// This route is now complete!
						/*
						Add the final distance between the last and the first
						city to the total length:
						*/
						
						new_route.length += *(distance + city * *num_cities + new_route.route[0]);

						update_best(best_route, new_route);
					}
					else {
						/*
						This route is still not complete, so let's add it to the
						stack:
						*/
						push(local_work, &new_route);
						//print_route(new_route, "Not complete:\n");
					}
				#ifdef ENABLE_BRANCH_AND_BOUND
				}
				#endif // ifdef ENABLE_BRANCH_AND_BOUND
				// Reset changes for next iteration
				new_route.cityUsed[city] = 0;
				new_route.length = current_route.length;
			}
		}
		return 1;
	}
	else {
		// This thread has no further routes in his stack!
		return 0;
	}
}

int faculty(int x) {
	if (x > 1) {
		return x * faculty(x - 1);
	}
	else {
		return 1;
	}
}

void init_thread_route(struct local_stack* local_work, int* num_cities, double *distance, int nProcs,struct route *best_route, int MyRank) {
	// Zwischenspeicehr für routes
	struct route current_route;

	// Init der Kopie vom originalen Stack
	struct local_stack copy_work;
	init_stack(&copy_work);

	// Init des Zwischenspeichers für expand top route
	struct local_stack tmp_work;
	init_stack(&tmp_work);

	// Faktor x in Abhängikeit vom Verhältins von nProcs zu num_cities würde die initiale Seedgenerierung optimieren (fürs anschließende Aufteilen: Mehr seeds = bessere Verteilungsmöglichkeiten)
	// int x = nProcs/num_cities * irgendwas
	// (nProcs * x > local_work->top_index && local_work->top_index < faculty(*num_cities - 1))

	// Noch nicht genug Seeds für jeden Prozess und weitere Aufteilung möglich
	if (nProcs > local_work->top_index && local_work->top_index < faculty(*num_cities - 1)) {

		// Verschieben der Elemente vom originalen auf den Kopie Stack
		while (!empty(local_work)) {
			pop(local_work, &current_route);
			push(&copy_work, &current_route);
		}

		// Benötigt, weil sich der Index inerhalb des for-loops verändert
		int start_index = copy_work.top_index;

		// Hole jeden Seed vom stack
		for (int i=0; i < start_index; i++) {

			// Seed wird expanded in Zwischenspeicher
			if (pop(&copy_work, &current_route)) {
				push(&tmp_work, &current_route);
				expand_top_route(&tmp_work, num_cities, best_route, distance, MyRank);
			}
			
			// Zwischenspeicher seeds werden umgepackt auf Endstack
			while (!empty(&tmp_work)) {
				pop(&tmp_work, &current_route);
				push(local_work, &current_route);
			}
		}
		// Rekursion bis IF clause nicht mehr zutrifft
		init_thread_route(local_work, num_cities, distance, nProcs, best_route, MyRank);
	}
}

void stack_to_array(struct local_stack* local_work, struct route* local_work_array, int array_size) {
	local_work_array = (struct route*)malloc(array_size * sizeof(struct route));
	for (int i = 0; i < array_size; i++) {
		pop(local_work, &local_work_array[i]);
		//printf("STACK TO ARRAY - INDEX:\t%i\tROUTE:\t%i %i %i\n", i, local_work_array[i].route[0], local_work_array[i].route[1], local_work_array[i].route[2]);
	}	
}

void array_to_stack(struct local_stack* local_work, struct route* local_work_array, int array_size) {
	for (int i = 0; i < array_size; i++) {
		/*
		printf("LENGTH:\t%f\n", local_work_array[i].length);
		printf("Route:\t");
		for (int k = 0; k < 5; k++) {
			printf("%i ", local_work_array[i].route[k]);
		}
		printf("\n");*/
		if (local_work_array[i].length != 0) {
			push(local_work, &local_work_array[i]);
		}
	}
}

void init_best_route(struct route *best_route) {
	init_route(best_route);
	// Let's start with a really bad best route:
	best_route->length = 1e100;
}


void save_times(double* debug_array, int rank, int nProcs, double step, int* num_cities) {

	/* DEBUG ARRAY Keys

	0 AFTER POPULATE DISTANCE MATRIX
	1 AFTER INIT
	2 AFTER STACK TO ARRAY
	3 AFTER BCAST
	4 AFTER SCATTER
	5 AFTER ARRAY TO STACK
	6 AFTER CALCULATION
	7 AFTER GATHER
	8 AFTER FINDING BEST

	*/
	char buf[20];

	snprintf(buf, 70, "%i_times_%i_%0.1lf.csv", num_cities, nProcs, step);

	FILE* fs = fopen(buf, "a");
	if (fs == NULL) {
		printf("Couldn't open file\n");
		return;
	}
	fprintf(fs, "%i,", rank);
	for (int i = 0; i < DEBUG_ARRAY_SIZE; i++) {
		fprintf(fs, "%lf,", debug_array[i]);
	}
	fprintf(fs, "\n");
	fclose(fs);

}

// x_time entfernen stattdessen den Parameter x (Bei der Probelmteilung)
void save_results(int *num_cities, int nProcs, double duration, struct route best_route, double x_time) {
	FILE *fs = fopen("results.csv", "a");
	if (fs == NULL) {
		printf("Couldn't open file\n");
		return;
	}

	//Standard error : In diese Datei wird stderr umgeleitet.Dies betrifft i.d.R.die Ausgabe von Fehlern.

	/*
	fprintf(stderr, "%f, %f ", duration, best_route.length);
	for (int i = 0; i < num_cities; i++) {
		fprintf(stderr, ",%i", best_route.route[i]);
	}
	fprintf(stderr, "\n");
	*/

	//[RUNTIME IN SEC], [ROUTE LENGTH IN KM], [ROUTE ...]
	fprintf(fs, "%i,%i,%f,%0.1lf,%s,%f,", num_cities, nProcs, duration, x_time, USED_CPU, best_route.length);
	for (int i = 0; i < num_cities; i++) {
		fprintf(fs, "%i ", best_route.route[i]);
	}
	fprintf(fs, "\n");
	fclose(fs);
}

int round_up(int numerator, int denominator) {
	if (numerator % denominator == 0) {
		return numerator / denominator;
	}
	else{
		return numerator / denominator + 1;
	}
}



int work_to_do(int* work_array, int nProcs) {
	for (int i = 0; i < nProcs; i++) {
		if (work_array[i] > 0) {
			return 1;
		}
	}
	return 0;
}

int main(int argc, char** argv) {
	// DEBUG STUFF _________________________________________________
	double debug_array[DEBUG_ARRAY_SIZE];
	// DEBUG STUFF _________________________________________________
	struct local_stack local_work;
	struct route best_route;
	struct route save_route;
	int num_cities;
	double* distance;
	char city_names[MAX_CITIES][MAX_CITY_NAME_LENGTH];
	int city;

	int nProcs;
	int MyRank;
	int lw_r, up_r;

	int buf = 10000000;

	double start_time;
	double end_time;
	double duration;

	double x_time;

	MPI_Init(&argc, &argv);
	// how many processes we have  // Size
	MPI_Comm_size(MPI_COMM_WORLD, &nProcs);
	// what is my rank
	MPI_Comm_rank(MPI_COMM_WORLD, &MyRank);
	MPI_Datatype newtype;
	init_mpi_new_type(&newtype);

	start_time = MPI_Wtime();

	// ALMOST ALL OF YOUR CODE BELONGS IN HERE

	if (argc == 2 || argc == 3) {
		num_cities = atoi(argv[1]);
		x_time = 2;
		if (argc == 3) {
			x_time = atof(argv[2]);
		}
	}
	else {
		num_cities = 0;
		x_time = 2;
	}
	populate_distance_matrix(&distance, &num_cities, city_names);
	init_best_route(&best_route);
	init_stack(&local_work);

	// DEBUG STUFF _________________________________________________
	// 0 AFTER POPULATE DISTANCE MATRIX
	debug_array[0] = MPI_Wtime() - start_time;
	// DEBUG STUFF _________________________________________________

	struct route local_work_array;
	struct route* best_array = (struct route*)malloc(nProcs * sizeof(struct route));
	struct local_stack temp_stack, best_stack;
	init_stack(&temp_stack);
	init_stack(&best_stack);

	if (nProcs != 1) {
		struct route temp_array;
		int* a_size, * a_offset;
		a_size = (int*)malloc(nProcs * sizeof(int));
		a_offset = (int*)malloc(nProcs * sizeof(int));
		
		// Prozess 0 initialisiert die Seeds
		if (MyRank == 0) {
			push_first_work_local(&local_work, num_cities);
			init_thread_route(&local_work, &num_cities, distance, nProcs, &best_route, MyRank);
		}

		// DEBUG STUFF _________________________________________________
		// 1 AFTER INIT
		debug_array[1] = MPI_Wtime() - start_time;
		// DEBUG STUFF _________________________________________________

		int array_size = local_work.top_index;
		int sub_array_size = array_size / nProcs;

		//Die Seeds werden in einem Array benötigt, damit MPI_Scatterv sie richtig verschicken kann
		stack_to_array(&local_work, &local_work_array, array_size);

		// DEBUG STUFF _________________________________________________
		// 2 AFTER STACK TO ARRAY
		debug_array[2] = MPI_Wtime() - start_time;
		// DEBUG STUFF _________________________________________________

		// Seeds werden verteilt
		MPI_Bcast(&array_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(&sub_array_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

		int mod = array_size % nProcs;

		// DEBUG STUFF _________________________________________________
		// 3 AFTER BCAST
		debug_array[3] = MPI_Wtime() - start_time;
		// DEBUG STUFF _________________________________________________

		//3 Schleifen für die Generierung der von MPI_Scatterv benötigten Arrays
		for (int i = 0; i < nProcs; i++) {
			a_size[i] = sub_array_size;
			//a_offset[i] = i * sub_array_size;
		}

		for (int i = 0; i <= mod; i++) {
			a_size[i] = a_size[i] + 1;
		}

		a_offset[0] = 0;
		for (int i = 1; i < nProcs; i++) {
			a_offset[i] = a_offset[i - 1] + a_size[i - 1];
		}
		/*
		printf("\n\n\nMYRANK:\t%i\n", MyRank);
		printf("ASIZE:\t");
		for (int i = 0; i < nProcs; i++) {
			printf("%i ", a_size[i]);
		}
		printf("\nAOFFSET:\t");
		for (int i = 0; i < nProcs; i++) {
			printf("%i ", a_offset[i]);
		}
		printf("\n\n\n");
		*/

		//Verteilen der durch P0 generierten Seeds
		MPI_Scatterv(&local_work_array, a_size, a_offset, newtype, &temp_array, sub_array_size + 1, newtype, 0, MPI_COMM_WORLD);

		// DEBUG STUFF _________________________________________________
		// 4 AFTER SCATTER
		debug_array[4] = MPI_Wtime() - start_time;
		// DEBUG STUFF _________________________________________________

		//Rückwandlung zu Stacks, damit wetergerechnet werden kann
		array_to_stack(&temp_stack, &temp_array, sub_array_size + 1);

		//printf("MYRANK:\t%i\tTOPINDEX:\t%i\n\n\n", MyRank, temp_stack.top_index);
		
		// DEBUG STUFF _________________________________________________
		// 5 AFTER ARRAY TO STACK
		debug_array[5] = MPI_Wtime() - start_time;
		// DEBUG STUFF _________________________________________________
		
		//Eigentliche Berechnung jedes Prozesses

		/*
		int counta = 0;
		MPI_Request req = MPI_REQUEST_NULL;


		while (!empty(&temp_stack)) {
			counta++;			
			expand_top_route(&temp_stack, &num_cities, &best_route, distance, MyRank;
		}
		*/

		int * work_array = (int*)malloc(nProcs * sizeof(int));
		for (int i = 0; i < nProcs; i++) {
			work_array[i] = 0;// temp_stack.top_index;
		}
		work_array[MyRank] = temp_stack.top_index;
		double t_count = MPI_Wtime();
		MPI_Request req_length, req_status;
		struct route buf;


		while (work_to_do(work_array, nProcs)) {
			/*
			printf("\nMYRANK:\t%i\n", MyRank);
			printf("DURATION:\t%lf\n", MPI_Wtime() - t_count);
			printf("X_TIME:\t%lf\nWORKTODO:\t", x_time);
			for (int i = 0; i < nProcs; i++) {
				printf("%i, ", work_array[i]);
			}
			printf("\n\n");*/


			if (MPI_Wtime() - t_count > x_time) {

				// Eigenen Status aktuallisieren

				work_array[MyRank] = temp_stack.top_index;

				// Bekanntmachen des eigenen Status
				for (int i = 0; i < nProcs; i++) {
					MPI_Bcast(&work_array[i], 1, MPI_INT, i, MPI_COMM_WORLD);
				}

				// Bekanntmachen des eigenen Status
				for (int i = 0; i < nProcs; i++) {
					buf = best_route;
					MPI_Bcast(&buf, 1, newtype, i, MPI_COMM_WORLD);
					update_best(&best_route, buf);
				}
				/*

				// Bekanntmachen des eigenen Status
				MPI_Ibcast(&work_array[MyRank], 1, MPI_INT, MyRank, MPI_COMM_WORLD, &req_status);

				// Bekanntmachen der eigenen kürzesten Länge
				MPI_Ibcast(&best_route, 1, newtype, MyRank, MPI_COMM_WORLD, &req_length);


				for (int i = 0; i < nProcs; i++) {
					if (i != MyRank) {
						MPI_Bcast(&buf, 1, newtype, i, MPI_COMM_WORLD);
						MPI_Bcast(&work_array[i], 1, newtype, i, MPI_COMM_WORLD);
						update_best(&best_route, buf);
					}
				}
				MPI_Wait(req_length, MPI_STATUS_IGNORE);
				MPI_Wait(req_status, MPI_STATUS_IGNORE);

				//bcast arbeit übrig
				//---- - optional verteilen*/
				t_count = MPI_Wtime();
			}
			else {
				expand_top_route(&temp_stack, &num_cities, &best_route, distance, MyRank);
			}
		}


		//printf("FROM PROCESS:\t%i\tCOUNTA:\t%i\n", MyRank, counta);

		// DEBUG STUFF _________________________________________________
		// 6 AFTER CALCULATION
		debug_array[6] = MPI_Wtime() - start_time;
		// DEBUG STUFF _________________________________________________

		//Es wird die jeweils beste Route der Prozesse gesammelt
		MPI_Gather(&best_route, 1, newtype, best_array, 1, newtype, 0, MPI_COMM_WORLD);
		
		// DEBUG STUFF _________________________________________________
		// 7 AFTER GATHER
		debug_array[7] = MPI_Wtime() - start_time;
		// DEBUG STUFF _________________________________________________

		// GEÄNDERT: 1. NUR RANK 0, 2. Nur abhägig von length
		//Prozess 0 hat alle Ergebnisse sucht das beste heraus
		if (MyRank == 0) {
			for (int i = 0; i < nProcs; i++) {
				update_best(&best_route, best_array[i]);
			}
			
		}

		// DEBUG STUFF _________________________________________________
		// 8 AFTER FINDING BEST
		debug_array[8] = MPI_Wtime() - start_time;
		// DEBUG STUFF _________________________________________________*/
	}
	else //Dieses Else ist nur für des Fal da, dass es nur einen Prozess gibt und das Programm Seriell bearbeitet werden kann.
	{
		push_first_work_local(&local_work, num_cities);
		expand_top_route(&local_work, &num_cities, &best_route, distance, MyRank);
		while (!empty(&local_work)) {
			expand_top_route(&local_work, &num_cities, &best_route, distance, MyRank);
		}
	}


	//printf("Best Route Rank %i Lenght:\t%lf ", MyRank, best_route.length);

	end_time = MPI_Wtime();

	if (MyRank == 0) {
		printf("==========================\n");
		print_route(best_route, "Best route:\n");
		for (city = 0; city < num_cities; city++) {
			printf("%2d\tCity %2d/%2d\t%s\n", city, best_route.route[city],
				best_route.route[city], city_names[best_route.route[city]]);
		}
		duration = end_time - start_time;
		printf("DURATION:\t%f\n", duration);
		/*
		save_results(num_cities, nProcs, duration, best_route, x_time);
		printf("\\\\     //\n");
		printf(" \\\\   //\n");
		printf("  \\\\_//\t\t Duration: %f\n", duration);
		*/
		save_results(num_cities, nProcs, duration, best_route, x_time);
	}
	save_times(debug_array, MyRank, nProcs, x_time, num_cities);
	free(distance);
	MPI_Type_free(&newtype);
	MPI_Finalize();
	return 0;
}

// EOF